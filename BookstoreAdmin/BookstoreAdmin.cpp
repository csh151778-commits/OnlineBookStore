
// BookstoreAdmin.cpp: 애플리케이션에 대한 클래스 동작을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "BookstoreAdmin.h"
#include "BookstoreAdminDlg.h"
#include "CLoginDlg.h"
#include "CDBManager.h"
#include "CAdminMainDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDBManager g_db;

// CBookstoreAdminApp

BEGIN_MESSAGE_MAP(CBookstoreAdminApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CBookstoreAdminApp 생성

CBookstoreAdminApp::CBookstoreAdminApp()
{
	// 다시 시작 관리자 지원
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 CBookstoreAdminApp 개체입니다.

CBookstoreAdminApp theApp;


// CBookstoreAdminApp 초기화

BOOL CBookstoreAdminApp::InitInstance()
{
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();
    AfxEnableControlContainer();

    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    if (!g_db.Connect()) {
        AfxMessageBox(L"DB 연결 실패!");
        return FALSE;
    }

    CLoginDlg dlg;
    if (dlg.DoModal() != IDOK)
        return FALSE;
    CAdminMainDlg mainDlg;
    m_pMainWnd = &mainDlg;
    mainDlg.DoModal();

    return FALSE;
}

