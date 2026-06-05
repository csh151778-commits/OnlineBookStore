# 📚 MFC를 활용한 온라인 서점 통합 주문·출고 시스템

> 웹 주문 → DB 재고 검증 → 실물 자동 출고까지 하나로 연결한 풀스택 + 하드웨어 프로젝트
> Flask · MySQL · MFC · C# WinForms · Raspberry Pi


시연 영상 : https://youtu.be/Ca0WswE9EAg
---

## 프로젝트 소개

온라인으로 들어온 주문이 화면 위에서만 끝나지 않고, 실제 물류 출고 장치까지 끊김 없이 이어지는 흐름을 직접 만들어보고 싶었습니다. 그래서 고객이 웹에서 도서를 검색·주문하면, 데이터베이스 재고가 실시간으로 검증되고, 유효한 주문은 컨베이어·서보·스텝모터로 구성된 실물 출고 장치까지 자동으로 연결되도록 시스템 전체를 설계했습니다.

웹(Flask), 데이터베이스(MySQL), 관리자 데스크톱(MFC), 하드웨어 제어(C# WinForms), 그리고 실제 GPIO를 구동하는 라즈베리파이까지 — 서로 다른 4개 계층을 하나의 주문 파이프라인으로 묶는 것이 이 프로젝트의 핵심 목표였습니다.


* 관리자 MFC

<img width="700" height="450" alt="image" src="https://github.com/user-attachments/assets/f02f5f9e-e5ca-489e-9c08-2e2ae3200ff4" />

* 고객 홈페이지 Flask

<img width="700" height="450" alt="image" src="https://github.com/user-attachments/assets/a347b0c2-7ed9-458c-9ca5-4cc96f3d7967" />


*하드웨어 제어 시스템

<img width="700" height="450" alt="image" src="https://github.com/user-attachments/assets/2a3f0f6d-45b3-4b34-9431-38e4340fed35" />


*하드웨어 물류 창고

<img width="500" height="800" alt="image" src="https://github.com/user-attachments/assets/1137e5a3-7d4e-4a01-bceb-ecf40d9ea4c0" />






---

## 시스템 구성도

```
┌─────────────────────────────────────────────────────────────┐
│                        고객 (웹 브라우저)                       │
└───────────────────────────────┬─────────────────────────────┘
                                 │ HTTP
                  ┌──────────────▼───────────────┐
                  │     Flask 웹 서버 (app.py)      │
                  │  도서검색 · 장바구니 · 주문/결제  │
                  │  재고 2중 검증 + 동시성 가드      │
                  └──────────────┬───────────────┘
                                 │ INSERT Purchase
                                 │ (OrderStatus=결제완료 / HardwareStatus=대기)
                  ┌──────────────▼───────────────┐
                  │       MySQL ― bookstore        │◄──── 관리자 (MFC)
                  │  Member·Book·Cart·Purchase     │   주문/재고 조회·관리
                  │  PurchaseItem·Review·QnA       │   (CColorListCtrl)
                  └──────────────┬───────────────┘
                                 │ 3초 폴링 (결제완료 & 대기 주문 탐지)
                  ┌──────────────▼───────────────┐
                  │   C# 하드웨어 제어 (Form1.cs)   │
                  │  주문 자동 감시 · 수동 제어판     │
                  │  config.xml · CSV 로그           │
                  └───────┬───────────────┬───────┘
                   SSH(22) │       TCP(5001)│ 명령            ▲
                          │                 │                │ WebView2
                  ┌───────▼─────────────────▼───────┐        │ (5000/video)
                  │       Raspberry Pi (main.py)     │────────┘
                  │  TCP 명령서버(5001)·영상서버(5000) │
                  └────────────────┬────────────────┘
                                   │ GPIO
        ┌──────────────────────────┼──────────────────────────┐
        ▼                          ▼                           ▼
  스텝모터(슬롯 A/B/C)        서보모터(배출 0°↔180°)       컨베이어 + 카메라
```

<img width="500" height="800" alt="image" src="https://github.com/user-attachments/assets/1137e5a3-7d4e-4a01-bceb-ecf40d9ea4c0" />

---

## 주문 → 출고 데이터 흐름

각 계층은 직접 통신하지 않고 **MySQL의 `Purchase` 테이블을 공유 상태**로 사용합니다. 주문의 진행 상태는 `HardwareStatus` 컬럼으로 표현되며, 다음 순서로 흐릅니다.

| 단계 | 주체 | 동작 | OrderStatus | HardwareStatus |
|------|------|------|-------------|----------------|
| ① 주문 | Flask | 재고 검증 후 `Purchase` INSERT | `결제완료` | `대기` |
| ② 감지 | C# 자동 감시 | 3초 폴링으로 `결제완료 & 대기` 주문 탐지 | `결제완료` | `대기` → `처리중` |
| ③ 출고 | C# → Pi | 도서 `StorageSlot`(A/B/C)로 `ORDER:slot` 전송 | `결제완료` | `처리중` |
| ④ 완료 | C# | Pi 배출 확인 후 상태 갱신 | `결제완료` | `완료` |
| ✖ 실패 | C# | Pi 응답 실패 시 롤백 | `결제완료` | `대기` |

> C#은 도서의 `StorageSlot` 앞글자(예: `A4` → `A`)로 출고 슬롯을 판단합니다. 슬롯 정보가 없는(`StorageSlot IS NULL`) 주문은 자동 출고 대상에서 제외됩니다.


<img width="500" height="800" alt="image" src="https://github.com/user-attachments/assets/1137e5a3-7d4e-4a01-bceb-ecf40d9ea4c0" />
---

## 주요 기능

### 🛒 웹 (Flask)

고객이 사용하는 온라인 서점 프론트입니다. 검색부터 주문, 마이페이지 관리까지 실제 서비스 흐름을 구현했습니다.

- **도서 검색** — 도서명 / 저자명 / 출판사 / 전체 검색, 가격·출판연도·장르 필터, 정렬(관련도/신간/가격/베스트셀러), 검색어 하이라이트, 자동완성 API
- **장바구니** — 담기 / 수량 변경 / 개별·선택 삭제, 총액 실시간 계산
- **주문/결제** — 주문 시 재고를 **2중 검증**하고 부족하거나 초과 주문이면 자동 차단 (아래 별도 설명)
- **마이페이지** — 주문 내역 / 주문 상세 / 주문 취소, 리뷰 작성·수정·삭제, 위시리스트, 프로필·아바타·비밀번호 변경, 회원 탈퇴
- **리뷰 / QnA** — 도서별 별점 리뷰(1~5)와 질문/답변
- **베스트셀러 / 신간 / 카테고리** — 재고가 적은 순을 "많이 팔린 것"으로 간주해 베스트셀러 집계

* 도서 검색

<img width="1378" height="449" alt="image" src="https://github.com/user-attachments/assets/af95c3b6-e06e-4591-b3c8-ade23ee4ee2d" />


*장바구니
<img width="1569" height="418" alt="image" src="https://github.com/user-attachments/assets/4f9d193a-8425-492e-9263-d31039e2a1d1" />


*주문/결제
<img width="1569" height="605" alt="image" src="https://github.com/user-attachments/assets/03ad4484-f45c-426f-8afb-57b769e341e2" />



*마이페이지
<img width="1673" height="754" alt="image" src="https://github.com/user-attachments/assets/99b040db-27a9-44b3-9a45-8edab742186d" />



*리뷰/QnA
<img width="1674" height="632" alt="image" src="https://github.com/user-attachments/assets/416948ef-8961-4055-80ca-fcd1c20ba32f" />

*베스트셀
<img width="1675" height="664" alt="image" src="https://github.com/user-attachments/assets/a64c4ab3-d1d5-48cb-83b0-ee725a60812b" />



---

### 📦 재고 실시간 검증 + 동시성 처리

이 시스템에서 "재고 실시간 검증"이 단순한 표현이 아니라 실제로 동작하도록, 주문 생성 로직(`cart_order`)에 두 단계 방어를 넣었습니다.

**1단계 — 사전 재고 확인**
주문 생성 전에 각 도서의 재고를 조회해, 주문 수량보다 재고가 적으면 즉시 차단합니다.

```
'{도서명}' 재고가 부족합니다. (현재 재고: N권, 주문 수량: M권)
```

**2단계 — 조건부 차감으로 동시 주문 방지**
재고를 차감할 때 `AND Stock >= 수량` 조건을 걸고, 영향받은 행이 0이면(= 사전 확인과 차감 사이에 다른 주문이 재고를 가져간 경우) 예외를 던져 트랜잭션 전체를 롤백합니다.

```sql
UPDATE Book SET Stock = Stock - %s
WHERE BookID = %s AND Stock >= %s;
-- rowcount == 0 이면 동시 주문으로 판단 → 롤백
```

두 명이 마지막 1권을 동시에 주문해도 한 명만 성공하고, 재고가 최대개수 위로는 안눌립니다.

<img width="1391" height="761" alt="image" src="https://github.com/user-attachments/assets/dedd4ce2-d4e2-4602-ae18-269d83bd89cc" />


---

### 🗄️ 데이터베이스 (MySQL)

`utf8mb4` 기반 단일 스키마(`bookstore`)로 7개 테이블을 구성했습니다.

| 테이블 | 역할 | 비고 |
|--------|------|------|
| `Member` | 회원 | `Role`(user/admin) 권한 분기 |
| `Book` | 도서 | 저자·출판사·상세를 속성으로 통합, `StorageSlot`(출고 슬롯), `Stock` |
| `Cart` | 장바구니 | 회원별 담은 도서·수량 |
| `Purchase` | 주문 | `OrderStatus` / `HardwareStatus` 상태 관리 |
| `PurchaseItem` | 주문 상세 | 복합키 `(OrderID, BookID)`, 주문 시점 가격 스냅샷 |
| `Review` | 리뷰 | 별점 1~5 CHECK 제약 |
| `QnA` | 질문/답변 | 도서별 문의 |

`Book.StorageSlot`이 웹 주문과 실물 출고 슬롯(A/B/C)을 잇는 연결 고리입니다. 검색·정렬 성능을 위해 장르·카테고리·저자·출판사 등에 인덱스를 추가했습니다.

---

### 🖥️ 관리자 프로그램 (MFC)

공유 DB를 바라보는 관리자용 데스크톱 클라이언트입니다. 주문·재고 데이터를 색상으로 구분되는 목록(`CColorListCtrl`)으로 조회·관리합니다. 짝/홀수 행과 헤더에 갈색·크림 톤 테마를 적용해 가독성을 높였습니다.

> 관리자 메인 로직(`CAdminMainDlg`)은 별도 모듈로 구성되어 있습니다.

<img width="700" height="450" alt="image" src="https://github.com/user-attachments/assets/f02f5f9e-e5ca-489e-9c08-2e2ae3200ff4" />

---

### 🔧 하드웨어 제어 (C# WinForms)

출고 장치를 직접 제어하는 데스크톱 프로그램입니다. 라즈베리파이를 원격 기동하고, TCP로 명령을 보내며, 카메라 영상을 모니터링합니다.

- **라즈베리파이 자동 기동** — 프로그램 실행 시 SSH로 접속해 기존 프로세스를 정리하고 `main.py`를 백그라운드로 실행, 8초 대기 후 `STATUS`로 준비 상태 확인
- **주문 자동 감시** — 3초마다 DB를 폴링해 새 주문을 감지하고 해당 슬롯으로 자동 출고 명령 전송
- **수동 제어판** — 컨베이어 정·역방향(속도 1~100%, 시간 지정 자동 정지), 서보 0°/180°, 스텝모터 무한 정·역회전 및 정지, 슬롯 A/B/C 직접 이동, 전체 장치 상태 확인
- **카메라** — 로컬 카메라 캡처(AForge) + 라즈베리파이 영상 라이브 스트림(WebView2, `http://<Pi>:5000/video`)
- **설정/로그** — `config.xml`로 동작 파라미터 로드, CSV 3종으로 이력 저장
- **로그인** — 제어 프로그램 진입 시 인증, 3회 실패 시 경고

<img width="700" height="450" alt="image" src="https://github.com/user-attachments/assets/2a3f0f6d-45b3-4b34-9431-38e4340fed35" />

---

### 🍓 라즈베리파이 (main.py)

실제 GPIO를 구동하는 임베디드 측 서버입니다. C#이 SSH로 기동하며, 두 개의 서버를 동시에 엽니다.

- **TCP 명령 서버 (5001)** — C#의 명령을 받아 스텝모터·서보·컨베이어 구동, 현재 상태(`위치`, `processing`, `target`) 응답
- **영상 서버 (5000)** — 카메라 MJPEG 스트림(`/video`)을 C# WebView2로 송출

> 라즈베리파이 측 코드(`main.py`)는 본 저장소에 포함되어 있지 않습니다. (경로: `/home/<user>/work/Second_Proj/Camera_Step`)



---

## 하드웨어 명령 프로토콜

C# ↔ 라즈베리파이는 단순 텍스트 명령으로 통신합니다. C#이 UTF-8 명령 문자열을 보내면 Pi가 `OK:...` 또는 `ERR:...` 형식으로 응답하며, 연결·응답 각각 3초 타임아웃이 걸려 있습니다.

| 명령 | 동작 | 응답(예) |
|------|------|----------|
| `CONV_FWD:{속도}` | 컨베이어 정방향 (1~100%) | `OK:...` |
| `CONV_BACK:{속도}` | 컨베이어 역방향 | `OK:...` |
| `CONV_STOP` | 컨베이어 정지 | `OK:...` |
| `SERVO:{각도}` | 서보 0° 또는 180° (배출) | `OK:...` |
| `STEP_FWD` / `STEP_BACK` | 스텝모터 무한 정/역회전 | `OK:...` |
| `STEP_STOP` | 스텝모터 정지 | `OK:...` |
| `SET_SLOT:{A\|B\|C}` | 지정 슬롯으로 스텝모터 이동 | `OK:...` |
| `ORDER:{A\|B\|C}` | 슬롯 이동 + 서보 배출까지 출고 시퀀스 | `OK:...` |
| `STATUS` | 현재 상태 조회 | `OK:...위치=A processing=False target=대기` |

출고(`ORDER`) 시 C#은 `STATUS`를 0.5초 간격으로 폴링하며 **슬롯 이동 → 서보 배출(0°→180°) → 완료**를 추적합니다(최대 60초).

---

## 슬롯 구성

스텝모터가 세 개의 도서 슬롯을 회전 이동하며, 슬롯마다 색상으로 상태를 표시합니다.

| 슬롯 | 표시 색상 | 슬롯 간 스텝 간격(config) |
|------|-----------|---------------------------|
| A | 🔴 빨강 | A → B : 134 step |
| B | 🟡 노랑 | B → C : 134 step |
| C | 🔵 파랑 | C → A : 135 step |

---

## ⚙ 설정 파일 (config.xml)

스텝모터 슬롯 간격·딜레이와 컨베이어 기본 속도를 코드 수정 없이 XML로 조정할 수 있습니다. 현장에서 모터 보정이 필요할 때 값만 바꿔 대응하도록 분리했습니다. 파일이 없으면 기본값으로 자동 생성됩니다.

```xml
<?xml version="1.0" encoding="utf-8"?>
<Config>
  <Stepper>
    <SlotGapAB>134</SlotGapAB>
    <SlotGapBC>134</SlotGapBC>
    <SlotGapCA>135</SlotGapCA>
    <Delay>0.08</Delay>
  </Stepper>
  <Conveyor>
    <DefaultSpeed>70</DefaultSpeed>
  </Conveyor>
</Config>
```
*XML 기능 
<img width="1267" height="311" alt="image" src="https://github.com/user-attachments/assets/32dfa9a6-9655-46ac-9184-4da807c1a0e1" />


---







## 📁 로그 파일 (CSV)

하드웨어 제어 프로그램이 동작 이력을 세 개의 CSV로 나눠 저장합니다. 모두 `시간, 동작, 내용` 형식입니다.

| 파일 | 내용 | 기록 예시 |
|------|------|-----------|
| `status_log.csv` | 장치 상태 점검 | `상태확인, 전체 장치 정상` |
| `conveyor_log.csv` | 컨베이어·서보·스텝모터 구동 | `정방향구동, 속도=70%, 시간=1000ms` / `서보, 90도` |
| `video_log.csv` | 영상 시작·캡처·중지 | `캡처, capture_20260428_121026.png` |




*CSV 파일






<img width="723" height="471" alt="image" src="https://github.com/user-attachments/assets/f1f29442-5110-48f9-96ec-c8d8730f6472" />

<img width="698" height="338" alt="image" src="https://github.com/user-attachments/assets/055cf13d-c5f7-4538-965e-ddc869f6f528" />


<img width="814" height="457" alt="image" src="https://github.com/user-attachments/assets/d8753826-8278-4f77-a541-ad7676267324" />

---

## 🛠 기술 스택

| 계층 | 사용 기술 |
|------|----------|
| 웹 | Flask, Jinja2, PyMySQL |
| 데이터베이스 | MySQL (utf8mb4) |
| 관리자 | C++ / MFC (Visual Studio) |
| 하드웨어 제어 | C# / .NET WinForms, AForge.Video.DirectShow, WebView2, SSH.NET, MySql.Data |
| 임베디드 | Raspberry Pi (Python), TCP Socket, GPIO |
| 통신 | HTTP, TCP Socket, SSH, MJPEG 스트림 |
| 설정 / 로그 | XML, CSV |

---

## ▶ 실행 방법

### 1. 데이터베이스

```bash
mysql -u root -p < database/01_schema.sql
mysql -u root -p < database/02_sample_data.sql
mysql -u root -p < database/03_books_data_magazine_fixed.sql
```

### 2. 웹 서버 (Flask)

```bash
pip install flask pymysql
# app.py 의 DB_CONFIG 에 본인 DB 접속 정보 입력
python app.py
```

### 3. 관리자 프로그램 (MFC)

1. Visual Studio 2022에서 `BookstoreAdmin.sln` 열기
2. 빌드 후 실행 (DB 연동 정보는 관리자 모듈 설정에 맞춰 입력)

### 4. 하드웨어 제어 (C# WinForms)

1. Visual Studio에서 프로젝트 열기
2. NuGet 패키지 복원: `MySql.Data`, `AForge.Video`, `AForge.Video.DirectShow`, `Microsoft.Web.WebView2`, `SSH.NET`
3. `Form1.cs` 상단의 라즈베리파이 IP·SSH 계정·DB 접속 정보 입력
4. 빌드 후 실행 (실행 시 라즈베리파이를 자동 기동)

### 5. 라즈베리파이

지정 경로에서 `main.py`를 실행하면 TCP(5001)·영상(5000) 서버가 기동됩니다. (C# 실행 시 SSH로 자동 기동되도록 구성)

> ⚠ **보안**: DB 비밀번호·SSH 비밀번호·로그인 정보 등 자격증명은 소스에 하드코딩하지 말고 환경변수 또는 별도 설정 파일로 분리하세요.

---

## 🗂 폴더 구조

```
.
├── bookstore-project/          # Flask 웹
│   ├── app.py                  # 메인 (라우트, 주문/결제, 재고 검증)
│   ├── search_route.py         # 도서 검색 Blueprint
│   ├── mypage_routes.py        # 마이페이지 Blueprint
│   ├── email_utils.py
│   ├── templates/  static/
│   ├── requirements.txt
│   └── database/
│       ├── 01_schema.sql
│       ├── 02_sample_data.sql
│       └── 03_books_data_magazine_fixed.sql
│
├── Admin/                       # MFC 관리자
│   ├── BookstoreAdmin.sln
│   ├── CAdminMainDlg.cpp/.h     # 관리자 메인 로직
│   ├── BookstoreAdminDlg.cpp/.h
│   └── CColorListCtrl.cpp/.h    # 색상 리스트 컨트롤
│
├── Hardware_Management/         # C# 하드웨어 제어
│   ├── Form1.cs                 # 제어 메인 (TCP/SSH/DB감시/카메라)
│   ├── LoginForm.cs
│   ├── config.xml
│   ├── status_log.csv / conveyor_log.csv / video_log.csv
│   └── Program.cs
│
└── (Raspberry Pi)               # main.py — TCP·영상 서버 (별도 호스트)
```

---

## ⚠ 한계 및 개선 방향

**관리자 승인 단계 부재**
현재 C# 자동 감시는 `결제완료 / 대기` 주문을 곧바로 출고합니다. 즉 결제와 동시에 출고가 진행되어, 관리자가 출고를 검토·승인하는 단계가 파이프라인에 들어가 있지 않습니다. `승인됨` 같은 상태값을 추가하고 MFC가 이를 설정, C#이 이를 감시하도록 분리하면 검토 게이트를 만들 수 있습니다.

**출고 완료 판정 일관성**
자동 출고 경로는 명령 전송 후 고정 2초 뒤 `완료`로 표시합니다. 수동 경로는 `STATUS` 폴링으로 실제 배출을 확인하므로, 자동 경로도 폴링 기반 완료 확인으로 통일하는 것이 정확합니다.

**비상정지 미구현**
전체 장치를 한 번에 멈추는 비상정지 명령·버튼이 없습니다. 컨베이어·스텝모터 개별 정지만 가능하므로, `ALL_STOP` 같은 통합 정지 명령을 추가하는 것이 안전상 바람직합니다.

**자격증명 하드코딩**
DB·SSH·로그인 정보가 소스에 직접 들어 있습니다. 공개 배포 전 환경변수·설정 파일로 분리하고 git 히스토리에서도 제거해야 합니다.

**동시성 — 낙관적 방식**
재고 차감은 조건부 UPDATE + rowcount 검사로 음수를 막지만, 행 잠금(`SELECT ... FOR UPDATE`)이나 트랜잭션 격리 수준 조정은 적용하지 않았습니다. 트래픽이 높아지면 명시적 락 전략이 필요합니다.

**통신 보안·견고성**
라즈베리파이 TCP·영상 서버에 인증이 없고 단일 연결을 가정합니다. 실제 운영 환경에서는 인증·재연결·다중 클라이언트 처리가 필요합니다.

---

> 본 프로젝트는 학습 및 포트폴리오 목적으로 제작되었습니다.
