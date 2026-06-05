#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"

class CReviewTab : public CDialogEx
{
    DECLARE_DYNAMIC(CReviewTab)
public:
    CReviewTab(CWnd* pParent = nullptr);
    virtual ~CReviewTab();
    enum { IDD = IDD_REVIEW_TAB };
    void SearchReviews(const CString& keyword);  // CAdminMainDlg에서 호출
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}
    afx_msg void OnBnClickedReply();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl m_listReview;
    HBRUSH    m_hBrushBg;
    HFONT     m_hFont;
    void LoadReviews(const CString& keyword = L"");
    void SetupListColumns();
    void ApplyListStyle();
};
