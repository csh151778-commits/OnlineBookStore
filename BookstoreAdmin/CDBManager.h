#pragma once
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <string>

class CDBManager
{
public:
    CDBManager();
    ~CDBManager();

    bool Connect();
    void Disconnect();
    bool ExecuteQuery(const std::wstring& query);
    bool Fetch();
    std::wstring GetData(int col);

private:
    SQLHENV  m_hEnv;
    SQLHDBC  m_hDbc;
    SQLHSTMT m_hStmt;
};

