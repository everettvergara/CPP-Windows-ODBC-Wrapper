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

            auto set_user_error(const WCHAR *error_msg) -> void {
                auto err = msg_.get_next_slot(); 
                wcscpy(err->last_message, error_msg);
                wcscpy(err->last_state, L"");
                err->last_exec_msg = -1;
            }

            auto handle_ret_code(SQLHANDLE handle, SQLSMALLINT type, RETCODE rc) -> bool {
                if(rc != SQL_SUCCESS) check_for_message(handle, type, rc);
                if(rc == SQL_ERROR) return false;
                return true;
            }

           auto alloc_env() -> bool {
                if(env_ != NULL) return true;
                if(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env_) == SQL_ERROR) {set_user_error(L"Could not allocate environment handle"); return false;}
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLSetEnvAttr(env_, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0)); 
            }

            auto alloc_connection() -> bool {
                if(dbc_ != NULL) return true;
                return handle_ret_code(env_, SQL_HANDLE_ENV, SQLAllocHandle(SQL_HANDLE_DBC, env_, &dbc_));
            }

            auto dealloc_connection() -> bool {
                if(dbc_ == NULL) return true;
                if(SQLRETURN rc = SQLDisconnect(dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {set_user_error(L"Could not disconnect db handle"); return false;}
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_DBC, dbc_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {set_user_error(L"Could not deallocate db handle"); return false;}
                dbc_ = NULL;
                return true;
            }

            auto dealloc_env() -> bool {
                if(env_ == NULL) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_ENV, env_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {set_user_error(L"Could not free up env handle"); return false;}
                env_ = NULL;
                return true;
            }

            auto alloc_statement() -> bool {
                if(stmt_ != NULL) return true;
                return handle_ret_code(dbc_, SQL_HANDLE_DBC, SQLAllocHandle(SQL_HANDLE_STMT, dbc_, &stmt_));
            }

            auto dealloc_statement() -> bool {
                if(stmt_ == NULL) return true;
                if(SQLRETURN rc = SQLFreeHandle(SQL_HANDLE_STMT, stmt_); rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO) {set_user_error(L"Could not free up stmt handle"); return false;}
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
                    if(e.last_row_count > 0)
                        out += L" - (" + std::to_wstring(e.last_row_count) + L" rows)";
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

            auto process_exec(const RETCODE rc) -> bool {
                if(static_cast<unsigned short>(rc) > SQL_SUCCESS_WITH_INFO) 
                    return handle_ret_code(stmt_, SQL_HANDLE_STMT, rc);

                if(rc == SQL_SUCCESS_WITH_INFO) 
                    check_for_message(stmt_, SQL_HANDLE_STMT, rc);

                SQLSMALLINT col_count;
                if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, 
                    SQLNumResultCols(stmt_, &col_count))) return false;
                
                if(col_count > 0) {
                    std::vector<col_binding> columns;                       
                    if(!bind_columns(col_count, columns)) return false;

                    do {
                        RETCODE frc = SQLFetch(stmt_);
                        if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, frc)) return false;
                        if(frc == SQL_NO_DATA_FOUND) break;
                        if(static_cast<unsigned short>(frc) > SQL_SUCCESS_WITH_INFO) return false;

                        for(auto &c : columns)
                            std::wcout << "col " << c.column_name << "(" << c.column_size << "): " << c.buffer << "\n";
                        
                    } while(true);
                }

                auto m = msg_.get_next_slot();
                m->last_exec_msg = 0;
                wcscpy(m->last_state, L"");
                wcscpy(m->last_message, L"Executed Successfully!");
                if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, 
                    SQLRowCount(stmt_, &m->last_row_count))) return false;
                
                return true;
            }

            auto exec(WCHAR *command) -> bool {
                msg_.reset();
                auto bool_exit = process_exec(SQLExecDirect(stmt_, command, SQL_NTS));
                if(!handle_ret_code(stmt_, SQL_HANDLE_STMT, SQLFreeStmt(stmt_, SQL_CLOSE))) return false;
                return bool_exit;
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
                    
                    columns[c].column_display_size = columns[c].column_display_size > DISPLAY_COLUMN_MAX ? 
                                                        DISPLAY_COLUMN_MAX : columns[c].column_display_size;
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