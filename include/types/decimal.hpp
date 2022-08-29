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

        static inline auto get_scale_mul(int8_t scale) -> int64_t {
            int64_t mul{1};
            for(int8_t s{0}; s < scale; ++s) mul *= 10;
            return mul;
        }

        inline auto data_on_scale(int8_t s) -> int64_t {
            int8_t ds = s - scale_;
            if(ds == 0) return data_;
            auto mul = decimal::get_scale_mul(ds < 0 ? -ds : ds);
            if(ds > 0) return data_ * mul;
            return data_ / mul;
        }

        public:

            decimal(int8_t scale = 2) : scale_{scale}, scale_mul_{get_scale_mul(scale_)} {}
            decimal(const decimal &r) : data_{r.data_}, scale_{r.scale_}, scale_mul_{r.scale_mul_} {}
            decimal(decimal &&r) : data_{r.data_}, scale_{r.scale_}, scale_mul_{r.scale_mul_} {}
            ~decimal() = default;
            auto operator=(const decimal &r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}, scale_mul_ = {r.scale_mul_}; return *this;}
            auto operator=(decimal &&r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}, scale_mul_ = {r.scale_mul_}; return *this;}
            template<typename T> requires std::is_integral<T>::value auto operator=(const T r) -> decimal & {data_ = r; return *this;}
            template<typename T> requires std::is_floating_point<T>::value auto operator=(const T r) -> decimal & {
                data_ = static_cast<int64_t>(r * scale_mul_);
                return *this;
            }

            auto get_as_int() -> int64_t {return data_;}
            auto get_as_ldouble() -> long double {
                return static_cast<long double>(1.0 * data_ / scale_mul_);
            }

            auto operator+=(decimal r) -> decimal & {
                auto gs = std::max(scale_, r.scale_);
                data_ = this->data_on_scale(gs) + r.data_on_scale(gs);
                return *this;
            }
        };
    }
}