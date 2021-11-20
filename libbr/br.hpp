/*
C++ implementation of the Barrett reduction.
32, 64 and 128-bit versions.

References:
https://en.wikipedia.org/wiki/Barrett_reduction
https://www.nayuki.io/page/barrett-reduction-algorithm
https://math.stackexchange.com/a/3455956/988129
*/

#pragma once

#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "libbr/util.hpp"

#ifdef __SIZEOF_INT128__
#define USE_128_BIT
#endif

namespace br {

    class BarrettRed32
    {
    public:
        BarrettRed32(const uint32_t _n) : n(_n), r(0)
        {
            if (n < 3) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must be >= 3.");
            }
            if ((n & (n - 1)) == 0) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must not be a power of 2.");
            }

            // r = 2^k / n.
            // Can be calculated as '(2^k - 1) / n' when n is not a power of 2.
            // This calculation alternative fits in 32-bit arithmetic.
            r = UINT32_MAX / n;
        }

        uint32_t calc(const uint32_t x) // x mod n
        {
            if (static_cast<uint64_t>(x) >= static_cast<uint64_t>(n)*static_cast<uint64_t>(n)) {
                std::cout << "x=" << x << ", n=" << n << ", n2=" << static_cast<uint64_t>(n)*static_cast<uint64_t>(n) << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            const uint64_t xr = static_cast<uint64_t>(x) * static_cast<uint64_t>(r);
            uint32_t q = xr >> 32;
            q = x - q * n;
            if (q >= n) q -= n;
            return q;
        }

    //private:
        uint32_t n;
        uint32_t r;
    };



    class BarrettRed64
    {
    public:
        BarrettRed64(const uint64_t _n) : n(_n), r(0)
        {
            if (n < 3) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must be >= 3.");
            }
            if ((n & (n - 1)) == 0) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must not be a power of 2.");
            }

            // r = 2^k / n.
            // Can be calculated as '(2^k - 1) / n' when n is not a power of 2.
            // This calculation alternative fits in 64-bit arithmetic.
            r = UINT64_MAX / n;

            // Pre-calculate n^2
            n2_lo = n * n;
            n2_hi = util::mulhi64(n, n);
        }

        uint64_t calc(const uint64_t x) // x mod n
        {
            if (n2_hi == 0 && x >= n2_lo) {
                std::cout << "x=" << x << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            const uint64_t xr_hi = util::mulhi64(x, r);
            // xr_hi = (x * r) >> k
            // Taking the higher 64 bits is the same as shifting right by 64.
            uint64_t q = xr_hi;
            q = x - q * n;
            if (q >= n) q -= n;
            return q;
        }

    //private:
        uint64_t n;
        uint64_t r;
        uint64_t n2_lo, n2_hi;
    };


    class BarrettRed128
    {
    using uint128_t = unsigned __int128;

    public:
        BarrettRed128(const uint64_t _n) : n(_n), r(0), s(0), t(0)
        {
            if (n < 3) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must be >= 3.");
            }
            if ((n & (n - 1)) == 0) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must not be a power of 2.");
            }

            // r = 2^(k/2) / n.
            // Can be calculated as '(2^(k/2) - 1) / n' when n is not a power of 2.
            // This calculation alternative fits in 64-bit arithmetic.
            r = UINT64_MAX / n;

            // s = 2^k / n - 2^(k/2) * r
            // The first part, '2^k / n', can be calculated as '(2^k - 1) / n' when n is not a power of 2.
            // This calculation alternative fits in 128-bit arithmetic.
            const uint128_t two_pow_128_div_n = (~static_cast<uint128_t>(0)) / n;
            // The second part, '2^(k/2) * r', can be ignored since it's the equivalent of a shift left by 64
            // and since 's' is only 64 bits, the lower 64 bits of '2^(k/2) * r' will always be zero.
            s = two_pow_128_div_n;

            // t = 2^(k/2) - r * n
            // Can be calculated as '2^(k/2) - 1 - r * n + 1' since 'r * n' will never exceed 64 bits.
            // This calculation alternative fits in 64-bit arithmetic.
            t = (UINT64_MAX - r * n) + 1;

            // Pre-calculate n^2
        #ifdef USE_128_BIT
            n2 = static_cast<uint128_t>(n) * static_cast<uint128_t>(n);
        #else
            n2_lo = n * n;
            n2_hi = util::mulhi64(n, n);
        #endif
        }

        // Use 128-bit arithmetic.
        uint64_t calc(const uint128_t x) // x mod n
        {
        #ifdef USE_128_BIT
            if (x >= n2) {
                uint64_t x_lo = static_cast<uint64_t>(x);
                uint64_t x_hi = x >> 64;
                uint64_t n2_lo = static_cast<uint64_t>(n2);
                uint64_t n2_hi = n2 >> 64;
        #else
            uint64_t x_lo = static_cast<uint64_t>(x);
            uint64_t x_hi = x >> 64;
            if (x_hi > n2_hi || (x_hi == n2_hi && x_lo >= n2_lo)) {
        #endif
                std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            const uint128_t a = x >> 64;
            const uint128_t b = static_cast<uint64_t>(x);
            const uint128_t qa = (a * s) >> 64;
            const uint128_t qb = (b * r) >> 64;
            uint128_t a1 = a * t - qa * n;
            if (a1 >= n) a1 -= n;
            uint128_t b1 = b - qb * n;
            if (b1 >= n) b1 -= n;
            uint128_t x1 = a1 + b1;
            if (x1 >= n) x1 -= n;
            return x1;
        }

        // Use 64-bit arithmetic.
        uint64_t calc(const uint64_t x_hi, const uint64_t x_lo) // x mod n
        {
            if (n >= (1UL<<63)) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must be < 2^63.");
            }
        #ifdef USE_128_BIT
            const uint64_t n2_lo = static_cast<uint64_t>(n2);
            const uint64_t n2_hi = n2 >> 64;
        #endif
            if (x_hi > n2_hi || (x_hi == n2_hi && x_lo >= n2_lo)) {
                std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            const uint64_t a = x_hi;
            const uint64_t b = x_lo;
            const uint64_t qa = util::mulhi64(a, s);
            const uint64_t qb = util::mulhi64(b, r);
            const uint64_t at = a * t;
            const uint64_t qan = qa * n;
            uint64_t a1;
            if (at < qan)
                a1 = ((1UL<<63) - qan) + (1UL<<63) + at;
            else
                a1 = at - qan;
            if (a1 >= n) a1 -= n;
            uint64_t b1 = b - qb * n;
            if (b1 >= n) b1 -= n;
            uint64_t x1 = a1 + b1;
            if (x1 >= n) x1 -= n;
            return x1;
        }

    //private:
        uint64_t n;
        uint64_t r;
        uint64_t s;
        uint64_t t;
    #ifdef USE_128_BIT
        uint128_t n2;
    #else
        uint64_t n2_lo, n2_hi;
    #endif
    };

} // namespace br
