#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"

#ifndef IDC_RADIO_WAITING
#define IDC_RADIO_WAITING            1204
#endif
#ifndef IDC_BTN_DISPATCH_APPROVE
#define IDC_BTN_DISPATCH_APPROVE     1210
#endif
#ifndef IDC_BTN_FORCE_DONE
#define IDC_BTN_FORCE_DONE           1211
#endif
#ifndef IDC_BTN_ORDER_CANCEL
#define IDC_BTN_ORDER_CANCEL         1212
#endif
#ifndef IDC_STATIC_DISPATCH_BOX
#define IDC_STATIC_DISPATCH_BOX      1213
#endif
#ifndef IDC_STATIC_SELECTED_ORDER
#define IDC_STATIC_SELECTED_ORDER    1214
#endif
#ifndef IDC_STATIC_HW_STATUS
#define IDC_STATIC_HW_STATUS         1215
#endif
#ifndef IDC_EDIT_DISPATCH_LOG
#define IDC_EDIT_DISPATCH_LOG        1216
#endif

class COrderTab : public CDialogEx
{
    DECLARE_DYNAMIC(COrderTab)

public:
    COrderTab(CWnd* pParent = nullptr);
    virtual ~COrderTab();

    enum { IDD = IDD_ORDER_TAB };
    void SearchOrders(const CString& keyword);

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK() {}
    virtual void OnCancel() {}

    afx_msg void OnBnClickedRadioAll();
    afx_msg void OnBnClickedRadioWaiting();
    afx_msg void OnBnClickedRadioShipping();
    afx_msg void OnBnClickedRadioDone();
    afx_msg void OnBnClickedOrderDetail();
    afx_msg void OnBnClickedStatus();
    afx_msg void OnBnClickedNotify();
    afx_msg void OnBnClickedDispatchApprove();
    afx_msg void OnBnClickedForceDone();
    afx_msg void OnBnClickedOrderCancel();
    afx_msg void OnItemChangedOrderList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnCustomDrawList(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:
    CListCtrl m_listOrder;
    HBRUSH    m_hBrushBg;
    HFONT     m_hFont;
    CString   m_currentFilter;

    void LoadOrders(const std::wstring& hardwareFilter = L"");
    void SetupListColumns();
    void ApplyListStyle();
    void LayoutControls(int cx, int cy);
    void RefreshDispatchPanel();
    void AppendDispatchLog(const CString& text);
    CString GetSelectedOrderID() const;
    CString GetSelectedOrderStatus() const;
    CString GetSelectedHardwareStatus() const;
    bool IsSelectedOrderCanceled(bool showMessage = true) const;
    std::wstring MakeOrderQuery(const std::wstring& whereClause = L"") const;
    CString EscapeSqlCString(const CString& value) const;
    void MoveDlgItem(int id, int x, int y, int w, int h);
};
