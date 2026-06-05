#include "pch.h"
#include "BookstoreAdmin.h"
#include "CReplyDlg.h"

BEGIN_MESSAGE_MAP(CReplyDlg, CDialogEx)
END_MESSAGE_MAP()

CReplyDlg::CReplyDlg(const CString& reviewContent, CWnd* pParent)
    : CDialogEx(IDD_REPLY_DIALOG, pParent), m_reviewContent(reviewContent) {
}

CReplyDlg::~CReplyDlg() {}

void CReplyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_REVIEW_CONTENT, m_editContent);
    DDX_Control(pDX, IDC_EDIT_REPLY_CONTENT, m_editReply);
}

BOOL CReplyDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_editContent.SetWindowText(m_reviewContent);
    return TRUE;
}

void CReplyDlg::OnOK()
{
    m_editReply.GetWindowText(m_reply);
    if (m_reply.IsEmpty()) {
        AfxMessageBox(L"답글 내용을 입력하세요.");
        return;
    }
    CDialogEx::OnOK();
}