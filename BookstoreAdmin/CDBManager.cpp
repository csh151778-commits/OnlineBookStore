#include "pch.h"
#include "CDBManager.h"

CDBManager::CDBManager() : m_hEnv(NULL), m_hDbc(NULL), m_hStmt(NULL) {}

CDBManager::~CDBManager() { Disconnect(); }

bool CDBManager::Connect()
{
    // 환경 핸들 생성
    SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
    SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    // 연결 핸들 생성
    SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDbc);

    // DSN 연결 (아까 등록한 BookstoreAdmin 사용)
    SQLWCHAR connStr[] = L"DSN=BookstoreAdmin;UID=root;PWD=gLAKIHdIhqFvgxCpnGPhAOFvFOeURBZx;";
    SQLRETURN ret = SQLDriverConnectW(m_hDbc, NULL, connStr, SQL_NTS,
        NULL, 0, NULL, SQL_DRIVER_NOPROMPT);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        AfxMessageBox(L"DB 연결 실패!");
        return false;
    }

    // 구문 핸들 생성
    SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &m_hStmt);
    return true;
}

void CDBManager::Disconnect()
{
    if (m_hStmt) { SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt); m_hStmt = NULL; }
    if (m_hDbc) { SQLDisconnect(m_hDbc); SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc); m_hDbc = NULL; }
    if (m_hEnv) { SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv); m_hEnv = NULL; }
}

bool CDBManager::ExecuteQuery(const std::wstring& query)
{
    SQLFreeStmt(m_hStmt, SQL_CLOSE);
    SQLRETURN ret = SQLExecDirectW(m_hStmt, (SQLWCHAR*)query.c_str(), SQL_NTS);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

bool CDBManager::Fetch()
{
    return SQLFetch(m_hStmt) == SQL_SUCCESS;
}

std::wstring CDBManager::GetData(int col)
{
    SQLWCHAR buf[512] = {};
    SQLLEN len = 0;
    SQLGetData(m_hStmt, col, SQL_C_WCHAR, buf, sizeof(buf), &len);
    return (len > 0) ? std::wstring(buf) : L"";
}