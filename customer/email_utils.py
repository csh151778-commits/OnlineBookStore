import smtplib
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from datetime import datetime

# ============================================================
#  ✉️  BookNest Gmail 설정  —  아래 두 줄만 본인 정보로 교체
# ============================================================
GMAIL_ADDRESS  = "GYUBOBOOK@gmail.com"   # 발신 Gmail 주소
GMAIL_APP_PW   = "xuek wqdq geoz wuah"        # 앱 비밀번호 (공백 포함 그대로)
# ============================================================


def send_email(to_email: str, subject: str, body: str) -> bool:
    """
    범용 텍스트 이메일 발송 (임시 비밀번호 등)
    """
    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"]    = f"KYUBOBOOK <{GMAIL_ADDRESS}>"
    msg["To"]      = to_email
    html_body = f"""
    <html><body style="font-family:'Malgun Gothic',sans-serif;background:#f4f1eb;padding:40px;">
      <div style="max-width:500px;margin:0 auto;background:white;border-radius:12px;padding:40px;box-shadow:0 4px 20px rgba(0,0,0,0.08);">
        <div style="font-size:24px;font-weight:800;color:#1A1410;margin-bottom:24px;">📚 KYUBOBOOK</div>
        <div style="font-size:14px;color:#555;line-height:1.8;white-space:pre-line;">{body}</div>
        <div style="margin-top:32px;padding-top:20px;border-top:1px solid #E8E0D5;font-size:12px;color:#aaa;">
          본 메일은 KYUBOBOOK에서 자동 발송됩니다.
        </div>
      </div>
    </body></html>
    """
    msg.attach(MIMEText(html_body, "html", "utf-8"))
    try:
        with smtplib.SMTP_SSL("smtp.gmail.com", 465) as server:
            server.login(GMAIL_ADDRESS, GMAIL_APP_PW.replace(" ", ""))
            server.sendmail(GMAIL_ADDRESS, to_email, msg.as_string())
        print(f"[KYUBOBOOK Email] ✅ 발송 성공 → {to_email}")
        return True
    except smtplib.SMTPAuthenticationError:
        print("[KYUBOBOOK Email] ❌ 인증 실패 — 앱 비밀번호를 확인하세요")
        return False
    except Exception as e:
        print(f"[KYUBOBOOK Email] ❌ 발송 실패: {e}")
        return False


def send_welcome_email(to_email: str, name: str, email: str,
                       phone: str, join_date: str = None) -> bool:
    """
    회원가입 환영 이메일 발송
    
    Args:
        to_email  : 수신자 이메일 (Member.Email)
        name      : 회원 이름   (Member.Name)
        email     : 가입 이메일 (표시용)
        phone     : 전화번호    (Member.Phone)
        join_date : 가입일 (없으면 오늘)
    
    Returns:
        True  : 발송 성공
        False : 발송 실패
    """
    if join_date is None:
        join_date = datetime.now().strftime("%Y년 %m월 %d일")

    subject = f"[BookNest] {name}님, 회원가입을 환영합니다! 🎉"
    html_body = _build_html(name, email, phone, join_date)

    msg = MIMEMultipart("alternative")
    msg["Subject"] = subject
    msg["From"]    = f"BookNest <{GMAIL_ADDRESS}>"
    msg["To"]      = to_email
    msg.attach(MIMEText(html_body, "html", "utf-8"))

    try:
        with smtplib.SMTP_SSL("smtp.gmail.com", 465) as server:
            server.login(GMAIL_ADDRESS, GMAIL_APP_PW.replace(" ", ""))
            server.sendmail(GMAIL_ADDRESS, to_email, msg.as_string())
        print(f"[BookNest Email] ✅ 발송 성공 → {to_email}")
        return True
    except smtplib.SMTPAuthenticationError:
        print("[BookNest Email] ❌ 인증 실패 — 앱 비밀번호를 확인하세요")
        return False
    except Exception as e:
        print(f"[BookNest Email] ❌ 발송 실패: {e}")
        return False


def _build_html(name: str, email: str, phone: str, join_date: str) -> str:
    """환영 이메일 HTML 템플릿"""
    return f"""
<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>BookNest 환영 이메일</title>
</head>
<body style="margin:0;padding:0;background:#f4f1eb;font-family:'Apple SD Gothic Neo',
             'Malgun Gothic',sans-serif;">

  <!-- 전체 래퍼 -->
  <table width="100%" cellpadding="0" cellspacing="0"
         style="background:#f4f1eb;padding:40px 0;">
    <tr><td align="center">

      <!-- 카드 -->
      <table width="560" cellpadding="0" cellspacing="0"
             style="background:#ffffff;border-radius:16px;
                    box-shadow:0 4px 24px rgba(0,0,0,0.08);overflow:hidden;">

        <!-- ── 헤더 ── -->
        <tr>
          <td style="background:linear-gradient(135deg,#2c3e50 0%,#3d5a80 100%);
                     padding:40px 48px 32px;text-align:center;">
            <div style="font-size:32px;font-weight:800;color:#ffffff;
                        letter-spacing:-0.5px;">📚 BookNest</div>
            <div style="font-size:13px;color:rgba(255,255,255,0.65);
                        margin-top:4px;letter-spacing:2px;">YOUR BOOK UNIVERSE</div>
          </td>
        </tr>

        <!-- ── 본문 ── -->
        <tr>
          <td style="padding:40px 48px 32px;">

            <p style="font-size:22px;font-weight:700;color:#1a1a2e;margin:0 0 8px;">
              {name}님, 환영합니다! 🎉
            </p>
            <p style="font-size:15px;color:#555;line-height:1.7;margin:0 0 28px;">
              BookNest 가족이 되어주셔서 감사합니다.<br>
              수천 권의 책이 여러분을 기다리고 있어요.
            </p>

            <!-- 가입 정보 박스 -->
            <table width="100%" cellpadding="0" cellspacing="0"
                   style="background:#f8f7f4;border-radius:10px;
                          border:1px solid #e8e4db;margin-bottom:28px;">
              <tr>
                <td style="padding:24px 28px;">
                  <div style="font-size:11px;font-weight:700;color:#888;
                              letter-spacing:2px;margin-bottom:16px;">
                    ── 가입 정보 요약
                  </div>
                  {_info_row("👤 이름",    name)}
                  {_info_row("📧 이메일",  email)}
                  {_info_row("📱 연락처",  phone if phone else "미입력")}
                  {_info_row("📅 가입일",  join_date, last=True)}
                </td>
              </tr>
            </table>

            <!-- CTA 버튼 -->
            <table width="100%" cellpadding="0" cellspacing="0">
              <tr>
                <td align="center">
                  <a href="http://localhost:5000"
                     style="display:inline-block;background:linear-gradient(135deg,#3d5a80,#2c3e50);
                            color:#ffffff;text-decoration:none;padding:14px 36px;
                            border-radius:50px;font-size:15px;font-weight:700;
                            letter-spacing:0.3px;">
                    📚 지금 BookNest 둘러보기
                  </a>
                </td>
              </tr>
            </table>

          </td>
        </tr>

        <!-- ── 푸터 ── -->
        <tr>
          <td style="background:#f8f7f4;border-top:1px solid #e8e4db;
                     padding:20px 48px;text-align:center;">
            <p style="font-size:12px;color:#aaa;margin:0;line-height:1.6;">
              본 메일은 BookNest 회원가입 시 자동 발송됩니다.<br>
              문의: booknest@example.com
            </p>
          </td>
        </tr>

      </table>
    </td></tr>
  </table>

</body>
</html>
"""


def _info_row(label: str, value: str, last: bool = False) -> str:
    border = "" if last else "border-bottom:1px solid #e8e4db;"
    return f"""
    <table width="100%" cellpadding="0" cellspacing="0"
           style="{border}margin-bottom:{'0' if last else '12px'};
                  padding-bottom:{'0' if last else '12px'};">
      <tr>
        <td width="90" style="font-size:13px;color:#888;vertical-align:top;
                              padding-top:1px;">{label}</td>
        <td style="font-size:14px;font-weight:600;color:#1a1a2e;">{value}</td>
      </tr>
    </table>
    """