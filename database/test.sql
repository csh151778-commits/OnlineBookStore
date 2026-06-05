-- -- ============================================================
-- -- DB 테스트 쿼리
-- -- ============================================================

-- USE bookstore;

-- -- 1. 책 목록 전체 보기
-- SELECT * FROM Book;

-- 2. 회원 목록 보기
SELECT MemberID, Password, Name, Email, Role FROM Member;

-- -- 3. 책 상세 조회 (JOIN 테스트) ⭐
-- SELECT 
--     B.BookID,
--     B.Title,
--     A.AuthorName AS '저자',
--     P.PublisherName AS '출판사',
--     B.Price AS '가격',
--     B.Stock AS '재고',
--     B.Genre AS '장르',
--     B.StorageSlot AS '보관함위치'
-- FROM Book B
-- JOIN Author A ON B.AuthorID = A.AuthorID
-- JOIN Publisher P ON B.PublisherID = P.PublisherID;

-- -- 4. 특정 회원의 주문 내역
-- SELECT 
--     P.OrderID,
--     P.PurchaseDate,
--     P.TotalPrice,
--     P.OrderStatus,
--     P.HardwareStatus,
--     B.Title AS '도서',
--     PI.Quantity AS '수량',
--     PI.Price AS '주문가격'
-- FROM Purchase P
-- JOIN PurchaseItem PI ON P.OrderID = PI.OrderID
-- JOIN Book B ON PI.BookID = B.BookID
-- WHERE P.MemberID = 'user01';

-- -- 5. 책별 평균 별점
-- SELECT 
--     B.Title,
--     AVG(R.Rating) AS '평균별점',
--     COUNT(R.ReviewID) AS '리뷰수'
-- FROM Book B
-- LEFT JOIN Review R ON B.BookID = R.BookID
-- GROUP BY B.BookID, B.Title;



