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


namespace br {

    class BarrettRed32
    {
    public:
        BarrettRed32(const uint32_t _n) : n(_n), k(32), r(0)
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

            uint64_t xr = static_cast<uint64_t>(x) * static_cast<uint64_t>(r);
            uint32_t q = static_cast<uint32_t>(xr >> k);
            q = x - q * n;
            if (q >= n) q -= n;
            return q;
        }

    //private:
        uint32_t n;
        uint32_t k;
        uint32_t r;
    };



    class BarrettRed64
    {
    public:
        BarrettRed64(const uint64_t _n) : n(_n), k(64), r(0)
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

            uint64_t xr_hi = util::mulhi64(x, r);
            // xr_hi = (x * r) >> k
            // Taking the higher 64 bits is the same as shifting right by 64.
            uint64_t q = xr_hi;
            q = x - q * n;
            if (q >= n) q -= n;
            return q;
        }

    //private:
        uint64_t n;
        uint64_t k;
        uint64_t r;
        uint64_t n2_lo, n2_hi;
    };



    class BarrettRed128
    {
    using uint128_t = unsigned __int128;

    public:
        BarrettRed128(const uint64_t _n) : n(_n), k(128), r(0), s(0), t(0)
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
            uint128_t two_pow_128_div_n = (~static_cast<uint128_t>(0)) / n;
            // Second part, '2^(k/2) * r'.
            uint128_t two_pow_64_times_r = static_cast<uint128_t>(r) << 64;
            // Combine part 1 and part 2.
            s = static_cast<uint64_t>(two_pow_128_div_n - two_pow_64_times_r);

            // t = 2^(k/2) - r * n
            uint128_t r_times_n = static_cast<uint128_t>(r) * static_cast<uint128_t>(n);
            t = static_cast<uint64_t>((static_cast<uint128_t>(1) << 64) - r_times_n);

            // Pre-calculate n^2
            n2_lo = n * n;
            n2_hi = util::mulhi64(n, n);
            n2 = static_cast<uint128_t>(n) * static_cast<uint128_t>(n);
        }

        // Use 64-bit arithmetic.
        uint64_t calc(const uint64_t x_hi, const uint64_t x_lo) // x mod n
        {
            if (n >= (1UL<<63)) {
                std::cout << "n=" << n << "\n";
                throw std::invalid_argument("Modulus must be < 2^63.");
            }
            if (x_hi > n2_hi || (x_hi == n2_hi && x_lo >= n2_lo)) {
                std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            const uint64_t a = x_hi;
            const uint64_t b = x_lo;
            uint64_t qa, qb, at, qan;
            qa = util::mulhi64(a, s);
            qb = util::mulhi64(b, r);
            at = a * t;
            qan = qa * n;
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

        // Use 128-bit arithmetic.
        uint64_t calc(const uint128_t x) // x mod n
        {
            if (x >= n2) {
                uint64_t x_lo = static_cast<uint64_t>(x);
                uint64_t x_hi = static_cast<uint64_t>(x >> 64);
                uint64_t n2_lo = static_cast<uint64_t>(n2);
                uint64_t n2_hi = static_cast<uint64_t>(n2 >> 64);
                std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
                throw std::invalid_argument("Input must be less than modulus^2.");
            }

            uint128_t a = x >> 64;
            uint128_t b = static_cast<uint64_t>(x);
            uint128_t qa = (a * s) >> 64;
            uint128_t qb = (b * r) >> 64;
            uint128_t a1 = a * t - qa * n;
            if (a1 >= n) a1 -= n;
            uint128_t b1 = b - qb * n;
            if (b1 >= n) b1 -= n;
            uint128_t x1 = a1 + b1;
            if (x1 >= n) x1 -= n;
            return x1;
        }

    //private:
        uint64_t n;
        uint64_t k;
        uint64_t r;
        uint64_t s;
        uint64_t t;
        uint64_t n2_lo, n2_hi;
        uint128_t n2;
    };

} // namespace br
