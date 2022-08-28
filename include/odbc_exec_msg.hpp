#pragma once
#define UNICODE 
#include <memory>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

namespace g80 {
    namespace odbc {

        #define EXEC_MESSAGE_SIZE 1024

        struct odbc_exec_msg {
            WCHAR       last_message[EXEC_MESSAGE_SIZE];
            WCHAR       last_state[SQL_SQLSTATE_SIZE+1];
            SQLINTEGER  last_exec_msg;
            SQLLEN      last_row_count;

            odbc_exec_msg() {};
            odbc_exec_msg(const WCHAR *msg, const WCHAR *state, SQLINTEGER exec_msg) : last_exec_msg(exec_msg) {
                wcscpy(last_message, msg);
                wcscpy(last_state, state);
            }
        };

        class odbc_exec_msg_mgr {
            
            size_t ix_{0};
            size_t max_exec_msgs_;
            std::unique_ptr<odbc_exec_msg[]> exec_msgs_;

            struct iterator {
            private:
                odbc_exec_msg *ptr;
                
            public:
                iterator(odbc_exec_msg *p) : ptr(p) {}
                auto operator*() -> odbc_exec_msg & {return *ptr;}
                auto operator->() -> odbc_exec_msg * {return ptr;}
                auto operator++() -> iterator & {++ptr; return *this;}
                auto operator++(int) -> iterator {iterator t = *this; ++ptr; return t;}
                friend auto operator==(const iterator &l, const iterator &r) -> bool {return l.ptr == r.ptr;}
                friend auto operator!=(const iterator &l, const iterator &r) -> bool {return l.ptr != r.ptr;}
            };       

        public:

            //TODO: validate max_exec_msgs; should be >= 1
            odbc_exec_msg_mgr(size_t max_exec_msgs = 5) : 
                max_exec_msgs_(max_exec_msgs), 
                exec_msgs_(std::unique_ptr<odbc_exec_msg[]>(new odbc_exec_msg[max_exec_msgs_])) {}
            
            auto begin() const -> iterator {
                return iterator(exec_msgs_.get());
            }
            
            auto end() const -> iterator {
                return iterator(exec_msgs_.get() + ix_);
            }

            auto size() const -> int {
                return ix_;
            }
            
            auto reset() -> void {
                ix_ = 0;
            }

            auto get_next_slot() -> odbc_exec_msg * {
                if(ix_ == max_exec_msgs_) --ix_;
                return exec_msgs_.get() + ix_++;
            }

            auto pop_last_slot() -> bool {
                if(ix_ == 0) return false;
                --ix_;
                return true;
            }
        };
    }
}