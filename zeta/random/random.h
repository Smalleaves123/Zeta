#ifndef ZETA_RANDOM_RANDOM_H
#define ZETA_RANDOM_RANDOM_H

/// @file   random/random.h
/// @brief  Fast, ergonomic random number generation.
///
/// `zeta::BitGen` is a Xoshiro256** pseudorandom engine with a
/// 256-bit state, period 2^256-1, and excellent statistical quality.
/// Paired with `Uniform()` and `Bernoulli()`, it replaces the verbose
/// `<random>` API with a clean, intuitive interface.
///
/// Example:
///   zeta::BitGen gen;
///   int    dice = zeta::Uniform(gen, 1, 6);
///   double prob = zeta::Uniform(gen, 0.0, 1.0);
///   bool  coin  = zeta::Bernoulli(gen, 0.5);

#include <bit>
#include <cstdint>
#include <limits>
#include <type_traits>

#include "zeta/bits/bit_ops.h"

namespace zeta {

// ── BitGen — Xoshiro256** engine ─────────────────────────────────────

/// Fast, high-quality 64-bit PRNG.
///
/// State: 4 × 64 bits = 256 bits (32 bytes).  Seeded via SplitMix64.
/// Quality: passes BigCrush and PractRand.
///
/// Thread-safety: each `BitGen` is independent.  Create one per thread
/// or use thread_local.  The engine itself has no internal locks.
class BitGen {
public:
    using result_type = uint64_t;

    BitGen() { seed(5489ULL); }

    /// Seed with a single 64-bit value (expanded to 256 bits via SplitMix64).
    explicit BitGen(uint64_t seed_val) { seed(seed_val); }

    // Compatible with std::uniform_int_distribution etc.
    static constexpr uint64_t min() noexcept { return 0; }
    static constexpr uint64_t max() noexcept { return UINT64_MAX; }

    /// Generate the next 64-bit random value.
    uint64_t operator()() noexcept {
        const uint64_t result = std::rotl(s_[1] * 5ULL, 7) * 9ULL;
        const uint64_t t = s_[1] << 17ULL;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3]  = std::rotl(s_[3], 45);
        return result;
    }

    /// Jump ahead by ~2^128 steps (for independent streams).
    void jump() noexcept {
        static constexpr uint64_t kJump[] = {
            0x180ec6d33cfd0abaULL, 0xd5a61266f0c9392cULL,
            0xa9582618e03fc9aaULL, 0x39abdc4529b1661cULL,
        };
        uint64_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        for (auto j : kJump) {
            for (int b = 0; b < 64; ++b) {
                if (j & (1ULL << b)) {
                    s0 ^= s_[0]; s1 ^= s_[1]; s2 ^= s_[2]; s3 ^= s_[3];
                }
                operator()();
            }
        }
        s_[0] = s0; s_[1] = s1; s_[2] = s2; s_[3] = s3;
    }

private:
    uint64_t s_[4];

    void seed(uint64_t seed_val) noexcept {
        // SplitMix64 expansion from a single seed to four state words.
        auto splitmix64 = [&]() -> uint64_t {
            uint64_t z = (seed_val += 0x9e3779b97f4a7c15ULL);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
            return z ^ (z >> 31);
        };
        s_[0] = splitmix64();
        s_[1] = splitmix64();
        s_[2] = splitmix64();
        s_[3] = splitmix64();
    }
};

// ── Uniform integer distribution ─────────────────────────────────────

namespace random_internal {

/// Unbiased random integer in [0, range).  Uses Lemire's method when
/// 128-bit multiply is available; falls back to rejection sampling on
/// platforms without __uint128_t (e.g. MSVC).
inline uint64_t uniform_uint64(BitGen& gen, uint64_t range) {
    if (range == 0) return 0;
    // Fast path: power of two
    if (zeta::has_single_bit(range)) {
        return gen() & (range - 1);
    }

#if defined(__SIZEOF_INT128__)
    // Lemire's nearly divisionless method (needs 128-bit multiply).
    uint64_t x = gen();
    __uint128_t m = __uint128_t(x) * __uint128_t(range);
    uint64_t l = static_cast<uint64_t>(m);
    if (l < range) {
        uint64_t t = (~range + 1) % range;
        while (l < t) {
            x = gen();
            m = __uint128_t(x) * __uint128_t(range);
            l = static_cast<uint64_t>(m);
        }
    }
    return static_cast<uint64_t>(m >> 64);
#else
    // Rejection sampling — correct on all platforms.
    uint64_t limit = (~range + 1) % range;
    while (true) {
        uint64_t x = gen();
        if (x >= limit) return x % range;
    }
#endif
}

} // namespace random_internal

// ── Uniform distribution ─────────────────────────────────────────────

/// Returns a value uniformly distributed in [a, b].
///
/// For integer types: [a, b] inclusive.
/// For floating-point types: [a, b) half-open.
///
/// Works with any arithmetic type (`int`, `int64_t`, `float`, `double`, etc.).
template <typename T>
T Uniform(BitGen& gen, T a, T b) {
    static_assert(std::is_arithmetic_v<T>, "Uniform requires an arithmetic type");
    if (a >= b) return a;

    if constexpr (std::is_integral_v<T>) {
        using UInt = std::make_unsigned_t<T>;
        UInt range = static_cast<UInt>(b) - static_cast<UInt>(a);
        uint64_t r = random_internal::uniform_uint64(gen, static_cast<uint64_t>(range) + 1);
        return static_cast<T>(static_cast<UInt>(a) + static_cast<UInt>(r));
    } else {
        // Floating point: [a, b) with 53 bits of precision.
        constexpr uint64_t kMantissaBits = 53;
        uint64_t r = gen() >> (64 - kMantissaBits);
        T unit = static_cast<T>(r) * static_cast<T>(0x1.0p-53);
        return a + unit * (b - a);
    }
}

// ── Bernoulli distribution ───────────────────────────────────────────

/// Returns true with probability `p`.  `p` must be in [0, 1].
inline bool Bernoulli(BitGen& gen, double p) {
    if (p <= 0.0) return false;
    if (p >= 1.0) return true;
    return Uniform(gen, 0.0, 1.0) < p;
}

} // namespace zeta

#endif // ZETA_RANDOM_RANDOM_H
