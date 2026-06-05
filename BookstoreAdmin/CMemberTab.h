#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include "CMemberDetailDlg.h"
#include "CColorHeaderCtrl.h"

class CMemberTab : public CDialogEx
{
    DECLARE_DYNAMIC(CMemberTab)
public:
    CMemberTab(CWnd* pParent = nullptr);
    virtual ~CMemberTab();
    enum { IDD = IDD_MEMBER_TAB };
    void SearchMembers(const CString& keyword);  // CAdminMainDlg에서 호출
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}
    afx_msg void OnBnClickedDetail();
    afx_msg void OnBnClickedEdit();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    DECLARE_MESSAGE_MAP()
private:
    CListCtrl          m_listMember;
    CColorHeaderCtrl   m_headerCtrl;
    CEdit              m_editSearch;    // 내부 검색창
    CButton            m_btnSearch;     // 내부 검색 버튼
    HBRUSH             m_hBrushBg;
    HFONT              m_hFont;
    void LoadMembers(const CString& keyword = L"");
    void SetupListColumns();
    void ApplyListStyle();
    void ResizeColumns();
    afx_msg void OnBnClickedSearch();
};
