// CLoginDlg.cpp: 구현 파일
//

#include "pch.h"
#include "BookstoreAdmin.h"
#include "afxdialogex.h"
#include "CLoginDlg.h"

extern CDBManager g_db;
// CLoginDlg 대화 상자

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DIALOG, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_ID, m_editID);
	DDX_Control(pDX, IDC_EDIT_PW, m_editPW);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CLoginDlg::OnBnClickedOk)
END_MESSAGE_MAP()

BOOL CLoginDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(L"관리자 로그인");
	return TRUE;
}
void CLoginDlg::OnBnClickedOk()
{
    CString id, pw;
    m_editID.GetWindowText(id);
    m_editPW.GetWindowText(pw);

    std::wstring query = L"SELECT COUNT(*) FROM Member WHERE MemberID='"
        + std::wstring(id) + L"' AND Password='" + std::wstring(pw)
        + L"' AND Role='admin'";

    if (g_db.ExecuteQuery(query) && g_db.Fetch()) {
        std::wstring cnt = g_db.GetData(1);
        if (cnt == L"1") {
            CDialogEx::OnOK();
            return;
        }
    }
    AfxMessageBox(L"ID 또는 비밀번호가 틀렸습니다.");
}
// CLoginDlg 메시지 처리기
