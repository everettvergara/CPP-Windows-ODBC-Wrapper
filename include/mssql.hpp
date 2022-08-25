#pragma once
#define UNICODE
#include <iostream>
#include <string>
#include <exception>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        #define MESSAGE_SIZE 1024

        class odbc {

        private:
            SQLHENV     env_{nullptr};
            SQLHDBC     dbc_{nullptr};
            SQLHSTMT    stmt_{nullptr};
            WCHAR       last_message_[MESSAGE_SIZE]{'\0'};
            WCHAR       last_state_[SQL_SQLSTATE_SIZE+1];
            SQLINTEGER  last_error_{0};

            void check_for_message(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE rc) {
                SQLSMALLINT i = 0;
                if (rc == SQL_INVALID_HANDLE) {
                    wcscpy(last_message_, L"Invalid Handle!");
                    return;
                } 

                while (SQLGetDiagRec(hType, hHandle, ++i, 
                        last_state_, &last_error_, last_message_, 
                        MESSAGE_SIZE, static_cast<SQLSMALLINT *>(NULL)) == SQL_SUCCESS);
            }

        public:

            odbc() {}
            odbc(const odbc &) = delete;
            odbc(odbc &&) = delete;
            auto operator=(const odbc &) -> odbc & = delete;
            auto operator=(odbc &&) -> odbc & = delete;
            ~odbc() {disconnect(); free_env();}

            inline auto get_last_error() const -> const wchar_t * {
                return last_message_;
            }

            auto handle_ret_code(SQLHANDLE handle, SQLSMALLINT type, RETCODE rc) -> bool {
                if(rc != SQL_SUCCESS) check_for_message(handle, type, rc);
                if(rc == SQL_ERROR) return false;
                return true;
            }

            auto alloc_env() -> bool {
                if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) == SQL_ERROR) return false;
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)); 
            }

            auto alloc_connection() -> bool {
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_));
            }

            auto connect_by_dsn(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLConnect(dbc_, 
                    const_cast<wchar_t *>(server.c_str()), server.size(), 
                    const_cast<wchar_t *>(user.c_str()), user.size(), 
                    const_cast<wchar_t *>(passwd.c_str()), passwd.size()));
            }

            auto connect_by_file_dsn(const std::wstring &str) -> bool {
                wchar_t buff[1024];
                SQLSMALLINT actualsize;
                std::wstring conn_str = L"FILEDSN=D:\\Everett\\Codes\\Projects\\Personal\\Tools\\MSSQL-Connector\\db\\local.dsn; UID=sa; PWD=Kerberos2014!";
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLDriverConnect(dbc_, NULL, 
                    const_cast<wchar_t *>(conn_str.c_str()),
                    conn_str.size(), 
                    buff, 1024, &actualsize, SQL_DRIVER_NOPROMPT));
            }

            auto disconnect() -> bool {
                if(!dbc_) return true;
                if(SQLRETURN rc = SQLDisconnect(dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return false;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return false;
                return true;
            }

            auto free_env() -> bool {
                if(!env_) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, env_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return false;
                return true;
            }

        };



    }
}