// CAdminMainDlg.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CAdminMainDlg.h"

extern CDBManager g_db;

static const COLORREF CLR_BG       = RGB(253, 246, 238);
static const COLORREF CLR_TITLE_BG = RGB(193, 127,  74);
static const COLORREF CLR_TITLE_FG = RGB(255, 255, 255);
static const COLORREF CLR_DIVIDER  = RGB(232, 221, 208);

#define IDC_LEFT_SEARCH_EDIT   9100
#define IDC_LEFT_SEARCH_BTN    9101

IMPLEMENT_DYNAMIC(CAdminMainDlg, CDialogEx)

CAdminMainDlg::CAdminMainDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent),
    m_pMemberTab(nullptr), m_pOrderTab(nullptr),
    m_pReviewTab(nullptr), m_pNotifyTab(nullptr),
    m_pStockAll(nullptr), m_pStockNovel(nullptr),
    m_pStockPoem(nullptr), m_pStockEssay(nullptr),
    m_pStockComic(nullptr), m_pStockEtc(nullptr),
    m_hBrushBg(nullptr), m_hFontTitle(nullptr), m_hFont(nullptr) {}

CAdminMainDlg::~CAdminMainDlg()
{
    if (m_pMemberTab)  delete m_pMemberTab;
    if (m_pOrderTab)   delete m_pOrderTab;
    if (m_pReviewTab)  delete m_pReviewTab;
    if (m_pNotifyTab)  delete m_pNotifyTab;
    if (m_pStockAll)   delete m_pStockAll;
    if (m_pStockNovel) delete m_pStockNovel;
    if (m_pStockPoem)  delete m_pStockPoem;
    if (m_pStockEssay) delete m_pStockEssay;
    if (m_pStockComic) delete m_pStockComic;
    if (m_pStockEtc)   delete m_pStockEtc;
    if (m_hBrushBg)    DeleteObject(m_hBrushBg);
    if (m_hFontTitle)  DeleteObject(m_hFontTitle);
    if (m_hFont)       DeleteObject(m_hFont);
}

void CAdminMainDlg::DoDataExchange(CDataExchange* pDX)
{ CDialogEx::DoDataExchange(pDX); }

BEGIN_MESSAGE_MAP(CAdminMainDlg, CDialogEx)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN,  &CAdminMainDlg::OnTcnSelchangeTabLeft)
    ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_RIGHT, &CAdminMainDlg::OnTcnSelchangeTabRight)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_PAINT()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CAdminMainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowText(L"BookNest 관리자");

    m_hBrushBg = CreateSolidBrush(CLR_BG);
    m_hFontTitle = CreateFont(15,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,
        HANGEUL_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    m_hFont = CreateFont(14,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
        HANGEUL_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");

    // ── 왼쪽 탭 ──
    m_tabLeft.Create(TCS_FLATBUTTONS|WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),this,IDC_TAB_MAIN);
    CFont* pFL = new CFont();
    pFL->CreateFont(14,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    m_tabLeft.SetFont(pFL);
    m_tabLeft.InsertItem(0, L" 👤 회원 ");
    m_tabLeft.InsertItem(1, L" 📦 주문 ");
    m_tabLeft.InsertItem(2, L" ⭐ 리뷰 ");
    m_tabLeft.InsertItem(3, L" 📧 알림/이메일 ");

    m_pMemberTab = new CMemberTab(this);
    m_pMemberTab->Create(IDD_MEMBER_TAB, this);
    m_pMemberTab->ShowWindow(SW_SHOW);
    m_pOrderTab = new COrderTab(this);
    m_pOrderTab->Create(IDD_ORDER_TAB, this);
    m_pOrderTab->ShowWindow(SW_HIDE);
    m_pReviewTab = new CReviewTab(this);
    m_pReviewTab->Create(IDD_REVIEW_TAB, this);
    m_pReviewTab->ShowWindow(SW_HIDE);
    m_pNotifyTab = new CNotifyTab(this);
    m_pNotifyTab->Create(IDD_NOTIFY_TAB, this);
    m_pNotifyTab->ShowWindow(SW_HIDE);
    m_tabLeft.SetCurSel(0);

    // ── 오른쪽 탭 ──
    m_tabRight.Create(TCS_FLATBUTTONS|WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),this,IDC_TAB_RIGHT);
    CFont* pFR = new CFont();
    pFR->CreateFont(14,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"맑은 고딕");
    m_tabRight.SetFont(pFR);
    m_tabRight.InsertItem(0, L" 전체 ");
    m_tabRight.InsertItem(1, L" 소설 ");
    m_tabRight.InsertItem(2, L" 시집 ");
    m_tabRight.InsertItem(3, L" 에세이 ");
    m_tabRight.InsertItem(4, L" 만화 ");
    m_tabRight.InsertItem(5, L" 기타 ");

    m_pStockAll   = new CStockTab(L"",      this);
    m_pStockNovel = new CStockTab(L"소설",  this);
    m_pStockPoem  = new CStockTab(L"시집",  this);
    m_pStockEssay = new CStockTab(L"에세이",this);
    m_pStockComic = new CStockTab(L"만화",  this);
    m_pStockEtc   = new CStockTab(L"기타",  this);

    m_pStockAll->Create(IDD_STOCK_TAB,   this);
    m_pStockNovel->Create(IDD_STOCK_TAB, this);
    m_pStockPoem->Create(IDD_STOCK_TAB,  this);
    m_pStockEssay->Create(IDD_STOCK_TAB, this);
    m_pStockComic->Create(IDD_STOCK_TAB, this);
    m_pStockEtc->Create(IDD_STOCK_TAB,   this);

    m_pStockAll->ShowWindow(SW_SHOW);
    m_pStockNovel->ShowWindow(SW_HIDE);
    m_pStockPoem->ShowWindow(SW_HIDE);
    m_pStockEssay->ShowWindow(SW_HIDE);
    m_pStockComic->ShowWindow(SW_HIDE);
    m_pStockEtc->ShowWindow(SW_HIDE);
    m_tabRight.SetCurSel(0);

    ShowWindow(SW_MAXIMIZE);
    UpdateWindow();
    ResizeControls();
    m_pStockAll->Reload();
    return TRUE;
}

void CAdminMainDlg::ResizeControls()
{
    if (!IsWindow(m_tabLeft.GetSafeHwnd()))  return;
    if (!IsWindow(m_tabRight.GetSafeHwnd())) return;

    CRect client; GetClientRect(&client);
    int margin=10, titleH=30, half=client.Width()/2;
    int lx = margin, lw = half - margin*2;

    // 왼쪽 탭 (오른쪽과 동일한 구조)
    int tabY = margin + titleH + 4;
    int lh = client.Height() - tabY - margin;
    m_tabLeft.MoveWindow(lx, tabY, lw, lh);

    CRect lc(lx, tabY, lx+lw, tabY+lh);
    m_tabLeft.AdjustRect(FALSE, &lc);

    if(m_pMemberTab&&IsWindow(m_pMemberTab->GetSafeHwnd())) m_pMemberTab->MoveWindow(lc);
    if(m_pOrderTab &&IsWindow(m_pOrderTab->GetSafeHwnd()))  m_pOrderTab->MoveWindow(lc);
    if(m_pReviewTab&&IsWindow(m_pReviewTab->GetSafeHwnd())) m_pReviewTab->MoveWindow(lc);
    if(m_pNotifyTab&&IsWindow(m_pNotifyTab->GetSafeHwnd())) m_pNotifyTab->MoveWindow(lc);

    int rx=half+margin, ry=margin+titleH+4, rw=client.Width()-half-margin*2, rh=client.Height()-ry-margin;
    m_tabRight.MoveWindow(rx,ry,rw,rh);
    CRect rc(rx,ry,rx+rw,ry+rh); m_tabRight.AdjustRect(FALSE,&rc);
    if(m_pStockAll  &&IsWindow(m_pStockAll->GetSafeHwnd()))   m_pStockAll->MoveWindow(rc);
    if(m_pStockNovel&&IsWindow(m_pStockNovel->GetSafeHwnd())) m_pStockNovel->MoveWindow(rc);
    if(m_pStockPoem &&IsWindow(m_pStockPoem->GetSafeHwnd()))  m_pStockPoem->MoveWindow(rc);
    if(m_pStockEssay&&IsWindow(m_pStockEssay->GetSafeHwnd())) m_pStockEssay->MoveWindow(rc);
    if(m_pStockComic&&IsWindow(m_pStockComic->GetSafeHwnd())) m_pStockComic->MoveWindow(rc);
    if(m_pStockEtc  &&IsWindow(m_pStockEtc->GetSafeHwnd()))   m_pStockEtc->MoveWindow(rc);

    m_rectTitleLeft  = CRect(lx, margin, lx+lw, margin+titleH);
    m_rectTitleRight = CRect(rx, margin, rx+rw, margin+titleH);
    Invalidate();
}

void CAdminMainDlg::ShowLeftTab(int sel)
{
    if(m_pMemberTab) m_pMemberTab->ShowWindow(sel==0?SW_SHOW:SW_HIDE);
    if(m_pOrderTab)  m_pOrderTab->ShowWindow(sel==1?SW_SHOW:SW_HIDE);
    if(m_pReviewTab) m_pReviewTab->ShowWindow(sel==2?SW_SHOW:SW_HIDE);
    if(m_pNotifyTab) m_pNotifyTab->ShowWindow(sel==3?SW_SHOW:SW_HIDE);
}

void CAdminMainDlg::ShowRightTab(int sel)
{
    CStockTab* tabs[]={m_pStockAll,m_pStockNovel,m_pStockPoem,m_pStockEssay,m_pStockComic,m_pStockEtc};
    for(int i=0;i<6;i++){if(tabs[i]){tabs[i]->ShowWindow(i==sel?SW_SHOW:SW_HIDE);if(i==sel)tabs[i]->Reload();}}
}

void CAdminMainDlg::OnTcnSelchangeTabLeft(NMHDR* p,LRESULT* r){ShowLeftTab(m_tabLeft.GetCurSel());*r=0;}
void CAdminMainDlg::OnTcnSelchangeTabRight(NMHDR* p,LRESULT* r){ShowRightTab(m_tabRight.GetCurSel());*r=0;}
void CAdminMainDlg::OnTcnSelchangeTabMain(NMHDR* p,LRESULT* r){ShowLeftTab(m_tabLeft.GetCurSel());*r=0;}
void CAdminMainDlg::OnSize(UINT t,int cx,int cy){CDialogEx::OnSize(t,cx,cy);ResizeControls();}

BOOL CAdminMainDlg::OnEraseBkgnd(CDC* pDC)
{CRect r;GetClientRect(&r);pDC->FillSolidRect(&r,CLR_BG);return TRUE;}

void CAdminMainDlg::OnPaint()
{
    CPaintDC dc(this);
    if(m_rectTitleLeft.IsRectEmpty()) return;
    HFONT hOld=(HFONT)SelectObject(dc.m_hDC,m_hFontTitle);
    dc.FillSolidRect(&m_rectTitleLeft,CLR_TITLE_BG);
    dc.SetTextColor(CLR_TITLE_FG); dc.SetBkMode(TRANSPARENT);
    CRect rL=m_rectTitleLeft; rL.left+=12;
    dc.DrawText(L"📋  종합 정보 관리",&rL,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    dc.FillSolidRect(&m_rectTitleRight,CLR_TITLE_BG);
    CRect rR=m_rectTitleRight; rR.left+=12;
    dc.DrawText(L"📦  도서 재고 관리",&rR,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    CRect client; GetClientRect(&client);
    dc.FillSolidRect(client.Width()/2-1,0,2,client.Height(),CLR_DIVIDER);
    SelectObject(dc.m_hDC,hOld);
}

HBRUSH CAdminMainDlg::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
    HBRUSH hBr=CDialogEx::OnCtlColor(pDC,pWnd,nCtlColor);
    if(nCtlColor==CTLCOLOR_DLG||nCtlColor==CTLCOLOR_STATIC){pDC->SetBkColor(CLR_BG);return(HBRUSH)m_hBrushBg;}
    return hBr;
}
