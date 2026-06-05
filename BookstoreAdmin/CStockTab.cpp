// CStockTab.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CStockTab.h"
#include "CPriceEditDlg.h"

extern CDBManager g_db;

static const COLORREF CLR_BG       = RGB(253, 246, 238);
static const COLORREF CLR_HDR_BG   = RGB(193, 127,  74);
static const COLORREF CLR_HDR_TEXT = RGB(255, 255, 255);
static const COLORREF CLR_ROW_ODD  = RGB(255, 255, 255);
static const COLORREF CLR_ROW_EVEN = RGB(253, 245, 236);
static const COLORREF CLR_ROW_TEXT = RGB( 92,  68,  51);

IMPLEMENT_DYNAMIC(CStockTab, CDialogEx)

CStockTab::CStockTab(const std::wstring& genre, CWnd* pParent)
    : CDialogEx(IDD_STOCK_TAB, pParent), m_genre(genre),
    m_hBrushBg(nullptr), m_hFont(nullptr) {}

CStockTab::~CStockTab()
{
    if (m_hBrushBg) DeleteObject(m_hBrushBg);
    if (m_hFont)    DeleteObject(m_hFont);
}

void CStockTab::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_STOCK_SEARCH, m_editSearch);
    DDX_Control(pDX, IDC_LIST_STOCK,        m_listStock);
    DDX_Control(pDX, IDC_CHK_LOW_STOCK,     m_chkLowStock);
}

BEGIN_MESSAGE_MAP(CStockTab, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_STOCK_SEARCH, &CStockTab::OnBnClickedStockSearch)
    ON_BN_CLICKED(IDC_BTN_STOCK_EDIT,   &CStockTab::OnBnClickedStockEdit)
    ON_BN_CLICKED(IDC_BTN_STOCK_EMAIL,  &CStockTab::OnBnClickedStockEmail)
    ON_BN_CLICKED(IDC_BTN_PRICE_EDIT,   &CStockTab::OnBnClickedPriceEdit)
    ON_BN_CLICKED(IDC_CHK_LOW_STOCK,    &CStockTab::OnBnClickedChkLowStock)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
    ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

BOOL CStockTab::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_hBrushBg = CreateSolidBrush(CLR_BG);
    m_hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        HANGEUL_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");

    m_editSearch.SetFont(CFont::FromHandle(m_hFont));
    m_chkLowStock.SetFont(CFont::FromHandle(m_hFont));

    SetupListColumns();
    ApplyListStyle();

    // 가격수정 버튼 동적 생성
    m_btnPriceEdit.Create(L"가격수정", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0,0,10,10), this, IDC_BTN_PRICE_EDIT);
    m_btnPriceEdit.SetFont(CFont::FromHandle(m_hFont));

    if (GetDlgItem(IDC_BTN_STOCK_SEARCH)) GetDlgItem(IDC_BTN_STOCK_SEARCH)->SetFont(CFont::FromHandle(m_hFont));
    if (GetDlgItem(IDC_BTN_STOCK_EDIT))   GetDlgItem(IDC_BTN_STOCK_EDIT)->SetFont(CFont::FromHandle(m_hFont));
    if (GetDlgItem(IDC_BTN_STOCK_EMAIL))  GetDlgItem(IDC_BTN_STOCK_EMAIL)->SetFont(CFont::FromHandle(m_hFont));

    m_db.Connect();
    return TRUE;
}

void CStockTab::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CDialogEx::OnShowWindow(bShow, nStatus);
    if (bShow && IsWindow(m_editSearch.GetSafeHwnd()))
        ::SendMessage(m_editSearch.GetSafeHwnd(), EM_SETCUEBANNER, TRUE, (LPARAM)L"도서 검색...");
}

void CStockTab::ApplyListStyle()
{
    m_listStock.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listStock.SetBkColor(CLR_BG);
    m_listStock.SetTextBkColor(CLR_ROW_ODD);
    m_listStock.SetTextColor(CLR_ROW_TEXT);
    if (m_hFont) m_listStock.SetFont(CFont::FromHandle(m_hFont));

    // 헤더 컨트롤 서브클래싱
    CHeaderCtrl* pHdr = m_listStock.GetHeaderCtrl();
    if (pHdr && IsWindow(pHdr->GetSafeHwnd())) {
        m_headerCtrl.SubclassWindow(pHdr->GetSafeHwnd());
        m_headerCtrl.SetColors(CLR_HDR_BG, CLR_HDR_TEXT);
    }
}

void CStockTab::SetupListColumns()
{
    while (m_listStock.DeleteColumn(0));
    m_listStock.InsertColumn(0, L"도서ID",   LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(1, L"제목",     LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(2, L"저자",     LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(3, L"출판사",   LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(4, L"가격",     LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(5, L"카테고리", LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(6, L"장르",     LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(7, L"재고",     LVCFMT_LEFT, 10);
    m_listStock.InsertColumn(8, L"보관위치", LVCFMT_LEFT, 10);
}

void CStockTab::ResizeColumns()
{
    if (!IsWindow(m_listStock.GetSafeHwnd())) return;
    CRect rc; m_listStock.GetClientRect(&rc);
    int total = rc.Width() - 4;
    int ratios[] = {6, 20, 11, 11, 8, 8, 10, 7, 8};
    for (int i = 0; i < 9; i++)
        m_listStock.SetColumnWidth(i, total * ratios[i] / 100);
}

void CStockTab::LoadStock(const CString& keyword, bool lowOnly)
{
    m_listStock.DeleteAllItems();
    std::wstring q = L"SELECT BookID, Title, AuthorName, PublisherName, Price, Category, Genre, Stock, StorageSlot FROM Book WHERE 1=1";
    if (!m_genre.empty()) q += L" AND Genre='" + m_genre + L"'";
    if (m_genre == L"기타")
        q = L"SELECT BookID, Title, AuthorName, PublisherName, Price, Category, Genre, Stock, StorageSlot FROM Book WHERE Genre NOT IN ('소설','시집','에세이','만화')";
    if (!keyword.IsEmpty()) q += L" AND Title LIKE '%" + std::wstring(keyword) + L"%'";
    if (lowOnly) q += L" AND Stock <= 5";
    q += L" ORDER BY Category ASC, BookID ASC";
    if (!m_db.ExecuteQuery(q)) return;
    int row = 0;
    while (m_db.Fetch()) {
        int n = m_listStock.InsertItem(row, m_db.GetData(1).c_str());
        for (int i = 1; i < 9; i++)
            m_listStock.SetItemText(n, i, m_db.GetData(i+1).c_str());
        row++;
    }
    ResizeColumns();
}

void CStockTab::OnBnClickedStockSearch()
{ CString k; m_editSearch.GetWindowText(k); LoadStock(k, (m_chkLowStock.GetCheck()==BST_CHECKED)); }
void CStockTab::OnBnClickedChkLowStock()
{ CString k; m_editSearch.GetWindowText(k); LoadStock(k, (m_chkLowStock.GetCheck()==BST_CHECKED)); }

void CStockTab::OnBnClickedStockEdit()
{
    int sel = m_listStock.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) { AfxMessageBox(L"수정할 도서를 선택하세요."); return; }
    CStockEditDlg dlg(m_listStock.GetItemText(sel,0), m_listStock.GetItemText(sel,1),
                      m_listStock.GetItemText(sel,7), this);
    dlg.DoModal(); LoadStock(L"", false);
}

void CStockTab::OnBnClickedPriceEdit()
{
    int sel = m_listStock.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) { AfxMessageBox(L"수정할 도서를 선택하세요."); return; }
    CPriceEditDlg dlg(m_listStock.GetItemText(sel,0), m_listStock.GetItemText(sel,1),
                      m_listStock.GetItemText(sel,4), this);
    dlg.DoModal(); LoadStock(L"", false);
}

void CStockTab::OnBnClickedStockEmail()
{
    if (!g_db.ExecuteQuery(L"SELECT Title, Stock FROM Book WHERE Stock <= 5 ORDER BY Stock ASC")) return;
    CString msg = L"[재고 부족 도서 목록]\n\n"; bool has = false;
    while (g_db.Fetch()) {
        msg += g_db.GetData(1).c_str(); msg += L" (재고: ";
        msg += g_db.GetData(2).c_str(); msg += L"권)\n"; has = true;
    }
    if (!has) { AfxMessageBox(L"재고 부족 도서가 없습니다."); return; }
    AfxMessageBox(msg);
}

void CStockTab::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    if (!IsWindow(m_listStock.GetSafeHwnd())) return;
    if (!GetDlgItem(IDC_BTN_STOCK_SEARCH)) return;
    CRect rect; GetClientRect(&rect);
    if (rect.Width()==0 || rect.Height()==0) return;

    int mg=10, sh=30, bh=32;
    int by = rect.Height() - bh - mg;
    int chkY = mg + sh + 4;
    int listY = chkY + 22;
    int listH = by - listY - mg;
    if (listH <= 0) return;

    m_editSearch.MoveWindow(mg, mg, rect.Width()-110-mg*2, sh);
    GetDlgItem(IDC_BTN_STOCK_SEARCH)->MoveWindow(rect.Width()-100-mg, mg, 100, sh);
    GetDlgItem(IDC_CHK_LOW_STOCK)->MoveWindow(mg, chkY, rect.Width()-mg*2, 18);
    m_listStock.MoveWindow(mg, listY, rect.Width()-mg*2, listH);

    // 버튼 3등분: 재고수정 | 가격수정 | 이메일발송
    int bw = (rect.Width() - mg*4) / 3;
    GetDlgItem(IDC_BTN_STOCK_EDIT)->MoveWindow(mg,           by, bw, bh);
    if (IsWindow(m_btnPriceEdit.GetSafeHwnd()))
        m_btnPriceEdit.MoveWindow(mg*2+bw,                   by, bw, bh);
    GetDlgItem(IDC_BTN_STOCK_EMAIL)->MoveWindow(mg*3+bw*2,   by, bw, bh);

    ResizeColumns();
}

BOOL CStockTab::OnEraseBkgnd(CDC* pDC)
{ CRect r; GetClientRect(&r); pDC->FillSolidRect(&r, CLR_BG); return TRUE; }

HBRUSH CStockTab::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT n)
{
    HBRUSH h = CDialogEx::OnCtlColor(pDC, pWnd, n);
    if (n==CTLCOLOR_DLG || n==CTLCOLOR_STATIC) { pDC->SetBkColor(CLR_BG); return (HBRUSH)m_hBrushBg; }
    return h;
}
