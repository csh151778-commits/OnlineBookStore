// CPriceEditDlg.cpp
#include "pch.h"
#include "CPriceEditDlg.h"
#include "BookstoreAdmin.h"

extern CDBManager g_db;

IMPLEMENT_DYNAMIC(CPriceEditDlg, CDialogEx)

CPriceEditDlg::CPriceEditDlg(const CString& bookID, const CString& title,
    const CString& price, CWnd* pParent)
    : CDialogEx(IDD_STOCK_EDIT, pParent),
    m_bookID(bookID), m_title(title), m_price(price) {}

CPriceEditDlg::~CPriceEditDlg() {}

void CPriceEditDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_STOCK_TITLE,   m_editTitle);
    DDX_Control(pDX, IDC_EDIT_STOCK_CURRENT, m_editCurrent);
    DDX_Control(pDX, IDC_EDIT_STOCK_NEW,     m_editNew);
}

BEGIN_MESSAGE_MAP(CPriceEditDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_STOCK_SAVE, &CPriceEditDlg::OnBnClickedSave)
END_MESSAGE_MAP()

BOOL CPriceEditDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowText(L"가격 수정");
    m_editTitle.SetWindowText(m_title);
    m_editCurrent.SetWindowText(m_price);
    m_editNew.SetWindowText(m_price);
    return TRUE;
}

void CPriceEditDlg::OnBnClickedSave()
{
    CString newPrice;
    m_editNew.GetWindowText(newPrice);

    if (newPrice.IsEmpty()) {
        AfxMessageBox(L"새 가격을 입력하세요.");
        return;
    }
    for (int i = 0; i < newPrice.GetLength(); i++) {
        if (!iswdigit(newPrice[i])) {
            AfxMessageBox(L"숫자만 입력하세요.");
            return;
        }
    }

    std::wstring query =
        L"UPDATE Book SET Price=" + std::wstring(newPrice) +
        L" WHERE BookID=" + std::wstring(m_bookID);

    if (g_db.ExecuteQuery(query)) {
        AfxMessageBox(L"가격이 " + newPrice + L"원으로 수정되었습니다.");
        CDialogEx::OnOK();
    }
}
