#pragma once
#define UNICODE 
#include <memory>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        #define ERROR_MESSAGE_SIZE 1024

        struct odbc_error {
            WCHAR       last_message[ERROR_MESSAGE_SIZE];
            WCHAR       last_state[SQL_SQLSTATE_SIZE+1];
            SQLINTEGER  last_error;

            odbc_error() {};
            odbc_error(const WCHAR *msg, const WCHAR *state, SQLINTEGER error) : last_error(error) {
                wcscpy(last_message, msg);
                wcscpy(last_state, state);
            }
        };

        class odbc_error_mgr {
            
            size_t ix_{0};
            size_t size_{0};
            size_t max_errors_;
            std::unique_ptr<odbc_error[]> errors_;

            struct iterator {
            private:
                odbc_error *ptr;
                
            public:
                iterator(odbc_error *p) : ptr(p) {}
                auto operator*() -> odbc_error & {return *ptr;}
                auto operator->() -> odbc_error * {return ptr;}
                auto operator++() -> iterator & {++ptr; return *this;}
                auto operator++(int) -> iterator {iterator t = *this; ++ptr; return t;}
                friend auto operator==(const iterator &l, const iterator &r) -> bool {return l.ptr == r.ptr;}
                friend auto operator!=(const iterator &l, const iterator &r) -> bool {return l.ptr != r.ptr;}
            };       

        public:

            odbc_error_mgr(size_t max_errors = 5) : 
                max_errors_(max_errors), 
                errors_(std::unique_ptr<odbc_error[]>(new odbc_error[max_errors_])) {}
            
            auto begin() -> iterator {
                return iterator(&errors_[0]);
            }
            
            auto end() -> iterator {
                return iterator(&errors_[size_]);
            }

            auto get_current_ix() -> int {
                return ix_;
            }
            
            auto reset() -> void {
                size_ = ix_ = 0;
            }

            auto get_next_slot() -> odbc_error * {
                if(ix_ == max_errors_) return nullptr;
                return &errors_[++ix_];
            }
        };
    }
}