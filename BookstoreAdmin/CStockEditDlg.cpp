#include "pch.h"
#include "CStockEditDlg.h"
#include "BookstoreAdmin.h"

extern CDBManager g_db;

IMPLEMENT_DYNAMIC(CStockEditDlg, CDialogEx)

CStockEditDlg::CStockEditDlg(const CString& bookID, const CString& title,
    const CString& stock, CWnd* pParent)
    : CDialogEx(IDD_STOCK_EDIT, pParent),
    m_bookID(bookID), m_title(title), m_stock(stock) {
}

CStockEditDlg::~CStockEditDlg() {}

void CStockEditDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_STOCK_TITLE, m_editTitle);
    DDX_Control(pDX, IDC_EDIT_STOCK_CURRENT, m_editCurrent);
    DDX_Control(pDX, IDC_EDIT_STOCK_NEW, m_editNew);
}

BEGIN_MESSAGE_MAP(CStockEditDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_STOCK_SAVE, &CStockEditDlg::OnBnClickedSave)
END_MESSAGE_MAP()

BOOL CStockEditDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_editTitle.SetWindowText(m_title);
    m_editCurrent.SetWindowText(m_stock);
    m_editNew.SetWindowText(m_stock);
    return TRUE;
}

void CStockEditDlg::OnBnClickedSave()
{
    CString newStock;
    m_editNew.GetWindowText(newStock);

    if (newStock.IsEmpty()) {
        AfxMessageBox(L"새 재고 수량을 입력하세요.");
        return;
    }

    // 숫자인지 확인
    for (int i = 0; i < newStock.GetLength(); i++) {
        if (!iswdigit(newStock[i])) {
            AfxMessageBox(L"숫자만 입력하세요.");
            return;
        }
    }

    std::wstring query =
        L"UPDATE Book SET Stock=" + std::wstring(newStock) +
        L" WHERE BookID=" + std::wstring(m_bookID);

    if (g_db.ExecuteQuery(query)) {
        AfxMessageBox(L"재고가 " + newStock + L"권으로 수정되었습니다.");
        CDialogEx::OnOK();
    }
}