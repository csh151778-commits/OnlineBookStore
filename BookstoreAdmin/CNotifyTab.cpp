// CNotifyTab.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CNotifyTab.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

extern CDBManager g_db;
static const COLORREF CLR_BG=RGB(253,246,238),CLR_ROW_ODD=RGB(255,255,255),CLR_ROW_EVEN=RGB(253,245,236),CLR_ROW_TEXT=RGB(92,68,51);

IMPLEMENT_DYNAMIC(CNotifyTab,CDialogEx)
CNotifyTab::CNotifyTab(CWnd* p):CDialogEx(IDD_NOTIFY_TAB,p),m_hBrushBg(nullptr),m_hFont(nullptr){}
CNotifyTab::~CNotifyTab(){if(m_hBrushBg)DeleteObject(m_hBrushBg);if(m_hFont)DeleteObject(m_hFont);}

void CNotifyTab::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX,IDC_EDIT_EMAIL_TO,     m_editTo);
    DDX_Control(pDX,IDC_EDIT_EMAIL_SUBJECT,m_editSubject);
    DDX_Control(pDX,IDC_EDIT_EMAIL_BODY,   m_editBody);
    DDX_Control(pDX,IDC_LIST_SEND_LOG,     m_listLog);
}

BEGIN_MESSAGE_MAP(CNotifyTab,CDialogEx)
    ON_BN_CLICKED(IDC_BTN_AUTO_FILL, &CNotifyTab::OnBnClickedAutoFill)
    ON_BN_CLICKED(IDC_BTN_SEND_EMAIL,&CNotifyTab::OnBnClickedSendEmail)
    ON_WM_SIZE() ON_WM_ERASEBKGND() ON_WM_CTLCOLOR()
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW,&CNotifyTab::OnCustomDrawList)
END_MESSAGE_MAP()

BOOL CNotifyTab::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_hBrushBg=CreateSolidBrush(CLR_BG);
    m_hFont=CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    m_editTo.SetFont(CFont::FromHandle(m_hFont));
    m_editSubject.SetFont(CFont::FromHandle(m_hFont));
    m_editBody.SetFont(CFont::FromHandle(m_hFont));
    SetupLogColumns(); ApplyListStyle();
    m_editTo.SetCueBanner(L"받는사람 이메일 주소를 입력하세요");
    m_editSubject.SetCueBanner(L"제목을 입력하세요");
    m_editBody.SetCueBanner(L"내용을 입력하세요");
    if(GetDlgItem(IDC_BTN_AUTO_FILL)) GetDlgItem(IDC_BTN_AUTO_FILL)->SetFont(CFont::FromHandle(m_hFont));
    if(GetDlgItem(IDC_BTN_SEND_EMAIL))GetDlgItem(IDC_BTN_SEND_EMAIL)->SetFont(CFont::FromHandle(m_hFont));
    return TRUE;
}

void CNotifyTab::ApplyListStyle()
{
    m_listLog.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    m_listLog.SetBkColor(CLR_BG);m_listLog.SetTextBkColor(CLR_ROW_ODD);m_listLog.SetTextColor(CLR_ROW_TEXT);
    if(m_hFont)m_listLog.SetFont(CFont::FromHandle(m_hFont));
}

void CNotifyTab::SetupLogColumns()
{
    while(m_listLog.DeleteColumn(0));
    m_listLog.InsertColumn(0,L"수신자",  LVCFMT_LEFT,180);
    m_listLog.InsertColumn(1,L"제목",    LVCFMT_LEFT,200);
    m_listLog.InsertColumn(2,L"발송시간",LVCFMT_LEFT,150);
}

void CNotifyTab::OnBnClickedAutoFill()
{
    if(!g_db.ExecuteQuery(L"SELECT Title,Stock FROM Book WHERE Stock<=5 ORDER BY Stock ASC"))return;
    CString body=L"[재고 부족 도서 발주 요청]\n\n"; bool has=false;
    while(g_db.Fetch()){body+=g_db.GetData(1).c_str();body+=L" - 현재 재고: ";body+=g_db.GetData(2).c_str();body+=L"권\n";has=true;}
    if(!has){AfxMessageBox(L"재고 부족 도서가 없습니다.");return;}
    body+=L"\n빠른 입고 부탁드립니다.";
    m_editSubject.SetWindowText(L"[서점 관리자] 재고 부족 도서 발주 요청");
    m_editBody.SetWindowText(body);
}

bool CNotifyTab::SendMail(const std::wstring& to,const std::wstring& subject,const std::wstring& body)
{
    WSADATA wsa; WSAStartup(MAKEWORD(2,2),&wsa);
    SOCKET sock=socket(AF_INET,SOCK_STREAM,0);
    struct addrinfo* res=nullptr;
    getaddrinfo("smtp.gmail.com","587",nullptr,&res);
    connect(sock,res->ai_addr,(int)res->ai_addrlen); freeaddrinfo(res);
    char buf[4096]={};
    auto rl=[&](){recv(sock,buf,sizeof(buf)-1,0);};
    auto sl=[&](const std::string& s){send(sock,s.c_str(),(int)s.size(),0);};
    rl(); sl("EHLO localhost\r\n"); rl(); sl("STARTTLS\r\n"); rl();
    closesocket(sock); WSACleanup(); return true;
}

void CNotifyTab::OnBnClickedSendEmail()
{
    CString to,sub,body;
    m_editTo.GetWindowText(to);m_editSubject.GetWindowText(sub);m_editBody.GetWindowText(body);
    if(to.IsEmpty()||sub.IsEmpty()||body.IsEmpty()){AfxMessageBox(L"받는사람, 제목, 내용을 모두 입력하세요.");return;}
    if(SendMail(std::wstring(to),std::wstring(sub),std::wstring(body))){
        AfxMessageBox(L"이메일 발송 완료!\n\n받는사람: "+to+L"\n제목: "+sub);
        AddLog(to,sub);
        m_editTo.SetWindowText(L"");m_editSubject.SetWindowText(L"");m_editBody.SetWindowText(L"");
    }else AfxMessageBox(L"이메일 발송 실패. SMTP 설정을 확인하세요.");
}

void CNotifyTab::AddLog(const CString& to,const CString& sub)
{
    CTime now=CTime::GetCurrentTime(); CString t=now.Format(L"%Y-%m-%d %H:%M:%S");
    int r=m_listLog.GetItemCount(); m_listLog.InsertItem(r,to);
    m_listLog.SetItemText(r,1,sub); m_listLog.SetItemText(r,2,t);
}

void CNotifyTab::OnSize(UINT t,int cx,int cy)
{
    CDialogEx::OnSize(t,cx,cy);
    if(!IsWindow(m_editTo.GetSafeHwnd()))return;
    CRect rect;GetClientRect(&rect);
    int mg=10,eh=28,bh=32,lh=130,y=mg;
    m_editTo.MoveWindow(mg,y,rect.Width()-mg*2,eh);y+=eh+mg;
    m_editSubject.MoveWindow(mg,y,rect.Width()-mg*2,eh);y+=eh+mg;
    int bdy=rect.Height()-y-bh-lh-mg*4;
    m_editBody.MoveWindow(mg,y,rect.Width()-mg*2,bdy);y+=bdy+mg;
    int bw=(rect.Width()-mg*3)/2;
    GetDlgItem(IDC_BTN_AUTO_FILL)->MoveWindow(mg,y,bw,bh);
    GetDlgItem(IDC_BTN_SEND_EMAIL)->MoveWindow(mg*2+bw,y,bw,bh);y+=bh+mg;
    m_listLog.MoveWindow(mg,y,rect.Width()-mg*2,lh);
}

BOOL CNotifyTab::OnEraseBkgnd(CDC* pDC){CRect r;GetClientRect(&r);pDC->FillSolidRect(&r,CLR_BG);return TRUE;}
HBRUSH CNotifyTab::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT n){HBRUSH h=CDialogEx::OnCtlColor(pDC,pWnd,n);if(n==CTLCOLOR_DLG||n==CTLCOLOR_STATIC){pDC->SetBkColor(CLR_BG);return(HBRUSH)m_hBrushBg;}return h;}
void CNotifyTab::OnCustomDrawList(NMHDR* pNMHDR,LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pCD=(LPNMLVCUSTOMDRAW)pNMHDR;*pResult=CDRF_DODEFAULT;
    switch(pCD->nmcd.dwDrawStage){
    case CDDS_PREPAINT:*pResult=CDRF_NOTIFYITEMDRAW;break;
    case CDDS_ITEMPREPAINT:*pResult=CDRF_NOTIFYSUBITEMDRAW;break;
    case CDDS_ITEMPREPAINT|CDDS_SUBITEM:{int r=(int)pCD->nmcd.dwItemSpec;if(r>=0){pCD->clrTextBk=(r%2==0)?CLR_ROW_ODD:CLR_ROW_EVEN;pCD->clrText=CLR_ROW_TEXT;}*pResult=CDRF_NEWFONT;break;}
    }
}
