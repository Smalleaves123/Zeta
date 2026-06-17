#include "zeta/random/random.h"
#include <cstdio>
#include <cstdlib>

int main() {
    // ── BitGen ───────────────────────────────────────────────────
    zeta::BitGen g1(42);
    zeta::BitGen g2(42);
    // Deterministic
    for (int i = 0; i < 100; ++i) {
        if (g1() != g2()) {
            std::printf("FAIL: BitGen not deterministic\n");
            return 1;
        }
    }

    // ── Uniform int ──────────────────────────────────────────────
    zeta::BitGen gen(123);
    for (int i = 0; i < 1000; ++i) {
        int v = zeta::Uniform(gen, 1, 6);
        if (v < 1 || v > 6) {
            std::printf("FAIL: Uniform<int> out of range: %d\n", v);
            return 1;
        }
    }

    // ── Uniform double ───────────────────────────────────────────
    for (int i = 0; i < 1000; ++i) {
        double v = zeta::Uniform(gen, 0.0, 1.0);
        if (v < 0.0 || v >= 1.0) {
            std::printf("FAIL: Uniform<double> out of range: %f\n", v);
            return 1;
        }
    }

    // ── Bernoulli ────────────────────────────────────────────────
    if (!zeta::Bernoulli(gen, 1.0)) { std::printf("FAIL: Bernoulli(1.0)\n"); return 1; }
    if (zeta::Bernoulli(gen, 0.0))  { std::printf("FAIL: Bernoulli(0.0)\n"); return 1; }

    // ── Jump ─────────────────────────────────────────────────────
    zeta::BitGen g3(42);
    zeta::BitGen g4(42);
    g4.jump();
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (g3() == g4()) ++same;
    }
    if (same >= 20) {
        std::printf("FAIL: jump divergence insufficient: %d/100 same\n", same);
        return 1;
    }

    // ── Type coverage ────────────────────────────────────────────
    (void)zeta::Uniform<int64_t>(gen, 0LL, 100LL);
    (void)zeta::Uniform<uint32_t>(gen, 0U, 100U);
    (void)zeta::Uniform<float>(gen, 0.0f, 1.0f);

    std::printf("OK: All random tests passed\n");
    return 0;
}
