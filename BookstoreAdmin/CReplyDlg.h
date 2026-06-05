#pragma once
#include "afxdialogex.h"
#include "Resource.h"

class CReplyDlg : public CDialogEx
{
public:
    CReplyDlg(const CString& reviewContent, CWnd* pParent = nullptr);
    virtual ~CReplyDlg();
    enum { IDD = IDD_REPLY_DIALOG };

    CString GetReply() { return m_reply; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    DECLARE_MESSAGE_MAP()

private:
    CString m_reviewContent;
    CString m_reply;
    CEdit   m_editContent;
    CEdit   m_editReply;
};