from flask import Flask, render_template, request, redirect, url_for, session, jsonify
import pymysql
from pymysql.cursors import DictCursor
from mypage_routes import mypage_bp
from search_route import search_bp, highlight_query
from email_utils import send_welcome_email
from datetime import datetime, date
from datetime import timedelta
import requests
import uuid


app = Flask(__name__)
app.secret_key = 'booknest_secret_key'
app.config['PERMANENT_SESSION_LIFETIME'] = timedelta(days=30)  # 로그인 유지 시 30일
app.register_blueprint(mypage_bp)

app.register_blueprint(search_bp)
app.jinja_env.filters['highlight_query'] = highlight_query

# ============================================================
# 결제 준비 정보 저장소
# Flask 기본 session은 브라우저 쿠키 기반이라 결제 팝업/개발 서버 재시작/쿠키 크기 문제에 취약할 수 있습니다.
# 이 프로젝트의 로컬 실행 환경에서는 서버 메모리에 merchant_uid 기준으로 잠시 보관하는 방식이 더 안정적입니다.
# 운영 서버에서는 DB 테이블로 옮기는 것이 가장 안전합니다.
# ============================================================
PAYMENT_PREPARES = {}

# ============================================================
# 카카오 OAuth 설정
# ⚠️ REST API 키를 카카오 개발자 콘솔에서 복사한 값으로 교체하세요
# ============================================================
KAKAO_CLIENT_ID     = 'f6918d20274ab9bd867f2a0434f04689'
KAKAO_CLIENT_SECRET = 'xMVRt9vLBmkYyFN0Z28cvxqgExTX9sTF'
KAKAO_REDIRECT_URI  = 'http://localhost:5000/auth/kakao/callback'

NAVER_CLIENT_ID     = 'zpb6XwcrXIvx2K0sbJeD'
NAVER_CLIENT_SECRET = 'uhzFD07CYr'
NAVER_REDIRECT_URI  = 'http://localhost:5000/auth/naver/callback'

PORTONE_API_KEY    = '4071216618287383'
PORTONE_API_SECRET = 'zmiwOWLK1lNRr2LTphB8vrEhJCBWTTjWtI0uA8M2TSAeTPk0zQnL93Hm46PV56T0ahykksXJTgs0RPFD'


def get_portone_token():
    res = requests.post(
        'https://api.iamport.kr/users/getToken',
        json={'imp_key': PORTONE_API_KEY, 'imp_secret': PORTONE_API_SECRET},
        timeout=10
    )
    return res.json()['response']['access_token']


# ============================================================
# DB 연결 설정
# ============================================================
DB_CONFIG = {
'host': 'switchback.proxy.rlwy.net',
'port': 36928,
'user': 'root',
'password': 'gLAKIHdIhqFvgxCpnGPhAOFvFOeURBZx',
'database': 'railway',
'charset': 'utf8mb4',
'cursorclass': DictCursor
}


def get_db_connection():
    """MySQL 연결 객체 반환"""
    return pymysql.connect(**DB_CONFIG)


def fetch_book_by_title(title):
    """책 제목으로 단일 책 조회"""
    conn = get_db_connection()
    try:
        with conn.cursor() as cursor:
            sql = """
                SELECT
                    B.BookID, B.Title, B.Price, B.Stock, B.Genre,
                    B.AuthorName, B.PublisherName, B.Description, B.ImageURL
                FROM Book B
                WHERE B.Title = %s
                LIMIT 1
            """
            cursor.execute(sql, (title,))
            return cursor.fetchone()
    finally:
        conn.close()


def get_books_by_titles(titles):
    """제목 리스트로 책들 조회 (입력 순서 유지)"""
    if not titles:
        return []
    
    conn = get_db_connection()
    try:
        with conn.cursor() as cursor:
            placeholders = ','.join(['%s'] * len(titles))
            sql = f"""
                SELECT
                    B.BookID,
                    B.Title,
                    B.Price,
                    B.Stock,
                    B.Genre,
                    B.AuthorName,
                    B.PublisherName,
                    B.ImageURL
                FROM Book B
                WHERE B.Title IN ({placeholders})
            """
            cursor.execute(sql, titles)
            books = cursor.fetchall()

            # 입력 순서대로 정렬
            title_order = {t: i for i, t in enumerate(titles)}
            books = sorted(books, key=lambda b: title_order.get(b['Title'], 999))
            return books
    finally:
        conn.close()


# ============================================================
# 섹션별 책 구성 (제목으로 매핑)
# ============================================================
BESTSELLER_TITLES = [
    '불편한 편의점 2',
    '파친코 (리커버판)',
    '채식주의자',
    '아몬드',
    '나미야 잡화점의 기적',
    '데미안',
]

TODAY_PICK_TITLES = [
    '어린 왕자 (특별판)',
    '나미야 잡화점의 기적',
    '채식주의자',
    '데미안',
]

NEW_RELEASE_FEATURED_TITLE = '빛의 제국'

NEW_RELEASE_TITLES = [
    '모든 것의 이론',
    '별의 언어',
    '숲에서 길을 잃다',
    '커피 한 잔 할까요',
    '심리의 함정',
    '경제의 온도',
]

HERO_BOOKS_TITLES = [
    '불편한 편의점 2',
    '파친코 (리커버판)',
    '채식주의자',
]

POEM_BOOK_TITLE = '서랍에 저녁을 넣어'


# ============================================================
# 홈
# ============================================================
@app.route('/')
def index():
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # ✅ 베스트셀러: 재고 낮은 순
            cur.execute("""
                SELECT BookID, Title, Price, Stock, Genre,
                       AuthorName, PublisherName, ImageURL, IsAdult
                FROM Book ORDER BY Stock ASC LIMIT 6
            """)
            bestsellers = cur.fetchall()

            # ✅ 신간: 최신 출간 순
            cur.execute("""
                SELECT BookID, Title, Price, Stock, Genre,
                       AuthorName, PublisherName, ImageURL, PublishDate, IsAdult
                FROM Book ORDER BY PublishDate DESC LIMIT 6
            """)
            new_releases = cur.fetchall()

            # ✅ TODAY 추천: 리뷰 많은 순 4권 (없으면 랜덤)
            cur.execute("""
                SELECT b.BookID, b.Title, b.Price, b.Stock, b.Genre,
                       b.AuthorName, b.PublisherName, b.ImageURL, b.IsAdult,
                       COUNT(r.ReviewID) AS review_cnt
                FROM Book b
                LEFT JOIN Review r ON b.BookID = r.BookID
                GROUP BY b.BookID
                ORDER BY review_cnt DESC, b.BookID ASC
                LIMIT 4
            """)
            today_picks = cur.fetchall()

            # ✅ 오늘의 시: 하드코딩된 시와 일치하는 책 (POEM_BOOK_TITLE 기준)
            cur.execute("""
                SELECT BookID, Title, Price, Stock, Genre,
                       AuthorName, PublisherName, ImageURL
                FROM Book
                WHERE Title = %s
                LIMIT 1
            """, (POEM_BOOK_TITLE,))
            poem_book = cur.fetchone()
            # DB에 없으면 제목 부분 일치로 재시도
            if not poem_book:
                cur.execute("""
                    SELECT BookID, Title, Price, Stock, Genre,
                           AuthorName, PublisherName, ImageURL
                    FROM Book
                    WHERE Title LIKE %s
                    LIMIT 1
                """, ('%서랍%',))
                poem_book = cur.fetchone()

            # ✅ 퀴즈 bookId 매핑: 퀴즈에 쓰이는 책 제목 → 실제 DB BookID
            quiz_titles = ['어린 왕자', '데미안', '아몬드', '파친코', '채식주의자',
                           '레 미제라블', '나는 나를 파괴할 권리가 있다']
            placeholders = ','.join(['%s'] * len(quiz_titles))
            cur.execute(f"""
                SELECT BookID, Title FROM Book
                WHERE Title IN ({placeholders})
            """, quiz_titles)
            rows = cur.fetchall()
            quiz_book_ids = {r['Title']: r['BookID'] for r in rows}

            # ✅ hero_books / featured_new: DB에서 최신 3권 / 최신 1권
            cur.execute("""
                SELECT BookID, Title, Price, Stock, Genre,
                       AuthorName, PublisherName, ImageURL, IsAdult
                FROM Book ORDER BY PublishDate DESC LIMIT 3
            """)
            hero_books = cur.fetchall()

            cur.execute("""
                SELECT BookID, Title, Price, Stock, Genre,
                       AuthorName, PublisherName, ImageURL, Description, IsAdult
                FROM Book ORDER BY PublishDate DESC LIMIT 1
            """)
            featured_new = cur.fetchone()

    finally:
        conn.close()

    return render_template(
        'index.html',
        bestsellers=bestsellers,
        today_picks=today_picks,
        new_releases=new_releases,
        hero_books=hero_books,
        featured_new=featured_new,
        poem_book=poem_book,
        quiz_book_ids=quiz_book_ids,
    )


# ============================================================
# 로그인
# ============================================================
@app.route('/login', methods=['GET', 'POST'])
def login():
    # 이미 로그인된 경우 홈으로
    if 'member_id' in session:
        return redirect(url_for('index'))
    if request.method == 'POST':
        data      = request.get_json()
        member_id = data.get('member_id', '').strip()
        password  = data.get('password', '')

        if not member_id or not password:
            return jsonify({'success': False, 'error': '아이디와 비밀번호를 입력해주세요.'})

        conn = get_db_connection()
        try:
            with conn.cursor() as cur:
                cur.execute("""
                    SELECT MemberID, Name, Role, IsAdultVerified
                    FROM Member
                    WHERE MemberID = %s AND Password = %s
                """, (member_id, password))
                member = cur.fetchone()
        finally:
            conn.close()

        if member:
            remember = data.get('remember', False)
            session.permanent = remember  # ✅ 체크 시 30일 유지, 미체크 시 브라우저 종료와 함께 만료
            session['member_id']   = member['MemberID']
            session['member_name'] = member['Name']
            session['member_role'] = member['Role']
            session['is_adult_verified']  = bool(member.get('IsAdultVerified', False))
            redirect_url = '/admin' if member['Role'] == 'admin' else '/'
            return jsonify({'success': True, 'redirect': redirect_url})
        else:
            return jsonify({'success': False, 'error': '아이디 또는 비밀번호가 올바르지 않습니다.'})

    return render_template('login.html')


@app.route('/logout')
def logout():
    session.clear()
    return redirect(url_for('index'))


# ============================================================
# 카카오 로그인
# ============================================================
def send_kakao_me(access_token, text):
    """카카오톡 나에게 메시지 보내기"""
    import json
    template = {
        'object_type': 'text',
        'text': text,
        'link': {
            'web_url':        'http://localhost:5000',
            'mobile_web_url': 'http://localhost:5000',
        },
        'button_title': 'KYUBOBOOK 바로가기',
    }
    try:
        requests.post(
            'https://kapi.kakao.com/v2/api/talk/memo/default/send',
            headers={'Authorization': f'Bearer {access_token}'},
            data={'template_object': json.dumps(template, ensure_ascii=False)},
            timeout=5,
        )
    except Exception:
        pass
@app.route('/auth/kakao')
def kakao_login():
    kakao_auth_url = (
        'https://kauth.kakao.com/oauth/authorize'
        f'?client_id={KAKAO_CLIENT_ID}'
        f'&redirect_uri={KAKAO_REDIRECT_URI}'
        '&response_type=code'
        '&prompt=login'
    )
    return redirect(kakao_auth_url)


@app.route('/auth/kakao/callback')
def kakao_callback():
    code  = request.args.get('code')
    error = request.args.get('error')

    if error or not code:
        return redirect(url_for('login'))

    # 1. 인가 코드 → 액세스 토큰 교환
    token_res = requests.post(
        'https://kauth.kakao.com/oauth/token',
        headers={'Content-Type': 'application/x-www-form-urlencoded'},
        data={
            'grant_type':    'authorization_code',
            'client_id':     KAKAO_CLIENT_ID,
            'client_secret': KAKAO_CLIENT_SECRET,
            'redirect_uri':  KAKAO_REDIRECT_URI,
            'code':          code,
        }
    )
    token_json   = token_res.json()
    access_token = token_json.get('access_token')
    if not access_token:
        return redirect(url_for('login'))

    # 2. 액세스 토큰 → 사용자 정보 조회
    user_res  = requests.get(
        'https://kapi.kakao.com/v2/user/me',
        headers={'Authorization': f'Bearer {access_token}'}
    )
    user_data     = user_res.json()
    kakao_id      = str(user_data.get('id', ''))
    kakao_account = user_data.get('kakao_account', {})
    nickname      = kakao_account.get('profile', {}).get('nickname', '카카오회원')
    email         = kakao_account.get('email', '')

    member_id = f'kakao_{kakao_id}'

    # 3. DB 회원 조회 → 없으면 추가 정보 입력 페이지로, 있으면 바로 로그인
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                'SELECT MemberID, Name, Role FROM Member WHERE MemberID = %s',
                (member_id,)
            )
            member = cur.fetchone()
    finally:
        conn.close()

    if not member:
        session['kakao_pending'] = {
            'member_id':    member_id,
            'name':         nickname,
            'email':        email,
            'access_token': access_token,
        }
        return redirect(url_for('kakao_register'))

    # 기존 회원 로그인 → 로그인 알림 전송
    from datetime import datetime as dt
    login_time = dt.now().strftime('%Y년 %m월 %d일 %H시 %M분')
    send_kakao_me(
        access_token,
        f'[KYUBOBOOK] 로그인 알림\n\n'
        f'안녕하세요, {member["Name"]}님!\n'
        f'{login_time}에 KYUBOBOOK에 로그인되었습니다.\n\n'
        f'본인이 아닌 경우 비밀번호를 즉시 변경해 주세요.'
    )

    session['member_id']   = member['MemberID']
    session['member_name'] = member['Name']
    session['member_role'] = member['Role']
    return redirect(url_for('index'))


@app.route('/kakao/register', methods=['GET', 'POST'])
def kakao_register():
    pending = session.get('kakao_pending')
    if not pending:
        return redirect(url_for('login'))

    if request.method == 'POST':
        data    = request.get_json()
        email   = (data.get('email')   or '').strip()
        phone   = (data.get('phone')   or '').strip()
        address = (data.get('address') or '').strip()
        birth   = (data.get('birth')   or '').strip() or None

        if not all([email, phone, address]):
            return jsonify({'success': False, 'message': '필수 항목을 모두 입력해주세요.'})
        if '@' not in email:
            return jsonify({'success': False, 'message': '올바른 이메일을 입력해주세요.'})

        member_id = pending['member_id']
        name      = pending['name']
        tmp_pw    = str(uuid.uuid4())

        conn = get_db_connection()
        try:
            with conn.cursor() as cur:
                cur.execute("SELECT MemberID FROM Member WHERE Email = %s", (email,))
                if cur.fetchone():
                    return jsonify({'success': False, 'message': '이미 사용 중인 이메일입니다.'})

                cur.execute(
                    """INSERT INTO Member (MemberID, Name, Email, Password, Phone, Address, Birth, Role)
                       VALUES (%s, %s, %s, %s, %s, %s, %s, 'user')""",
                    (member_id, name, email, tmp_pw, phone or None, address, birth)
                )
            conn.commit()
        except Exception as e:
            conn.rollback()
            return jsonify({'success': False, 'message': f'서버 오류: {str(e)}'})
        finally:
            conn.close()

        # 회원가입 완료 → 카카오톡 환영 메시지 + 이메일 발송
        kakao_token = pending.get('access_token')
        if kakao_token:
            send_kakao_me(
                kakao_token,
                f'[KYUBOBOOK] 회원가입을 환영합니다!\n\n'
                f'안녕하세요, {name}님!\n'
                f'KYUBOBOOK 회원이 되신 것을 진심으로 환영합니다. 📚\n\n'
                f'다양한 도서와 함께 즐거운 독서 생활을 시작해보세요!'
            )
        if email:
            join_date = datetime.now().strftime("%Y년 %m월 %d일")
            send_welcome_email(
                to_email  = email,
                name      = name,
                email     = email,
                phone     = phone or '미입력',
                join_date = join_date
            )

        session.pop('kakao_pending', None)
        session['member_id']   = member_id
        session['member_name'] = name
        session['member_role'] = 'user'
        return jsonify({'success': True})

    return render_template('kakao_register.html', pending=pending)


# ============================================================
# 네이버 로그인
# ============================================================
@app.route('/auth/naver')
def naver_login():
    state = str(uuid.uuid4())
    session['naver_state'] = state
    naver_auth_url = (
        'https://nid.naver.com/oauth2.0/authorize'
        '?response_type=code'
        f'&client_id={NAVER_CLIENT_ID}'
        f'&redirect_uri={NAVER_REDIRECT_URI}'
        f'&state={state}'
    )
    return redirect(naver_auth_url)


@app.route('/auth/naver/callback')
def naver_callback():
    code  = request.args.get('code')
    state = request.args.get('state')
    error = request.args.get('error')

    if error or not code or state != session.pop('naver_state', None):
        return redirect(url_for('login'))

    # 1. 인가 코드 → 액세스 토큰 교환
    token_res = requests.post(
        'https://nid.naver.com/oauth2.0/token',
        data={
            'grant_type':    'authorization_code',
            'client_id':     NAVER_CLIENT_ID,
            'client_secret': NAVER_CLIENT_SECRET,
            'code':          code,
            'state':         state,
        }
    )
    token_json   = token_res.json()
    access_token = token_json.get('access_token')
    if not access_token:
        return redirect(url_for('login'))

    # 2. 액세스 토큰 → 사용자 정보 조회
    user_res  = requests.get(
        'https://openapi.naver.com/v1/nid/me',
        headers={'Authorization': f'Bearer {access_token}'}
    )
    user_data = user_res.json().get('response', {})
    naver_id  = str(user_data.get('id', ''))
    name      = user_data.get('name', '네이버회원')
    email     = user_data.get('email', '')
    phone     = user_data.get('mobile', '')    # "010-1234-5678"
    birthday  = user_data.get('birthday', '')  # "03-10"
    birthyear = user_data.get('birthyear', '') # "1995"

    birth = f'{birthyear}-{birthday}' if birthyear and birthday else None

    member_id = f'naver_{naver_id}'

    # 3. DB 회원 조회 → 없으면 추가 정보 입력 페이지로, 있으면 바로 로그인
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute(
                'SELECT MemberID, Name, Role FROM Member WHERE MemberID = %s',
                (member_id,)
            )
            member = cur.fetchone()
    finally:
        conn.close()

    if not member:
        session['naver_pending'] = {
            'member_id': member_id,
            'name':      name,
            'email':     email,
            'phone':     phone,
            'birth':     birth,
        }
        return redirect(url_for('naver_register'))

    session['member_id']   = member['MemberID']
    session['member_name'] = member['Name']
    session['member_role'] = member['Role']
    return redirect(url_for('index'))


@app.route('/naver/register', methods=['GET', 'POST'])
def naver_register():
    pending = session.get('naver_pending')
    if not pending:
        return redirect(url_for('login'))

    if request.method == 'POST':
        data    = request.get_json()
        email   = (data.get('email')   or '').strip()
        phone   = (data.get('phone')   or '').strip()
        address = (data.get('address') or '').strip()
        birth   = (data.get('birth')   or '').strip() or None

        if not all([email, phone, address]):
            return jsonify({'success': False, 'message': '필수 항목을 모두 입력해주세요.'})
        if '@' not in email:
            return jsonify({'success': False, 'message': '올바른 이메일을 입력해주세요.'})

        member_id = pending['member_id']
        name      = pending['name']
        tmp_pw    = str(uuid.uuid4())

        conn = get_db_connection()
        try:
            with conn.cursor() as cur:
                cur.execute("SELECT MemberID FROM Member WHERE Email = %s", (email,))
                if cur.fetchone():
                    return jsonify({'success': False, 'message': '이미 사용 중인 이메일입니다.'})

                cur.execute(
                    """INSERT INTO Member (MemberID, Name, Email, Password, Phone, Address, Birth, Role)
                       VALUES (%s, %s, %s, %s, %s, %s, %s, 'user')""",
                    (member_id, name, email, tmp_pw, phone or None, address, birth)
                )
            conn.commit()
        except Exception as e:
            conn.rollback()
            return jsonify({'success': False, 'message': f'서버 오류: {str(e)}'})
        finally:
            conn.close()

        # 회원가입 완료 → 환영 이메일 발송
        if email:
            join_date = datetime.now().strftime("%Y년 %m월 %d일")
            send_welcome_email(
                to_email  = email,
                name      = name,
                email     = email,
                phone     = phone or '미입력',
                join_date = join_date
            )

        session.pop('naver_pending', None)
        session['member_id']   = member_id
        session['member_name'] = name
        session['member_role'] = 'user'
        return jsonify({'success': True})

    return render_template('naver_register.html', pending=pending)


# ============================================================
# 아이디/비밀번호 찾기
# ============================================================
@app.route('/find')
def find():
    return render_template('find.html')

@app.route('/find-id', methods=['POST'])
def find_id():
    data  = request.get_json()
    name  = (data.get('name') or '').strip()
    email = (data.get('email') or '').strip()
    if not name or not email:
        return jsonify({'success': False, 'message': '필수 항목을 입력해주세요.'})
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT MemberID FROM Member WHERE Name=%s AND Email=%s", (name, email))
            row = cur.fetchone()
    finally:
        conn.close()
    if row:
        return jsonify({'success': True, 'member_id': row['MemberID']})
    return jsonify({'success': False, 'message': '일치하는 회원 정보가 없습니다.'})

@app.route('/find-pw', methods=['POST'])
def find_pw():
    import random, string
    from email_utils import send_email
    data      = request.get_json()
    member_id = (data.get('member_id') or '').strip()
    email     = (data.get('email') or '').strip()
    if not member_id or not email:
        return jsonify({'success': False, 'message': '필수 항목을 입력해주세요.'})
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT MemberID FROM Member WHERE MemberID=%s AND Email=%s", (member_id, email))
            row = cur.fetchone()
            if not row:
                return jsonify({'success': False, 'message': '일치하는 회원 정보가 없습니다.'})
            # 임시 비밀번호 생성
            tmp_pw = ''.join(random.choices(string.ascii_letters + string.digits, k=10))
            cur.execute("UPDATE Member SET Password=%s WHERE MemberID=%s", (tmp_pw, member_id))
        conn.commit()
    finally:
        conn.close()
    # 이메일 발송
    try:
        send_email(email, '[KYUBOBOOK] 임시 비밀번호 안내',
            f'임시 비밀번호: {tmp_pw}\n로그인 후 반드시 비밀번호를 변경해주세요.')
    except Exception as e:
        print(f'[find_pw] 이메일 발송 실패: {e}')
    return jsonify({'success': True})


# ============================================================
# 회원가입
# ============================================================
@app.route('/register', methods=['POST', 'GET'])
def register():
    #로그인 안한 상태에서 책을 누르고 회원가입을 할 때 Method Not Allowed가 나오는 걸 수정 
    if request.method == 'GET':
        return render_template('register.html')
    
    # JSON 또는 Form 데이터 둘 다 지원
    if request.is_json:
        data = request.get_json()
    else:
        data = request.form

    member_id        = (data.get('member_id') or '').strip()
    is_adult_checked = data.get('is_adult_verified') in (True, 'true', 'True', '1', 'on')
    name      = (data.get('name')      or '').strip()
    email     = (data.get('email')     or '').strip()
    password  = (data.get('password')  or '').strip()
    phone     = (data.get('phone')     or '').strip() or None
    address   = (data.get('address')   or '').strip() or None
    birth            = (data.get('birth')     or '').strip() or None

    # ✅ 생년월일 입력 범위 제한: 1930-01-01 ~ 2020-12-31
    if birth:
        try:
            birth_tmp = date.fromisoformat(birth)
            if birth_tmp < date(1930, 1, 1) or birth_tmp > date(2020, 12, 31):
                return jsonify({'success': False, 'message': '생년월일은 1930-01-01부터 2020-12-31까지만 입력할 수 있습니다.'}), 400
        except ValueError:
            return jsonify({'success': False, 'message': '올바른 생년월일을 입력해주세요.'}), 400

    # ✅ 생년월일 기반 만 19세 이상 체크
    if birth and is_adult_checked:
        try:
            birth_date = date.fromisoformat(birth)
            today = date.today()
            age = today.year - birth_date.year - (
                (today.month, today.day) < (birth_date.month, birth_date.day)
            )
            is_adult_verified = age >= 19
        except ValueError:
            is_adult_verified = False
    else:
        is_adult_verified = False

    # 유효성 검사
    if not all([member_id, name, email, password]):
        return jsonify({'success': False, 'message': '필수 항목을 입력해주세요.'}), 400

    conn = None
    try:
        conn = get_db_connection()
        with conn.cursor() as cursor:

            # 중복 아이디 확인
            cursor.execute("SELECT MemberID FROM Member WHERE MemberID = %s", (member_id,))
            row = cursor.fetchone()
            print(f"[register] 입력된 아이디: '{member_id}', DB조회결과: {row}")
            if row:
                return jsonify({'success': False, 'message': f'이미 사용 중인 아이디입니다. (조회된 값: {row})'}), 400

            # 중복 이메일 확인
            cursor.execute("SELECT MemberID FROM Member WHERE Email = %s", (email,))
            if cursor.fetchone():
                return jsonify({'success': False, 'message': '이미 사용 중인 이메일입니다.'}), 400

            # DB 저장 (MemberID 포함)
            cursor.execute("""
                INSERT INTO Member (MemberID, Name, Email, Password, Phone, Address, Birth, Role, IsAdultVerified)
                VALUES (%s, %s, %s, %s, %s, %s, %s, 'user', %s)
            """, (member_id, name, email, password, phone, address, birth, is_adult_verified))
            conn.commit()

        # ✅ 회원가입 환영 이메일 발송
        join_date = datetime.now().strftime("%Y년 %m월 %d일")
        send_welcome_email(
            to_email  = email,
            name      = name,
            email     = email,
            phone     = phone or '미입력',
            join_date = join_date
        )

        return jsonify({'success': True, 'message': f'{name}님, 가입을 환영합니다!'})

    except Exception as e:
        if conn:
            conn.rollback()
        print(f"[register] 오류: {e}")
        return jsonify({'success': False, 'message': f'서버 오류: {str(e)}'}), 500

    finally:
        if conn:
            conn.close()




# ============================================================
# 책 상세
# ============================================================
@app.route('/book/<int:book_id>')
def book_detail(book_id):
    conn = get_db_connection()
    try:
        with conn.cursor() as cursor:
            # 책 정보
            cursor.execute("""
                SELECT
                    B.BookID, B.Title, B.Price, B.Stock, B.Genre,
                    B.PublishDate, B.IsAdult, B.StorageSlot,
                    B.AuthorName, B.PublisherName,
                    B.Description, B.ImageURL
                FROM Book B
                WHERE B.BookID = %s
            """, (book_id,))
            book = cursor.fetchone()

            if not book:
                return "책을 찾을 수 없습니다.", 404

            # ✅ 19금 차단
            if book['IsAdult'] and not session.get('is_adult_verified'):
                return redirect(url_for('index', adult_block='1'))

            # 리뷰 목록
            cursor.execute("""
                SELECT MemberID, Rating, Content, CreatedAt
                FROM Review
                WHERE BookID = %s
                ORDER BY CreatedAt DESC
            """, (book_id,))
            reviews = cursor.fetchall()

            # QnA 목록
            cursor.execute("""
                SELECT MemberID, Question, Answer, CreatedAt
                FROM QnA
                WHERE BookID = %s
                ORDER BY CreatedAt DESC
            """, (book_id,))
            qna_list = cursor.fetchall()

    finally:
        conn.close()

    return render_template('book_detail.html', book=book, reviews=reviews, qna_list=qna_list)






# ============================================================
# 리뷰 작성
# ============================================================
@app.route('/review/write', methods=['POST'])
def review_write():
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    data    = request.get_json()
    book_id = data.get('book_id')
    rating  = int(data.get('rating', 0))
    content = data.get('content', '').strip()

    if not book_id or not rating or not content:
        return jsonify({'success': False, 'error': '필수 항목을 입력해주세요.'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # ✅ 구매 여부 확인 (COUNT 대신 LIMIT 1)
            cur.execute("""
                SELECT pi.OrderID
                FROM Purchase p
                JOIN PurchaseItem pi ON p.OrderID = pi.OrderID
                WHERE p.MemberID = %s
                  AND pi.BookID = %s
                  AND p.OrderStatus != '취소'
                LIMIT 1
            """, (session['member_id'], book_id))
            if not cur.fetchone():
                return jsonify({'success': False, 'error': '구매한 도서만 리뷰를 작성할 수 있습니다.'})

            # ✅ 중복 리뷰 확인 (COUNT 대신 LIMIT 1)
            cur.execute("""
                SELECT ReviewID FROM Review
                WHERE MemberID = %s AND BookID = %s
                LIMIT 1
            """, (session['member_id'], book_id))
            if cur.fetchone():
                return jsonify({'success': False, 'error': '이미 리뷰를 작성하셨습니다.'})

            cur.execute("""
                INSERT INTO Review (MemberID, BookID, Rating, Content)
                VALUES (%s, %s, %s, %s)
            """, (session['member_id'], book_id, rating, content))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ============================================================
# Q&A
# ============================================================
# ============================================================
#  app.py에 추가할 QnA 라우트
#  기존 /qna 라우트를 아래 코드로 교체하세요
# ============================================================

PER_PAGE_QNA = 10

@app.route('/qna')
def qna():
    page            = max(int(request.args.get('page', 1)), 1)
    selected_book_id = request.args.get('book_id', '', type=int) or None
    filter_type     = request.args.get('filter', 'all')
    mine            = request.args.get('mine', '')

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:

            # ── 사이드바용: Q&A 있는 도서 목록 ──
            cur.execute("""
                SELECT b.BookID, b.Title,
                       COUNT(q.QnAID) AS qna_count
                FROM Book b
                JOIN QnA q ON b.BookID = q.BookID
                GROUP BY b.BookID, b.Title
                ORDER BY qna_count DESC
            """)
            book_list = cur.fetchall()

            # ── 내 질문 수 ──
            my_count = 0
            if session.get('member_id'):
                cur.execute("SELECT COUNT(*) AS cnt FROM QnA WHERE MemberID=%s",
                            (session['member_id'],))
                my_count = cur.fetchone()['cnt']

            # ── WHERE 조건 ──
            where_clauses = []
            params = []

            if selected_book_id:
                where_clauses.append("q.BookID = %s")
                params.append(selected_book_id)

            if mine and session.get('member_id'):
                where_clauses.append("q.MemberID = %s")
                params.append(session['member_id'])

            if filter_type == 'answered':
                where_clauses.append("q.Answer IS NOT NULL AND q.Answer != ''")
            elif filter_type == 'waiting':
                where_clauses.append("(q.Answer IS NULL OR q.Answer = '')")

            where_sql = ("WHERE " + " AND ".join(where_clauses)) if where_clauses else ""

            # ── 전체 개수 ──
            cur.execute(f"SELECT COUNT(*) AS cnt FROM QnA q {where_sql}", params)
            total = cur.fetchone()['cnt']
            total_pages = max((total + PER_PAGE_QNA - 1) // PER_PAGE_QNA, 1)
            offset = (page - 1) * PER_PAGE_QNA

            # ── Q&A 목록 ──
            cur.execute(f"""
                SELECT q.QnAID, q.MemberID, q.BookID,
                       q.Question, q.Answer, q.CreatedAt,
                       b.Title AS book_title
                FROM QnA q
                JOIN Book b ON q.BookID = b.BookID
                {where_sql}
                ORDER BY q.CreatedAt DESC
                LIMIT %s OFFSET %s
            """, params + [PER_PAGE_QNA, offset])
            qna_list = cur.fetchall()

    finally:
        conn.close()

    return render_template(
        'qna.html',
        qna_list=qna_list,
        book_list=book_list,
        total=total,
        total_pages=total_pages,
        page=page,
        selected_book_id=selected_book_id,
        filter=filter_type,
        mine=bool(mine),
        my_count=my_count,
    )


@app.route('/qna/write', methods=['POST'])
def qna_write():
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    data     = request.get_json()
    book_id  = data.get('book_id')
    question = data.get('question', '').strip()

    if not book_id or not question:
        return jsonify({'success': False, 'error': '필수 항목을 입력해주세요.'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                INSERT INTO QnA (MemberID, BookID, Question)
                VALUES (%s, %s, %s)
            """, (session['member_id'], book_id, question))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


@app.route('/qna/delete/<int:qna_id>', methods=['POST'])
def qna_delete(qna_id):
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                DELETE FROM QnA
                WHERE QnAID = %s AND MemberID = %s
            """, (qna_id, session['member_id']))
            if cur.rowcount == 0:
                return jsonify({'success': False, 'error': '삭제 권한이 없습니다.'})
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()

# NOTE: /mypage → mypage_routes.py Blueprint
# NOTE: /search → search_route.py  Blueprint


# ============================================================
# 관리자
# ============================================================
@app.route('/admin')
def admin():
    return render_template('admin/index.html')


# ============================================================
# DB 연결 테스트
# ============================================================
@app.route('/db-test')
def db_test():
    try:
        conn = get_db_connection()
        with conn.cursor() as cursor:
            cursor.execute("SELECT COUNT(*) AS cnt FROM Book")
            result = cursor.fetchone()
        conn.close()
        return f"<h1>✅ DB 연결 성공!</h1><p>책 {result['cnt']}권이 등록되어 있습니다.</p>"
    except Exception as e:
        return f"<h1>❌ DB 연결 실패</h1><p>{str(e)}</p>"





# ============================================================
# 홈페이지 국내도서 외국도서 기능
# ============================================================

PER_PAGE_BOOKS = 15

@app.route('/books/<book_type>')
def book_list(book_type):
    if book_type not in ('local', 'foreign'):
        return redirect(url_for('index'))

    is_local       = (book_type == 'local')
    page           = max(int(request.args.get('page', 1)), 1)
    selected_genre = request.args.get('genre', '')
    sort           = request.args.get('sort', 'newest')

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:

            # ── WHERE 조건 ──
            where_clauses = ["b.Category = %s"]
            params        = ['국내' if is_local else '국외']

            if selected_genre:
                where_clauses.append("b.Genre = %s")
                params.append(selected_genre)

            where_sql = "WHERE " + " AND ".join(where_clauses)

            # ── 정렬 ──
            order_map = {
                'newest':    "b.PublishDate DESC",
                'price_asc': "b.Price ASC",
                'price_desc':"b.Price DESC",
                'title':     "b.Title ASC",
            }
            order_sql = order_map.get(sort, "b.PublishDate DESC")

            # ── 전체 개수 ──
            cur.execute(f"SELECT COUNT(*) AS cnt FROM Book b {where_sql}", params)
            total       = cur.fetchone()['cnt']
            total_pages = max((total + PER_PAGE_BOOKS - 1) // PER_PAGE_BOOKS, 1)
            offset      = (page - 1) * PER_PAGE_BOOKS

            # ── 도서 목록 ──
            cur.execute(f"""
                SELECT
                    b.BookID, b.Title, b.Price, b.Stock,
                    b.Genre, b.PublishDate,
                    b.AuthorName, b.PublisherName,
                    b.Description, b.ImageURL, b.IsAdult
                FROM Book b
                {where_sql}
                ORDER BY {order_sql}
                LIMIT %s OFFSET %s
            """, params + [PER_PAGE_BOOKS, offset])
            books = cur.fetchall()

            # ── 장르 집계 (사이드바) ──
            cur.execute(f"""
                SELECT Genre, COUNT(*) AS cnt
                FROM Book b
                WHERE b.Category = %s AND Genre IS NOT NULL
                GROUP BY Genre
                ORDER BY cnt DESC
            """, ['국내' if is_local else '국외'])
            genres = cur.fetchall()

    finally:
        conn.close()

    return render_template(
        'book_list.html',
        books=books,
        genres=genres,
        total=total,
        total_pages=total_pages,
        page=page,
        is_local=is_local,
        selected_genre=selected_genre,
        sort=sort,
    )




# ============================================================
#  app.py에 추가할 라우트들
#  베스트셀러 / 신간 / 카테고리 / 재고확인
# ============================================================

# ── 베스트셀러 ──────────────────────────────────────────────
@app.route('/bestseller')
def bestseller():
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # 재고 적은 순 = 많이 팔린 것으로 간주 (Stock > 0 인 것만)
            cur.execute("""
                SELECT
                    b.BookID, b.Title, b.Price, b.Stock, b.Genre, b.PublishDate,
                    b.AuthorName, b.PublisherName, b.Description, b.ImageURL, b.IsAdult
                FROM Book b
                ORDER BY b.Stock ASC
                LIMIT 15
            """)
            books = cur.fetchall()
    finally:
        conn.close()

    return render_template(
        'book_list.html',
        books=books,
        genres=[],
        total=len(books),
        total_pages=1,
        page=1,
        is_local=None,        # 국내/외국 구분 없음
        selected_genre='',
        sort='bestseller',
        page_title='🏆베스트셀러',
        page_desc='가장 많이 팔린 인기 도서를 만나보세요',
    )


# ── 신간 ────────────────────────────────────────────────────
@app.route('/new-releases')
def new_releases():
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # 전체 신간 수 (장르 필터용)
            cur.execute("SELECT COUNT(*) AS cnt FROM Book")
            total_new = cur.fetchone()['cnt']

            cur.execute("""
                SELECT
                    b.BookID, b.Title, b.Price, b.Stock, b.Genre, b.PublishDate,
                    b.AuthorName, b.PublisherName, b.Description, b.ImageURL, b.IsAdult
                FROM Book b
                ORDER BY b.PublishDate DESC
                LIMIT 20
            """)
            books = cur.fetchall()

            # 장르 집계 (전체 기준)
            cur.execute("""
                SELECT Genre, COUNT(*) AS cnt
                FROM Book WHERE Genre IS NOT NULL
                GROUP BY Genre ORDER BY cnt DESC
            """)
            genres = cur.fetchall()
    finally:
        conn.close()

    return render_template(
        'book_list.html',
        books=books,
        genres=genres,
        total=total_new,
        total_pages=1,
        page=1,
        is_local=None,
        selected_genre='',
        sort='newest',
        page_title='✨신간',
        page_desc='최근 출간된 따끈따끈한 신간 도서',
    )


# ── 카테고리 ─────────────────────────────────────────────────
@app.route('/category')
def category():
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT Genre, COUNT(*) AS cnt
                FROM Book WHERE Category = '국내' AND Genre IS NOT NULL
                GROUP BY Genre ORDER BY cnt DESC
            """)
            local_genres = cur.fetchall()

            cur.execute("""
                SELECT Genre, COUNT(*) AS cnt
                FROM Book WHERE Category = '국외' AND Genre IS NOT NULL
                GROUP BY Genre ORDER BY cnt DESC
            """)
            foreign_genres = cur.fetchall()
    finally:
        conn.close()

    return render_template(
        'category.html',
        local_genres=local_genres,
        foreign_genres=foreign_genres,
    )


# ── 재고확인 ─────────────────────────────────────────────────
@app.route('/inventory')
def inventory():
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT
                    b.BookID, b.Title, b.Price, b.Stock,
                    b.Genre, b.StorageSlot,
                    b.AuthorName
                FROM Book b
                ORDER BY b.StorageSlot ASC
            """)
            books = cur.fetchall()

            # 통계
            total       = len(books)
            in_stock    = sum(1 for b in books if b['Stock'] > 0)
            out_stock   = sum(1 for b in books if b['Stock'] == 0)
            total_stock = sum(b['Stock'] for b in books)

    finally:
        conn.close()

    stats = {
        'total':       total,
        'in_stock':    in_stock,
        'out_stock':   out_stock,
        'total_stock': total_stock,
    }

    return render_template('inventory.html', books=books, stats=stats)






# ============================================================
#  app.py에 추가할 장바구니 라우트
#  기존 /cart 라우트를 아래로 교체하세요
# ============================================================


# ── 장바구니 담기 (AJAX) ──────────────────────────────────────
@app.route('/cart/add-ajax/<int:book_id>', methods=['POST'])
def cart_add_ajax(book_id):
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    data     = request.get_json() or {}
    quantity = int(data.get('quantity', 1))
    member_id = session['member_id']
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT CartID, Quantity FROM Cart
                WHERE MemberID = %s AND BookID = %s
            """, (member_id, book_id))
            existing = cur.fetchone()

            if existing:
                cur.execute("""
                    UPDATE Cart SET Quantity = Quantity + %s
                    WHERE CartID = %s
                """, (quantity, existing['CartID']))
            else:
                cur.execute("""
                    INSERT INTO Cart (MemberID, BookID, Quantity)
                    VALUES (%s, %s, %s)
                """, (member_id, book_id, quantity))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()

# ── 수량 변경 ─────────────────────────────────────────────────

# ── 장바구니 페이지 ──────────────────────────────────────────
@app.route('/cart')
def cart():
    if 'member_id' not in session:
        return render_template('cart.html', cart_items=[])

    member_id = session['member_id']
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT
                    c.CartID, c.BookID, c.Quantity,
                    b.Title, b.Price, b.Genre, b.Stock,
                    b.AuthorName, b.PublisherName, b.ImageURL
                FROM Cart c
                JOIN Book b ON c.BookID = b.BookID
                WHERE c.MemberID = %s
                ORDER BY c.CartID DESC
            """, (member_id,))
            cart_items = cur.fetchall()
    finally:
        conn.close()

    return render_template('cart.html', cart_items=cart_items)


# ── 장바구니 담기 ─────────────────────────────────────────────
@app.route('/cart/add/<int:book_id>')
def cart_add(book_id):
    if 'member_id' not in session:
        return redirect(url_for('login'))

    member_id = session['member_id']
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # 이미 있으면 수량 +1, 없으면 새로 추가
            cur.execute("""
                SELECT CartID, Quantity FROM Cart
                WHERE MemberID = %s AND BookID = %s
            """, (member_id, book_id))
            existing = cur.fetchone()

            if existing:
                cur.execute("""
                    UPDATE Cart SET Quantity = Quantity + 1
                    WHERE CartID = %s
                """, (existing['CartID'],))
            else:
                cur.execute("""
                    INSERT INTO Cart (MemberID, BookID, Quantity)
                    VALUES (%s, %s, 1)
                """, (member_id, book_id))
        conn.commit()
    except Exception as e:
        conn.rollback()
    finally:
        conn.close()

    # 이전 페이지로 돌아가거나 장바구니로
    return redirect(request.referrer or url_for('cart'))




# ── 장바구니 담기 (AJAX) ──────────────────────────────────────
@app.route('/cart/update/<int:cart_id>', methods=['POST'])
def cart_update(cart_id):
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    data     = request.get_json()
    quantity = int(data.get('quantity', 1))
    if quantity < 1: quantity = 1
    if quantity > 99: quantity = 99

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                UPDATE Cart SET Quantity = %s
                WHERE CartID = %s AND MemberID = %s
            """, (quantity, cart_id, session['member_id']))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ── 단일 아이템 삭제 ─────────────────────────────────────────
@app.route('/cart/remove/<int:cart_id>', methods=['POST'])
def cart_remove(cart_id):
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                DELETE FROM Cart
                WHERE CartID = %s AND MemberID = %s
            """, (cart_id, session['member_id']))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ── 아이템 삭제 ───────────────────────────────────────────────
@app.route('/cart/remove-selected', methods=['POST'])
def cart_remove_selected():
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    data = request.get_json()
    cart_ids = data.get('cart_ids', [])

    if not cart_ids:
        return jsonify({'success': False, 'error': '선택 항목 없음'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            placeholders = ','.join(['%s'] * len(cart_ids))

            sql = f"""
                DELETE FROM Cart
                WHERE MemberID = %s
                AND CartID IN ({placeholders})
            """

            cur.execute(sql, [session['member_id']] + cart_ids)

        conn.commit()
        return jsonify({'success': True})

    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})

    finally:
        conn.close()


# ── 주문 생성 (결제) ──────────────────────────────────────────
@app.route('/cart/order', methods=['POST'])
def cart_order():
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    member_id   = session['member_id']
    data        = request.get_json() or {}
    items       = data.get('items', [])
    address     = (data.get('address') or '').strip()
    used_point  = int(data.get('used_point', 0) or 0)
    req_amount  = int(data.get('amount', 0) or 0)

    if not items:
        return jsonify({'success': False, 'error': '주문할 상품이 없습니다.'})

    if not address:
        return jsonify({'success': False, 'error': '배송지를 입력해주세요.'})

    original_price = 0
    clean_items = []

    try:
        for i in items:
            book_id   = int(i['book_id'])
            quantity  = int(i['quantity'])
            price     = int(i['price'])
            cart_id   = i.get('cart_id')

            if quantity < 1:
                return jsonify({'success': False, 'error': '수량이 올바르지 않습니다.'})
            if price < 0:
                return jsonify({'success': False, 'error': '상품 금액이 올바르지 않습니다.'})

            clean_items.append({
                'book_id': book_id,
                'quantity': quantity,
                'price': price,
                'cart_id': cart_id
            })
            original_price += price * quantity

    except Exception:
        return jsonify({'success': False, 'error': '주문 데이터가 올바르지 않습니다.'})

    if used_point < 0:
        return jsonify({'success': False, 'error': '포인트 사용 값이 올바르지 않습니다.'})
    if used_point > original_price:
        return jsonify({'success': False, 'error': '상품 금액보다 많은 포인트를 사용할 수 없습니다.'})
    if used_point % 100 != 0:
        return jsonify({'success': False, 'error': '포인트는 100P 단위로 사용할 수 있습니다.'})

    final_price = original_price - used_point
    if final_price <= 0:
        return jsonify({'success': False, 'error': '최종 결제 금액이 올바르지 않습니다.'})

    if req_amount and req_amount != final_price:
        return jsonify({'success': False, 'error': '결제 요청 금액이 주문 금액과 일치하지 않습니다.'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # 회원 포인트 잠금 조회
            cur.execute("""
                SELECT Point
                FROM Member
                WHERE MemberID = %s
                FOR UPDATE
            """, (member_id,))
            member = cur.fetchone()

            if not member:
                return jsonify({'success': False, 'error': '회원 정보를 찾을 수 없습니다.'})

            current_point = int(member.get('Point') or 0)
            if used_point > current_point:
                return jsonify({'success': False, 'error': '보유 포인트가 부족합니다.'})

           # ✅ 재고 사전 확인 (주문 생성 전에 먼저 체크)
            for item in clean_items:
                cur.execute("""
                    SELECT Title, Stock
                    FROM Book
                    WHERE BookID = %s
                    FOR UPDATE
                """, (item['book_id'],))
                book = cur.fetchone()

                if not book:
                    return jsonify({'success': False, 'error': '존재하지 않는 상품입니다.'})

                if int(book['Stock']) < int(item['quantity']):
                    return jsonify({
                        'success': False,
                        'error': f"'{book['Title']}' 재고가 부족합니다. (현재 재고: {book['Stock']}권, 주문 수량: {item['quantity']}권"
                    })

            # 포인트 차감
            if used_point > 0:
                cur.execute("""
                    UPDATE Member
                    SET Point = Point - %s
                    WHERE MemberID = %s AND Point >= %s
                """, (used_point, session['member_id'], used_point))

                if cur.rowcount == 0:
                    raise Exception('포인트 차감에 실패했습니다.')

            # 주문 생성
            cur.execute("""
                INSERT INTO Purchase (
                MemberID, TotalPrice, OriginalPrice, UsedPoint,
                ShippingAddress, OrderStatus, HardwareStatus
            )
            VALUES (%s, %s, %s, %s, %s, '결제완료', '승인대기')
            """, (member_id, final_price, original_price, used_point, address))
            order_id = cur.lastrowid
            
            # 주문상품 저장 + 재고 차감
            for item in clean_items:
                cur.execute("""
                    INSERT INTO PurchaseItem (OrderID, BookID, Quantity, Price)
                    VALUES (%s, %s, %s, %s)
                """, (order_id, item['book_id'], item['quantity'], item['price']))

                cur.execute("""
                    UPDATE Book
                    SET Stock = Stock - %s
                    WHERE BookID = %s AND Stock >= %s
                """, (item['quantity'], item['book_id'], item['quantity']))

                if cur.rowcount == 0:
                    raise Exception('재고 부족으로 주문을 처리할 수 없습니다.')

            # 장바구니 삭제
            for item in clean_items:
                if item.get('cart_id'):
                    cur.execute("""
                        DELETE FROM Cart
                        WHERE CartID = %s AND MemberID = %s
                    """, (item['cart_id'], member_id))

        conn.commit()
        return jsonify({'success': True, 'order_id': order_id})

    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ── 포트원 결제 준비 + 완료 검증 + 주문 생성 ──────────────────────────
@app.route('/payment/prepare', methods=['POST'])
def payment_prepare():
    """
    결제 전 서버가 merchant_uid를 발급하고 결제 예정 정보를 서버 메모리에 저장합니다.
    기존처럼 Flask session에만 저장하면 결제 팝업 이후 쿠키/세션 동기화 문제로
    /payment/complete에서 준비 정보를 못 찾는 경우가 생길 수 있어, merchant_uid 기준
    서버 저장소(PAYMENT_PREPARES)를 사용합니다.
    """
    if 'member_id' not in session:
        return jsonify({'success': False, 'error': '로그인 필요'}), 401

    data = request.get_json() or {}
    items = data.get('items', [])
    address = (data.get('address') or '').strip()
    used_point = int(data.get('used_point', 0) or 0)
    order_name = (data.get('order_name') or 'KYUBOBOOK 도서 주문').strip()

    if not items:
        return jsonify({'success': False, 'error': '주문할 상품이 없습니다.'})

    try:
        clean_items = []
        original_total_price = 0
        for item in items:
            book_id = int(item.get('book_id'))
            quantity = int(item.get('quantity', 1))
            price = int(item.get('price', 0))
            cart_id = item.get('cart_id')

            if quantity < 1:
                return jsonify({'success': False, 'error': '수량이 올바르지 않습니다.'})
            if price < 0:
                return jsonify({'success': False, 'error': '상품 금액이 올바르지 않습니다.'})

            clean_items.append({
                'cart_id': cart_id,
                'book_id': book_id,
                'quantity': quantity,
                'price': price,
            })
            original_total_price += price * quantity
    except Exception:
        return jsonify({'success': False, 'error': '주문 데이터가 올바르지 않습니다.'})

    if used_point < 0:
        return jsonify({'success': False, 'error': '포인트 사용 값이 올바르지 않습니다.'})
    if used_point > original_total_price:
        return jsonify({'success': False, 'error': '상품 금액보다 많은 포인트를 사용할 수 없습니다.'})
    if used_point % 100 != 0:
        return jsonify({'success': False, 'error': '포인트는 100P 단위로 사용할 수 있습니다.'})

    final_total_price = original_total_price - used_point
    if final_total_price <= 0:
        return jsonify({'success': False, 'error': '결제 금액이 올바르지 않습니다.'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            for item in clean_items:
                cur.execute("SELECT Title, Stock FROM Book WHERE BookID = %s", (item['book_id'],))
                book = cur.fetchone()
                if not book:
                    return jsonify({'success': False, 'error': '존재하지 않는 상품입니다.'})
                if int(book.get('Stock') or 0) < item['quantity']:
                    return jsonify({'success': False, 'error': f"'{book['Title']}' 재고가 부족합니다."})

            cur.execute("SELECT Point FROM Member WHERE MemberID = %s", (session['member_id'],))
            member = cur.fetchone()
            if not member:
                return jsonify({'success': False, 'error': '회원 정보를 찾을 수 없습니다.'})
            if used_point > int(member.get('Point') or 0):
                return jsonify({'success': False, 'error': '보유 포인트가 부족합니다.'})
    finally:
        conn.close()

    merchant_uid = f"order_{uuid.uuid4().hex[:20]}"

    now = datetime.now()
    stale_keys = []
    for key, val in PAYMENT_PREPARES.items():
        created_at = val.get('created_at_dt')
        if created_at and (now - created_at).total_seconds() > 60 * 60:
            stale_keys.append(key)
    for key in stale_keys:
        PAYMENT_PREPARES.pop(key, None)

    if len(PAYMENT_PREPARES) > 100:
        for key in list(PAYMENT_PREPARES.keys())[:50]:
            PAYMENT_PREPARES.pop(key, None)

    PAYMENT_PREPARES[merchant_uid] = {
        'member_id': session['member_id'],
        'items': clean_items,
        'address': address,
        'original_total_price': original_total_price,
        'used_point': used_point,
        'final_total_price': final_total_price,
        'order_name': order_name,
        'completed': False,
        'order_id': None,
        'created_at': now.isoformat(timespec='seconds'),
        'created_at_dt': now,
    }

    print('[payment_prepare] member_id =', session['member_id'])
    print('[payment_prepare] merchant_uid =', merchant_uid)
    print('[payment_prepare] amount =', final_total_price)
    print('[payment_prepare] stored_keys =', list(PAYMENT_PREPARES.keys()))

    return jsonify({
        'success': True,
        'merchant_uid': merchant_uid,
        'amount': final_total_price,
        'order_name': order_name,
    })


@app.route('/payment/complete', methods=['POST'])
def payment_complete():
    """
    결제 완료 검증 + 주문 생성.

    원칙적으로는 /payment/prepare에서 서버가 발급한 merchant_uid를 기준으로 처리합니다.
    다만 브라우저 캐시/구버전 cart.html/개발 서버 재시작 때문에 prepare 정보가 비어도,
    포트원에서 merchant_uid 결제가 실제 paid로 검증되고 요청 본문에 items가 있으면 주문을 생성합니다.
    """
    if 'member_id' not in session:
        return jsonify({'success': False, 'error': '로그인 필요'}), 401

    data = request.get_json() or {}
    imp_uid = data.get('imp_uid')
    merchant_uid = data.get('merchant_uid')
    incoming_items = data.get('items') or []
    incoming_address = (data.get('address') or '').strip()
    incoming_amount = int(data.get('amount', 0) or 0)
    incoming_used_point = int(data.get('used_point', 0) or 0)

    print('[payment_complete] incoming member_id =', session.get('member_id'))
    print('[payment_complete] incoming imp_uid =', imp_uid)
    print('[payment_complete] incoming merchant_uid =', merchant_uid)
    print('[payment_complete] incoming amount =', incoming_amount)
    print('[payment_complete] incoming items_count =', len(incoming_items))
    print('[payment_complete] stored_keys =', list(PAYMENT_PREPARES.keys()))

    if not merchant_uid:
        return jsonify({'success': False, 'error': '결제 주문번호가 없습니다. 다시 결제해주세요.'})

    pending = PAYMENT_PREPARES.get(merchant_uid)

    # /payment/prepare를 거친 정상 흐름
    if pending:
        if pending.get('member_id') != session['member_id']:
            return jsonify({'success': False, 'error': '결제 요청 회원 정보가 일치하지 않습니다. 다시 로그인 후 결제해주세요.'})
        if pending.get('completed') and pending.get('order_id'):
            return jsonify({'success': True, 'order_id': pending['order_id'], 'already_completed': True})

        items = pending['items']
        address = pending.get('address', '')
        used_point = int(pending.get('used_point') or 0)
        final_total_price = int(pending.get('final_total_price') or 0)
        print('[payment_complete] mode = prepared')

    # 구버전 cart.html 또는 개발 중 서버 재시작으로 prepare 정보가 없는 경우의 안전 fallback
    else:
        print('[payment_complete] mode = fallback_no_prepare')
        if not incoming_items:
            return jsonify({
                'success': False,
                'error': '서버에 결제 준비 정보가 없고 주문 상품 정보도 없습니다. 장바구니 페이지를 새로고침한 뒤 다시 결제해주세요.'
            })

        try:
            items = []
            original_total_price = 0
            for item in incoming_items:
                book_id = int(item.get('book_id'))
                quantity = int(item.get('quantity', 1))
                price = int(item.get('price', 0))
                cart_id = item.get('cart_id')
                if quantity < 1:
                    return jsonify({'success': False, 'error': '수량이 올바르지 않습니다.'})
                if price < 0:
                    return jsonify({'success': False, 'error': '상품 금액이 올바르지 않습니다.'})
                items.append({
                    'cart_id': cart_id,
                    'book_id': book_id,
                    'quantity': quantity,
                    'price': price,
                })
                original_total_price += price * quantity
        except Exception:
            return jsonify({'success': False, 'error': '주문 데이터가 올바르지 않습니다.'})

        if incoming_used_point < 0:
            return jsonify({'success': False, 'error': '포인트 사용 값이 올바르지 않습니다.'})
        if incoming_used_point > original_total_price:
            return jsonify({'success': False, 'error': '상품 금액보다 많은 포인트를 사용할 수 없습니다.'})
        if incoming_used_point % 100 != 0:
            return jsonify({'success': False, 'error': '포인트는 100P 단위로 사용할 수 있습니다.'})

        final_total_price = original_total_price - incoming_used_point
        if incoming_amount and incoming_amount != final_total_price:
            return jsonify({'success': False, 'error': '결제 요청 금액이 주문 금액과 일치하지 않습니다.'})

        address = incoming_address
        used_point = incoming_used_point

        PAYMENT_PREPARES[merchant_uid] = {
            'member_id': session['member_id'],
            'items': items,
            'address': address,
            'used_point': used_point,
            'final_total_price': final_total_price,
            'completed': False,
            'order_id': None,
            'created_at': datetime.now().isoformat(timespec='seconds'),
            'fallback': True,
        }
        pending = PAYMENT_PREPARES[merchant_uid]

    # 포트원 결제 검증: merchant_uid를 기준으로 조회합니다.
    try:
        token = get_portone_token()

        verify_by_merchant = requests.get(
            f'https://api.iamport.kr/payments/find/{merchant_uid}',
            headers={'Authorization': token},
            timeout=10
        )
        merchant_json = verify_by_merchant.json()
        payment = merchant_json.get('response')

        imp_json = None
        if not payment and imp_uid:
            verify_by_imp = requests.get(
                f'https://api.iamport.kr/payments/{imp_uid}',
                headers={'Authorization': token},
                timeout=10
            )
            imp_json = verify_by_imp.json()
            payment = imp_json.get('response')

        print('[payment_complete] expected_amount =', final_total_price)
        print('[payment_complete] verify by merchant_uid =', merchant_json)
        if imp_json is not None:
            print('[payment_complete] verify by imp_uid =', imp_json)

        if not payment:
            msg = merchant_json.get('message') or '포트원 결제 정보를 찾을 수 없습니다.'
            return jsonify({'success': False, 'error': f'결제 검증 실패: {msg}'})

        if payment.get('status') != 'paid':
            return jsonify({'success': False, 'error': '결제가 완료되지 않았습니다.'})

        if int(payment.get('amount') or 0) != int(final_total_price):
            return jsonify({'success': False, 'error': '결제 금액이 일치하지 않습니다.'})

        if payment.get('merchant_uid') != merchant_uid:
            return jsonify({'success': False, 'error': '주문번호가 일치하지 않습니다.'})

    except Exception as e:
        return jsonify({'success': False, 'error': f'결제 검증 실패: {str(e)}'})

    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT Point
                FROM Member
                WHERE MemberID = %s
                FOR UPDATE
            """, (session['member_id'],))
            member = cur.fetchone()

            if not member:
                return jsonify({'success': False, 'error': '회원 정보를 찾을 수 없습니다.'})

            current_point = int(member.get('Point') or 0)
            if used_point > current_point:
                return jsonify({'success': False, 'error': '보유 포인트가 부족합니다.'})

            for item in items:
                cur.execute("SELECT Title, Stock FROM Book WHERE BookID = %s FOR UPDATE", (item['book_id'],))
                book = cur.fetchone()
                if not book:
                    return jsonify({'success': False, 'error': '존재하지 않는 상품입니다.'})
                if int(book.get('Stock') or 0) < int(item['quantity']):
                    return jsonify({'success': False, 'error': f"'{book['Title']}' 재고가 부족합니다."})
            original_total_price = sum(int(item['price']) * int(item['quantity']) for item in items)
            
            if used_point > 0:
                cur.execute("""
                    UPDATE Member
                    SET Point = Point - %s
                    WHERE MemberID = %s AND Point >= %s
                """, (used_point, session['member_id'], used_point))

                if cur.rowcount == 0:
                    raise Exception('포인트 차감에 실패했습니다.')
            

            cur.execute("""
                INSERT INTO Purchase (
                    MemberID, TotalPrice, OriginalPrice, UsedPoint,
                    ShippingAddress, OrderStatus, HardwareStatus
                )
                VALUES (%s, %s, %s, %s, %s, '결제완료', '승인대기')
            """, (
                session['member_id'],
                final_total_price,
                original_total_price,
                used_point,
                address
            ))
            order_id = cur.lastrowid
            
             # PurchaseItem 생성 + 재고 차감
            for item in items:
                cur.execute("""
                    INSERT INTO PurchaseItem (OrderID, BookID, Quantity, Price)
                    VALUES (%s, %s, %s, %s)
                """, (order_id, item['book_id'], item['quantity'], item['price']))

                # 재고 차감 (AND Stock >= %s 조건으로 음수 방지)
                cur.execute("""
                    UPDATE Book SET Stock = Stock - %s
                    WHERE BookID = %s AND Stock >= %s
                """, (item['quantity'], item['book_id'], item['quantity']))

                # ✅ rowcount 확인: 0이면 동시 주문으로 재고 소진된 경우
                if cur.rowcount == 0:
                    raise Exception('재고 부족으로 주문을 처리할 수 없습니다. 다시 시도해주세요.')

            # 주문한 항목 장바구니에서 삭제 (direct 주문이면 cart_id 없음)
            for item in items:
                if item.get('cart_id'):
                    cur.execute("""
                        DELETE FROM Cart WHERE CartID = %s AND MemberID = %s
                    """, (item['cart_id'], session['member_id']))

        conn.commit()

        pending['completed'] = True
        pending['order_id'] = order_id
        pending['imp_uid'] = imp_uid
        PAYMENT_PREPARES[merchant_uid] = pending

        return jsonify({'success': True, 'order_id': order_id})

    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()

# 검색 자동완성 API
# ============================================================
@app.route('/api/search-suggest')
def search_suggest():
    q = request.args.get('q', '').strip()
    if not q:
        return jsonify([])
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            # 책 제목 + BookID (바로 이동용)
            cur.execute("""
                SELECT BookID AS book_id, Title AS text, '도서' AS type
                FROM Book WHERE Title LIKE %s LIMIT 7
            """, (f'%{q}%',))
            results = cur.fetchall()
        return jsonify(results)
    except Exception as e:
        print(f"[search-suggest] 오류: {e}")
        return jsonify([])
    finally:
        conn.close()


# ============================================================
# 회원 전화번호·주소 반환 API (결제 모달 자동입력용)
# 정서현 추가 부분 : 결제창에 출석 체크 포인트 적용
# ============================================================
@app.route('/api/my-info')
def api_my_info():
    if 'member_id' not in session:
        return jsonify({'success': False, 'name': '', 'phone': '', 'address': '', 'point': 0})
    conn = get_db_connection()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT Name, Phone, Address, Point FROM Member
                WHERE MemberID = %s
            """, (session['member_id'],))
            row = cur.fetchone()
        return jsonify({
            'success': True,
            'name':    row['Name']    or '' if row else '',
            'phone':   row['Phone']   or '' if row else '',
            'address': row['Address'] or '' if row else '',
            'point': int(row['Point'] or 0) if row else 0
        })
    finally:
        conn.close()


# ── 정적 페이지 라우트 ────────────────────────────────────────
@app.route('/notice')
def notice():
    return render_template('notice.html')

@app.route('/faq')
def faq():
    return render_template('faq.html')

@app.route('/shipping')
def shipping():
    return render_template('shipping.html')

@app.route('/about')
def about():
    return render_template('about.html')

# ============================================================
# 정서현 담당 기능: 출석체크 스탬프판 페이지
# 로그인한 사용자가 월별 출석 도장을 확인하는 화면
# ============================================================
@app.route('/attendance')
def attendance_page():
    if 'member_id' not in session:
        return redirect(url_for('login'))

    return render_template('attendance.html')

# ============================================================
# 정서현 담당 기능: 월별 출석 데이터 조회 API
# 예: /attendance/month-data?year=2026&month=4
# - AttendanceLog에서 해당 월 출석 날짜 조회
# - Member에서 보유 포인트, 연속 출석일, 총 출석 횟수 조회
# ============================================================
@app.route('/attendance/month-data')
def attendance_month_data():
    if 'member_id' not in session:
        return jsonify({
            'success': False,
            'message': '로그인이 필요합니다.'
        }), 401

    member_id = session['member_id']

    today = date.today()
    year = int(request.args.get('year', today.year))
    month = int(request.args.get('month', today.month))

    start_date = date(year, month, 1)

    if month == 12:
        end_date = date(year + 1, 1, 1)
    else:
        end_date = date(year, month + 1, 1)

    conn = get_db_connection()

    try:
        with conn.cursor() as cur:
            # 회원의 현재 포인트/출석 상태 조회
            cur.execute("""
                SELECT Point, LastAttendanceDate, AttendanceStreak, TotalAttendanceCount
                FROM Member
                WHERE MemberID = %s
            """, (member_id,))
            member = cur.fetchone()

            # 해당 월 출석 기록 조회
            cur.execute("""
                SELECT AttendanceDate, EarnedPoint, BonusPoint, StreakDay
                FROM AttendanceLog
                WHERE MemberID = %s
                  AND AttendanceDate >= %s
                  AND AttendanceDate < %s
                ORDER BY AttendanceDate
            """, (member_id, start_date, end_date))
            logs = cur.fetchall()

            # 정서현 담당 기능: 전체 첫 출석일 조회
            cur.execute("""
                 SELECT MIN(AttendanceDate) AS FirstAttendanceDate
                 FROM AttendanceLog
                 WHERE MemberID = %s
            """, (member_id,))
            first_row = cur.fetchone()

        attendance_list = []
        bonus_dates = []

        for row in logs:
            attend_date = row['AttendanceDate']

            if isinstance(attend_date, str):
                attend_date_str = attend_date
            else:
                attend_date_str = attend_date.isoformat()

            attendance_list.append({
                'date': attend_date_str,
                'earned_point': int(row.get('EarnedPoint') or 0),
                'bonus_point': int(row.get('BonusPoint') or 0),
                'streak_day': int(row.get('StreakDay') or 0)
            })

            if int(row.get('BonusPoint') or 0) > 0:
                bonus_dates.append(attend_date_str)

        first_attendance_date = None

        last_attendance_date = None

        if member and member.get('LastAttendanceDate'):
            last_attendance_date = member.get('LastAttendanceDate')

            if not isinstance(last_attendance_date, str):
                last_attendance_date = last_attendance_date.isoformat()

        if first_row and first_row.get('FirstAttendanceDate'):
            first_attendance_date = first_row.get('FirstAttendanceDate')

            if not isinstance(first_attendance_date, str):
                first_attendance_date = first_attendance_date.isoformat()

        return jsonify({
            'success': True,
            'year': year,
            'month': month,
            'point': int(member.get('Point') or 0) if member else 0,
            'streak': int(member.get('AttendanceStreak') or 0) if member else 0,
            'total_count': int(member.get('TotalAttendanceCount') or 0) if member else 0,
            'attendance': attendance_list,
            'bonus_dates': bonus_dates,
            'first_attendance_date': first_attendance_date,
            'last_attendance_date': last_attendance_date
        })

    except Exception as e:
        print('월별 출석 데이터 조회 오류:', e)
        return jsonify({
            'success': False,
            'message': '출석 데이터를 불러오는 중 오류가 발생했습니다.'
        }), 500

    finally:
        conn.close()

# ============================================================
# 출석체크 기능
# 하루 1회 출석, 기본 100P 지급
# 연속 출석 기준 7일마다 보너스 500P 지급
# 중간에 하루라도 출석하지 않으면 연속 출석일이 1일차로 초기화됨
# 100P = 100원 가치
# ============================================================

@app.route('/attendance/check', methods=['POST'])
def attendance_check():
    if 'member_id' not in session:
        return jsonify({
            'success': False,
            'message': '로그인이 필요합니다.'
        }), 401

    member_id = session['member_id']
    today = date.today()
    yesterday = today - timedelta(days=1)

    conn = get_db_connection()

    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT Point, LastAttendanceDate, AttendanceStreak, TotalAttendanceCount
                FROM Member
                WHERE MemberID = %s
            """, (member_id,))
            member = cur.fetchone()

            if not member:
                return jsonify({
                    'success': False,
                    'message': '회원 정보를 찾을 수 없습니다.'
                }), 404

            last_date = member.get('LastAttendanceDate')
            current_streak = member.get('AttendanceStreak') or 0
            current_point = member.get('Point') or 0
            total_count = member.get('TotalAttendanceCount') or 0

            if isinstance(last_date, str):
                last_date = date.fromisoformat(last_date)

            if last_date == today:
                return jsonify({
                    'success': False,
                    'checked_today': True,
                    'message': '오늘은 이미 출석체크를 완료했습니다.',
                    'point': current_point,
                    'point_value': current_point,
                    'streak': current_streak,
                    'total_count': total_count
                })

            if last_date == yesterday:
                new_streak = current_streak + 1
            else:
                new_streak = 1

            base_point = 100

            # 정서현 담당 기능: 연속 출석 기준 7일마다 보너스 500P 지급
            bonus_point = 500 if new_streak > 0 and new_streak % 7 == 0 else 0
            total_gain = base_point + bonus_point

            cur.execute("""
                UPDATE Member
                SET Point = Point + %s,
                    LastAttendanceDate = %s,
                    AttendanceStreak = %s,
                    TotalAttendanceCount = TotalAttendanceCount + 1
                WHERE MemberID = %s
            """, (total_gain, today, new_streak, member_id))

            # 정서현 담당 기능: 월별 스탬프판 표시를 위한 출석 기록 저장
            cur.execute("""
                INSERT IGNORE INTO AttendanceLog
                    (MemberID, AttendanceDate, EarnedPoint, BonusPoint, StreakDay)
                VALUES
                    (%s, %s, %s, %s, %s)
            """, (member_id, today, base_point, bonus_point, new_streak))

        conn.commit()

        new_point = current_point + total_gain

        return jsonify({
            'success': True,
            'message': '출석체크가 완료되었습니다!',
            'base_point': base_point,
            'bonus_point': bonus_point,
            'total_gain': total_gain,
            'point': new_point,
            'point_value': new_point,
            'streak': new_streak,
            'total_count': total_count + 1
        })

    except Exception as e:
        conn.rollback()
        print('출석체크 오류:', e)
        return jsonify({
            'success': False,
            'message': '출석체크 처리 중 오류가 발생했습니다.'
        }), 500

    finally:
        conn.close()


@app.route('/attendance/status')
def attendance_status():
    if 'member_id' not in session:
        return jsonify({
            'logged_in': False,
            'checked_today': False
        })

    member_id = session['member_id']
    today = date.today()

    conn = get_db_connection()

    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT Point, LastAttendanceDate, AttendanceStreak, TotalAttendanceCount
                FROM Member
                WHERE MemberID = %s
            """, (member_id,))
            member = cur.fetchone()

        if not member:
            return jsonify({
                'logged_in': False,
                'checked_today': False
            })

        last_date = member.get('LastAttendanceDate')

        if isinstance(last_date, str):
            last_date = date.fromisoformat(last_date)

        checked_today = last_date == today
        point = member.get('Point') or 0

        return jsonify({
            'logged_in': True,
            'checked_today': checked_today,
            'point': point,
            'point_value': point,
            'streak': member.get('AttendanceStreak') or 0,
            'total_count': member.get('TotalAttendanceCount') or 0
        })

    except Exception as e:
        print('출석 상태 조회 오류:', e)
        return jsonify({
            'logged_in': True,
            'checked_today': False,
            'point': 0,
            'point_value': 0,
            'streak': 0,
            'total_count': 0
        })

    finally:
        conn.close()

print("현재 app.py 수정본 실행됨")
if __name__ == '__main__':
    app.run(host='0.0.0.0', debug=True, port=5000)