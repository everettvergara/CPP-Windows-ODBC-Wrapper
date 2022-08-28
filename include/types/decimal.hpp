#pragma 
#include <cstdint>


namespace g80 {
    namespace odbc {

        class decimal {

        private:
            int64_t data_{0};
            uint8_t scale_;

        public:
            decimal(uint8_t scale = 2) : scale_{scale} {}
            ~decimal() = default;
            decimal(const decimal &r) : data_{r.data_}, scale_{r.scale_} {}
            decimal(decimal &&r) : data_{r.data_}, scale_{r.scale_} {}
            auto operator=(const decimal &r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}; return *this;}
            auto operator=(decimal &&r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}; return *this;}

        };
    }
}