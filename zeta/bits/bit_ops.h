#ifndef ZETA_BITS_BIT_OPS_H
#define ZETA_BITS_BIT_OPS_H

/// @file   bits/bit_ops.h
/// @brief  Portable, zero-overhead bit manipulation primitives.
///
/// Every function delegates to C++20 `<bit>` or compiler builtins where
/// available, with a generic fallback for portability.  All functions are
/// `constexpr`, `noexcept`, `[[nodiscard]]`, and constrained to unsigned
/// integral types via `std::unsigned_integral`.
///
/// Naming follows the C++20 `<bit>` convention (snake_case):
///   popcount, countl_zero, rotl, has_single_bit, bit_ceil, byteswap, …
///
/// Example:
///   #include <zeta/bits/bit_ops.h>
///   uint64_t x = 0b10110000;
///   int n = zeta::popcount(x);          // 3
///   int z = zeta::countr_zero(x);       // 4
///   uint64_t y = zeta::rotl(x, 3);      // 0b10110000000

#include <bit>
#include <climits>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Population count & leading / trailing zeros / ones
// ═══════════════════════════════════════════════════════════════════════

/// Number of set bits (population count).
template <std::unsigned_integral T>
[[nodiscard]] constexpr int popcount(T x) noexcept {
    return std::popcount(x);
}

/// Number of consecutive 0 bits starting from the most-significant bit.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int countl_zero(T x) noexcept {
    return std::countl_zero(x);
}

/// Number of consecutive 0 bits starting from the least-significant bit.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int countr_zero(T x) noexcept {
    return std::countr_zero(x);
}

/// Number of consecutive 1 bits starting from the most-significant bit.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int countl_one(T x) noexcept {
    return countl_zero(static_cast<T>(~x));
}

/// Number of consecutive 1 bits starting from the least-significant bit.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int countr_one(T x) noexcept {
    return countr_zero(static_cast<T>(~x));
}

// ═══════════════════════════════════════════════════════════════════════
// Rotation
// ═══════════════════════════════════════════════════════════════════════

/// Rotate `x` left by `s` bits.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T rotl(T x, int s) noexcept {
    return std::rotl(x, s);
}

/// Rotate `x` right by `s` bits.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T rotr(T x, int s) noexcept {
    return std::rotr(x, s);
}

// ═══════════════════════════════════════════════════════════════════════
// Power-of-two queries
// ═══════════════════════════════════════════════════════════════════════

/// True iff `x` is a power of two (and `x > 0`).
template <std::unsigned_integral T>
[[nodiscard]] constexpr bool has_single_bit(T x) noexcept {
    return std::has_single_bit(x);
}

/// Smallest power of two >= `x`.  Returns 0 if the result cannot be
/// represented in `T` (overflow).
template <std::unsigned_integral T>
[[nodiscard]] constexpr T bit_ceil(T x) noexcept {
    return std::bit_ceil(x);
}

/// Largest power of two <= `x`.  Returns 0 when `x == 0`.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T bit_floor(T x) noexcept {
    return std::bit_floor(x);
}

/// Minimum number of bits required to represent `x`.
/// Returns 0 when `x == 0`.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int bit_width(T x) noexcept {
    return std::bit_width(x);
}

// ═══════════════════════════════════════════════════════════════════════
// Bit scanning (0-indexed from LSB)
// ═══════════════════════════════════════════════════════════════════════

/// Index of the least-significant set bit.
/// Returns `std::numeric_limits<T>::digits` when `x == 0`.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int find_first_set(T x) noexcept {
    return countr_zero(x);
}

/// Index of the most-significant set bit.
/// Returns -1 when `x == 0`.
template <std::unsigned_integral T>
[[nodiscard]] constexpr int find_last_set(T x) noexcept {
    return x == 0 ? -1 : (bit_width(x) - 1);
}

// ═══════════════════════════════════════════════════════════════════════
// Individual bit manipulation (pure — returns a new value)
// ═══════════════════════════════════════════════════════════════════════

/// Set bit `bit` in `value` (0 = LSB).
template <std::unsigned_integral T>
[[nodiscard]] constexpr T set_bit(T value, int bit) noexcept {
    return value | (static_cast<T>(1) << bit);
}

/// Clear bit `bit` in `value` (0 = LSB).
template <std::unsigned_integral T>
[[nodiscard]] constexpr T clear_bit(T value, int bit) noexcept {
    return value & ~(static_cast<T>(1) << bit);
}

/// Toggle bit `bit` in `value` (0 = LSB).
template <std::unsigned_integral T>
[[nodiscard]] constexpr T toggle_bit(T value, int bit) noexcept {
    return value ^ (static_cast<T>(1) << bit);
}

/// Test whether bit `bit` in `value` is set.
template <std::unsigned_integral T>
[[nodiscard]] constexpr bool test_bit(T value, int bit) noexcept {
    return (value >> bit) & 1;
}

// ═══════════════════════════════════════════════════════════════════════
// Range extraction / insertion
// ═══════════════════════════════════════════════════════════════════════

/// Extract `len` bits from `value` starting at `pos` (0 = LSB).
/// Clamps out-of-range `pos` / `len` instead of invoking UB.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T bit_extract(T value, int pos, int len) noexcept {
    constexpr int digits = std::numeric_limits<T>::digits;
    if (len <= 0 || pos < 0 || pos >= digits) return T{0};
    if (len > digits - pos) len = digits - pos;
    if (len == digits) return value >> pos;  // avoid (T(1) << digits) overflow
    return (value >> pos) & ((static_cast<T>(1) << len) - 1);
}

/// Insert `bits` into `value` at position `pos` of width `len`.
/// Only the lower `len` bits of `bits` are used.  Clamps out-of-range.
/// The `bits` parameter uses `std::type_identity_t` so that it does not
/// participate in template argument deduction — implicit conversion from
/// integer literals (e.g. `0b101`) is allowed.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T bit_insert(T value, int pos, int len,
                                     std::type_identity_t<T> bits) noexcept {
    constexpr int digits = std::numeric_limits<T>::digits;
    if (len <= 0 || pos < 0 || pos >= digits) return value;
    if (len > digits - pos) len = digits - pos;
    T mask;
    if (len == digits) {
        mask = ~T{0};
    } else {
        mask = ((static_cast<T>(1) << len) - 1) << pos;
    }
    return (value & ~mask) | ((bits << pos) & mask);
}

// ═══════════════════════════════════════════════════════════════════════
// Byte swap
// ═══════════════════════════════════════════════════════════════════════

namespace bits_internal {

template <std::unsigned_integral T, size_t Size = sizeof(T)>
struct byteswap_impl {
    static constexpr T apply(T x) noexcept {
        // Generic byte-by-byte reverse (reachable only for unusual widths).
        union {
            T val;
            unsigned char bytes[sizeof(T)];
        } u = {x};
        for (size_t i = 0, j = sizeof(T) - 1; i < j; ++i, --j) {
            unsigned char tmp = u.bytes[i];
            u.bytes[i] = u.bytes[j];
            u.bytes[j] = tmp;
        }
        return u.val;
    }
};

// Specializations use compiler builtins for optimal codegen.
template <std::unsigned_integral T>
struct byteswap_impl<T, 1> {
    static constexpr T apply(T x) noexcept { return x; }
};

template <std::unsigned_integral T>
struct byteswap_impl<T, 2> {
    static constexpr T apply(T x) noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
        return static_cast<T>(_byteswap_ushort(static_cast<unsigned short>(x)));
#else
        return static_cast<T>(__builtin_bswap16(static_cast<uint16_t>(x)));
#endif
    }
};

template <std::unsigned_integral T>
struct byteswap_impl<T, 4> {
    static constexpr T apply(T x) noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
        return static_cast<T>(_byteswap_ulong(static_cast<unsigned long>(x)));
#else
        return static_cast<T>(__builtin_bswap32(static_cast<uint32_t>(x)));
#endif
    }
};

template <std::unsigned_integral T>
struct byteswap_impl<T, 8> {
    static constexpr T apply(T x) noexcept {
#if defined(_MSC_VER) && !defined(__clang__)
        return static_cast<T>(
            _byteswap_uint64(static_cast<unsigned __int64>(x)));
#else
        return static_cast<T>(__builtin_bswap64(static_cast<uint64_t>(x)));
#endif
    }
};

} // namespace bits_internal

/// Reverse the byte order of `x`.
///   byteswap(uint16_t{0x0102}) == 0x0201
///   byteswap(uint32_t{0x01020304}) == 0x04030201
///   byteswap(uint64_t{0x0102030405060708}) == 0x0807060504030201
template <std::unsigned_integral T>
[[nodiscard]] constexpr T byteswap(T x) noexcept {
    return bits_internal::byteswap_impl<T>::apply(x);
}

// ═══════════════════════════════════════════════════════════════════════
// Endianness conversions
// ═══════════════════════════════════════════════════════════════════════

static_assert(std::endian::native == std::endian::little ||
                  std::endian::native == std::endian::big,
              "Only little-endian and big-endian platforms are supported");

/// Convert from big-endian to native byte order.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T big_to_native(T x) noexcept {
    if constexpr (std::endian::native == std::endian::big) return x;
    else return byteswap(x);
}

/// Convert from native to big-endian byte order.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T native_to_big(T x) noexcept {
    if constexpr (std::endian::native == std::endian::big) return x;
    else return byteswap(x);
}

/// Convert from little-endian to native byte order.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T little_to_native(T x) noexcept {
    if constexpr (std::endian::native == std::endian::little) return x;
    else return byteswap(x);
}

/// Convert from native to little-endian byte order.
template <std::unsigned_integral T>
[[nodiscard]] constexpr T native_to_little(T x) noexcept {
    if constexpr (std::endian::native == std::endian::little) return x;
    else return byteswap(x);
}

} // namespace zeta

#endif // ZETA_BITS_BIT_OPS_H
