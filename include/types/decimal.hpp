#pragma once
#include <iostream>
#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace g80 {
    namespace odbc {


        /**
         * 
         *      a: 250
         *      b: 15 
         * 
         *      +
         * 
         *      max_allowance: 255 - a: 0
         * 
         *      if b <= max_allowance, co = 0
         *      else co = max - b
         * 
         *      
         * 
         */



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

            template<typename I> requires std::is_integral<I>::value
            decimal(const I data, const int8_t scale = 2) : data_{data}, scale_{scale}, scale_mul_{get_scale_mul(scale_)} {}

            template<typename F> requires std::is_floating_point<F>::value
            decimal(const F data, const int8_t scale = 2) : data_{data}, scale_{scale}, scale_mul_{get_scale_mul(scale_)} {}

            decimal(const decimal &r) : data_ {r.data_}, scale_{r.scale_}, scale_mul_{r.scale_mul_} {}

            decimal(decimal &&r) : data_{r.data_}, scale_{r.scale_}, scale_mul_{r.scale_mul_} {}

            ~decimal() = default;
            
            auto operator=(const decimal &r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}, scale_mul_ = {r.scale_mul_}; return *this;}
            auto operator=(decimal &&r) -> decimal & {data_ = {r.data_}, scale_ = {r.scale_}, scale_mul_ = {r.scale_mul_}; return *this;}
            template<typename T> requires std::is_integral<T>::value auto operator=(const T r) -> decimal & {data_ = r; return *this;}
            
            template<typename T> requires std::is_floating_point<T>::value auto operator=(const T r) -> decimal & {
                std::cout << "here\n";
                data_ = static_cast<int64_t>(r * scale_mul_);
                return *this;
            }

            auto get_as_int() -> int64_t {return data_;}
            auto get_as_ldouble() -> long double {
                return static_cast<long double>(1.0 * data_ / scale_mul_);
            }

            auto operator+=(decimal r) -> decimal & {
                auto gs = std::max(scale_, r.scale_); 

                // std::cout << "gs: " << (int) gs << "\n";
                // std::cout << "this->data_on_scale(gs): " << this->data_on_scale(gs) << "\n";
                // std::cout << "r->data_on_scale(gs): " << r.data_on_scale(gs) << "\n";
                data_ = this->data_on_scale(gs) + r.data_on_scale(gs); 
                return *this;
            }
        };
    }
}