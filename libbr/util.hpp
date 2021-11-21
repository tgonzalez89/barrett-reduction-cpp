#pragma once

#include <cstdint>


namespace br
{

    namespace util
    {

        // https://stackoverflow.com/a/50958815/6553631
    #ifdef __SIZEOF_INT128__    // GNU C
        static inline
        uint64_t mulhi64(uint64_t a, uint64_t b) {
            unsigned __int128 prod =  a * (unsigned __int128)b;
            return prod >> 64;
        }

    #elif defined(_M_X64) || defined(_M_ARM64)    // MSVC
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
        static inline
        uint64_t mulhi64(uint64_t a, uint64_t b) {
            unsigned __int64 HighProduct;
            (void)_umul128(a, b, &HighProduct);
            return HighProduct;
        }

    #else
        // a * b = (k*a_hi + a_lo) * (k*b_hi + b_lo)
        // k = 2^32
        // https://stackoverflow.com/a/31662911/6553631
        uint64_t mulhi64(uint64_t a, uint64_t b)
        {
            const uint64_t a_lo = static_cast<uint32_t>(a);
            const uint64_t a_hi = a >> 32;
            const uint64_t b_lo = static_cast<uint32_t>(b);
            const uint64_t b_hi = b >> 32;

            const uint64_t p0 = a_lo * b_lo;
            const uint64_t p1 = a_lo * b_hi;
            const uint64_t p2 = a_hi * b_lo;
            const uint64_t p3 = a_hi * b_hi;

            const uint32_t cy = ((p0 >> 32) + static_cast<uint32_t>(p1) + static_cast<uint32_t>(p2)) >> 32;

            return p3 + (p1 >> 32) + (p2 >> 32) + cy;
        }
    #endif

        // https://en.wikipedia.org/wiki/Division_algorithm#Integer_division_(unsigned)_with_remainder

        uint64_t longdiv64(uint64_t n, uint64_t d) {
            uint64_t q = 0, r = 0;

            for (int i = 63; i >= 0; --i) {
                r <<= 1;

                const uint64_t n_i = (n & (1UL << i)) != 0;
                r = (r & ~1UL) | n_i;

                if (r >= d) {
                    r -= d;
                    q |= 1UL << i;
                }
            }
            return q;
        }

        uint64_t longdiv128(uint64_t n_hi, uint64_t n_lo, uint64_t d) {
            uint64_t q = 0, r_hi = 0, r_lo = 0;

            for (int i = 127; i >= 0; --i) {
                r_hi = (r_hi << 1) | (r_lo >> 63);
                r_lo <<= 1;

                uint64_t n_i;
                if (i >= 64)
                    n_i = (n_hi & (1UL << (i - 64))) != 0;
                else
                    n_i = (n_lo & (1UL << i)) != 0;
                r_lo = (r_lo & ~1UL) | n_i;

                if (r_hi || r_lo >= d) {
                    const uint64_t borrow = d > r_lo;
                    r_lo -= d;
                    r_hi -= borrow;

                    if (i < 64)
                        q |= 1UL << i;
                }
            }
            return q;
        }

        // (2^128 - 1) / d
        uint64_t longdiv128_1s(uint64_t d) {
            uint64_t q = 0, r_hi = 0, r_lo = 0;

            for (int i = 127; i >= 0; --i) {
                r_hi = (r_hi << 1) | (r_lo >> 63);
                r_lo <<= 1;

                r_lo |= 1UL;

                if (r_hi || r_lo >= d) {
                    const uint64_t borrow = d > r_lo;
                    r_lo -= d;
                    r_hi -= borrow;

                    if (i < 64)
                        q |= 1UL << i;
                }
            }
            return q;
        }

    } // namespace util

} // namespace br
