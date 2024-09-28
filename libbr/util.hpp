#pragma once

#include <cstdint>

namespace br::util
{

#ifdef __SIZEOF_INT128__ // GNU C
// https://stackoverflow.com/a/50958815/6553631
static inline auto mulhi64(const uint64_t a, const uint64_t b) -> uint64_t
{
    const unsigned __int128 prod = static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b);
    return prod >> 64U;
}

#elif defined(_M_X64) || defined(_M_ARM64) // MSVC
// MSVC for x86-64 or AArch64
// possibly also || defined(_M_IA64) || defined(_WIN64)
// but the docs only guarantee x86-64! Don't use *just* _WIN64; it doesn't include AArch64 Android / Linux

// https://docs.microsoft.com/en-gb/cpp/intrinsics/umulh
#include <intrin.h>
#define mulhi64 __umulh

#elif defined(_M_IA64) // || defined(_M_ARM)    // MSVC
// https://docs.microsoft.com/en-gb/cpp/intrinsics/umul128
// incorrectly say that _umul128 is available for ARM
// which would be weird because there's no single insn on AArch32
#include <intrin.h>
static inline auto mulhi64(const uint64_t a, const uint64_t b) -> uint64_t
{
    unsigned __int64 product_high;
    _umul128(a, b, &product_high);
    return product_high;
}

#else
// a * b = (k*a_hi + a_lo) * (k*b_hi + b_lo)
// k = 2^32
// https://stackoverflow.com/a/31662911/6553631
static inline auto mulhi64(const uint64_t a, const uint64_t b) -> uint64_t
{
    const uint64_t a_lo = static_cast<uint32_t>(a);
    const uint64_t a_hi = a >> 32U;
    const uint64_t b_lo = static_cast<uint32_t>(b);
    const uint64_t b_hi = b >> 32U;

    const uint64_t p0 = a_lo * b_lo;
    const uint64_t p1 = a_lo * b_hi;
    const uint64_t p2 = a_hi * b_lo;
    const uint64_t p3 = a_hi * b_hi;

    const uint32_t cy = ((p0 >> 32U) + static_cast<uint32_t>(p1) + static_cast<uint32_t>(p2)) >> 32U;

    return p3 + (p1 >> 32U) + (p2 >> 32U) + cy;
}
#endif

// https://en.wikipedia.org/wiki/Division_algorithm#Integer_division_(unsigned)_with_remainder

static inline auto longdiv64(const uint64_t n, const uint64_t d) -> uint64_t
{
    uint64_t q = 0;
    uint64_t r = 0;

    for (int64_t i = 63; i >= 0; --i)
    {
        const auto iu = static_cast<uint64_t>(i);

        r <<= 1U;

        const auto n_i = static_cast<uint64_t>((n & (1UL << iu)) != 0);
        r = (r & ~1UL) | n_i;

        if (r >= d)
        {
            r -= d;
            q |= 1UL << iu;
        }
    }
    return q;
}

static inline auto longdiv128(const uint64_t n_hi, const uint64_t n_lo, const uint64_t d) -> uint64_t
{
    uint64_t q = 0;
    uint64_t r_hi = 0;
    uint64_t r_lo = 0;

    for (int64_t i = 127; i >= 0; --i)
    {
        const auto iu = static_cast<uint64_t>(i);

        r_hi = (r_hi << 1U) | (r_lo >> 63U);
        r_lo <<= 1U;

        uint64_t n_i = 0;
        if (i >= 64)
        {
            n_i = static_cast<uint64_t>((n_hi & (1UL << (iu - 64))) != 0);
        }
        else
        {
            n_i = static_cast<uint64_t>((n_lo & (1UL << iu)) != 0);
        }
        r_lo = (r_lo & ~1UL) | n_i;

        if (r_hi != 0 || r_lo >= d)
        {
            const auto borrow = static_cast<uint64_t>(d > r_lo);
            r_lo -= d;
            r_hi -= borrow;

            if (i < 64)
            {
                q |= 1UL << iu;
            }
        }
    }
    return q;
}

// (2^128 - 1) / d
static inline auto longdiv128_1s(const uint64_t d) -> uint64_t
{
    uint64_t q = 0;
    uint64_t r_hi = 0;
    uint64_t r_lo = 0;

    for (int64_t i = 127; i >= 0; --i)
    {
        const auto iu = static_cast<uint64_t>(i);

        r_hi = (r_hi << 1U) | (r_lo >> 63U);
        r_lo <<= 1U;

        r_lo |= 1UL;

        if (r_hi != 0 || r_lo >= d)
        {
            const auto borrow = static_cast<const uint64_t>(d > r_lo);
            r_lo -= d;
            r_hi -= borrow;

            if (i < 64)
            {
                q |= 1UL << iu;
            }
        }
    }
    return q;
}

} // namespace br::util
