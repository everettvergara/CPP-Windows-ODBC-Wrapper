#pragma 
#include <cstdint>


namespace g80 {
    namespace odbc {

        class decimal {

        private:
            int64_t data_;
            uint8_t scale_;

        public:
            decimal(uint8_t scale) : scale_{scale} {}
            
        };
    }
}