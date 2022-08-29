#pragma once
#include <iostream>
#include <algorithm>
#include <cstdint>


namespace g80 {
    namespace odbc {

        class decimal {

        private:
            int64_t data_{0};
            int8_t scale_;


        auto inline data_on_scale(int8_t s) -> int64_t {
            int8_t ds = s - scale_;
            if(ds == 0) return data_;
            
            int8_t ads = ds < 0 ? -ds : ds;
            int8_t mul{1};
            for(int8_t as{0}; as < ads; ++as) mul *= 10;

            if(ds > 0) return data_ * mul;
            return data_ / mul;
        }

        public:
            decimal(int8_t scale = 2) : scale_{scale} {}
            decimal(const decimal &r) : data_{r.data_}, scale_{r.scale_} {}
            decimal(decimal &&r) : data_{r.data_}, scale_{r.scale_} {}
            ~decimal() = default;
            auto operator=(const decimal &r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}; return *this;}
            auto operator=(decimal &&r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}; return *this;}

            auto operator+=(decimal r) -> decimal & {

                // 1234.56000
                //   12.34567
                auto gs = std::max(scale_, r.scale_);
                data_ = this->data_on_scale(gs) + r.data_on_scale(gs);

                // 1234.5678
                //   12.34


                return *this;
            }
        };
    }
}