#include "zeta/random/random.h"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                      std::size_t size) {
    std::uint64_t seed = 0;
    for (std::size_t i = 0; i < size && i < 8; ++i) {
        seed = (seed << 8) | data[i];
    }

    zeta::BitGen g1(seed);
    zeta::BitGen g2(seed);
    for (int i = 0; i < 16; ++i) {
        if (g1() != g2()) __builtin_trap();
    }

    zeta::BitGen g3(seed);
    if (size > 8 && (data[8] & 1U)) g3.jump();

    int a = size > 9 ? static_cast<int>(static_cast<int8_t>(data[9])) : -10;
    int b = size > 10 ? static_cast<int>(static_cast<int8_t>(data[10])) : 10;
    if (a > b) {
        int tmp = a;
        a = b;
        b = tmp;
    }

    for (int i = 0; i < 64; ++i) {
        int iv = zeta::Uniform(g3, a, b);
        if (iv < a || iv > b) __builtin_trap();
    }

    double lo = size > 11 ? static_cast<double>(data[11]) / 16.0 : 0.0;
    double hi = size > 12 ? static_cast<double>(data[12]) / 8.0 : 1.0;
    if (lo > hi) {
        double tmp = lo;
        lo = hi;
        hi = tmp;
    }
    for (int i = 0; i < 64; ++i) {
        double dv = zeta::Uniform(g3, lo, hi);
        if (lo < hi) {
            if (dv < lo || dv >= hi) __builtin_trap();
        } else if (dv != lo) {
            __builtin_trap();
        }
    }

    if (zeta::Bernoulli(g3, -0.1)) __builtin_trap();
    if (!zeta::Bernoulli(g3, 1.1)) __builtin_trap();

    volatile std::uint64_t sink = g3();
    (void)sink;
    return 0;
}
