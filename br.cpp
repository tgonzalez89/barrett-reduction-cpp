/*
C++ implementation of the Barrett reduction.
32, 64 and 128-bit versions.

References:
https://en.wikipedia.org/wiki/Barrett_reduction
https://www.nayuki.io/page/barrett-reduction-algorithm
https://math.stackexchange.com/a/3455956/988129
*/

#include <cstdint>
#include <stdexcept>
#include <random>
#include <iostream>

class BarrettRed32
{
public:
    BarrettRed32(uint32_t _n) : n(_n), k(32), r(0)
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
        r = UINT32_MAX / n;
    }

    uint32_t calc(const uint32_t x) // x mod n
    {
        if (static_cast<uint64_t>(x) >= static_cast<uint64_t>(n)*static_cast<uint64_t>(n)) {
            std::cout << "x=" << x << ", n=" << n << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }
        uint32_t q = static_cast<uint32_t>((static_cast<uint64_t>(x) * static_cast<uint64_t>(r)) >> k);
        q = x - q * n;
        if (q >= n) q -= n;
        return q;
    }

//private:
    uint32_t n;
    uint32_t k;
    uint32_t r;
};

static void test_br32()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    for (uint32_t bitlen = 1; bitlen <= 31; ++bitlen) {
        const uint32_t min_n = (1U << bitlen) + 1;
        const uint32_t max_n = UINT32_MAX >> (31 - bitlen);
        std::uniform_int_distribution<uint32_t> distr_n(min_n, max_n);
        for (size_t i = 0; i < 1000; ++i) {
            const uint32_t n = distr_n(gen);
            BarrettRed32 br(n);
            uint32_t n2 = n > UINT16_MAX ? UINT32_MAX : n*n - 1;
            std::uniform_int_distribution<uint32_t> distr_x(0, n2);
            for (size_t j = 0; j < 1000; ++j) {
                const uint32_t x = distr_x(gen);
                const uint32_t res = br.calc(x);
                const uint32_t ref = x % n;
                if (res != ref) {
                    std::cout << "res=" << res << ", ref=" << ref << "\n";
                    std::cout << "x=" << x << ", n=" << n << ", k=" << br.k << ", r=" << br.r << "\n";
                    throw std::runtime_error("Barrett reduction test failed.");
                }
            }
        }
    }
}

static void mult_64_128(uint64_t a, uint64_t b, uint64_t & hi, uint64_t & lo)
{
    uint64_t a_lo = a & UINT32_MAX;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = b & UINT32_MAX;
    uint64_t b_hi = b >> 32;

    uint64_t p0 = a_lo * b_lo;
    uint64_t p1 = a_lo * b_hi;
    uint64_t p2 = a_hi * b_lo;
    uint64_t p3 = a_hi * b_hi;

    uint32_t cy = static_cast<uint32_t>(((p0 >> 32) + (p1 & UINT32_MAX) + (p2 & UINT32_MAX)) >> 32);

    lo = p0 + (p1 << 32) + (p2 << 32);
    hi = p3 + (p1 >> 32) + (p2 >> 32) + cy;
}

class BarrettRed64
{
public:
    BarrettRed64(uint64_t _n) : n(_n), k(64), r(0)
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
    }

    uint64_t calc(const uint64_t x) // x mod n
    {
        uint64_t n2_lo, n2_hi;
        mult_64_128(n, n, n2_hi, n2_lo);
        if (n2_hi == 0 && x >= n2_lo) {
            std::cout << "x=" << x << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }
        uint64_t xr_hi, xr_lo;
        mult_64_128(x, r, xr_hi, xr_lo);
        // Same as (x * n) >> k.
        // k = 64, so taking the higher 64 bits is the same as shifting right by 64.
        uint64_t q = xr_hi;
        q = x - q * n;
        if (q >= n) q -= n;
        return q;
    }

//private:
    uint64_t n;
    uint64_t k;
    uint64_t r;
};

static void test_br64()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    for (uint64_t bitlen = 1; bitlen <= 63; ++bitlen) {
        const uint64_t min_n = (1UL << bitlen) + 1;
        const uint64_t max_n = UINT64_MAX >> (63 - bitlen);
        std::uniform_int_distribution<uint64_t> distr_n(min_n, max_n);
        for (size_t i = 0; i < 1000; ++i) {
            const uint64_t n = distr_n(gen);
            BarrettRed64 br(n);
            uint64_t n2 = n > UINT32_MAX ? UINT64_MAX : n*n - 1;
            std::uniform_int_distribution<uint64_t> distr_x(0, n2);
            for (size_t j = 0; j < 1000; ++j) {
                const uint64_t x = distr_x(gen);
                const uint64_t res = br.calc(x);
                const uint64_t ref = x % n;
                if (res != ref) {
                    std::cout << "res=" << res << ", ref=" << ref << "\n";
                    std::cout << "x=" << x << ", n=" << n << ", k=" << br.k << ", r=" << br.r << "\n";
                    throw std::runtime_error("Barrett reduction test failed.");
                }
            }
        }
    }
}

typedef unsigned __int128 uint128_t;

class BarrettRed128
{
public:
    BarrettRed128(uint64_t _n) : n(_n), k(128), r(0), s(0), t(0)
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
        uint128_t two_pow_128_minus_1 = UINT64_MAX;
        two_pow_128_minus_1 <<= (k / 2);
        two_pow_128_minus_1 |= UINT64_MAX;
        uint128_t two_pow_128_minus_1_div_n = two_pow_128_minus_1 / n;
        // Second part, '2^(k/2) * r'.
        uint128_t two_pow_64 = UINT64_MAX;
        two_pow_64 += 1;
        uint128_t two_pow_64_times_r = two_pow_64 * r;
        // Combine part 1 and part 2.
        s = static_cast<uint64_t>(two_pow_128_minus_1_div_n - two_pow_64_times_r);

        // t = 2^(k/2) - r * n
        uint128_t r_times_n = static_cast<uint128_t>(r) * static_cast<uint128_t>(n);
        t = static_cast<uint64_t>(two_pow_64 - r_times_n);
    }

    // Uses 64-bit arithmetic.
    uint64_t calc1(const uint64_t x_hi, const uint64_t x_lo) // x mod n
    {
        if (n >= (1UL<<63)) {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Modulus must be < 2^63.");
        }
        uint64_t n2_lo, n2_hi;
        mult_64_128(n, n, n2_hi, n2_lo);
        if (x_hi > n2_hi || (x_hi == n2_hi && x_lo >= n2_lo)) {
            std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", n2_hi=" << n2_hi << ", n2_lo=" << n2_lo << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        const uint64_t a = x_hi;
        const uint64_t b = x_lo;
        uint64_t qa, qb, at, qan;
        uint64_t tmp;
        mult_64_128(a, s, qa, tmp);
        mult_64_128(b, r, qb, tmp);
        mult_64_128(a, t, tmp, at);
        mult_64_128(qa, n, tmp, qan);
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

    // Uses 128-bit arithmetic.
    uint64_t calc2(const uint128_t x) // x mod n
    {
        uint128_t n2 = n;
        n2 *= n;
        if (x >= n2) {
            std::cout << "n=" << n << "\n";
            throw std::invalid_argument("Input must be less than modulus^2.");
        }

        uint128_t a = x >> 64;
        uint128_t b = x - (a << 64);
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
};

int main()
{
    test_br32();
    test_br64();
    // TODO: Test 128-bit version.
    BarrettRed128 br(123);
    uint64_t res = br.calc1(0, 4567);
    if (res != 4567 % 123) {
        std::cout << "ERROR: " << res << " vs " << 4567 % 123 << "\n";
    } else {
        std::cout << "OK: " << res << " vs " << 4567 % 123 << "\n";
    }
    return 0;
}
