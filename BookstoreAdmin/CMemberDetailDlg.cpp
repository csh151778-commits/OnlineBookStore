#include "pch.h"
#include "BookstoreAdmin.h"
#include "CMemberDetailDlg.h"

extern CDBManager g_db;

BEGIN_MESSAGE_MAP(CMemberDetailDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_MEM_SAVE, &CMemberDetailDlg::OnBnClickedSave)
END_MESSAGE_MAP()

CMemberDetailDlg::CMemberDetailDlg(const CString& memberID, CWnd* pParent)
    : CDialogEx(IDD_MEMBER_DETAIL, pParent), m_memberID(memberID) {
}

CMemberDetailDlg::~CMemberDetailDlg() {}

void CMemberDetailDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_MEM_ID, m_editID);
    DDX_Control(pDX, IDC_EDIT_MEM_NAME, m_editName);
    DDX_Control(pDX, IDC_EDIT_MEM_EMAIL, m_editEmail);
    DDX_Control(pDX, IDC_EDIT_MEM_PHONE, m_editPhone);
    DDX_Control(pDX, IDC_EDIT_MEM_ADDR, m_editAddr);
    DDX_Control(pDX, IDC_EDIT_MEM_BIRTH, m_editBirth);
    DDX_Control(pDX, IDC_EDIT_MEM_DATE, m_editDate);
}

BOOL CMemberDetailDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    LoadMemberInfo();
    return TRUE;
}

void CMemberDetailDlg::LoadMemberInfo()
{
    std::wstring query =
        L"SELECT MemberID, Name, Email, Phone, Address, Birth, CreatedAt "
        L"FROM Member WHERE MemberID='" + std::wstring(m_memberID) + L"'";

    if (!g_db.ExecuteQuery(query) || !g_db.Fetch()) return;

    m_editID.SetWindowText(g_db.GetData(1).c_str());
    m_editName.SetWindowText(g_db.GetData(2).c_str());
    m_editEmail.SetWindowText(g_db.GetData(3).c_str());
    m_editPhone.SetWindowText(g_db.GetData(4).c_str());
    m_editAddr.SetWindowText(g_db.GetData(5).c_str());
    m_editBirth.SetWindowText(g_db.GetData(6).c_str());
    m_editDate.SetWindowText(g_db.GetData(7).c_str());
}

void CMemberDetailDlg::OnBnClickedSave()
{
    CString name, email, phone, addr;
    m_editName.GetWindowText(name);
    m_editEmail.GetWindowText(email);
    m_editPhone.GetWindowText(phone);
    m_editAddr.GetWindowText(addr);

    if (name.IsEmpty() || email.IsEmpty()) {
        AfxMessageBox(L"이름과 이메일은 필수 입력입니다.");
        return;
    }

    std::wstring query =
        L"UPDATE Member SET "
        L"Name='" + std::wstring(name) + L"', "
        L"Email='" + std::wstring(email) + L"', "
        L"Phone='" + std::wstring(phone) + L"', "
        L"Address='" + std::wstring(addr) + L"' "
        L"WHERE MemberID='" + std::wstring(m_memberID) + L"'";

    if (g_db.ExecuteQuery(query)) {
        AfxMessageBox(L"회원 정보가 저장되었습니다.");
        CDialogEx::OnOK();
    }
}