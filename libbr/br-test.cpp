#include <iostream>
#include <random>

#include "libbr/br.hpp"


void test_br32()
{
    std::cout << "Testing BR32.\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    for (uint32_t bitlen = 1; bitlen <= 31; ++bitlen) {
        const uint32_t min_n = (1U << bitlen) + 1;
        const uint32_t max_n = UINT32_MAX >> (31 - bitlen);
        std::uniform_int_distribution<uint32_t> distr_n(min_n, max_n);
        for (size_t i = 0; i < 1000; ++i) {
            const uint32_t n = distr_n(gen);
            br::BarrettRed32 br(n);
            uint32_t n2 = n > UINT16_MAX ? UINT32_MAX : n*n - 1;
            std::uniform_int_distribution<uint32_t> distr_x(0, n2);
            for (size_t j = 0; j < 1000; ++j) {
                const uint32_t x = distr_x(gen);
                const uint32_t res = br.calc(x);
                const uint32_t ref = x % n;
                if (res != ref) {
                    std::cout << "res=" << res << ", ref=" << ref << "\n";
                    std::cout << "x=" << x << ", n=" << n << ", r=" << br.r << "\n";
                    throw std::runtime_error("Barrett reduction test failed.");
                }
            }
        }
    }
}

void test_br64()
{
    std::cout << "Testing BR64.\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    for (uint64_t bitlen = 1; bitlen <= 63; ++bitlen) {
        const uint64_t min_n = (1UL << bitlen) + 1;
        const uint64_t max_n = UINT64_MAX >> (63 - bitlen);
        std::uniform_int_distribution<uint64_t> distr_n(min_n, max_n);
        for (size_t i = 0; i < 1000; ++i) {
            const uint64_t n = distr_n(gen);
            br::BarrettRed64 br(n);
            uint64_t n2 = n > UINT32_MAX ? UINT64_MAX : n*n - 1;
            std::uniform_int_distribution<uint64_t> distr_x(0, n2);
            for (size_t j = 0; j < 1000; ++j) {
                const uint64_t x = distr_x(gen);
                const uint64_t res = br.calc(x);
                const uint64_t ref = x % n;
                if (res != ref) {
                    std::cout << "res=" << res << ", ref=" << ref << "\n";
                    std::cout << "x=" << x << ", n=" << n << ", r=" << br.r << "\n";
                    throw std::runtime_error("Barrett reduction test failed.");
                }
            }
        }
    }
}

void test_br128()
{
    using uint128_t = unsigned __int128;

    std::cout << "Testing BR128.\n";

    std::random_device rd;
    std::mt19937 gen(rd());
    for (uint64_t bitlen = 1; bitlen <= 63; ++bitlen) {
        const uint64_t min_n = (1UL << bitlen) + 1;
        const uint64_t max_n = UINT64_MAX >> (63 - bitlen);
        std::uniform_int_distribution<uint64_t> distr_n(min_n, max_n);
        for (size_t i = 0; i < 1000; ++i) {
            const uint64_t n = distr_n(gen);
            br::BarrettRed128 br(n);
            uint128_t n2 = static_cast<uint128_t>(n) * static_cast<uint128_t>(n) - 1;
            std::uniform_int_distribution<uint128_t> distr_x(0, n2);
            for (size_t j = 0; j < 1000; ++j) {
                const uint128_t x = distr_x(gen);
                const uint64_t x_lo = static_cast<uint64_t>(x);
                const uint64_t x_hi = static_cast<uint64_t>(x >> 64);
                uint64_t res1 = 0;
                if (n < (1UL<<63))
                    res1 = br.calc(x_hi, x_lo);
                const uint64_t res2 = br.calc(x);
                const uint64_t ref = x % n;
                if ((res1 != ref && n < (1UL<<63)) || res2 != ref) {
                    std::cout << "res1=" << res1 << ", res2=" << res2 << ", ref=" << ref << "\n";
                    std::cout << "x_hi=" << x_hi << ", x_lo=" << x_lo << ", n=" << n << ", r=" << br.r << ", s=" << br.s << ", t=" << br.t << "\n";
                    throw std::runtime_error("Barrett reduction test failed.");
                }
            }
        }
    }
}

int main()
{
    test_br32();
    test_br64();
    test_br128();
    return 0;
}
