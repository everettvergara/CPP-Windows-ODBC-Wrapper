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

namespace g80 {
    namespace odbc {

        #define SQL_QUERY_SIZE 8192
        #define NULL_SIZE      6
        #define DISPLAY_MAX 50        

        typedef struct STR_BINDING {
            SQLSMALLINT         cDisplaySize;
            WCHAR               *wszBuffer;
            SQLLEN              indPtr;
            BOOL                fChar;
            struct STR_BINDING  *sNext;
        } BINDING;

        class odbc {
        
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

            odbc(const size_t max_err = 5) : msg_(max_err) {}
            odbc(const odbc &) = delete;
            odbc(odbc &&) = delete;
            auto operator=(const odbc &) -> odbc & = delete;
            auto operator=(odbc &&) -> odbc & = delete;
            ~odbc() {disconnect();}

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
                        
                        }
                }




                return true;
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


            auto bind(SQLSMALLINT col_count, BINDING **ppBinding) -> bool {
                
                SQLSMALLINT     c;
                BINDING         *pThisBinding, *pLastBinding = NULL;
                SQLLEN          cchDisplay, ssType;
                SQLSMALLINT     cchColumnNameLength;

                for (c = 1; c <= col_count; ++c) {

                    pThisBinding = (BINDING *)(malloc(sizeof(BINDING)));
                    if (!(pThisBinding)) {
                        set_user_error(L"Out of memory!");
                        return false;
                    }

                    if (c == 1) *ppBinding = pThisBinding;
                    else pLastBinding->sNext = pThisBinding;
                    pLastBinding = pThisBinding;


                    // Figure out the display length of the column (we will
                    // bind to char since we are only displaying data, in general
                    // you should bind to the appropriate C type if you are going
                    // to manipulate data since it is much faster...)

                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT,
                            SQLColAttribute(stmt_, c, SQL_DESC_DISPLAY_SIZE,
                                NULL, 0, NULL, &cchDisplay))) return false;


                    // Figure out if this is a character or numeric column; this is
                    // used to determine if we want to display the data left- or right-
                    // aligned.

                    // SQL_DESC_CONCISE_TYPE maps to the 1.x SQL_COLUMN_TYPE.
                    // This is what you must use if you want to work
                    // against a 2.x driver.

                    if(!handle_ret_code(stmt_,  SQL_HANDLE_STMT,
                            SQLColAttribute(stmt_, c, SQL_DESC_CONCISE_TYPE,
                                NULL, 0, NULL, &ssType))) return false;

                    pThisBinding->fChar = ssType == SQL_CHAR || ssType == SQL_VARCHAR || ssType == SQL_LONGVARCHAR;

                    pThisBinding->sNext = NULL;

                    // Arbitrary limit on display size
                    if (cchDisplay > DISPLAY_MAX)
                        cchDisplay = DISPLAY_MAX;

                    // Allocate a buffer big enough to hold the text representation
                    // of the data.  Add one character for the null terminator

                    pThisBinding->wszBuffer = (WCHAR *)malloc((cchDisplay+1) * sizeof(WCHAR));

                    if (!(pThisBinding->wszBuffer)) {
                        set_user_error(L"Out of memory!");
                        return false;
                    }

                    // Map this buffer to the driver's buffer.   At Fetch time,
                    // the driver will fill in this data.  Note that the size is
                    // count of bytes (for Unicode).  All ODBC functions that take
                    // SQLPOINTER use count of bytes; all functions that take only
                    // strings use count of characters.

                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT,
                            SQLBindCol(stmt_, c, SQL_C_TCHAR,
                                (SQLPOINTER) pThisBinding->wszBuffer,
                                sizeof(wchar_t) * (cchDisplay + 1), &pThisBinding->indPtr))) 
                                return false;

                    // Now set the display size that we will use to display
                    // the data.   Figure out the length of the column name

                    if(!handle_ret_code(stmt_, SQL_HANDLE_STMT,
                            SQLColAttribute(stmt_, c, SQL_DESC_NAME,
                                NULL, 0, &cchColumnNameLength, NULL))) return false;

                    pThisBinding->cDisplaySize = std::max((SQLSMALLINT)cchDisplay, cchColumnNameLength);
                    if (pThisBinding->cDisplaySize < NULL_SIZE)
                        pThisBinding->cDisplaySize = NULL_SIZE;

                }

                return true;
            }
        };
    }
}