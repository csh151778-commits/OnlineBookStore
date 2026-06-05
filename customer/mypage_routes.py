# ============================================================
#  mypage_routes.py  (v2 - 실제 DB 스키마 기준)
#  컬럼명 기준: bookstore_schema.sql (2026.04.22)
#
#  Purchase  : OrderID, MemberID, PurchaseDate, TotalPrice,
#              OrderStatus, HardwareStatus, TrackingNumber
#  PurchaseItem: ItemID, OrderID, BookID, Quantity, Price
#  Review    : ReviewID, MemberID, BookID, Rating, Content, CreatedAt
#  Member    : MemberID, Password, Name, Address, Email,
#              Birth, Phone, Role, CreatedAt
#  Cart      : CartID, MemberID, BookID, Quantity, AddedAt
#  Author    : AuthorID, AuthorName
#  Publisher : PublisherID, PublisherName
# ============================================================

from flask import Blueprint, render_template, request, redirect, \
                  url_for, session, flash, jsonify
import pymysql
import os

mypage_bp = Blueprint('mypage', __name__)


def get_db():
    from app import DB_CONFIG
    return pymysql.connect(**DB_CONFIG)


def login_required(f):
    from functools import wraps
    @wraps(f)
    def decorated(*args, **kwargs):
        if 'member_id' not in session:
            flash('로그인이 필요합니다.', 'warning')
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated


# ──────────────────────────────────────────────
#  GET /mypage
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage')
@login_required
def mypage():
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:

            # 1) 회원 정보
            cur.execute("SELECT * FROM Member WHERE MemberID = %s", (member_id,))
            member = cur.fetchone()

            # 2) 주문 목록 (최신순)
            cur.execute("""
                SELECT OrderID, PurchaseDate, TotalPrice, OriginalPrice, UsedPoint, ShippingAddress,
                       OrderStatus, HardwareStatus, TrackingNumber
                FROM Purchase
                WHERE MemberID = %s
                ORDER BY PurchaseDate DESC
            """, (member_id,))
            orders = cur.fetchall()
            
            # 각 주문의 아이템 목록
            for order in orders:
                order['original_price'] = order.get('OriginalPrice', 0)
                order['used_point'] = order.get('UsedPoint', 0)
                order['shipping_address'] = order.get('ShippingAddress', '-')

                cur.execute("""
                    SELECT pi.Quantity, pi.Price,
                           b.Title AS Title,
                           b.AuthorName AS AuthorName
                    FROM PurchaseItem pi
                    JOIN Book b ON pi.BookID = b.BookID
                    WHERE pi.OrderID = %s
                """, (order['OrderID'],))
                order['items'] = cur.fetchall()
                # 템플릿 호환용 alias
                order['purchase_id']   = order['OrderID']
                order['purchase_date'] = order['PurchaseDate']
                order['total_amount']  = order['TotalPrice']

            # 3) 통계 — 취소 주문 제외
            total_books = 0
            for o in orders:
                if o['OrderStatus'] != '취소':
                    for i in o['items']:
                        total_books += i['Quantity']
            stats = {
                'total_orders': sum(1 for o in orders if o['OrderStatus'] != '취소'),  # ✅ 취소 제외
                'shipping':  sum(1 for o in orders if o['OrderStatus'] == '배송중'),
                'delivered': sum(1 for o in orders if o['OrderStatus'] == '배송완료'),
                'total_books': total_books
            }

            # 4) 내가 쓴 리뷰
            cur.execute("""
                SELECT r.ReviewID AS review_id,
                       r.Rating   AS rating,
                       r.Content  AS content,
                       r.CreatedAt AS created_at,
                       b.Title    AS book_title,
                       b.AuthorName AS author
                FROM Review r
                JOIN Book b ON r.BookID = b.BookID
                WHERE r.MemberID = %s
                ORDER BY r.CreatedAt DESC
            """, (member_id,))
            reviews = cur.fetchall()
            # admin_reply 없으므로 None 처리
            for rv in reviews:
                rv['admin_reply'] = None

    finally:
        conn.close()

    # 디버깅
    print('member:', type(member), member)
    print('orders count:', len(orders))
    print('stats:', stats)
    print('reviews count:', len(reviews))

    return render_template(
        'mypage.html',
        member=member,
        orders=orders,
        stats=stats,
        reviews=reviews,
    )


# ──────────────────────────────────────────────
#  GET /mypage/order-detail/<id>  (AJAX)
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/order-detail/<int:order_id>')
@login_required
def order_detail(order_id):
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT * FROM Purchase
                WHERE OrderID = %s AND MemberID = %s
            """, (order_id, member_id))
            order = cur.fetchone()
            if not order:
                return jsonify({'error': '주문을 찾을 수 없습니다.'}), 404

            cur.execute("""
                SELECT pi.Quantity, pi.Price, b.Title
                FROM PurchaseItem pi
                JOIN Book b ON pi.BookID = b.BookID
                WHERE pi.OrderID = %s
            """, (order_id,))
            items = cur.fetchall()
            print(items)

    finally:
        conn.close()

    return jsonify({
        'purchase_id': order['OrderID'],
        'purchase_date': order['PurchaseDate'].strftime('%Y-%m-%d %H:%M') if order['PurchaseDate'] else '-',
        'OrderStatus': order['OrderStatus'],
        'HardwareStatus': order['HardwareStatus'],
        'TrackingNumber': order.get('TrackingNumber', '-') or '-',
        'address': order.get('ShippingAddress') or '-',
        'original_price': int(order.get('OriginalPrice') or 0),
        'used_point': int(order.get('UsedPoint') or 0),
        'total_amount': int(order['TotalPrice']),
        'items': [
            {'title': i['Title'], 'quantity': int(i['Quantity']), 'price': int(i['Price'])}
            for i in items
        ]
    })


def member_address(member_id):
    """회원 주소 조회 헬퍼"""
    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("SELECT Address FROM Member WHERE MemberID=%s", (member_id,))
            row = cur.fetchone()
            return row['Address'] if row else '-'
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/cancel-order/<id>
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/cancel-order/<int:order_id>', methods=['POST'])
@login_required
def cancel_order(order_id):
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT OrderID, UsedPoint FROM Purchase
                WHERE OrderID = %s AND MemberID = %s AND OrderStatus = '결제완료'
            """, (order_id, member_id))
            order = cur.fetchone()
            if not order:
                return jsonify({'success': False, 'error': '취소 가능한 주문이 아닙니다.'})

            # ✅ 재고 복구 — 취소된 수량만큼 Book.Stock 증가
            cur.execute("""
                SELECT BookID, Quantity FROM PurchaseItem
                WHERE OrderID = %s
            """, (order_id,))
            items = cur.fetchall()
            for item in items:
                cur.execute("""
                    UPDATE Book SET Stock = Stock + %s
                    WHERE BookID = %s
                """, (item['Quantity'], item['BookID']))
                
            # 포인트 복구
            used_point = int(order.get('UsedPoint') or 0)
            if used_point > 0:
                cur.execute("""
                    UPDATE Member
                    SET Point = Point + %s
                    WHERE MemberID = %s
                """, (used_point, member_id))
            # 주문 상태 취소
            cur.execute("""
                UPDATE Purchase SET OrderStatus = '취소'
                WHERE OrderID = %s
            """, (order_id,))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/delete-order/<id>  (취소된 주문 삭제)
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/delete-order/<int:order_id>', methods=['POST'])
@login_required
def delete_order(order_id):
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:
            # 본인 주문인지 + 취소 상태인지 확인
            cur.execute("""
                SELECT OrderID, OrderStatus FROM Purchase
                WHERE OrderID = %s AND MemberID = %s
            """, (order_id, member_id))
            order = cur.fetchone()
            if not order:
                return jsonify({'success': False, 'error': '주문을 찾을 수 없습니다.'})
            if order['OrderStatus'] != '취소':
                return jsonify({'success': False, 'error': '취소된 주문만 삭제할 수 있습니다.'})

            # PurchaseItem 먼저 삭제 (외래키 제약)
            cur.execute("DELETE FROM PurchaseItem WHERE OrderID = %s", (order_id,))
            # Purchase 삭제
            cur.execute("DELETE FROM Purchase WHERE OrderID = %s AND MemberID = %s",
                        (order_id, member_id))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/update-avatar  (프로필 사진 변경)
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/update-avatar', methods=['POST'])
@login_required
def update_avatar():
    member_id = session['member_id']
    file = request.files.get('avatar')
    if not file or file.filename == '':
        return jsonify({'success': False, 'error': '파일이 없습니다.'})

    # 허용 확장자 체크
    allowed = {'png', 'jpg', 'jpeg', 'gif', 'webp'}
    ext = file.filename.rsplit('.', 1)[-1].lower() if '.' in file.filename else ''
    if ext not in allowed:
        return jsonify({'success': False, 'error': '이미지 파일만 업로드 가능합니다.'})

    # static/avatars/ 폴더에 저장 — 확장자 무관하게 항상 {member_id}.jpg 로 저장
    from flask import current_app
    avatar_dir = os.path.join(current_app.root_path, 'static', 'avatars')
    os.makedirs(avatar_dir, exist_ok=True)

    # 기존 파일 전부 삭제
    for old_ext in allowed:
        old_path = os.path.join(avatar_dir, f'{member_id}.{old_ext}')
        if os.path.exists(old_path):
            os.remove(old_path)

    # 항상 .jpg 로 저장 (HTML img src 와 일치)
    save_path = os.path.join(avatar_dir, f'{member_id}.jpg')
    file.save(save_path)

    avatar_url = f'/static/avatars/{member_id}.jpg?t={__import__("time").time():.0f}'
    return jsonify({'success': True, 'avatar_url': avatar_url})


# ──────────────────────────────────────────────
#  POST /mypage/update-profile
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/update-profile', methods=['POST'])
@login_required
def update_profile():
    member_id = session['member_id']
    name    = request.form.get('name', '').strip()
    email   = request.form.get('email', '').strip()
    phone   = request.form.get('phone', '').strip()
    address = request.form.get('address', '').strip()

    if not name or not email:
        flash('이름과 이메일은 필수 항목입니다.', 'danger')
        return redirect(url_for('mypage.mypage') + '#profile')

    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                UPDATE Member
                SET Name=%s, Email=%s, Phone=%s, Address=%s
                WHERE MemberID=%s
            """, (name, email, phone, address, member_id))
        conn.commit()
        # ✅ 세션 이름 즉시 갱신 → 헤더에 바로 반영
        session['member_name'] = name
        flash('회원 정보가 수정되었습니다.', 'success')
    except Exception as e:
        conn.rollback()
        flash(f'오류: {str(e)}', 'danger')
    finally:
        conn.close()

    return redirect(url_for('mypage.mypage') + '#profile')


# ──────────────────────────────────────────────
#  POST /mypage/change-password
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/change-password', methods=['POST'])
@login_required
def change_password():
    member_id  = session['member_id']
    current_pw = request.form.get('current_pw', '')
    new_pw     = request.form.get('new_pw', '')
    confirm_pw = request.form.get('confirm_pw', '')

    if new_pw != confirm_pw:
        flash('새 비밀번호가 일치하지 않습니다.', 'danger')
        return redirect(url_for('mypage.mypage') + '#password')
    if len(new_pw) < 8:
        flash('비밀번호는 8자 이상이어야 합니다.', 'danger')
        return redirect(url_for('mypage.mypage') + '#password')

    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                SELECT MemberID FROM Member
                WHERE MemberID=%s AND Password=%s
            """, (member_id, current_pw))
            if not cur.fetchone():
                flash('현재 비밀번호가 올바르지 않습니다.', 'danger')
                return redirect(url_for('mypage.mypage') + '#password')

            cur.execute("UPDATE Member SET Password=%s WHERE MemberID=%s",
                        (new_pw, member_id))
        conn.commit()
        flash('비밀번호가 변경되었습니다.', 'success')
    except Exception as e:
        conn.rollback()
        flash(f'오류: {str(e)}', 'danger')
    finally:
        conn.close()

    return redirect(url_for('mypage.mypage') + '#password')


# ──────────────────────────────────────────────
#  POST /mypage/review/edit
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/review/edit', methods=['POST'])
@login_required
def edit_review():
    member_id = session['member_id']
    review_id = request.form.get('review_id')
    rating    = request.form.get('rating', 5)
    content   = request.form.get('content', '').strip()

    if not content:
        return jsonify({'success': False, 'error': '내용을 입력해주세요.'})

    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                UPDATE Review SET Rating=%s, Content=%s
                WHERE ReviewID=%s AND MemberID=%s
            """, (rating, content, review_id, member_id))
            if cur.rowcount == 0:
                return jsonify({'success': False, 'error': '수정 권한이 없습니다.'})
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/review/delete/<id>
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/review/delete/<int:review_id>', methods=['POST'])
@login_required
def delete_review(review_id):
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                DELETE FROM Review
                WHERE ReviewID=%s AND MemberID=%s
            """, (review_id, member_id))
            if cur.rowcount == 0:
                return jsonify({'success': False, 'error': '삭제 권한이 없습니다.'})
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/wishlist/remove/<book_id>
#  Cart에서 해당 책 삭제
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/wishlist/remove/<int:book_id>', methods=['POST'])
@login_required
def remove_wishlist(book_id):
    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor() as cur:
            cur.execute("""
                DELETE FROM Cart
                WHERE MemberID=%s AND BookID=%s
            """, (member_id, book_id))
        conn.commit()
        return jsonify({'success': True})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()


# ──────────────────────────────────────────────
#  POST /mypage/withdraw  — 회원 탈퇴
# ──────────────────────────────────────────────
@mypage_bp.route('/mypage/withdraw', methods=['POST'])
@login_required
def withdraw():
    member_id = session['member_id']
    data      = request.get_json()
    password  = (data or {}).get('password', '')

    if not password:
        return jsonify({'success': False, 'error': '비밀번호를 입력해주세요.'})

    conn = get_db()
    try:
        with conn.cursor() as cur:

            # 1) 비밀번호 확인
            cur.execute("""
                SELECT MemberID FROM Member
                WHERE MemberID = %s AND Password = %s
            """, (member_id, password))
            if not cur.fetchone():
                return jsonify({'success': False, 'error': '비밀번호가 올바르지 않습니다.'})

            # 2) 관련 데이터 삭제 (외래키 순서 주의)
            # PurchaseItem → Purchase → Cart → Review → Member
            cur.execute("""
                DELETE pi FROM PurchaseItem pi
                JOIN Purchase p ON pi.OrderID = p.OrderID
                WHERE p.MemberID = %s
            """, (member_id,))

            cur.execute("DELETE FROM Purchase WHERE MemberID = %s", (member_id,))
            cur.execute("DELETE FROM Cart    WHERE MemberID = %s", (member_id,))
            cur.execute("DELETE FROM Review  WHERE MemberID = %s", (member_id,))
            cur.execute("DELETE FROM QnA     WHERE MemberID = %s", (member_id,))
            cur.execute("DELETE FROM Member  WHERE MemberID = %s", (member_id,))

        conn.commit()

        # 3) 세션 초기화
        session.clear()
        return jsonify({'success': True})

    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()