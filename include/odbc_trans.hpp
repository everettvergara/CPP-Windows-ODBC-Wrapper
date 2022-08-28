#pragma once
#define UNICODE

#include <iostream>
#include <sstream>
#include <tuple>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <exception>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include "odbc_exec_msg.hpp"
#include "odbc_col_binding.hpp"

namespace g80 {
    namespace odbc {

        #define SQL_QUERY_SIZE      8192


        class odbc_trans {
        
        private:
        
            SQLHENV     env_{NULL};
            SQLHDBC     dbc_{NULL};
            SQLHSTMT    stmt_{NULL};
            odbc_exec_msg_mgr  msg_;

            auto check_for_message(SQLHANDLE handle, SQLSMALLINT type, RETCODE rc) -> void {
                if(rc == SQL_INVALID_HANDLE) {set_user_error(L"Invalid Handle"); return;}
                SQLSMALLINT i = 0;
                odbc_exec_msg *m = nullptr; 
                do m = msg_.get_next_slot();
                while(SQLGetDiagRec(type, handle, ++i, m->last_state, &m->last_exec_msg, m->last_message, EXEC_MESSAGE_SIZE, static_cast<SQLSMALLINT *>(NULL)) == SQL_SUCCESS);
                msg_.pop_last_slot();
            }

            auto set_user_error(const WCHAR *error_msg) -> bool {
                if(auto err = msg_.get_next_slot(); err) {
                    wcscpy(err->last_message, error_msg);
                    wcscpy(err->last_state, L"");
                    err->last_exec_msg = -1;
                    return true;
                }
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

            odbc_trans(const size_t max_err = 60) : msg_(max_err) {}
            odbc_trans(const odbc_trans &) = delete;
            odbc_trans(odbc_trans &&) = delete;
            auto operator=(const odbc_trans &) -> odbc_trans & = delete;
            auto operator=(odbc_trans &&) -> odbc_trans & = delete;
            ~odbc_trans() {disconnect();}

            inline auto get_last_msg() const -> const odbc_exec_msg_mgr & {
                return msg_;
            }

            inline auto get_formatted_last_msg() const -> std::wstring {
                std::wstring out;
                for(const auto &e : msg_) {
                    out += e.last_state;
                    out += L": ("; 
                    out += std::to_wstring(e.last_exec_msg);
                    out += L") ";
                    out += e.last_message;
                    out += L"\n";
                }
                return out;
            }

            auto connect_by_dsn(const std::wstring &server, const std::wstring &user, const std::wstring &passwd) -> bool {
                msg_.reset();
                if(!init()) return false;
                if(!handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLConnect(dbc_, 
                    const_cast<wchar_t *>(server.c_str()), server.size(), 
                    const_cast<wchar_t *>(user.c_str()), user.size(), 
                    const_cast<wchar_t *>(passwd.c_str()), passwd.size()))) return false;
                return alloc_statement();
            }

            auto connect_by_file_dsn(const std::wstring &dsn, const std::wstring &user, const std::wstring &passwd) -> bool {
                msg_.reset();
                if(!init()) return false;
                std::wstring conn_str = L"FILEDSN=" + dsn  + L"; UID=" + user + L"; PWD=" + passwd; 
                if(!handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLDriverConnect(dbc_, NULL, 
                    const_cast<wchar_t *>(conn_str.c_str()), conn_str.size(), 
                    NULL, 0, NULL, SQL_DRIVER_NOPROMPT))) return false;
                return alloc_statement();
            }

            auto disconnect() -> bool {
                msg_.reset();
                if(!dealloc_statement()) return false;
                if(!dealloc_connection()) return false;
                if(!dealloc_env()) return false;
                return true;
            }

            auto exec(wchar_t *command) -> bool {
                msg_.reset();

                RETCODE rc = SQLExecDirect(stmt_, command, SQL_NTS);
                SQLSMALLINT col_count;
                switch(rc) {
                    case SQL_SUCCESS_WITH_INFO:
                        check_for_message(stmt_, SQL_HANDLE_STMT, rc);

                    case SQL_SUCCESS:
                        if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, SQLNumResultCols(stmt_, &col_count))) return false;
                        if(col_count > 0) {
                            std::vector<col_binding> columns;                       
                            if(!bind_columns(col_count, columns)) return false;
                            do {
                                if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, rc = SQLFetch(stmt_))) return false;
                                if(rc != SQL_NO_DATA_FOUND) {
                                    for(auto &c : columns) {
                                        std::wcout << c.column_name << " (" << c.column_size << "): " << c.buffer << std::endl;
                                        // if(c.indicator != SQL_NULL_DATA)
                                        //     std::wcout << c.column_name << ": " << c.buffer << "\n";
                                        // else 
                                        //     std::wcout << c.column_name << ": <NULL>\n";
                                    }
                                }
                            } while(rc != SQL_NO_DATA_FOUND);
                        }
                }




                return true;
            }

            auto bind_columns(SQLSMALLINT col_count, std::vector<col_binding> &columns) -> bool {
                
                columns.clear();
                columns.resize(col_count);

                for(SQLSMALLINT c{0}, sql_col{1}; c < col_count; c = sql_col++) {

                    if(!handle_ret_code(stmt_,  SQL_HANDLE_STMT,
                        SQLColAttribute(stmt_, sql_col, SQL_DESC_CONCISE_TYPE,
                            NULL, 0, NULL, &columns[c].column_type))) return false;

                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT,
                            SQLColAttribute(stmt_, sql_col, SQL_DESC_LENGTH,
                                NULL, 0, NULL, &columns[c].column_size))) return false;

                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT,
                            SQLColAttribute(stmt_, sql_col, SQL_DESC_NAME,
                                columns[c].column_name, DISPLAY_COLUMN_MAX, 
                                &columns[c].column_display_size, NULL))) return false;
                    
                    columns[c].column_display_size = columns[c].column_display_size > DISPLAY_COLUMN_MAX ? DISPLAY_COLUMN_MAX : columns[c].column_display_size;
                    columns[c].buffer = std::make_unique<WCHAR[]>(columns[c].column_size + 1);
                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, 
                        SQLBindCol(stmt_, sql_col, SQL_C_TCHAR, 
                            static_cast<SQLPOINTER>(columns[c].buffer.get()),
                            (columns[c].column_size + 1) * sizeof(WCHAR), &columns[c].indicator))) return false;
                }

                return true;
            }
        };
    }
}