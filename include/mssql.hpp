#pragma once
#define UNICODE

#include <iostream>
#include <sstream>
#include <tuple>
#include <string>
#include <exception>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        #define MESSAGE_SIZE 1024

        class odbc {
        private:
            SQLHENV     env_{NULL};
            SQLHDBC     dbc_{NULL};
            SQLHSTMT    stmt_{NULL};
            WCHAR       last_message_[MESSAGE_SIZE]{'\0'};
            WCHAR       last_state_[SQL_SQLSTATE_SIZE+1];
            SQLINTEGER  last_error_{0};

            void check_for_message(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE rc) {
                if(rc == SQL_INVALID_HANDLE) {
                    wcscpy(last_message_, L"Invalid Handle!");
                    return;
                } 
                SQLSMALLINT i = 0;
                while(SQLGetDiagRec(hType, hHandle, ++i, 
                        last_state_, &last_error_, last_message_, 
                        MESSAGE_SIZE, static_cast<SQLSMALLINT *>(NULL)) == SQL_SUCCESS);
            }

            auto set_user_error(const std::wstring &error_msg) -> bool {
                wcscpy(last_message_, error_msg.c_str()); 
                std::fill_n(last_state_, SQL_SQLSTATE_SIZE, TCHAR('?')); 
                last_state_[SQL_SQLSTATE_SIZE] = TCHAR('\0');
                last_error_ = 50001;
                return false;
            }

            auto handle_ret_code(SQLHANDLE handle, SQLSMALLINT type, RETCODE rc) -> bool {
                if(rc != SQL_SUCCESS) check_for_message(handle, type, rc);
                if(rc == SQL_ERROR) return false;
                return true;
            }

           auto alloc_env() -> bool {
                if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) == SQL_ERROR) return set_user_error(L"Could not allocate environment handle");
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)); 
            }

            auto alloc_connection() -> bool {
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_));
            }

            auto init() -> bool {
                if(env_ == NULL && !alloc_env()) return false;
                if(dbc_ == NULL && !alloc_connection()) return false;
                return true;
            }

            auto dealloc_connection() -> bool {
                if(!dbc_) return true;
                if(SQLRETURN rc = SQLDisconnect(dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not disconnect db handle");
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not deallocate db handle");
                return true;
            }

            auto dealloc_env() -> bool {
                if(!env_) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, env_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not free up env handle");
                return true;
            }

        public:

            odbc() {}
            odbc(const odbc &) = delete;
            odbc(odbc &&) = delete;
            auto operator=(const odbc &) -> odbc & = delete;
            auto operator=(odbc &&) -> odbc & = delete;
            ~odbc() {disconnect();}

            inline auto get_last_error() const -> const std::tuple<const WCHAR *, const WCHAR *, SQLINTEGER> {
                return {last_state_, last_message_, last_error_};
            }

            inline auto get_formatted_last_error() const -> std::wstring {
                std::wstring out = last_state_;
                    out += L": ("; 
                    out += std::to_wstring(last_error_);
                    out += L") ";
                    out += last_message_;
                    out += L"\n";
                return out;
            }

            auto connect_by_dsn(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
                if(!init()) return false;
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLConnect(dbc_, 
                    const_cast<wchar_t *>(server.c_str()), server.size(), 
                    const_cast<wchar_t *>(user.c_str()), user.size(), 
                    const_cast<wchar_t *>(passwd.c_str()), passwd.size()));
            }

            auto connect_by_file_dsn(const std::wstring &dsn, const std::wstring &user, const std::wstring &passwd) -> bool {
                if(!init()) return false;
                std::wstring conn_str = L"FILEDSN=" + dsn  + L"; UID=" + user + L"; PWD=" + passwd; 
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLDriverConnect(dbc_, NULL, 
                    const_cast<wchar_t *>(conn_str.c_str()), conn_str.size(), 
                    NULL, 0, NULL, SQL_DRIVER_NOPROMPT));
            }

            auto disconnect() -> bool {
                if(!dealloc_env()) return false;
                if(!dealloc_connection()) return false;
                return true;
           }
        };
    }
}