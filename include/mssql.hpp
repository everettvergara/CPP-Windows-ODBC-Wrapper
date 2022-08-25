#pragma once
#define UNICODE
#include <iostream>
#include <string>
#include <exception>
#include <sqltypes.h>

typedef struct STR_BINDING {
    SQLSMALLINT         cDisplaySize;           /* size to display  */
    WCHAR               *wszBuffer;             /* display buffer   */
    SQLLEN              indPtr;                 /* size or null     */
    BOOL                fChar;                  /* character col?   */
    struct STR_BINDING  *sNext;                 /* linked list      */
} BINDING;


void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
void DisplayResults(HSTMT hStmt, SQLSMALLINT cCols);
void AllocateBindings(HSTMT hStmt, SQLSMALLINT cCols, BINDING** ppBinding, SQLSMALLINT*  pDisplay);

namespace g80 {

    #define SQL_QUERY_SIZE 1000 

    class mssql {

    private:
        SQLHENV     hEnv = NULL;
        SQLHDBC     hDbc = NULL;
        SQLHSTMT    hStmt = NULL;
        WCHAR*      pwszConnStr = NULL;
        WCHAR       wszInput[SQL_QUERY_SIZE];

    public:

        mssql() {}

        auto alloc_null_env() -> bool {
            if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR) return false;
            return true;
        }

        auto set_env_attr() -> bool {
            RETCODE rc = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0); 
            if(rc != SQL_SUCCESS) HandleDiagnosticRecord(hEnv, SQL_HANDLE_ENV, rc);
            if(rc == SQL_ERROR) return false;
            return true;
        }

        auto alloc_handle() -> bool {
            RETCODE rc = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
            if(rc != SQL_SUCCESS) HandleDiagnosticRecord(hEnv, SQL_HANDLE_ENV, rc);
            if(rc == SQL_ERROR) return false;
            return true;
        }

        auto connect(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
            RETCODE rc = SQLConnect(hDbc, 
                const_cast<wchar_t *>(server.c_str()), server.size(), 
                const_cast<wchar_t *>(user.c_str()), user.size(), 
                const_cast<wchar_t *>(passwd.c_str()), passwd.size());                      
            if(rc != SQL_SUCCESS) HandleDiagnosticRecord(hEnv, SQL_HANDLE_DBC, rc);
            if(rc == SQL_ERROR) return false;
            return true;
        }

        auto connect2(const std::wstring &str) -> bool {
            //wchar_t value[500];
            // SQLReadFileDSN(
            //     L"D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn", 
            //     L"ODBC",
            //     L"DRIVER",
            //     value, 500, &actualsize);
            wchar_t buff[1024];
            SQLSMALLINT actualsize;
            std::wstring conn_str = L"FILEDSN=D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn; UID=sa; PWD=Kerberos2014!";
            RETCODE rc = SQLDriverConnect(hDbc, NULL, 
                const_cast<wchar_t *>(conn_str.c_str()),
                conn_str.size(), 
                buff, 1024, &actualsize, SQL_DRIVER_NOPROMPT);

            //     );                      
            // if(rc != SQL_SUCCESS) HandleDiagnosticRecord(hEnv, SQL_HANDLE_DBC, rc);
            // if(rc == SQL_ERROR) return false;
            return true;
        }

    };
}


void HandleDiagnosticRecord (SQLHANDLE      hHandle,
                             SQLSMALLINT    hType,
                             RETCODE        RetCode)
{
    SQLSMALLINT iRec = 0;
    SQLINTEGER  iError;
    WCHAR       wszMessage[1000];
    WCHAR       wszState[SQL_SQLSTATE_SIZE+1];


    if (RetCode == SQL_INVALID_HANDLE)
    {
        fwprintf(stderr, L"Invalid handle!\n");
        return;
    }

    while (SQLGetDiagRec(hType,
                         hHandle,
                         ++iRec,
                         wszState,
                         &iError,
                         wszMessage,
                         (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)),
                         (SQLSMALLINT *)NULL) == SQL_SUCCESS)
    {
        // Hide data truncated..
        if (wcsncmp(wszState, L"01004", 5))
        {
            fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
        }
    }
}