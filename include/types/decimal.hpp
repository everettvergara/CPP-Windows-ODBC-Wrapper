#pragma once
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace g80 {
    namespace odbc {

        class decimal {

        private:
            int64_t data_{0};
            int8_t scale_;
            int64_t scale_mul_;

        auto inline scale_mul(int8_t s) -> int64_t {
        
        }
        
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
            template<typename T> requires std::is_integral<T>::value auto operator=(const T r) -> decimal & {data_ = r;}
            template<typename T> requires std::is_floating_point<T>::value auto operator=(const T r) -> decimal & {
                // data_ = static_cast<int64_t>(r * ;

                return *this;
            }


            auto operator+=(decimal r) -> decimal & {
                auto gs = std::max(scale_, r.scale_);
                data_ = this->data_on_scale(gs) + r.data_on_scale(gs);
                return *this;
            }
        };
    }
}