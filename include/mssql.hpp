#pragma once
#define UNICODE

#include <iostream>
#include <sstream>
#include <tuple>
#include <vector>
#include <string>
#include <exception>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        #define SQL_QUERY_SIZE 8192
        #define MESSAGE_SIZE 1024

        struct odbc_error {
            WCHAR       last_message[MESSAGE_SIZE];
            WCHAR       last_state[SQL_SQLSTATE_SIZE+1];
            SQLINTEGER  last_error;

            odbc_error() {};
            odbc_error(const WCHAR *msg, const WCHAR *state, SQLINTEGER error) : last_error(error) {
                wcscpy(last_message, msg);
                wcscpy(last_state, state);
            }
        };

        class odbc_error_mgr {
            
            size_t max_errors_;
            int ix_ = -1;
            std::vector<odbc_error> errors_;

        public:
            odbc_error_mgr(size_t max_errors) : max_errors_(max_errors), errors_(max_errors_) {}

            auto get_current_ix() -> int {return ix_;}
            auto get_current_message() -> WCHAR * {return ix_ < 0 ? NULL : errors_[ix_].last_message;}
            auto get_current_state() -> WCHAR * {return ix_ < 0 ? NULL : errors_[ix_].last_state;}       
            auto get_current_error() -> SQLINTEGER * {return ix_ < 0 ? NULL : &errors_[ix_].last_error;}       
            
            auto next() -> void {
                ix_ = ix_ == max_errors_ ? ix_ : ix_ + 1;
            }
            
            auto prev() -> void {
                ix_ = ix_ == -1 ? ix_ : ix_ - 1;
            }

            auto reset() -> void {
                ix_ = -1;
            }

            auto get_raw_errors() const -> const std::vector<odbc_error> & {
                return errors_;
            }

            auto get_formatted_errors() const -> std::wstring {
                std::wstring out;
                for(int i{0}; i <= ix_; ++i) {
                    out = errors_[i].last_state;
                        out += L": ("; 
                        out += std::to_wstring(errors_[i].last_error);
                        out += L") ";
                        out += errors_[i].last_message;
                        out += L"\n";
                }
                return out;
            }     
        };

        class odbc {
        private:
            SQLHENV     env_{NULL};
            SQLHDBC     dbc_{NULL};
            SQLHSTMT    stmt_{NULL};
            // WCHAR       last_message_[MESSAGE_SIZE]{'\0'};
            // WCHAR       last_state_[SQL_SQLSTATE_SIZE+1];
            // SQLINTEGER  last_error_{0};

            void check_for_message(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE rc) {
                if(rc == SQL_INVALID_HANDLE) {
                    set_user_error(L"Invalid Handle");
                    return;
                } 
                SQLSMALLINT i = 0;
                do {
                    // auto &m = msg_.emplace_back();

                    // RETCODE rc = SQLGetDiagRec(hType, hHandle, ++i, m.last_state, &m.last_error, m.last_message, 
                    //                 MESSAGE_SIZE, static_cast<SQLSMALLINT *>(NULL));
                    
                    // if(rc != SQL_SUCCESS) {msg_.pop_back(); break;}                

               } while(true);
            }

            auto set_user_error(const WCHAR *error_msg) -> bool {
                // msg_.emplace_back(L"Custom Error here", L"", 50001);
                return false;
            }

            auto handle_ret_code(SQLHANDLE handle, SQLSMALLINT type, RETCODE rc) -> bool {
                if(rc != SQL_SUCCESS) check_for_message(handle, type, rc);
                if(rc == SQL_ERROR) return false;
                return true;
            }

           auto alloc_env() -> bool {
                if(env_ != NULL) return true;
                if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) == SQL_ERROR) return set_user_error(L"Could not allocate environment handle");
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)); 
            }

            auto alloc_connection() -> bool {
                if(dbc_ != NULL) return true;
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_));
            }

            auto dealloc_connection() -> bool {
                if(dbc_ == NULL) return true;
                if(SQLRETURN rc = SQLDisconnect(dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not disconnect db handle");
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not deallocate db handle");
                dbc_ = NULL;
                return true;
            }

            auto dealloc_env() -> bool {
                if(env_ == NULL) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, env_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not free up env handle");
                env_ = NULL;
                return true;
            }

            auto alloc_statement() -> bool {
                if(stmt_ != NULL) return true;
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt_));
            }

            auto dealloc_statement() -> bool {
                if(stmt_ == NULL) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, stmt_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) return set_user_error(L"Could not free up stmt handle");
                stmt_ = NULL;
                return true;
            }

            auto init() -> bool {
                if(env_ == NULL && !alloc_env()) return false;
                if(dbc_ == NULL && !alloc_connection()) return false;
                return true;
            }

        public:

            odbc() {}
            odbc(const odbc &) = delete;
            odbc(odbc &&) = delete;
            auto operator=(const odbc &) -> odbc & = delete;
            auto operator=(odbc &&) -> odbc & = delete;
            ~odbc() {disconnect();}

            // inline auto get_last_error() const -> const std::tuple<const WCHAR *, const WCHAR *, SQLINTEGER> {
            //     return {last_state_, last_message_, last_error_};
            // }

            // inline auto get_formatted_last_error() const -> std::wstring {
            //     // std::wstring out = last_state_;
            //     //     out += L": ("; 
            //     //     out += std::to_wstring(last_error_);
            //     //     out += L") ";
            //     //     out += last_message_;
            //     //     out += L"\n";
            //     return out;
            // }

            auto connect_by_dsn(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
                if(!init()) return false;
                if(!handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLConnect(dbc_, 
                    const_cast<wchar_t *>(server.c_str()), server.size(), 
                    const_cast<wchar_t *>(user.c_str()), user.size(), 
                    const_cast<wchar_t *>(passwd.c_str()), passwd.size()))) return false;
                return alloc_statement();
            }

            auto connect_by_file_dsn(const std::wstring &dsn, const std::wstring &user, const std::wstring &passwd) -> bool {
                if(!init()) return false;
                std::wstring conn_str = L"FILEDSN=" + dsn  + L"; UID=" + user + L"; PWD=" + passwd; 
                if(!handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLDriverConnect(dbc_, NULL, 
                    const_cast<wchar_t *>(conn_str.c_str()), conn_str.size(), 
                    NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) return false;
                return alloc_statement();
            }

            auto disconnect() -> bool {
                if(!dealloc_statement()) return false;
                if(!dealloc_connection()) return false;
                if(!dealloc_env()) return false;
                return true;
            }

            auto exec(wchar_t *command) -> RETCODE {
                return SQLExecDirect(stmt_, command, SQL_NTS);
            }

            auto handle_exec(RETCODE rc) -> bool {

                // switch(rc) {
                //     case SQL_SUCCESS_WITH_INFO: 
                //         check_for_message(stmt_, SQL_HANDLE_STMT, rc);
                //     case SQL_SUCCESS:
                //         // check_for_mess
                // }
                return true;
            }
        };
    }
}