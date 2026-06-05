// CMemberTab.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CMemberTab.h"

extern CDBManager g_db;
static const COLORREF CLR_BG=RGB(253,246,238),CLR_HDR_BG=RGB(193,127,74),CLR_HDR_TEXT=RGB(255,255,255);
static const COLORREF CLR_ROW_ODD=RGB(255,255,255),CLR_ROW_EVEN=RGB(253,245,236),CLR_ROW_TEXT=RGB(92,68,51);

#define IDC_MEMBER_SEARCH_EDIT  9200
#define IDC_MEMBER_SEARCH_BTN   9201
#define IDC_MEMBER_LOWSTOCK_CHK 9202  // 재고부족만 보기와 동일 위치감을 위해 여백용

IMPLEMENT_DYNAMIC(CMemberTab,CDialogEx)
CMemberTab::CMemberTab(CWnd* p):CDialogEx(IDD_MEMBER_TAB,p),m_hBrushBg(nullptr),m_hFont(nullptr){}
CMemberTab::~CMemberTab(){if(m_hBrushBg)DeleteObject(m_hBrushBg);if(m_hFont)DeleteObject(m_hFont);}

void CMemberTab::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX,IDC_LIST_MEMBER,m_listMember);
}

BEGIN_MESSAGE_MAP(CMemberTab,CDialogEx)
    ON_BN_CLICKED(IDC_BTN_DETAIL,&CMemberTab::OnBnClickedDetail)
    ON_BN_CLICKED(IDC_BTN_EDIT,  &CMemberTab::OnBnClickedEdit)
    ON_BN_CLICKED(IDC_MEMBER_SEARCH_BTN, &CMemberTab::OnBnClickedSearch)
    ON_WM_SIZE() ON_WM_ERASEBKGND() ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CMemberTab::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_hBrushBg=CreateSolidBrush(CLR_BG);
    m_hFont=CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");

    // ── 검색창/버튼 동적 생성 (도서 재고 관리와 동일한 구조) ──
    m_editSearch.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL,
        CRect(0,0,10,10), this, IDC_MEMBER_SEARCH_EDIT);
    m_editSearch.SetFont(CFont::FromHandle(m_hFont));
    ::SendMessage(m_editSearch.GetSafeHwnd(), EM_SETCUEBANNER, TRUE, (LPARAM)L"회원 검색...");

    m_btnSearch.Create(L"검색", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
        CRect(0,0,10,10), this, IDC_MEMBER_SEARCH_BTN);
    m_btnSearch.SetFont(CFont::FromHandle(m_hFont));

    SetupListColumns(); ApplyListStyle(); LoadMembers();

    if(GetDlgItem(IDC_BTN_DETAIL))GetDlgItem(IDC_BTN_DETAIL)->SetFont(CFont::FromHandle(m_hFont));
    if(GetDlgItem(IDC_BTN_EDIT))  GetDlgItem(IDC_BTN_EDIT)->SetFont(CFont::FromHandle(m_hFont));
    // 기존 리소스 검색창/버튼 숨김
    if(GetDlgItem(IDC_EDIT_SEARCH)) GetDlgItem(IDC_EDIT_SEARCH)->ShowWindow(SW_HIDE);
    if(GetDlgItem(IDC_BTN_SEARCH))  GetDlgItem(IDC_BTN_SEARCH)->ShowWindow(SW_HIDE);
    return TRUE;
}

void CMemberTab::OnBnClickedSearch()
{
    CString keyword;
    m_editSearch.GetWindowText(keyword);
    LoadMembers(keyword);
}

void CMemberTab::ApplyListStyle()
{
    m_listMember.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    m_listMember.SetBkColor(CLR_BG);
    m_listMember.SetTextBkColor(CLR_ROW_ODD);
    m_listMember.SetTextColor(CLR_ROW_TEXT);
    if(m_hFont) m_listMember.SetFont(CFont::FromHandle(m_hFont));
    CHeaderCtrl* pHdr=m_listMember.GetHeaderCtrl();
    if(pHdr&&IsWindow(pHdr->GetSafeHwnd())){
        m_headerCtrl.SubclassWindow(pHdr->GetSafeHwnd());
        m_headerCtrl.SetColors(CLR_HDR_BG,CLR_HDR_TEXT);
    }
}

void CMemberTab::SetupListColumns()
{
    while(m_listMember.DeleteColumn(0));
    m_listMember.InsertColumn(0,L"아이디",  LVCFMT_LEFT,10);
    m_listMember.InsertColumn(1,L"비밀번호",LVCFMT_LEFT,10);
    m_listMember.InsertColumn(2,L"이름",    LVCFMT_LEFT,10);
    m_listMember.InsertColumn(3,L"주소",    LVCFMT_LEFT,10);
    m_listMember.InsertColumn(4,L"전화번호",LVCFMT_LEFT,10);
    m_listMember.InsertColumn(5,L"이메일",  LVCFMT_LEFT,10);
}

void CMemberTab::ResizeColumns()
{
    if(!IsWindow(m_listMember.GetSafeHwnd())) return;
    CRect rc; m_listMember.GetClientRect(&rc);
    int total=rc.Width()-4;
    int ratios[]={15,15,10,25,15,20};
    for(int i=0;i<6;i++) m_listMember.SetColumnWidth(i,total*ratios[i]/100);
}

void CMemberTab::LoadMembers(const CString& keyword)
{
    m_listMember.DeleteAllItems();
    std::wstring q=L"SELECT MemberID,Password,Name,Address,Phone,Email FROM Member WHERE Role='user'";
    if(!keyword.IsEmpty())
        q+=L" AND (Name LIKE '%"+std::wstring(keyword)+L"%' OR MemberID LIKE '%"+std::wstring(keyword)+L"%')";
    if(!g_db.ExecuteQuery(q)) return;
    int row=0;
    while(g_db.Fetch()){
        m_listMember.InsertItem(row,g_db.GetData(1).c_str());
        for(int i=1;i<6;i++) m_listMember.SetItemText(row,i,g_db.GetData(i+1).c_str());
        row++;
    }
    ResizeColumns();
}

// CAdminMainDlg에서 호출하는 공개 검색 함수
void CMemberTab::SearchMembers(const CString& keyword) { LoadMembers(keyword); }

void CMemberTab::OnBnClickedDetail()
{
    int sel=m_listMember.GetNextItem(-1,LVNI_SELECTED);
    if(sel<0){AfxMessageBox(L"회원을 선택하세요.");return;}
    CMemberDetailDlg dlg(m_listMember.GetItemText(sel,0),this);
    dlg.DoModal(); LoadMembers();
}
void CMemberTab::OnBnClickedEdit()
{
    int sel=m_listMember.GetNextItem(-1,LVNI_SELECTED);
    if(sel<0){AfxMessageBox(L"수정할 회원을 선택하세요.");return;}
    CMemberDetailDlg dlg(m_listMember.GetItemText(sel,0),this);
    dlg.DoModal(); LoadMembers();
}

void CMemberTab::OnSize(UINT t,int cx,int cy)
{
    CDialogEx::OnSize(t,cx,cy);
    if(!IsWindow(m_listMember.GetSafeHwnd())) return;

    CRect rect; GetClientRect(&rect);
    int mg=6;
    int sh=28; // 검색창 높이 (도서 재고 관리와 동일)
    int bh=32; // 하단 버튼 높이

    // 검색창: 맨 위 (도서 재고 관리의 "도서 검색..." 위치와 동일)
    int searchY = mg;
    int searchW = rect.Width() - mg*2 - 80;
    if(IsWindow(m_editSearch.GetSafeHwnd()))
        m_editSearch.MoveWindow(mg, searchY , searchW, sh);
    if(IsWindow(m_btnSearch.GetSafeHwnd()))
        m_btnSearch.MoveWindow(mg + searchW + 4, searchY, 76, sh);

    // 리스트: 검색창 아래 ~ 하단 버튼 위
    int listY  = searchY + sh + mg;
    int listH  = rect.Height() - listY - bh - mg*2;
    m_listMember.MoveWindow(mg, listY, rect.Width()-mg*2, listH);

    // 하단 버튼: "회원정보 수정" 1개 (재고수정/가격수정 버튼 구조와 동일)
    int by = rect.Height() - bh - mg;
    if(GetDlgItem(IDC_BTN_EDIT))
        GetDlgItem(IDC_BTN_EDIT)->MoveWindow(mg, by, rect.Width()-mg*2, bh);
    // 상세보기 버튼은 숨김 (수정 버튼 하나로 통합)
    if(GetDlgItem(IDC_BTN_DETAIL))
        GetDlgItem(IDC_BTN_DETAIL)->ShowWindow(SW_HIDE);

    ResizeColumns();
}

BOOL CMemberTab::OnEraseBkgnd(CDC* pDC){CRect r;GetClientRect(&r);pDC->FillSolidRect(&r,CLR_BG);return TRUE;}
HBRUSH CMemberTab::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT n){HBRUSH h=CDialogEx::OnCtlColor(pDC,pWnd,n);if(n==CTLCOLOR_DLG||n==CTLCOLOR_STATIC){pDC->SetBkColor(CLR_BG);return(HBRUSH)m_hBrushBg;}return h;}
