#pragma once
#include "afxdialogex.h"
#include "Resource.h"
#include "CDBManager.h"

class CMemberDetailDlg : public CDialogEx
{
public:
    CMemberDetailDlg(const CString& memberID, CWnd* pParent = nullptr);
    virtual ~CMemberDetailDlg();
    enum { IDD = IDD_MEMBER_DETAIL };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedSave();
    DECLARE_MESSAGE_MAP()

private:
    CString m_memberID;
    CEdit   m_editID;
    CEdit   m_editName;
    CEdit   m_editEmail;
    CEdit   m_editPhone;
    CEdit   m_editAddr;
    CEdit   m_editBirth;
    CEdit   m_editDate;

    void LoadMemberInfo();
};