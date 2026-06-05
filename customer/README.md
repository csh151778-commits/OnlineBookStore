# 📚 BookNest - 온라인 주문 및 재고관리 시스템

## 프로젝트 구조

```
project2/
├── app.py                      ← Flask 메인 서버 (포트 5000)
├── requirements.txt            ← 필요 패키지
├── static/
│   ├── css/                    ← CSS 파일
│   ├── js/                     ← JS 파일
│   └── images/                 ← 이미지 파일
└── templates/
    ├── index.html              ← 홈페이지 ✅
    ├── login.html              ← 로그인/회원가입
    ├── register.html           ← 회원가입
    ├── book_detail.html        ← 책 상세 + 구매
    ├── cart.html               ← 장바구니/결제
    ├── mypage.html             ← 마이페이지
    ├── qna.html                ← Q&A
    ├── search.html             ← 검색/카테고리
    └── admin/
        └── index.html          ← 관리자 대시보드
```

## 실행 방법

### 1. 패키지 설치
```bash
pip install -r requirements.txt
```

### 2. 서버 실행
```bash
python app.py
```

### 3. 브라우저에서 확인
```
http://localhost:5000
```

## 기술 스택
- **Frontend**: HTML, CSS, JavaScript
- **Backend**: Python Flask
- **통신**: HTTP (포트 5000), TCP/IP (포트 4001, 라즈베리파이)
