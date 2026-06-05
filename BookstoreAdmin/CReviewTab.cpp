// CReviewTab.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CReviewTab.h"
#include "CReplyDlg.h"

extern CDBManager g_db;
static const COLORREF CLR_BG=RGB(253,246,238),CLR_ROW_ODD=RGB(255,255,255),CLR_ROW_EVEN=RGB(253,245,236),CLR_ROW_TEXT=RGB(92,68,51);

IMPLEMENT_DYNAMIC(CReviewTab,CDialogEx)
CReviewTab::CReviewTab(CWnd* p):CDialogEx(IDD_REVIEW_TAB,p),m_hBrushBg(nullptr),m_hFont(nullptr){}
CReviewTab::~CReviewTab(){if(m_hBrushBg)DeleteObject(m_hBrushBg);if(m_hFont)DeleteObject(m_hFont);}
void CReviewTab::DoDataExchange(CDataExchange* pDX){CDialogEx::DoDataExchange(pDX);DDX_Control(pDX,IDC_LIST_REVIEW,m_listReview);}

BEGIN_MESSAGE_MAP(CReviewTab,CDialogEx)
    ON_BN_CLICKED(IDC_BTN_REPLY,&CReviewTab::OnBnClickedReply)
    ON_WM_SIZE() ON_WM_ERASEBKGND() ON_WM_CTLCOLOR()
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW,&CReviewTab::OnCustomDrawList)
END_MESSAGE_MAP()

BOOL CReviewTab::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_hBrushBg=CreateSolidBrush(CLR_BG);
    m_hFont=CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    SetupListColumns(); ApplyListStyle(); LoadReviews();
    if(GetDlgItem(IDC_BTN_REPLY))GetDlgItem(IDC_BTN_REPLY)->SetFont(CFont::FromHandle(m_hFont));
    return TRUE;
}

void CReviewTab::ApplyListStyle()
{
    m_listReview.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
    m_listReview.SetBkColor(CLR_BG);m_listReview.SetTextBkColor(CLR_ROW_ODD);m_listReview.SetTextColor(CLR_ROW_TEXT);
    if(m_hFont)m_listReview.SetFont(CFont::FromHandle(m_hFont));
}

void CReviewTab::SetupListColumns()
{
    while(m_listReview.DeleteColumn(0));
    m_listReview.InsertColumn(0,L"리뷰ID",LVCFMT_LEFT, 60);
    m_listReview.InsertColumn(1,L"도서명",LVCFMT_LEFT,160);
    m_listReview.InsertColumn(2,L"작성자",LVCFMT_LEFT, 80);
    m_listReview.InsertColumn(3,L"별점",  LVCFMT_LEFT, 60);
    m_listReview.InsertColumn(4,L"내용",  LVCFMT_LEFT,200);
    m_listReview.InsertColumn(5,L"작성일",LVCFMT_LEFT,130);
}

void CReviewTab::LoadReviews(const CString& keyword)
{
    m_listReview.DeleteAllItems();
    std::wstring q=L"SELECT R.ReviewID,B.Title,R.MemberID,R.Rating,R.Content,R.CreatedAt FROM Review R JOIN Book B ON R.BookID=B.BookID";
    if(!keyword.IsEmpty())
        q+=L" WHERE B.Title LIKE '%"+std::wstring(keyword)+L"%' OR R.MemberID LIKE '%"+std::wstring(keyword)+L"%'";
    q+=L" ORDER BY R.CreatedAt DESC";
    if(!g_db.ExecuteQuery(q))return;
    int row=0;
    while(g_db.Fetch()){
        m_listReview.InsertItem(row,g_db.GetData(1).c_str());
        m_listReview.SetItemText(row,1,g_db.GetData(2).c_str());
        m_listReview.SetItemText(row,2,g_db.GetData(3).c_str());
        std::wstring s; int n=_wtoi(g_db.GetData(4).c_str());
        for(int i=0;i<n;i++)s+=L"★"; for(int i=n;i<5;i++)s+=L"☆";
        m_listReview.SetItemText(row,3,s.c_str());
        m_listReview.SetItemText(row,4,g_db.GetData(5).c_str());
        m_listReview.SetItemText(row,5,g_db.GetData(6).c_str());
        row++;
    }
}

// CAdminMainDlg에서 호출
void CReviewTab::SearchReviews(const CString& keyword) { LoadReviews(keyword); }

void CReviewTab::OnBnClickedReply()
{
    int sel=m_listReview.GetNextItem(-1,LVNI_SELECTED);
    if(sel<0){AfxMessageBox(L"답글을 달 리뷰를 선택하세요.");return;}
    CString rid=m_listReview.GetItemText(sel,0),mid=m_listReview.GetItemText(sel,2),con=m_listReview.GetItemText(sel,4);
    std::wstring qb=L"SELECT BookID FROM Review WHERE ReviewID="+std::wstring(rid);
    if(!g_db.ExecuteQuery(qb)||!g_db.Fetch())return;
    std::wstring bid=g_db.GetData(1);
    CReplyDlg dlg(con,this); if(dlg.DoModal()!=IDOK)return;
    CString rep=dlg.GetReply();
    std::wstring qi=L"INSERT INTO QnA(MemberID,BookID,Question,Answer)VALUES('"+std::wstring(mid)+L"',"+bid+L",'"+std::wstring(con)+L"','"+std::wstring(rep)+L"')";
    if(g_db.ExecuteQuery(qi))AfxMessageBox(L"답글이 등록되었습니다.");
}

void CReviewTab::OnSize(UINT t,int cx,int cy)
{
    CDialogEx::OnSize(t,cx,cy);
    if(!IsWindow(m_listReview.GetSafeHwnd()))return;
    CRect rect;GetClientRect(&rect);
    int mg=10,bh=32,by=rect.Height()-bh-mg;
    m_listReview.MoveWindow(mg,mg,rect.Width()-mg*2,by-mg*2);
    GetDlgItem(IDC_BTN_REPLY)->MoveWindow(mg,by,rect.Width()-mg*2,bh);
}

BOOL CReviewTab::OnEraseBkgnd(CDC* pDC){CRect r;GetClientRect(&r);pDC->FillSolidRect(&r,CLR_BG);return TRUE;}
HBRUSH CReviewTab::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT n){HBRUSH h=CDialogEx::OnCtlColor(pDC,pWnd,n);if(n==CTLCOLOR_DLG||n==CTLCOLOR_STATIC){pDC->SetBkColor(CLR_BG);return(HBRUSH)m_hBrushBg;}return h;}
void CReviewTab::OnCustomDrawList(NMHDR* pNMHDR,LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pCD=(LPNMLVCUSTOMDRAW)pNMHDR;*pResult=CDRF_DODEFAULT;
    switch(pCD->nmcd.dwDrawStage){
    case CDDS_PREPAINT:*pResult=CDRF_NOTIFYITEMDRAW;break;
    case CDDS_ITEMPREPAINT:*pResult=CDRF_NOTIFYSUBITEMDRAW;break;
    case CDDS_ITEMPREPAINT|CDDS_SUBITEM:{int r=(int)pCD->nmcd.dwItemSpec;if(r>=0){pCD->clrTextBk=(r%2==0)?CLR_ROW_ODD:CLR_ROW_EVEN;pCD->clrText=CLR_ROW_TEXT;}*pResult=CDRF_NEWFONT;break;}
    }
}
