# ============================================================
#  search_route.py  (v3 - 실제 DB 스키마 기준)
#  컬럼명 기준: bookstore_schema.sql (2026.04.22)
# ============================================================

from flask import Blueprint, render_template, request, session, jsonify
import pymysql

search_bp = Blueprint('search', __name__)

PER_PAGE = 15

def get_db():
    from app import DB_CONFIG
    return pymysql.connect(**DB_CONFIG)


# ── Jinja2 커스텀 필터: 검색어 강조 ──
def highlight_query(text, query):
    if not query or not text:
        return text
    import re
    pattern = re.compile(re.escape(query), re.IGNORECASE)
    return pattern.sub(
        lambda m: f'<mark class="highlight">{m.group()}</mark>',
        str(text)
    )


# ──────────────────────────────────────────────
#  /search  (GET: 검색 결과)
# ──────────────────────────────────────────────
@search_bp.route('/search')
def search():
    query        = request.args.get('q', '').strip()
    search_type  = request.args.get('type', '전체')
    sort         = request.args.get('sort', 'relevance')
    page         = max(int(request.args.get('page', 1)), 1)
    price_min    = request.args.get('price_min', '')
    price_max    = request.args.get('price_max', '')
    selected_year       = request.args.get('year', '')
    selected_categories = request.args.getlist('category')

    books       = []
    total       = 0
    categories  = []
    total_pages = 1

    if query:
        conn = get_db()
        try:
            with conn.cursor(pymysql.cursors.DictCursor) as cur:

                # ── WHERE 조건 빌드 ──
                where_clauses = []
                params = []

                # 검색 타입별 조건
                if search_type == '도서명':
                    where_clauses.append("b.Title LIKE %s")
                    params.append(f'%{query}%')
                elif search_type == '저자명':
                    where_clauses.append("b.AuthorName LIKE %s")
                    params.append(f'%{query}%')
                elif search_type == '출판사':
                    where_clauses.append("b.PublisherName LIKE %s")
                    params.append(f'%{query}%')
                elif search_type == 'ISBN':
                    # DB에 ISBN 컬럼 없으므로 도서명으로 대체
                    where_clauses.append("b.Title LIKE %s")
                    params.append(f'%{query}%')
                else:  # 전체
                    where_clauses.append("""
                        (b.Title LIKE %s
                         OR b.AuthorName LIKE %s
                         OR b.PublisherName LIKE %s)
                    """)
                    params.extend([f'%{query}%'] * 3)

                # 가격 필터
                if price_min:
                    where_clauses.append("b.Price >= %s")
                    params.append(int(price_min))
                if price_max:
                    where_clauses.append("b.Price <= %s")
                    params.append(int(price_max))

                # 출판연도 필터 (Book.PublishDate)
                if selected_year:
                    year_map = {
                        '2024': "b.PublishDate >= '2024-01-01'",
                        '2022': "b.PublishDate BETWEEN '2022-01-01' AND '2023-12-31'",
                        '2020': "b.PublishDate BETWEEN '2020-01-01' AND '2021-12-31'",
                        '2019': "b.PublishDate < '2020-01-01'",
                    }
                    if selected_year in year_map:
                        where_clauses.append(year_map[selected_year])

                # 장르 필터 (Book.Genre)
                if selected_categories:
                    placeholders = ','.join(['%s'] * len(selected_categories))
                    where_clauses.append(f"b.Genre IN ({placeholders})")
                    params.extend(selected_categories)

                where_sql = ("WHERE " + " AND ".join(where_clauses)) if where_clauses else ""

                # ── 정렬 ──
                order_map = {
                    'relevance':  "b.Title ASC",
                    'newest':     "b.PublishDate DESC",
                    'price_asc':  "b.Price ASC",
                    'price_desc': "b.Price DESC",
                    'bestseller': "b.Stock ASC",
                }
                order_sql = order_map.get(sort, "b.Title ASC")

                # ── 전체 개수 ──
                cur.execute(f"""
                    SELECT COUNT(DISTINCT b.BookID) AS cnt
                    FROM Book b
                    {where_sql}
                """, params)
                total = cur.fetchone()['cnt']

                offset      = (page - 1) * PER_PAGE
                total_pages = max((total + PER_PAGE - 1) // PER_PAGE, 1)

                # ── 도서 목록 ──
                cur.execute(f"""
                    SELECT
                        b.BookID                   AS book_id,
                        b.Title                    AS title,
                        b.Price                    AS price,
                        b.Stock                    AS stock,
                        b.Genre                    AS category,
                        b.PublishDate              AS pub_date,
                        b.AuthorName               AS author,
                        b.PublisherName            AS publisher,
                        b.Description              AS description,
                        b.ImageURL                 AS image_url,
                        b.IsAdult                  AS is_adult,
                        AVG(r.Rating)              AS avg_rating,
                        COUNT(DISTINCT r.ReviewID) AS review_count
                    FROM Book b
                    LEFT JOIN Review r ON b.BookID = r.BookID
                    {where_sql}
                    GROUP BY
                        b.BookID, b.Title, b.Price, b.Stock, b.Genre,
                        b.PublishDate, b.AuthorName, b.PublisherName,
                        b.Description, b.ImageURL, b.IsAdult
                    ORDER BY {order_sql}
                    LIMIT %s OFFSET %s
                """, params + [PER_PAGE, offset])
                books = cur.fetchall()

                for book in books:
                    if book.get('pub_date'):
                        book['pub_date'] = str(book['pub_date'])[:7]  # YYYY-MM

                # ── 장르 집계 (사이드바) ──
                cur.execute(f"""
                    SELECT b.Genre AS name, COUNT(*) AS count
                    FROM Book b
                    {where_sql}
                    AND b.Genre IS NOT NULL
                    GROUP BY b.Genre
                    ORDER BY count DESC
                    LIMIT 10
                """, params)
                categories = cur.fetchall()

        finally:
            conn.close()

    return render_template(
        'search.html',
        query=query,
        search_type=search_type,
        sort=sort,
        page=page,
        books=books,
        total=total,
        total_pages=total_pages,
        categories=categories,
        price_min=price_min,
        price_max=price_max,
        selected_year=selected_year,
        selected_categories=selected_categories,
    )


# ──────────────────────────────────────────────
#  /wishlist/toggle/<book_id>  (POST: 찜 토글)
#  Cart에 is_wishlist 없으므로 장바구니 담기로 대체
# ──────────────────────────────────────────────
@search_bp.route('/wishlist/toggle/<int:book_id>', methods=['POST'])
def toggle_wishlist(book_id):
    if 'member_id' not in session:
        return jsonify({'error': '로그인 필요'}), 401

    member_id = session['member_id']
    conn = get_db()
    try:
        with conn.cursor(pymysql.cursors.DictCursor) as cur:
            cur.execute("""
                SELECT CartID FROM Cart
                WHERE MemberID = %s AND BookID = %s
            """, (member_id, book_id))
            existing = cur.fetchone()

            if existing:
                cur.execute("DELETE FROM Cart WHERE MemberID=%s AND BookID=%s",
                            (member_id, book_id))
                wished = False
            else:
                cur.execute("""
                    INSERT INTO Cart (MemberID, BookID, Quantity)
                    VALUES (%s, %s, 1)
                """, (member_id, book_id))
                wished = True

        conn.commit()
        return jsonify({'success': True, 'wished': wished})
    except Exception as e:
        conn.rollback()
        return jsonify({'success': False, 'error': str(e)})
    finally:
        conn.close()