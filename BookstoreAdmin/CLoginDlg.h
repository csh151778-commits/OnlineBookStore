#pragma once
#include "afxdialogex.h"
#include "CDBManager.h"


// CLoginDlg 대화 상자

class CLoginDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLoginDlg)

public:
	CLoginDlg(CWnd* pParent = nullptr);
	virtual ~CLoginDlg();
	enum { IDD = IDD_LOGIN_DIALOG };

// 대화 상자 데이터입니다.
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	DECLARE_MESSAGE_MAP()    // DDX/DDV 지원입니다.

private:
	CEdit m_editID;
	CEdit m_editPW;
};
