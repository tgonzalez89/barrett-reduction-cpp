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

namespace br
{

class BarrettRed32
{
  public:
    explicit BarrettRed32(const uint32_t _n) : n(_n)
    {
        if (n < 3)
        {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must be >= 3.");
        }
        if ((n & (n - 1)) == 0)
        {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must not be a power of 2.");
        }

        // r = 2^k / n.
        // Can be calculated as '(2^k - 1) / n' when n is not a power of 2.
        // This calculation alternative fits in 32-bit arithmetic.
        r = UINT32_MAX / n;
    }

    [[nodiscard]] auto calc(const uint32_t x) const -> uint32_t // x mod n
    {
        if (static_cast<uint64_t>(x) >= static_cast<uint64_t>(n) * static_cast<uint64_t>(n))
        {
            std::cout << "x=" << x << ", n=" << n << ", n2=" << static_cast<uint64_t>(n) * static_cast<uint64_t>(n)
                      << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        const uint64_t xr = static_cast<uint64_t>(x) * static_cast<uint64_t>(r);
        uint32_t q = xr >> 32U;
        q = x - q * n;
        if (q >= n)
        {
            q -= n;
        }
        return q;
    }

    [[nodiscard]] auto get_r() const -> uint32_t
    {
        return r;
    }

  private:
    uint32_t n;
    uint32_t r{0};
};

class BarrettRed64
{
  public:
    explicit BarrettRed64(const uint64_t _n) : n(_n)
    {
        if (n < 3)
        {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must be >= 3.");
        }
        if ((n & (n - 1)) == 0)
        {
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

    [[nodiscard]] auto calc(const uint64_t x) const -> uint64_t // x mod n
    {
        if (n2_hi == 0 && x >= n2_lo)
        {
            std::cout << "x=" << x << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        const uint64_t xr_hi = util::mulhi64(x, r);
        // xr_hi = (x * r) >> k
        // Taking the higher 64 bits is the same as shifting right by 64.
        uint64_t q = xr_hi;
        q = x - q * n;
        if (q >= n)
        {
            q -= n;
        }
        return q;
    }

    [[nodiscard]] auto get_r() const -> uint64_t
    {
        return r;
    }

  private:
    uint64_t n;
    uint64_t r{0};
    uint64_t n2_lo, n2_hi;
};

class BarrettRed128
{
#ifdef __SIZEOF_INT128__
    using uint128_t = unsigned __int128;
#endif

  public:
    explicit BarrettRed128(const uint64_t _n) : n(_n)
    {
        if (n < 3)
        {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must be >= 3.");
        }
        if ((n & (n - 1)) == 0)
        {
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
        // The second part, '2^(k/2) * r', can be ignored since it's the equivalent of a shift left by 64
        // and since 's' is only 64 bits, the lower 64 bits of '2^(k/2) * r' will always be zero.
#ifdef __SIZEOF_INT128__
        s = ~static_cast<uint128_t>(0) / n;
#else
        s = util::longdiv128_1s(n);
#endif

        // t = 2^(k/2) - r * n
        // Can be calculated as '((2^(k/2) - 1) - r * n) + 1' since 'r * n' will never exceed 64 bits.
        // This calculation alternative fits in 64-bit arithmetic.
        t = (UINT64_MAX - r * n) + 1;

        // Pre-calculate n^2
#ifdef __SIZEOF_INT128__
        n2 = static_cast<uint128_t>(n) * static_cast<uint128_t>(n);
#else
        n2_lo = n * n;
        n2_hi = util::mulhi64(n, n);
#endif
    }

#ifdef __SIZEOF_INT128__
    // Use 128-bit arithmetic.
    [[nodiscard]] auto calc(const uint128_t x) const -> uint64_t // x mod n
    {
        if (x >= n2)
        {
            const uint64_t x_lo = x;
            const uint64_t x_hi = x >> 64U;
            const uint64_t n2_lo = n2;
            const uint64_t n2_hi = n2 >> 64U;
            std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi
                      << ", n2_lo=" << n2_lo << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        const uint128_t a = x >> 64U;
        const uint64_t b = x;
        const uint128_t qa = (a * s) >> 64U;
        const uint64_t qb = (static_cast<uint128_t>(b) * r) >> 64U;
        uint128_t a1 = a * t - qa * n;
        if (a1 >= n)
        {
            a1 -= n;
        }
        uint64_t b1 = b - qb * n;
        if (b1 >= n)
        {
            b1 -= n;
        }
        uint128_t x1 = a1 + b1;
        if (x1 >= n)
        {
            x1 -= n;
        }
        return x1;
    }
#endif

    // Use 64-bit arithmetic.
    [[nodiscard]] auto calc(const uint64_t x_hi, const uint64_t x_lo) const -> uint64_t // x mod n
    {
        if (n >= (1UL << 63U))
        {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must be < 2^63.");
        }
#ifdef __SIZEOF_INT128__
        if (((static_cast<uint128_t>(x_hi) << 64U) | x_lo) >= n2)
        {
            const uint64_t n2_lo = n2;
            const uint64_t n2_hi = n2 >> 64U;
#else
        if (x_hi > n2_hi || (x_hi == n2_hi && x_lo >= n2_lo))
        {
#endif
            std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi
                      << ", n2_lo=" << n2_lo << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        const uint64_t a = x_hi;
        const uint64_t b = x_lo;
        const uint64_t qa = util::mulhi64(a, s);
        const uint64_t qb = util::mulhi64(b, r);
        const uint64_t at = a * t;
        const uint64_t qan = qa * n;
        uint64_t a1 = 0;
        if (at < qan)
        {
            a1 = ((1UL << 63U) - qan) + (1UL << 63U) + at;
        }
        else
        {
            a1 = at - qan;
        }
        if (a1 >= n)
        {
            a1 -= n;
        }
        uint64_t b1 = b - qb * n;
        if (b1 >= n)
        {
            b1 -= n;
        }
        uint64_t x1 = a1 + b1;
        if (x1 >= n)
        {
            x1 -= n;
        }
        return x1;
    }

    [[nodiscard]] auto get_r() const -> uint64_t
    {
        return r;
    }

    [[nodiscard]] auto get_s() const -> uint64_t
    {
        return s;
    }

    [[nodiscard]] auto get_t() const -> uint64_t
    {
        return t;
    }

  private:
    uint64_t n;
    uint64_t r{0};
    uint64_t s{0};
    uint64_t t{0};
#ifdef __SIZEOF_INT128__
    uint128_t n2;
#else
    uint64_t n2_lo, n2_hi;
#endif
};

} // namespace br
