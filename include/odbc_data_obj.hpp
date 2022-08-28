#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <variant>
#include <ctime>
#include <sqltypes.h>

#include "odbc_col_binding.hpp"


namespace g80 {
    namespace odbc {

        struct col {
            std::wstring name;
            SQLLEN type;
            SQLLEN column_size;

        };

        struct col_value {
            bool is_null;
            std::variant<
                std::wstring, std::string, 
                bool, uint8_t, uint16_t, uint32_t, uint64_t, 
                int8_t, int16_t, int32_t, int64_t, 
                float, double, long double> value;
        };

        class odbc_data_obj {
        
            std::vector<col> cols_;
            std::vector<std::vector<std::variant<col_value>>> values_;

        public:
            odbc_data_obj() {}
        };

    }
}