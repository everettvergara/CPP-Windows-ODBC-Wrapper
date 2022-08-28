#pragma once

namespace g80 {
    namespace odbc {
        #define NULL_COLUMN_SIZE    6
        #define DISPLAY_COLUMN_MAX  50
        struct col_binding {    
            SQLLEN                      column_size;
            SQLSMALLINT                 column_display_size;
            SQLLEN                      column_type;
            WCHAR                       column_name[DISPLAY_COLUMN_MAX+1];
            std::unique_ptr<WCHAR[]>    buffer{nullptr};
        };
    }
}