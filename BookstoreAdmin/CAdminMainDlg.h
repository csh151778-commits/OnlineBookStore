#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include "CMemberTab.h"
#include "COrderTab.h"
#include "CStockTab.h"
#include "CReviewTab.h"
#include "CNotifyTab.h"

class CAdminMainDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CAdminMainDlg)
public:
    CAdminMainDlg(CWnd* pParent = nullptr);
    virtual ~CAdminMainDlg();
    enum { IDD = IDD_MAIN_DIALOG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() { CDialogEx::OnCancel(); }
    afx_msg void OnTcnSelchangeTabLeft(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTcnSelchangeTabRight(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnTcnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnPaint();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    DECLARE_MESSAGE_MAP()
private:
    CTabCtrl    m_tabLeft;
    CMemberTab* m_pMemberTab;
    COrderTab*  m_pOrderTab;
    CReviewTab* m_pReviewTab;
    CNotifyTab* m_pNotifyTab;

    CTabCtrl    m_tabRight;
    CStockTab*  m_pStockAll;
    CStockTab*  m_pStockNovel;
    CStockTab*  m_pStockPoem;
    CStockTab*  m_pStockEssay;
    CStockTab*  m_pStockComic;
    CStockTab*  m_pStockEtc;

    // 왼쪽 공용 검색창

    HBRUSH  m_hBrushBg;
    HFONT   m_hFontTitle;
    HFONT   m_hFont;
    CRect   m_rectTitleLeft;
    CRect   m_rectTitleRight;

    void ResizeControls();
    void ShowLeftTab(int sel);
    void ShowRightTab(int sel);
};
