#include "zeta/random/random.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <set>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// BitGen: basic properties
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("BitGen: default construction produces deterministic sequence", "[random]") {
    zeta::BitGen g1;
    zeta::BitGen g2;
    // Same default seed → same sequence
    for (int i = 0; i < 10; ++i) {
        REQUIRE(g1() == g2());
    }
}

TEST_CASE("BitGen: explicit seed produces deterministic sequence", "[random]") {
    zeta::BitGen g1(42);
    zeta::BitGen g2(42);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(g1() == g2());
    }
}

TEST_CASE("BitGen: different seeds produce different sequences", "[random]") {
    zeta::BitGen g1(42);
    zeta::BitGen g2(99);
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (g1() == g2()) ++same;
    }
    // Extremely unlikely that all match
    REQUIRE(same < 100);
}

TEST_CASE("BitGen: min() returns 0", "[random]") {
    REQUIRE(zeta::BitGen::min() == 0);
}

TEST_CASE("BitGen: max() returns UINT64_MAX", "[random]") {
    REQUIRE(zeta::BitGen::max() == UINT64_MAX);
}

TEST_CASE("BitGen: produces non-zero values eventually", "[random]") {
    zeta::BitGen g(777);
    bool saw_nonzero = false;
    for (int i = 0; i < 100; ++i) {
        if (g() != 0) { saw_nonzero = true; break; }
    }
    REQUIRE(saw_nonzero);
}

TEST_CASE("BitGen: jump produces independent stream", "[random]") {
    zeta::BitGen g1(42);
    zeta::BitGen g2(42);
    g2.jump();
    int same = 0;
    for (int i = 0; i < 100; ++i) {
        if (g1() == g2()) ++same;
    }
    // After jump, streams should diverge significantly
    REQUIRE(same < 20); // conservative bound
}

// ═══════════════════════════════════════════════════════════════════
// Uniform: integer
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Uniform<int>: values are in range [a, b]", "[random][uniform]") {
    zeta::BitGen gen(123);
    for (int i = 0; i < 1000; ++i) {
        int v = zeta::Uniform(gen, 10, 20);
        REQUIRE(v >= 10);
        REQUIRE(v <= 20);
    }
}

TEST_CASE("Uniform<int>: single element range returns that element", "[random][uniform]") {
    zeta::BitGen gen(456);
    for (int i = 0; i < 10; ++i) {
        REQUIRE(zeta::Uniform(gen, 7, 7) == 7);
    }
}

TEST_CASE("Uniform<int>: a == b returns a", "[random][uniform]") {
    zeta::BitGen gen(1);
    REQUIRE(zeta::Uniform(gen, 5, 5) == 5);
    REQUIRE(zeta::Uniform(gen, -3, -3) == -3);
}

TEST_CASE("Uniform<int>: covers all values in small range", "[random][uniform]") {
    zeta::BitGen gen(999);
    std::set<int> seen;
    for (int i = 0; i < 5000; ++i) {
        seen.insert(zeta::Uniform(gen, 1, 10));
    }
    // With 5000 samples over [1,10], should see all 10 values
    for (int v = 1; v <= 10; ++v) {
        REQUIRE(seen.count(v) == 1);
    }
}

TEST_CASE("Uniform<int>: negative range", "[random][uniform]") {
    zeta::BitGen gen(42);
    for (int i = 0; i < 500; ++i) {
        int v = zeta::Uniform(gen, -10, -1);
        REQUIRE(v >= -10);
        REQUIRE(v <= -1);
    }
}

TEST_CASE("Uniform<int64_t>: large 64-bit range", "[random][uniform]") {
    zeta::BitGen gen(77);
    for (int i = 0; i < 100; ++i) {
        int64_t v = zeta::Uniform<int64_t>(gen, 0, 1000000000);
        REQUIRE(v >= 0);
        REQUIRE(v <= 1000000000);
    }
}

TEST_CASE("Uniform<uint64_t>: full 64-bit range", "[random][uniform]") {
    zeta::BitGen gen(88);
    // Just verify it doesn't crash / compile
    uint64_t v = zeta::Uniform<uint64_t>(gen, 0, UINT64_MAX);
    (void)v;
    SUCCEED("full 64-bit range compiles");
}

// ═══════════════════════════════════════════════════════════════════
// Uniform: floating point
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Uniform<double>: values in [a, b)", "[random][uniform]") {
    zeta::BitGen gen(42);
    for (int i = 0; i < 1000; ++i) {
        double v = zeta::Uniform(gen, 0.0, 1.0);
        REQUIRE(v >= 0.0);
        REQUIRE(v < 1.0);
    }
}

TEST_CASE("Uniform<double>: a == b returns a", "[random][uniform]") {
    zeta::BitGen gen(1);
    REQUIRE(zeta::Uniform(gen, 3.14, 3.14) == 3.14);
}

TEST_CASE("Uniform<double>: spread covers the range", "[random][uniform]") {
    zeta::BitGen gen(123);
    bool saw_low  = false; // < 0.33
    bool saw_mid  = false; // 0.33 - 0.66
    bool saw_high = false; // > 0.66
    for (int i = 0; i < 1000; ++i) {
        double v = zeta::Uniform(gen, 0.0, 1.0);
        if (v < 0.33) saw_low = true;
        else if (v < 0.66) saw_mid = true;
        else saw_high = true;
    }
    REQUIRE(saw_low);
    REQUIRE(saw_mid);
    REQUIRE(saw_high);
}

TEST_CASE("Uniform<float>: values in range", "[random][uniform]") {
    zeta::BitGen gen(55);
    for (int i = 0; i < 500; ++i) {
        float v = zeta::Uniform(gen, 0.0f, 100.0f);
        REQUIRE(v >= 0.0f);
        REQUIRE(v < 100.0f);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Bernoulli
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Bernoulli: p=1.0 always true", "[random][bernoulli]") {
    zeta::BitGen gen(42);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(zeta::Bernoulli(gen, 1.0) == true);
    }
}

TEST_CASE("Bernoulli: p=0.0 always false", "[random][bernoulli]") {
    zeta::BitGen gen(42);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(zeta::Bernoulli(gen, 0.0) == false);
    }
}

TEST_CASE("Bernoulli: p=0.5 approximates fair coin", "[random][bernoulli]") {
    zeta::BitGen gen(999);
    int heads = 0;
    const int N = 10000;
    for (int i = 0; i < N; ++i) {
        if (zeta::Bernoulli(gen, 0.5)) ++heads;
    }
    double rate = static_cast<double>(heads) / N;
    // 3-sigma bound for N=10000: ~0.5 ± 0.015
    REQUIRE(rate > 0.47);
    REQUIRE(rate < 0.53);
}

TEST_CASE("Bernoulli: p outside [0,1] handled", "[random][bernoulli]") {
    zeta::BitGen gen(1);
    REQUIRE(zeta::Bernoulli(gen, -0.5) == false);
    REQUIRE(zeta::Bernoulli(gen, 1.5) == true);
}

// ═══════════════════════════════════════════════════════════════════
// Statistical: mean of uniform [0,N) ≈ N/2
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Uniform<int>: mean approximates midpoint", "[random][statistical]") {
    zeta::BitGen gen(42);
    const int N = 10000;
    const int range = 100;
    double sum = 0;
    for (int i = 0; i < N; ++i) {
        sum += zeta::Uniform(gen, 0, range - 1);
    }
    double mean = sum / N;
    // Expected mean ≈ 49.5 with standard error ≈ 28.9 / sqrt(10000) ≈ 0.29
    REQUIRE(mean > 48.5);
    REQUIRE(mean < 50.5);
}
