#pragma once
#define UNICODE
#include <iostream>
#include <string>
#include <exception>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);

        class odbc {

        private:
            SQLHENV         hEnv_{nullptr};
            SQLHDBC         hDbc_{nullptr};
            SQLHSTMT        hStmt_{nullptr};
            std::wstring    last_error_{};

        public:

            odbc() {}
            odbc(const odbc &) = delete;
            odbc(odbc &&) = delete;
            auto operator=(const odbc &) -> odbc & = delete;
            auto operator=(odbc &&) -> odbc & = delete;
            ~odbc() {disconnect(); free_env();}

            inline auto get_last_error() -> std::wstring & {
                return last_error_;
            }

            auto handle_ret_code(RETCODE rc) -> bool {
                if(rc != SQL_SUCCESS) HandleDiagnosticRecord(hEnv_, SQL_HANDLE_ENV, rc);
                if(rc == SQL_ERROR) return false;
                return true;
            }

            auto alloc_env() -> bool {
                if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv_) == SQL_ERROR) return false;
                return handle_ret_code(SQLSetEnvAttr(hEnv_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)); 
            }

            auto alloc_handle() -> bool {
                return handle_ret_code(SQLAllocHandle(SQL_HANDLE_DBC, hEnv_, &hDbc_));
            }

            auto connect_by_dsn(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
                return handle_ret_code(SQLConnect(hDbc_, 
                    const_cast<wchar_t *>(server.c_str()), server.size(), 
                    const_cast<wchar_t *>(user.c_str()), user.size(), 
                    const_cast<wchar_t *>(passwd.c_str()), passwd.size()));
            }

            auto connect_by_conn_string(const std::wstring &str) -> bool {
                wchar_t buff[1024];
                SQLSMALLINT actualsize;
                std::wstring conn_str = L"FILEDSN=D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn; UID=sa; PWD=Kerberos2014!";
                return handle_ret_code(SQLDriverConnect(hDbc_, NULL, 
                    const_cast<wchar_t *>(conn_str.c_str()),
                    conn_str.size(), 
                    buff, 1024, &actualsize, SQL_DRIVER_NOPROMPT));
            }

            auto disconnect() -> void {
                if(hDbc_) {
                    SQLDisconnect(hDbc_);
                    SQLFreeHandle(SQL_HANDLE_DBC, hDbc_);
                }
            }

            auto free_env() -> void {
                if(hEnv_) {
                    SQLFreeHandle(SQL_HANDLE_ENV, hEnv_);
                }
            }

        };


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
    }
}