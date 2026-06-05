// COrderTab.cpp
#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "COrderTab.h"

extern CDBManager g_db;

static const COLORREF CLR_BG       = RGB(253, 246, 238);
static const COLORREF CLR_ROW_ODD  = RGB(255, 255, 255);
static const COLORREF CLR_ROW_EVEN = RGB(255, 249, 240);
static const COLORREF CLR_TEXT     = RGB(92, 68, 51);

IMPLEMENT_DYNAMIC(COrderTab, CDialogEx)

COrderTab::COrderTab(CWnd* pParent)
    : CDialogEx(IDD_ORDER_TAB, pParent), m_hBrushBg(nullptr), m_hFont(nullptr)
{
}

COrderTab::~COrderTab()
{
    if (m_hBrushBg) DeleteObject(m_hBrushBg);
    if (m_hFont) DeleteObject(m_hFont);
}

void COrderTab::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST_ORDER, m_listOrder);
}

BEGIN_MESSAGE_MAP(COrderTab, CDialogEx)
    ON_BN_CLICKED(IDC_RADIO_ALL,            &COrderTab::OnBnClickedRadioAll)
    ON_BN_CLICKED(IDC_RADIO_WAITING,        &COrderTab::OnBnClickedRadioWaiting)
    ON_BN_CLICKED(IDC_RADIO_SHIPPING,       &COrderTab::OnBnClickedRadioShipping)
    ON_BN_CLICKED(IDC_RADIO_DONE,           &COrderTab::OnBnClickedRadioDone)
    ON_BN_CLICKED(IDC_BTN_ORDER_DETAIL,     &COrderTab::OnBnClickedOrderDetail)
    ON_BN_CLICKED(IDC_BTN_STATUS,           &COrderTab::OnBnClickedStatus)
    ON_BN_CLICKED(IDC_BTN_NOTIFY,           &COrderTab::OnBnClickedNotify)
    ON_BN_CLICKED(IDC_BTN_DISPATCH_APPROVE, &COrderTab::OnBnClickedDispatchApprove)
    ON_BN_CLICKED(IDC_BTN_FORCE_DONE,       &COrderTab::OnBnClickedForceDone)
    ON_BN_CLICKED(IDC_BTN_ORDER_CANCEL,     &COrderTab::OnBnClickedOrderCancel)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_ORDER, &COrderTab::OnItemChangedOrderList)
    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW,        &COrderTab::OnCustomDrawList)
    ON_WM_SIZE()
    ON_WM_ERASEBKGND()
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL COrderTab::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    m_hBrushBg = CreateSolidBrush(CLR_BG);
    m_hFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGEUL_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");

    SetupListColumns();
    ApplyListStyle();

    int ids[] = {
        IDC_RADIO_ALL, IDC_RADIO_WAITING, IDC_RADIO_SHIPPING, IDC_RADIO_DONE,
        IDC_BTN_ORDER_DETAIL, IDC_BTN_STATUS, IDC_BTN_NOTIFY,
        IDC_BTN_DISPATCH_APPROVE, IDC_BTN_FORCE_DONE, IDC_BTN_ORDER_CANCEL,
        IDC_STATIC_SELECTED_ORDER, IDC_STATIC_HW_STATUS, IDC_EDIT_DISPATCH_LOG
    };
    int count = sizeof(ids) / sizeof(ids[0]);
    for (int i = 0; i < count; ++i)
    {
        CWnd* w = GetDlgItem(ids[i]);
        if (w && m_hFont) w->SetFont(CFont::FromHandle(m_hFont));
    }

    CheckRadioButton(IDC_RADIO_ALL, IDC_RADIO_DONE, IDC_RADIO_ALL);
    LoadOrders();
    RefreshDispatchPanel();
    AppendDispatchLog(L"주문 탭 초기화 완료");

    CRect rect;
    GetClientRect(&rect);
    LayoutControls(rect.Width(), rect.Height());
    return TRUE;
}

void COrderTab::SetupListColumns()
{
    while (m_listOrder.DeleteColumn(0)) {}

    m_listOrder.InsertColumn(0, L"주문번호", LVCFMT_LEFT, 70);
    m_listOrder.InsertColumn(1, L"회원ID",   LVCFMT_LEFT, 85);
    m_listOrder.InsertColumn(2, L"도서명",   LVCFMT_LEFT, 180);
    m_listOrder.InsertColumn(3, L"구매수량", LVCFMT_LEFT, 70);
    m_listOrder.InsertColumn(4, L"보관위치", LVCFMT_LEFT, 70);
    m_listOrder.InsertColumn(5, L"총금액",   LVCFMT_LEFT, 85);
    m_listOrder.InsertColumn(6, L"주문상태", LVCFMT_LEFT, 85);
    m_listOrder.InsertColumn(7, L"하드웨어상태", LVCFMT_LEFT, 95);
}

void COrderTab::ApplyListStyle()
{
    m_listOrder.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    m_listOrder.SetBkColor(CLR_BG);
    m_listOrder.SetTextBkColor(CLR_ROW_ODD);
    m_listOrder.SetTextColor(CLR_TEXT);
    if (m_hFont) m_listOrder.SetFont(CFont::FromHandle(m_hFont));
}

std::wstring COrderTab::MakeOrderQuery(const std::wstring& whereClause) const
{
    std::wstring q;
    q += L"SELECT p.OrderID, p.MemberID, ";
    q += L"CASE WHEN COUNT(pi.BookID) > 1 ";
    q += L"THEN CONCAT(MIN(b.Title), ' 외 ', COUNT(pi.BookID)-1, '권') ";
    q += L"ELSE MIN(b.Title) END AS BookTitle, ";
    q += L"SUM(pi.Quantity) AS TotalQty, ";
    q += L"IFNULL(MIN(b.StorageSlot), '-') AS StorageSlot, ";
    q += L"p.TotalPrice, IFNULL(p.OrderStatus, '-') AS OrderStatus, IFNULL(p.HardwareStatus, '-') AS HardwareStatus ";
    q += L"FROM Purchase p ";
    q += L"LEFT JOIN PurchaseItem pi ON p.OrderID = pi.OrderID ";
    q += L"LEFT JOIN Book b ON pi.BookID = b.BookID ";
    if (!whereClause.empty()) q += whereClause;
    q += L" GROUP BY p.OrderID, p.MemberID, p.TotalPrice, p.OrderStatus, p.HardwareStatus, p.PurchaseDate ";
    q += L" ORDER BY p.PurchaseDate DESC";
    return q;
}

void COrderTab::LoadOrders(const std::wstring& hardwareFilter)
{
    m_currentFilter = hardwareFilter.c_str();
    m_listOrder.DeleteAllItems();

    std::wstring whereClause;
    if (!hardwareFilter.empty())
    {
        if (hardwareFilter == L"완료")
            whereClause = L"WHERE p.HardwareStatus='완료' OR p.OrderStatus='배송완료' ";
        else
            whereClause = L"WHERE p.HardwareStatus='" + hardwareFilter + L"' ";
    }

    std::wstring q = MakeOrderQuery(whereClause);
    if (!g_db.ExecuteQuery(q)) return;

    int row = 0;
    while (g_db.Fetch())
    {
        CString orderId = g_db.GetData(1).c_str();
        CString memberId = g_db.GetData(2).c_str();
        CString title = g_db.GetData(3).c_str();
        CString qty = g_db.GetData(4).c_str();
        CString slot = g_db.GetData(5).c_str();
        CString total = g_db.GetData(6).c_str();
        CString orderStatus = g_db.GetData(7).c_str();
        CString hw = g_db.GetData(8).c_str();

        int idx = m_listOrder.InsertItem(row, orderId);
        m_listOrder.SetItemText(idx, 1, memberId);
        m_listOrder.SetItemText(idx, 2, title);
        m_listOrder.SetItemText(idx, 3, qty + L"권");
        m_listOrder.SetItemText(idx, 4, slot);
        m_listOrder.SetItemText(idx, 5, total + L"원");
        m_listOrder.SetItemText(idx, 6, orderStatus);
        m_listOrder.SetItemText(idx, 7, hw);
        row++;
    }

    RefreshDispatchPanel();
}

CString COrderTab::EscapeSqlCString(const CString& value) const
{
    CString escaped(value);
    escaped.Replace(L"'", L"''");
    return escaped;
}

void COrderTab::SearchOrders(const CString& keyword)
{
    m_listOrder.DeleteAllItems();

    CString safe = EscapeSqlCString(keyword);
    std::wstring whereClause = L"WHERE p.MemberID LIKE '%" + std::wstring((LPCTSTR)safe) + L"%' ";
    std::wstring q = MakeOrderQuery(whereClause);

    if (!g_db.ExecuteQuery(q)) return;

    int row = 0;
    while (g_db.Fetch())
    {
        CString orderId = g_db.GetData(1).c_str();
        int idx = m_listOrder.InsertItem(row, orderId);
        m_listOrder.SetItemText(idx, 1, g_db.GetData(2).c_str());
        m_listOrder.SetItemText(idx, 2, g_db.GetData(3).c_str());
        CString qty = g_db.GetData(4).c_str();
        CString total = g_db.GetData(6).c_str();
        m_listOrder.SetItemText(idx, 3, qty + L"권");
        m_listOrder.SetItemText(idx, 4, g_db.GetData(5).c_str());
        m_listOrder.SetItemText(idx, 5, total + L"원");
        m_listOrder.SetItemText(idx, 6, g_db.GetData(7).c_str());
        m_listOrder.SetItemText(idx, 7, g_db.GetData(8).c_str());
        row++;
    }
    RefreshDispatchPanel();
}

void COrderTab::OnBnClickedRadioAll() { LoadOrders(); }
void COrderTab::OnBnClickedRadioWaiting() { LoadOrders(L"승인대기"); }
void COrderTab::OnBnClickedRadioShipping() { LoadOrders(L"대기"); }
void COrderTab::OnBnClickedRadioDone() { LoadOrders(L"완료"); }

CString COrderTab::GetSelectedOrderID() const
{
    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) return L"";
    return m_listOrder.GetItemText(sel, 0);
}

CString COrderTab::GetSelectedOrderStatus() const
{
    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) return L"";
    return m_listOrder.GetItemText(sel, 6);
}

CString COrderTab::GetSelectedHardwareStatus() const
{
    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) return L"";
    return m_listOrder.GetItemText(sel, 7);
}

bool COrderTab::IsSelectedOrderCanceled(bool showMessage) const
{
    CString orderStatus = GetSelectedOrderStatus();
    CString hardwareStatus = GetSelectedHardwareStatus();

    if (orderStatus == L"취소" || hardwareStatus == L"취소")
    {
        if (showMessage)
        {
            AfxMessageBox(L"취소된 주문은 복구하거나 출고 처리할 수 없습니다.", MB_ICONWARNING);
        }
        return true;
    }
    return false;
}

void COrderTab::RefreshDispatchPanel()
{
    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    CWnd* wSelected = GetDlgItem(IDC_STATIC_SELECTED_ORDER);
    CWnd* wStatus = GetDlgItem(IDC_STATIC_HW_STATUS);

    if (sel < 0)
    {
        if (wSelected) wSelected->SetWindowText(L"선택된 주문: -");
        if (wStatus) wStatus->SetWindowText(L"하드웨어 상태: -");
        return;
    }

    CString msg;
    msg.Format(L"선택된 주문: #%s / %s / %s %s",
        m_listOrder.GetItemText(sel, 0).GetString(),
        m_listOrder.GetItemText(sel, 1).GetString(),
        m_listOrder.GetItemText(sel, 2).GetString(),
        m_listOrder.GetItemText(sel, 3).GetString());

    CString hw;
    hw.Format(L"주문상태: %s    하드웨어 상태: %s",
        m_listOrder.GetItemText(sel, 6).GetString(),
        m_listOrder.GetItemText(sel, 7).GetString());

    if (wSelected) wSelected->SetWindowText(msg);
    if (wStatus) wStatus->SetWindowText(hw);
}

void COrderTab::AppendDispatchLog(const CString& text)
{
    CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_DISPATCH_LOG);
    if (!edit) return;

    CString oldText;
    edit->GetWindowText(oldText);

    CTime now = CTime::GetCurrentTime();
    CString line;
    line.Format(L"[%02d:%02d:%02d] %s\r\n",
        now.GetHour(), now.GetMinute(), now.GetSecond(), text.GetString());

    oldText += line;
    edit->SetWindowText(oldText);
    edit->LineScroll(edit->GetLineCount());
}

void COrderTab::OnBnClickedDispatchApprove()
{
    CString oid = GetSelectedOrderID();
    if (oid.IsEmpty()) { AfxMessageBox(L"주문을 선택하세요."); return; }
    if (IsSelectedOrderCanceled()) return;
    if (GetSelectedOrderStatus() != L"결제완료") { AfxMessageBox(L"결제완료 상태의 주문만 출고 승인할 수 있습니다.", MB_ICONWARNING); return; }

    CString sql;
    sql.Format(L"UPDATE Purchase SET HardwareStatus='대기' WHERE OrderID=%s", oid.GetString());
    if (g_db.ExecuteQuery((LPCTSTR)sql))
    {
        AppendDispatchLog(L"출고 승인 완료: HardwareStatus=대기");
        LoadOrders(std::wstring((LPCTSTR)m_currentFilter));
    }
}

void COrderTab::OnBnClickedForceDone()
{
    CString oid = GetSelectedOrderID();
    if (oid.IsEmpty()) { AfxMessageBox(L"주문을 선택하세요."); return; }
    if (IsSelectedOrderCanceled()) return;

    CString sql;
    sql.Format(L"UPDATE Purchase SET HardwareStatus='완료', OrderStatus='배송완료' WHERE OrderID=%s", oid.GetString());
    if (g_db.ExecuteQuery((LPCTSTR)sql))
    {
        AppendDispatchLog(L"강제 완료 처리");
        LoadOrders(std::wstring((LPCTSTR)m_currentFilter));
    }
}

void COrderTab::OnBnClickedOrderCancel()
{
    CString oid = GetSelectedOrderID();
    if (oid.IsEmpty()) { AfxMessageBox(L"주문을 선택하세요."); return; }
    if (IsSelectedOrderCanceled()) return;

    if (AfxMessageBox(L"선택한 주문을 취소하시겠습니까?", MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    CString sql;
    sql.Format(L"UPDATE Purchase SET HardwareStatus='취소', OrderStatus='취소' WHERE OrderID=%s", oid.GetString());
    if (g_db.ExecuteQuery((LPCTSTR)sql))
    {
        AppendDispatchLog(L"주문 취소 처리");
        LoadOrders(std::wstring((LPCTSTR)m_currentFilter));
    }
}

void COrderTab::OnBnClickedOrderDetail()
{
    CString oid = GetSelectedOrderID();
    if (oid.IsEmpty()) { AfxMessageBox(L"주문을 선택하세요."); return; }

    std::wstring q = L"SELECT B.Title, PI.Quantity, PI.Price "
        L"FROM PurchaseItem PI JOIN Book B ON PI.BookID=B.BookID "
        L"WHERE PI.OrderID=" + std::wstring((LPCTSTR)oid);

    if (!g_db.ExecuteQuery(q)) return;

    CString msg;
    msg.Format(L"[주문 %s] 상세내역\n\n", oid.GetString());
    while (g_db.Fetch())
    {
        msg += g_db.GetData(1).c_str();
        msg += L"\t";
        msg += g_db.GetData(2).c_str();
        msg += L"권\t";
        msg += g_db.GetData(3).c_str();
        msg += L"원\n";
    }
    AfxMessageBox(msg);
}

void COrderTab::OnBnClickedStatus()
{
    CString oid = GetSelectedOrderID();
    if (oid.IsEmpty()) { AfxMessageBox(L"주문을 선택하세요."); return; }
    if (IsSelectedOrderCanceled()) return;

    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    CString cur = m_listOrder.GetItemText(sel, 7);
    CString next;

    if (cur == L"승인대기") next = L"대기";
    else if (cur == L"대기") next = L"완료";
    else { AfxMessageBox(L"더 이상 변경할 상태가 없습니다."); return; }

    CString sql;
    sql.Format(L"UPDATE Purchase SET HardwareStatus='%s' WHERE OrderID=%s", next.GetString(), oid.GetString());
    if (g_db.ExecuteQuery((LPCTSTR)sql))
    {
        CString log;
        log.Format(L"상태변경: %s", next.GetString());
        AppendDispatchLog(log);
        LoadOrders(std::wstring((LPCTSTR)m_currentFilter));
    }
}

void COrderTab::OnBnClickedNotify()
{
    int sel = m_listOrder.GetNextItem(-1, LVNI_SELECTED);
    if (sel < 0) { AfxMessageBox(L"주문을 선택하세요."); return; }
    if (IsSelectedOrderCanceled()) return;

    CString m;
    m.Format(L"[%s] 님께 '%s' 알림을 발송합니다.",
        m_listOrder.GetItemText(sel, 1).GetString(),
        m_listOrder.GetItemText(sel, 7).GetString());
    AfxMessageBox(m);
}

void COrderTab::OnItemChangedOrderList(NMHDR* pNMHDR, LRESULT* pResult)
{
    RefreshDispatchPanel();
    *pResult = 0;
}

void COrderTab::MoveDlgItem(int id, int x, int y, int w, int h)
{
    CWnd* item = GetDlgItem(id);
    if (item) item->MoveWindow(x, y, w, h);
}

void COrderTab::LayoutControls(int cx, int cy)
{
    if (!IsWindow(m_listOrder.GetSafeHwnd())) return;

    int margin = 10;
    int radioY = 10;
    int listY = 35;
    int buttonH = 28;
    int dispatchH = 100;
    int bottom = cy - margin;
    int dispatchY = bottom - dispatchH;
    int buttonY = dispatchY - buttonH - 8;
    int listH = buttonY - listY - 8;
    if (listH < 70) listH = 70;

    MoveDlgItem(IDC_RADIO_ALL,      margin,      radioY, 70, 20);
    MoveDlgItem(IDC_RADIO_WAITING,  margin + 75, radioY, 85, 20);
    MoveDlgItem(IDC_RADIO_SHIPPING, margin + 165, radioY, 75, 20);
    MoveDlgItem(IDC_RADIO_DONE,     margin + 245, radioY, 90, 20);

    m_listOrder.MoveWindow(margin, listY, cx - margin * 2, listH);

    int bw = (cx - margin * 4) / 3;
    MoveDlgItem(IDC_BTN_ORDER_DETAIL, margin, buttonY, bw, buttonH);
    MoveDlgItem(IDC_BTN_STATUS, margin * 2 + bw, buttonY, bw, buttonH);
    MoveDlgItem(IDC_BTN_NOTIFY, margin * 3 + bw * 2, buttonY, bw, buttonH);

    MoveDlgItem(IDC_STATIC_DISPATCH_BOX, margin, dispatchY, cx - margin * 2, dispatchH);
    MoveDlgItem(IDC_STATIC_SELECTED_ORDER, margin + 12, dispatchY + 20, cx - margin * 4, 18);
    MoveDlgItem(IDC_STATIC_HW_STATUS, margin + 12, dispatchY + 38, cx - margin * 4, 18);

    int dbw = (cx - margin * 5) / 3;
    MoveDlgItem(IDC_BTN_DISPATCH_APPROVE, margin + 12, dispatchY + 57, dbw, 24);
    MoveDlgItem(IDC_BTN_FORCE_DONE, margin + 24 + dbw, dispatchY + 57, dbw, 24);
    MoveDlgItem(IDC_BTN_ORDER_CANCEL, margin + 36 + dbw * 2, dispatchY + 57, dbw, 24);
    MoveDlgItem(IDC_EDIT_DISPATCH_LOG, margin + 12, dispatchY + 83, cx - margin * 4, 14);
}

void COrderTab::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    LayoutControls(cx, cy);
}

BOOL COrderTab::OnEraseBkgnd(CDC* pDC)
{
    CRect r;
    GetClientRect(&r);
    pDC->FillSolidRect(&r, CLR_BG);
    return TRUE;
}

HBRUSH COrderTab::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH h = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
    if (nCtlColor == CTLCOLOR_DLG || nCtlColor == CTLCOLOR_STATIC)
    {
        pDC->SetBkColor(CLR_BG);
        return (HBRUSH)m_hBrushBg;
    }
    return h;
}

void COrderTab::OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLVCUSTOMDRAW pCD = (LPNMLVCUSTOMDRAW)pNMHDR;
    *pResult = CDRF_DODEFAULT;

    switch (pCD->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        *pResult = CDRF_NOTIFYITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT:
        *pResult = CDRF_NOTIFYSUBITEMDRAW;
        break;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        {
            int r = (int)pCD->nmcd.dwItemSpec;
            if (r >= 0)
            {
                pCD->clrTextBk = (r % 2 == 0) ? CLR_ROW_ODD : CLR_ROW_EVEN;
                pCD->clrText = CLR_TEXT;
            }
            *pResult = CDRF_NEWFONT;
        }
        break;
    default:
        break;
    }
}
