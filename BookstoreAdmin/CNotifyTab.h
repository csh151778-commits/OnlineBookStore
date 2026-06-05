#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include <string>

class CNotifyTab : public CDialogEx
{
    DECLARE_DYNAMIC(CNotifyTab)

public:
    CNotifyTab(CWnd* pParent = nullptr);
    virtual ~CNotifyTab();
    enum { IDD = IDD_NOTIFY_TAB };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}
    afx_msg void OnBnClickedAutoFill();
    afx_msg void OnBnClickedSendEmail();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()

private:
    CEdit     m_editTo;
    CEdit     m_editSubject;
    CEdit     m_editBody;
    CListCtrl m_listLog;
    HBRUSH    m_hBrushBg;
    HFONT     m_hFont;

    void SetupLogColumns();
    void ApplyListStyle();
    void AddLog(const CString& to, const CString& subject);
    bool SendMail(const std::wstring& to,
        const std::wstring& subject,
        const std::wstring& body);
};
