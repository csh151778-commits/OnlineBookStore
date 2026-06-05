#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include "CStockEditDlg.h"
#include "CColorHeaderCtrl.h"

#define IDC_BTN_PRICE_EDIT  9001

class CStockTab : public CDialogEx
{
    DECLARE_DYNAMIC(CStockTab)
public:
    CStockTab(const std::wstring& genre = L"", CWnd* pParent = nullptr);
    virtual ~CStockTab();
    enum { IDD = IDD_STOCK_TAB };
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}
    afx_msg void OnBnClickedStockSearch();
    afx_msg void OnBnClickedStockEdit();
    afx_msg void OnBnClickedStockEmail();
    afx_msg void OnBnClickedPriceEdit();
    afx_msg void OnBnClickedChkLowStock();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    DECLARE_MESSAGE_MAP()
private:
    CDBManager         m_db;
    CEdit              m_editSearch;
    CListCtrl          m_listStock;
    CButton            m_chkLowStock;
    CButton            m_btnPriceEdit;
    CColorHeaderCtrl   m_headerCtrl;
    std::wstring       m_genre;
    HBRUSH             m_hBrushBg;
    HFONT              m_hFont;
    void LoadStock(const CString& keyword = L"", bool lowOnly = false);
    void SetupListColumns();
    void ApplyListStyle();
    void ResizeColumns();
public:
    void Reload() { LoadStock(L"", false); }
};
