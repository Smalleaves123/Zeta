#ifndef ZETA_HASH_HASH_H
#define ZETA_HASH_HASH_H

/// @file   hash/hash.h
/// @brief  Composable hash framework inspired by `absl::Hash`.
///
/// Provides:
///   - `zeta::Hash<T>` — primary template users specialize for their types
///   - `zeta::HashState`    — hash state machine (xxHash64-based)
///   - `zeta::hash_combine` — mix a seed with a single value
///   - `zeta::hash_range`   — hash a range of hashable elements
///
/// To make a user-defined type hashable, provide a `Hash` specialization:
///
///   struct Point { int x, y; };
///   template <>
///   struct zeta::Hash<Point> {
///       uint64_t operator()(const Point& p) const noexcept {
///           return zeta::hash_combine(0, p.x, p.y);
///       }
///   };
///
/// Built-in support exists for integral types, floating point, strings,
/// and all `std::hash`-enabled types (via a fallback).
///
/// Example:
///   uint64_t h = zeta::Hash<std::string>()("hello world");
///   uint64_t h2 = zeta::hash_combine(42, 3.14, "abc");

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// HashState — xxHash64-based incremental hash state machine
// ═══════════════════════════════════════════════════════════════════════

namespace hash_internal {

// xxHash64 constants
constexpr uint64_t kPrime1 = 0x9E3779B185EBCA87ULL;
constexpr uint64_t kPrime2 = 0xC2B2AE3D27D4EB4FULL;
constexpr uint64_t kPrime3 = 0x165667B19E3779F9ULL;
constexpr uint64_t kPrime4 = 0x85EBCA77C2B2AE63ULL;
constexpr uint64_t kPrime5 = 0x27D4EB2F165667C5ULL;

[[nodiscard]] inline uint64_t Rotl64(uint64_t x, int r) noexcept {
    return (x << r) | (x >> (64 - r));
}

[[nodiscard]] inline uint64_t MixRound(uint64_t acc, uint64_t lane) noexcept {
    acc += lane * kPrime2;
    acc = Rotl64(acc, 31);
    acc *= kPrime1;
    return acc;
}

[[nodiscard]] inline uint64_t Avalanche(uint64_t h) noexcept {
    h ^= h >> 33;
    h *= kPrime2;
    h ^= h >> 29;
    h *= kPrime3;
    h ^= h >> 32;
    return h;
}

} // namespace hash_internal

/// A hash state machine for incremental hashing.
///
/// Encapsulates the xxHash64 algorithm.  Use `Combined(...)` for simple
/// cases, or `Feed()` / `Finalize()` for incremental streaming.
class HashState {
public:
    /// Start with a seed.
    explicit HashState(uint64_t seed = 0) noexcept
        : acc_(seed + hash_internal::kPrime5), len_(0) {
        buf_[0] = 0;  // zero the 32-byte buffer
        buf_[1] = 0;
        buf_[2] = 0;
        buf_[3] = 0;
    }

    /// Feed a block of bytes into the hash.
    void Feed(const void* data, size_t size) noexcept {
        const uint8_t* p = static_cast<const uint8_t*>(data);
        size_t remaining = size;
        size_t offset = len_ & 31;

        // Fill the buffer with leftover from previous call.
        if (offset > 0) {
            size_t fill = 32 - offset;
            if (fill > remaining) fill = remaining;
            std::memcpy(reinterpret_cast<uint8_t*>(buf_) + offset, p, fill);
            offset += fill;
            p += fill;
            remaining -= fill;
            if (offset == 32) {
                accumulate(buf_);
            }
        }

        // Process full 32-byte stripes.
        while (remaining >= 32) {
            const uint64_t* stripe = reinterpret_cast<const uint64_t*>(p);
            accumulate(stripe);
            p += 32;
            remaining -= 32;
        }

        // Save leftover.
        if (remaining > 0) {
            std::memcpy(reinterpret_cast<uint8_t*>(buf_), p, remaining);
        }
        len_ += size;
    }

    /// Finalize and return the 64-bit hash value.
    [[nodiscard]] uint64_t Finalize() noexcept {
        uint64_t h = acc_;

        // Mix in length.
        h += len_;

        // Process remaining bytes.
        size_t remaining = len_ & 31;
        const uint8_t* p = reinterpret_cast<const uint8_t*>(buf_);

        // 8-byte blocks
        while (remaining >= 8) {
            uint64_t lane;
            std::memcpy(&lane, p, 8);
            h ^= hash_internal::MixRound(0, lane);
            h = hash_internal::Rotl64(h, 27) * hash_internal::kPrime1 + hash_internal::kPrime4;
            p += 8;
            remaining -= 8;
        }

        // 4-byte block
        if (remaining >= 4) {
            uint32_t lane;
            std::memcpy(&lane, p, 4);
            h ^= static_cast<uint64_t>(lane) * hash_internal::kPrime1;
            h = hash_internal::Rotl64(h, 23) * hash_internal::kPrime2 + hash_internal::kPrime3;
            p += 4;
            remaining -= 4;
        }

        // Remaining bytes (1-3)
        while (remaining > 0) {
            h ^= static_cast<uint64_t>(*p) * hash_internal::kPrime5;
            h = hash_internal::Rotl64(h, 11) * hash_internal::kPrime1;
            ++p;
            --remaining;
        }

        return hash_internal::Avalanche(h);
    }

    /// One-shot: hash a sequence of values into a single `uint64_t`.
    template <typename... Ts>
    [[nodiscard]] static uint64_t Combined(uint64_t seed, const Ts&... values) noexcept {
        HashState hs(seed);
        (hs.FeedValue(values), ...);
        return hs.Finalize();
    }

    /// One-shot: hash a contiguous byte range.
    [[nodiscard]] static uint64_t CombinedBytes(
        const void* data, size_t len, uint64_t seed = 0) noexcept {
        HashState hs(seed);
        hs.Feed(data, len);
        return hs.Finalize();
    }

private:
    uint64_t acc_;
    uint64_t buf_[4];
    size_t   len_;

    void accumulate(const uint64_t* stripe) noexcept {
        uint64_t v1 = acc_ + stripe[0] * hash_internal::kPrime2;
        uint64_t v2 = acc_ + stripe[1] * hash_internal::kPrime2;
        uint64_t v3 = acc_ + stripe[2] * hash_internal::kPrime2;
        uint64_t v4 = acc_ + stripe[3] * hash_internal::kPrime2;
        v1 = hash_internal::Rotl64(v1, 31) * hash_internal::kPrime1;
        v2 = hash_internal::Rotl64(v2, 31) * hash_internal::kPrime1;
        v3 = hash_internal::Rotl64(v3, 31) * hash_internal::kPrime1;
        v4 = hash_internal::Rotl64(v4, 31) * hash_internal::kPrime1;
        acc_ = hash_internal::Rotl64(v1, 1) +
               hash_internal::Rotl64(v2, 7) +
               hash_internal::Rotl64(v3, 12) +
               hash_internal::Rotl64(v4, 18);
    }

    template <typename T>
    void FeedValue(const T& value) noexcept {
        if constexpr (std::is_integral_v<T>) {
            // Hash integers via their byte representation (little-endian safe).
            uint64_t v = 0;
            if constexpr (sizeof(T) <= sizeof(uint64_t)) {
                // Zero-extend to 64 bits.
                using U = std::make_unsigned_t<T>;
                v = static_cast<uint64_t>(static_cast<U>(value));
            } else {
                // For 128-bit integers, hash as two 64-bit parts.
                HashState hs(0);
                uint64_t lo = static_cast<uint64_t>(value);
                uint64_t hi = static_cast<uint64_t>(value >> 64);
                Feed(static_cast<const void*>(&lo), sizeof(lo));
                Feed(static_cast<const void*>(&hi), sizeof(hi));
                return;
            }
            v = hash_internal::MixRound(0, v);
            v = hash_internal::Rotl64(v, 27) * hash_internal::kPrime1 + hash_internal::kPrime4;
            // Mix into accumulator state.
            uint64_t stripe[4] = {v, 0, 0, 0};
            accumulate(stripe);
        } else if constexpr (std::is_floating_point_v<T>) {
            // Hash floating point via its byte representation.
            // +0.0 and -0.0 are deliberately different (matches IEEE).
            uint64_t bits;
            std::memcpy(&bits, &value, sizeof(bits));
            Feed(static_cast<const void*>(&bits), sizeof(bits));
        } else {
            // Generic: fall back to std::hash.
            uint64_t h = std::hash<std::decay_t<T>>{}(value);
            Feed(static_cast<const void*>(&h), sizeof(h));
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════
// Hash<T> — primary template (users specialize)
// ═══════════════════════════════════════════════════════════════════════

/// Primary hash functor.
///
/// By default falls back to `std::hash<T>` for types that have it.
/// Specialize for custom types to integrate with `hash_combine` / `hash_range`.
template <typename T>
struct Hash {
    [[nodiscard]] uint64_t operator()(const T& value) const noexcept {
        return std::hash<T>{}(value);
    }
};

// ── Built-in specializations ────────────────────────────────────────

/// `Hash<std::string>` uses xxHash64 over the string bytes.
template <>
struct Hash<std::string> {
    [[nodiscard]] uint64_t operator()(const std::string& s) const noexcept {
        return HashState::CombinedBytes(s.data(), s.size());
    }
};

/// `Hash<std::string_view>` uses xxHash64 over the string bytes.
template <>
struct Hash<std::string_view> {
    [[nodiscard]] uint64_t operator()(const std::string_view& sv) const noexcept {
        return HashState::CombinedBytes(sv.data(), sv.size());
    }
};

/// `Hash<const char*>` — for C string literals.
template <>
struct Hash<const char*> {
    [[nodiscard]] uint64_t operator()(const char* s) const noexcept {
        return HashState::CombinedBytes(s, std::strlen(s));
    }
};

// ═══════════════════════════════════════════════════════════════════════
// Free functions: hash_combine, hash_range
// ═══════════════════════════════════════════════════════════════════════

/// Combines a seed with one or more hashable values.
///
///   uint64_t h = zeta::hash_combine(0, a, b, c);
template <typename... Ts>
[[nodiscard]] uint64_t hash_combine(uint64_t seed,
                                     const Ts&... values) noexcept {
    return HashState::Combined(seed, values...);
}

/// Hashes a range of elements identified by [begin, end).
///
///   std::vector<int> v = {1, 2, 3};
///   uint64_t h = zeta::hash_range(v.begin(), v.end());
template <typename It>
[[nodiscard]] uint64_t hash_range(It begin, It end,
                                   uint64_t seed = 0) noexcept {
    HashState hs(seed);
    for (; begin != end; ++begin) {
        using T = typename std::iterator_traits<It>::value_type;
        uint64_t elem_h = Hash<T>{}(*begin);
        hs.Feed(&elem_h, sizeof(elem_h));
    }
    return hs.Finalize();
}

/// Hashes a contiguous range of bytes.
///
///   uint64_t h = zeta::hash_bytes(ptr, len, seed);
[[nodiscard]] inline uint64_t hash_bytes(
    const void* data, size_t len, uint64_t seed = 0) noexcept {
    return HashState::CombinedBytes(data, len, seed);
}

} // namespace zeta

#endif // ZETA_HASH_HASH_H
