#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"
#include "resource.h"


// CStockEditDlg 대화 상자

class CStockEditDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CStockEditDlg)

public:
	CStockEditDlg(const CString& bookID, const CString& title,
		const CString& stock, CWnd* pParent = nullptr);
	virtual ~CStockEditDlg();
	enum { IDD = IDD_STOCK_EDIT };

// 대화 상자 데이터입니다.


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedSave();
	DECLARE_MESSAGE_MAP()

private:
	CString m_bookID;
	CString m_title;
	CString m_stock;

	CEdit m_editTitle;
	CEdit m_editCurrent;
	CEdit m_editNew;
};
