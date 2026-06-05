#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include "resource.h"

class CPriceEditDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CPriceEditDlg)
public:
    CPriceEditDlg(const CString& bookID, const CString& title,
                  const CString& price, CWnd* pParent = nullptr);
    virtual ~CPriceEditDlg();
    enum { IDD = IDD_STOCK_EDIT };   // 기존 재고수정 다이얼로그 재활용

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedSave();
    DECLARE_MESSAGE_MAP()

private:
    CString m_bookID;
    CString m_title;
    CString m_price;
    CEdit   m_editTitle;
    CEdit   m_editCurrent;
    CEdit   m_editNew;
};
