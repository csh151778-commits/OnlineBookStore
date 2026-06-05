-- ============================================================
-- 01_schema.sql
-- 온라인 서점 DB 생성 스크립트 (MySQL)
-- Version: v3 (2026.04.23)
-- 수정사항: 
--   - Author/Publisher 테이블 제거 → Book 속성으로 통합
--   - BookDetail 테이블 제거 → Book 속성으로 통합
--   - Cart.AddedAt 제거
--   - PurchaseItem.ItemID 제거 → 복합키 (OrderID, BookID)
--   - Book 장르별 카테고리 구분 (국내/국외)
-- ============================================================

-- ============================================================
-- 1. Member (회원)
-- ============================================================
CREATE TABLE Member (
    MemberID    VARCHAR(50)   NOT NULL,
    Password    VARCHAR(255)  NOT NULL,
    Name        VARCHAR(50)   NOT NULL,
    Address     VARCHAR(255),
    Email       VARCHAR(100)  UNIQUE,
    Birth       DATE,
    Phone       VARCHAR(20),
    Role             VARCHAR(10)   NOT NULL DEFAULT 'user',
    IsAdultVerified  BOOLEAN       NOT NULL DEFAULT FALSE, -- 성인인증 여부
    CreatedAt        DATETIME      DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (MemberID),
    CONSTRAINT chk_member_role CHECK (Role IN ('user', 'admin'))
) ENGINE=InnoDB;


-- ============================================================
-- 2. Book (도서 — 저자/출판사/상세 모두 통합)
-- ============================================================
CREATE TABLE Book (
    BookID        INT           NOT NULL AUTO_INCREMENT,
    Title         VARCHAR(200)  NOT NULL,
    AuthorName    VARCHAR(100)  NOT NULL,
    PublisherName VARCHAR(100)  NOT NULL,
    Price         INT           NOT NULL,
    Stock         INT           NOT NULL DEFAULT 0,
    PublishDate   DATE,
    Category      VARCHAR(20)   NOT NULL DEFAULT '국내',  -- '국내' or '국외'
    Genre         VARCHAR(50)   NOT NULL,
    IsAdult       BOOLEAN       NOT NULL DEFAULT FALSE,
    StorageSlot   VARCHAR(10),
    Description   TEXT,
    ImageURL      VARCHAR(500),

    PRIMARY KEY (BookID),
    CONSTRAINT chk_book_price    CHECK (Price >= 0),
    CONSTRAINT chk_book_stock    CHECK (Stock >= 0),
    CONSTRAINT chk_book_category CHECK (Category IN ('국내', '국외'))
) ENGINE=InnoDB;


-- ============================================================
-- 3. Cart (장바구니) — AddedAt 제거
-- ============================================================
CREATE TABLE Cart (
    CartID     INT          NOT NULL AUTO_INCREMENT,
    MemberID   VARCHAR(50)  NOT NULL,
    BookID     INT          NOT NULL,
    Quantity   INT          NOT NULL DEFAULT 1,

    PRIMARY KEY (CartID),
    CONSTRAINT fk_cart_member FOREIGN KEY (MemberID)
        REFERENCES Member(MemberID) ON DELETE CASCADE,
    CONSTRAINT fk_cart_book   FOREIGN KEY (BookID)
        REFERENCES Book(BookID) ON DELETE CASCADE,
    CONSTRAINT chk_cart_qty   CHECK (Quantity > 0)
) ENGINE=InnoDB;


-- ============================================================
-- 4. Purchase (주문)
-- ============================================================
CREATE TABLE Purchase (
    OrderID         INT          NOT NULL AUTO_INCREMENT,
    MemberID        VARCHAR(50)  NOT NULL,
    PurchaseDate    DATETIME     DEFAULT CURRENT_TIMESTAMP,
    TotalPrice      INT          NOT NULL,
    OrderStatus     VARCHAR(20)  NOT NULL DEFAULT '결제완료',
    HardwareStatus  VARCHAR(20)  NOT NULL DEFAULT '대기',
    TrackingNumber  VARCHAR(50),

    PRIMARY KEY (OrderID),
    CONSTRAINT fk_purchase_member FOREIGN KEY (MemberID)
        REFERENCES Member(MemberID),
    CONSTRAINT chk_purchase_total CHECK (TotalPrice >= 0)
) ENGINE=InnoDB;


-- ============================================================
-- 5. PurchaseItem (주문 상세) — ItemID 제거, 복합키 사용
-- ============================================================
CREATE TABLE PurchaseItem (
    OrderID   INT  NOT NULL,
    BookID    INT  NOT NULL,
    Quantity  INT  NOT NULL,
    Price     INT  NOT NULL,  -- 주문 시점 가격 스냅샷

    PRIMARY KEY (OrderID, BookID),
    CONSTRAINT fk_pitem_order FOREIGN KEY (OrderID)
        REFERENCES Purchase(OrderID) ON DELETE CASCADE,
    CONSTRAINT fk_pitem_book  FOREIGN KEY (BookID)
        REFERENCES Book(BookID),
    CONSTRAINT chk_pitem_qty   CHECK (Quantity > 0),
    CONSTRAINT chk_pitem_price CHECK (Price >= 0)
) ENGINE=InnoDB;


-- ============================================================
-- 6. Review (리뷰)
-- ============================================================
CREATE TABLE Review (
    ReviewID   INT          NOT NULL AUTO_INCREMENT,
    MemberID   VARCHAR(50)  NOT NULL,
    BookID     INT          NOT NULL,
    Rating     INT          NOT NULL,
    Content    TEXT,
    CreatedAt  DATETIME     DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (ReviewID),
    CONSTRAINT fk_review_member FOREIGN KEY (MemberID)
        REFERENCES Member(MemberID) ON DELETE CASCADE,
    CONSTRAINT fk_review_book   FOREIGN KEY (BookID)
        REFERENCES Book(BookID) ON DELETE CASCADE,
    CONSTRAINT chk_review_rating CHECK (Rating BETWEEN 1 AND 5)
) ENGINE=InnoDB;


-- ============================================================
-- 7. QnA (질문/답변)
-- ============================================================
CREATE TABLE QnA (
    QnAID      INT          NOT NULL AUTO_INCREMENT,
    MemberID   VARCHAR(50)  NOT NULL,
    BookID     INT          NOT NULL,
    Question   TEXT         NOT NULL,
    Answer     TEXT,
    CreatedAt  DATETIME     DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (QnAID),
    CONSTRAINT fk_qna_member FOREIGN KEY (MemberID)
        REFERENCES Member(MemberID) ON DELETE CASCADE,
    CONSTRAINT fk_qna_book   FOREIGN KEY (BookID)
        REFERENCES Book(BookID) ON DELETE CASCADE
) ENGINE=InnoDB;


-- ============================================================
-- 성능 향상용 인덱스
-- ============================================================
CREATE INDEX idx_book_genre        ON Book(Genre);
CREATE INDEX idx_book_category     ON Book(Category);
CREATE INDEX idx_book_author       ON Book(AuthorName);
CREATE INDEX idx_book_publisher    ON Book(PublisherName);
CREATE INDEX idx_cart_member       ON Cart(MemberID);
CREATE INDEX idx_purchase_member   ON Purchase(MemberID);
CREATE INDEX idx_purchase_date     ON Purchase(PurchaseDate);
CREATE INDEX idx_review_book       ON Review(BookID);
CREATE INDEX idx_qna_book          ON QnA(BookID);


-- ============================================================
-- 끝! 
-- 다음으로 02_sample_data.sql, 03_books_data.sql 실행
-- ============================================================
