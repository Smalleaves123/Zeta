#ifndef ZETA_NUMERIC_INT128_H
#define ZETA_NUMERIC_INT128_H

/// @file   numeric/int128.h
/// @brief  Portable 128-bit integer types: `Int128` and `Uint128`.
///
/// 128-bit integers are useful for fixed-width arithmetic, hash mixing,
/// and precise financial calculations.  This implementation uses compiler
/// builtins (`__int128` / `__uint128_t`) when available (GCC, Clang) and
/// falls back to a two-word (64+64) representation on MSVC and other
/// compilers.
///
/// Example:
///   zeta::Int128 big = zeta::Int128::FromPair(1ULL << 60, 0);
///   big *= 1024;
///   std::string s = big.ToString();
///
/// @warning  `int128.h` is a pure arithmetic type.  For hashing, use
///           `zeta/hash/hash.h`.

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>

// ── Platform detection ─────────────────────────────────────────────────
#if defined(__SIZEOF_INT128__) || (defined(__GNUC__) && defined(__LP64__))
  #define ZETA_HAS_BUILTIN_INT128 1
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define ZETA_PRAGMA(x) _Pragma(#x)
#  define ZETA_DIAGNOSTIC_PUSH ZETA_PRAGMA(GCC diagnostic push)
#  define ZETA_DIAGNOSTIC_POP ZETA_PRAGMA(GCC diagnostic pop)
#  define ZETA_DIAGNOSTIC_IGNORE_PEDANTIC ZETA_PRAGMA(GCC diagnostic ignored "-Wpedantic")
#else
#  define ZETA_DIAGNOSTIC_PUSH
#  define ZETA_DIAGNOSTIC_POP
#  define ZETA_DIAGNOSTIC_IGNORE_PEDANTIC
#endif

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Uint128 (unsigned)
// ═══════════════════════════════════════════════════════════════════════

#if ZETA_HAS_BUILTIN_INT128

ZETA_DIAGNOSTIC_PUSH
ZETA_DIAGNOSTIC_IGNORE_PEDANTIC

class Uint128 {
    __uint128_t val_;

    friend class Int128;

public:
    constexpr Uint128() noexcept : val_(0) {}
    constexpr Uint128(uint64_t lo) noexcept : val_(lo) {}  // NOLINT

    constexpr Uint128& operator=(uint64_t v) noexcept { val_ = v; return *this; }

    [[nodiscard]] static constexpr Uint128 FromPair(uint64_t hi, uint64_t lo) noexcept {
        Uint128 r;
        r.val_ = (static_cast<__uint128_t>(hi) << 64) | lo;
        return r;
    }

    /// Raw access to the underlying `__uint128_t` (for interop).
    [[nodiscard]] constexpr __uint128_t Raw() const noexcept { return val_; }

    [[nodiscard]] constexpr uint64_t Low64()  const noexcept { return static_cast<uint64_t>(val_); }
    [[nodiscard]] constexpr uint64_t High64() const noexcept { return static_cast<uint64_t>(val_ >> 64); }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return val_ != 0; }
    [[nodiscard]] constexpr explicit operator uint64_t() const noexcept { return Low64(); }

    // ── Arithmetic ──────────────────────────────────────────────────

    [[nodiscard]] friend constexpr Uint128 operator+(Uint128 a, Uint128 b) noexcept { Uint128 r; r.val_ = a.val_ + b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator-(Uint128 a, Uint128 b) noexcept { Uint128 r; r.val_ = a.val_ - b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator*(Uint128 a, Uint128 b) noexcept { Uint128 r; r.val_ = a.val_ * b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator/(Uint128 a, Uint128 b) noexcept {
        assert(b.val_ != 0);
        Uint128 r;
        r.val_ = a.val_ / b.val_;
        return r;
    }
    [[nodiscard]] friend constexpr Uint128 operator%(Uint128 a, Uint128 b) noexcept {
        assert(b.val_ != 0);
        Uint128 r;
        r.val_ = a.val_ % b.val_;
        return r;
    }
    [[nodiscard]] friend constexpr Uint128 operator<<(Uint128 a, int bits) noexcept {
        if (bits >= 128 || bits < 0) return Uint128();
        Uint128 r;
        r.val_ = a.val_ << bits;
        return r;
    }
    [[nodiscard]] friend constexpr Uint128 operator>>(Uint128 a, int bits) noexcept {
        if (bits >= 128 || bits < 0) return Uint128();
        Uint128 r;
        r.val_ = a.val_ >> bits;
        return r;
    }
    [[nodiscard]] friend constexpr Uint128 operator&(Uint128 a, Uint128 b) noexcept  { Uint128 r; r.val_ = a.val_ & b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator|(Uint128 a, Uint128 b) noexcept  { Uint128 r; r.val_ = a.val_ | b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator^(Uint128 a, Uint128 b) noexcept  { Uint128 r; r.val_ = a.val_ ^ b.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator~(Uint128 a) noexcept             { Uint128 r; r.val_ = ~a.val_; return r; }
    [[nodiscard]] friend constexpr Uint128 operator-(Uint128 a) noexcept             { Uint128 r; r.val_ = -a.val_; return r; }

    constexpr Uint128& operator+=(Uint128 o) noexcept { val_ += o.val_; return *this; }
    constexpr Uint128& operator-=(Uint128 o) noexcept { val_ -= o.val_; return *this; }
    constexpr Uint128& operator*=(Uint128 o) noexcept { val_ *= o.val_; return *this; }
    constexpr Uint128& operator/=(Uint128 o) noexcept { assert(o.val_); val_ /= o.val_; return *this; }
    constexpr Uint128& operator%=(Uint128 o) noexcept { assert(o.val_); val_ %= o.val_; return *this; }
    constexpr Uint128& operator<<=(int bits) noexcept { if (bits >= 128 || bits < 0) val_ = 0; else val_ <<= bits; return *this; }
    constexpr Uint128& operator>>=(int bits) noexcept { if (bits >= 128 || bits < 0) val_ = 0; else val_ >>= bits; return *this; }
    constexpr Uint128& operator&=(Uint128 o) noexcept { val_ &= o.val_; return *this; }
    constexpr Uint128& operator|=(Uint128 o) noexcept { val_ |= o.val_; return *this; }
    constexpr Uint128& operator^=(Uint128 o) noexcept { val_ ^= o.val_; return *this; }
    constexpr Uint128& operator++() noexcept { ++val_; return *this; }
    constexpr Uint128  operator++(int) noexcept { Uint128 t = *this; ++val_; return t; }
    constexpr Uint128& operator--() noexcept { --val_; return *this; }
    constexpr Uint128  operator--(int) noexcept { Uint128 t = *this; --val_; return t; }

    // ── Comparison ──────────────────────────────────────────────────

    [[nodiscard]] friend constexpr bool operator==(Uint128 a, Uint128 b) noexcept { return a.val_ == b.val_; }
    [[nodiscard]] friend constexpr bool operator!=(Uint128 a, Uint128 b) noexcept { return a.val_ != b.val_; }
    [[nodiscard]] friend constexpr bool operator<(Uint128 a, Uint128 b) noexcept  { return a.val_ < b.val_; }
    [[nodiscard]] friend constexpr bool operator<=(Uint128 a, Uint128 b) noexcept { return a.val_ <= b.val_; }
    [[nodiscard]] friend constexpr bool operator>(Uint128 a, Uint128 b) noexcept  { return a.val_ > b.val_; }
    [[nodiscard]] friend constexpr bool operator>=(Uint128 a, Uint128 b) noexcept { return a.val_ >= b.val_; }
};

#else  // !ZETA_HAS_BUILTIN_INT128 — two-word fallback

class Uint128 {
    uint64_t lo_;
    uint64_t hi_;

    static constexpr Uint128 from_raw(uint64_t hi, uint64_t lo) noexcept {
        Uint128 r; r.lo_ = lo; r.hi_ = hi; return r;
    }

public:
    constexpr Uint128() noexcept : lo_(0), hi_(0) {}
    constexpr Uint128(uint64_t lo) noexcept : lo_(lo), hi_(0) {} // NOLINT

    constexpr Uint128& operator=(uint64_t v) noexcept { lo_ = v; hi_ = 0; return *this; }

    [[nodiscard]] static constexpr Uint128 FromPair(uint64_t hi, uint64_t lo) noexcept {
        return from_raw(hi, lo);
    }

    [[nodiscard]] constexpr uint64_t Low64()  const noexcept { return lo_; }
    [[nodiscard]] constexpr uint64_t High64() const noexcept { return hi_; }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return lo_ || hi_; }
    [[nodiscard]] constexpr explicit operator uint64_t() const noexcept { return lo_; }

    // ── Arithmetic ──────────────────────────────────────────────────

    [[nodiscard]] friend constexpr Uint128 operator+(Uint128 a, Uint128 b) noexcept {
        uint64_t lo = a.lo_ + b.lo_;
        uint64_t hi = a.hi_ + b.hi_ + (lo < a.lo_ ? 1U : 0U);
        return from_raw(hi, lo);
    }
    [[nodiscard]] friend constexpr Uint128 operator-(Uint128 a, Uint128 b) noexcept {
        uint64_t lo = a.lo_ - b.lo_;
        uint64_t hi = a.hi_ - b.hi_ - (lo > a.lo_ ? 1U : 0U);
        return from_raw(hi, lo);
    }
    [[nodiscard]] friend constexpr Uint128 operator*(Uint128 a, Uint128 b) noexcept {
        // Split into 32-bit halves for portable 64x64→128 multiply.
        uint64_t a_lo = static_cast<uint32_t>(a.lo_);
        uint64_t a_hi = a.lo_ >> 32;
        uint64_t b_lo = static_cast<uint32_t>(b.lo_);
        uint64_t b_hi = b.lo_ >> 32;
        uint64_t p0 = a_lo * b_lo;
        uint64_t p1 = a_lo * b_hi;
        uint64_t p2 = a_hi * b_lo;
        uint64_t p3 = a_hi * b_hi;
        uint64_t mid = (p0 >> 32) + static_cast<uint32_t>(p1) + static_cast<uint32_t>(p2);
        uint64_t lo  = (mid << 32) | static_cast<uint32_t>(p0);
        uint64_t hi  = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);
        hi += a.lo_ * b.hi_ + a.hi_ * b.lo_;
        return from_raw(hi, lo);
    }
    // Division and modulus (runtime-only, not constexpr)
    [[nodiscard]] friend Uint128 operator/(Uint128 a, Uint128 b) noexcept {
        assert(b); return divide(a, b).first;
    }
    [[nodiscard]] friend Uint128 operator%(Uint128 a, Uint128 b) noexcept {
        assert(b); return divide(a, b).second;
    }
    [[nodiscard]] friend constexpr Uint128 operator<<(Uint128 a, int bits) noexcept {
        if (bits >= 128) return Uint128();
        if (bits >= 64) return from_raw(a.lo_ << (bits - 64), 0);
        if (bits == 0)  return a;
        return from_raw((a.hi_ << bits) | (a.lo_ >> (64 - bits)), a.lo_ << bits);
    }
    [[nodiscard]] friend constexpr Uint128 operator>>(Uint128 a, int bits) noexcept {
        if (bits >= 128) return Uint128();
        if (bits >= 64)  return from_raw(0, a.hi_ >> (bits - 64));
        if (bits == 0)   return a;
        return from_raw(a.hi_ >> bits, (a.lo_ >> bits) | (a.hi_ << (64 - bits)));
    }
    [[nodiscard]] friend constexpr Uint128 operator&(Uint128 a, Uint128 b) noexcept { return from_raw(a.hi_ & b.hi_, a.lo_ & b.lo_); }
    [[nodiscard]] friend constexpr Uint128 operator|(Uint128 a, Uint128 b) noexcept { return from_raw(a.hi_ | b.hi_, a.lo_ | b.lo_); }
    [[nodiscard]] friend constexpr Uint128 operator^(Uint128 a, Uint128 b) noexcept { return from_raw(a.hi_ ^ b.hi_, a.lo_ ^ b.lo_); }
    [[nodiscard]] friend constexpr Uint128 operator~(Uint128 a) noexcept             { return from_raw(~a.hi_, ~a.lo_); }
    [[nodiscard]] friend constexpr Uint128 operator-(Uint128 a) noexcept             { return ~a + Uint128(1); }

    constexpr Uint128& operator+=(Uint128 o) noexcept { *this = *this + o; return *this; }
    constexpr Uint128& operator-=(Uint128 o) noexcept { *this = *this - o; return *this; }
    Uint128& operator*=(Uint128 o) noexcept { *this = *this * o; return *this; }
    Uint128& operator/=(Uint128 o) noexcept { *this = *this / o; return *this; }
    Uint128& operator%=(Uint128 o) noexcept { *this = *this % o; return *this; }
    constexpr Uint128& operator<<=(int bits) noexcept { *this = *this << bits; return *this; }
    constexpr Uint128& operator>>=(int bits) noexcept { *this = *this >> bits; return *this; }
    constexpr Uint128& operator&=(Uint128 o) noexcept { *this = *this & o; return *this; }
    constexpr Uint128& operator|=(Uint128 o) noexcept { *this = *this | o; return *this; }
    constexpr Uint128& operator^=(Uint128 o) noexcept { *this = *this ^ o; return *this; }
    constexpr Uint128& operator++() noexcept { *this = *this + Uint128(1); return *this; }
    constexpr Uint128  operator++(int) noexcept { Uint128 t = *this; ++(*this); return t; }
    constexpr Uint128& operator--() noexcept { *this = *this - Uint128(1); return *this; }
    constexpr Uint128  operator--(int) noexcept { Uint128 t = *this; --(*this); return t; }

    // ── Comparison ──────────────────────────────────────────────────

    [[nodiscard]] friend constexpr bool operator==(Uint128 a, Uint128 b) noexcept { return a.hi_ == b.hi_ && a.lo_ == b.lo_; }
    [[nodiscard]] friend constexpr bool operator!=(Uint128 a, Uint128 b) noexcept { return !(a == b); }
    [[nodiscard]] friend constexpr bool operator<(Uint128 a, Uint128 b) noexcept  { return a.hi_ < b.hi_ || (a.hi_ == b.hi_ && a.lo_ < b.lo_); }
    [[nodiscard]] friend constexpr bool operator<=(Uint128 a, Uint128 b) noexcept { return !(b < a); }
    [[nodiscard]] friend constexpr bool operator>(Uint128 a, Uint128 b) noexcept  { return b < a; }
    [[nodiscard]] friend constexpr bool operator>=(Uint128 a, Uint128 b) noexcept { return !(a < b); }

private:
    static std::pair<Uint128, Uint128> divide(Uint128 a, Uint128 b) noexcept;
};

#endif

// ═══════════════════════════════════════════════════════════════════════
// Int128 (signed)
// ═══════════════════════════════════════════════════════════════════════

#if ZETA_HAS_BUILTIN_INT128

class Int128 {
    __int128 val_;

public:
    constexpr Int128() noexcept : val_(0) {}
    constexpr Int128(int64_t lo) noexcept : val_(lo) {} // NOLINT
    constexpr Int128(Uint128 u) noexcept : val_(static_cast<__int128>(u.Raw())) {} // NOLINT

    [[nodiscard]] static constexpr Int128 FromPair(int64_t hi, uint64_t lo) noexcept {
        Int128 r;
        // Use unsigned shift to avoid UB on negative hi (C++ prohibits
        // left-shifting signed negative values).
        __uint128_t u = (static_cast<__uint128_t>(static_cast<uint64_t>(hi)) << 64) | lo;
        r.val_ = static_cast<__int128>(u);
        return r;
    }

    [[nodiscard]] constexpr int64_t  Low64()  const noexcept { return static_cast<int64_t>(static_cast<uint64_t>(static_cast<__uint128_t>(val_))); }
    [[nodiscard]] constexpr int64_t  High64() const noexcept { return static_cast<int64_t>(static_cast<__uint128_t>(val_) >> 64); }
    [[nodiscard]] constexpr Uint128 Abs() const noexcept {
        return val_ >= 0 ? Uint128(static_cast<__uint128_t>(val_)) : Uint128(static_cast<__uint128_t>(-val_));
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return val_ != 0; }
    [[nodiscard]] constexpr explicit operator int64_t() const noexcept { return Low64(); }
    [[nodiscard]] constexpr bool IsNegative() const noexcept { return val_ < 0; }

    // Arithmetic
    [[nodiscard]] friend constexpr Int128 operator+(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ + b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator-(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ - b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator*(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ * b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator/(Int128 a, Int128 b) noexcept { assert(b); Int128 r; r.val_ = a.val_ / b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator%(Int128 a, Int128 b) noexcept { assert(b); Int128 r; r.val_ = a.val_ % b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator-(Int128 a) noexcept { Int128 r; r.val_ = -a.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator<<(Int128 a, int bits) noexcept {
        if (bits >= 128 || bits < 0) return Int128(0);
        Int128 r;
        r.val_ = a.val_ << bits;
        return r;
    }
    [[nodiscard]] friend constexpr Int128 operator>>(Int128 a, int bits) noexcept {
        if (bits >= 128 || bits < 0) return Int128(a.val_ < 0 ? -1 : 0);
        Int128 r;
        r.val_ = a.val_ >> bits;
        return r;
    }
    [[nodiscard]] friend constexpr Int128 operator&(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ & b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator|(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ | b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator^(Int128 a, Int128 b) noexcept { Int128 r; r.val_ = a.val_ ^ b.val_; return r; }
    [[nodiscard]] friend constexpr Int128 operator~(Int128 a) noexcept { Int128 r; r.val_ = ~a.val_; return r; }

    constexpr Int128& operator+=(Int128 o) noexcept { val_ += o.val_; return *this; }
    constexpr Int128& operator-=(Int128 o) noexcept { val_ -= o.val_; return *this; }
    constexpr Int128& operator*=(Int128 o) noexcept { val_ *= o.val_; return *this; }
    constexpr Int128& operator/=(Int128 o) noexcept { assert(o); val_ /= o.val_; return *this; }
    constexpr Int128& operator%=(Int128 o) noexcept { assert(o); val_ %= o.val_; return *this; }
    constexpr Int128& operator<<=(int bits) noexcept { *this = *this << bits; return *this; }
    constexpr Int128& operator>>=(int bits) noexcept { *this = *this >> bits; return *this; }
    constexpr Int128& operator&=(Int128 o) noexcept { val_ &= o.val_; return *this; }
    constexpr Int128& operator|=(Int128 o) noexcept { val_ |= o.val_; return *this; }
    constexpr Int128& operator^=(Int128 o) noexcept { val_ ^= o.val_; return *this; }
    constexpr Int128& operator++() noexcept { ++val_; return *this; }
    constexpr Int128  operator++(int) noexcept { Int128 t = *this; ++val_; return t; }
    constexpr Int128& operator--() noexcept { --val_; return *this; }
    constexpr Int128  operator--(int) noexcept { Int128 t = *this; --val_; return t; }

    // Comparison
    [[nodiscard]] friend constexpr bool operator==(Int128 a, Int128 b) noexcept { return a.val_ == b.val_; }
    [[nodiscard]] friend constexpr bool operator!=(Int128 a, Int128 b) noexcept { return a.val_ != b.val_; }
    [[nodiscard]] friend constexpr bool operator<(Int128 a, Int128 b) noexcept  { return a.val_ < b.val_; }
    [[nodiscard]] friend constexpr bool operator<=(Int128 a, Int128 b) noexcept { return a.val_ <= b.val_; }
    [[nodiscard]] friend constexpr bool operator>(Int128 a, Int128 b) noexcept  { return a.val_ > b.val_; }
    [[nodiscard]] friend constexpr bool operator>=(Int128 a, Int128 b) noexcept { return a.val_ >= b.val_; }
};

#else

class Int128 {
    Uint128 u_;

    static constexpr Uint128 ToUnsigned(int64_t hi, uint64_t lo) noexcept {
        return Uint128::FromPair(static_cast<uint64_t>(hi), lo);
    }

public:
    constexpr Int128() noexcept : u_(0) {}
    constexpr Int128(int64_t lo) noexcept : u_(ToUnsigned(lo < 0 ? -1 : 0, static_cast<uint64_t>(lo))) {} // NOLINT
    explicit constexpr Int128(Uint128 u) noexcept : u_(u) {}

    [[nodiscard]] static constexpr Int128 FromPair(int64_t hi, uint64_t lo) noexcept {
        return Int128(ToUnsigned(hi, lo));
    }

    [[nodiscard]] constexpr int64_t  Low64()  const noexcept { return static_cast<int64_t>(u_.Low64()); }
    [[nodiscard]] constexpr int64_t  High64() const noexcept { return static_cast<int64_t>(u_.High64()); }
    [[nodiscard]] constexpr Uint128 Abs() const noexcept {
        return IsNegative() ? -u_ : u_;
    }
    [[nodiscard]] constexpr Uint128 ToUint128() const noexcept { return u_; }

    [[nodiscard]] constexpr explicit operator bool() const noexcept { return static_cast<bool>(u_); }
    [[nodiscard]] constexpr explicit operator int64_t() const noexcept { return Low64(); }

    [[nodiscard]] constexpr bool IsNegative() const noexcept {
        return static_cast<int64_t>(u_.High64()) < 0;
    }

    // Arithmetic (delegate to Uint128)
    [[nodiscard]] friend constexpr Int128 operator+(Int128 a, Int128 b) noexcept { return Int128(a.u_ + b.u_); }
    [[nodiscard]] friend constexpr Int128 operator-(Int128 a, Int128 b) noexcept { return Int128(a.u_ - b.u_); }
    [[nodiscard]] friend constexpr Int128 operator*(Int128 a, Int128 b) noexcept {
        bool neg = a.IsNegative() ^ b.IsNegative();
        Uint128 ua = a.Abs(), ub = b.Abs();
        return neg ? Int128(- (ua * ub)) : Int128(ua * ub);
    }
    [[nodiscard]] friend Int128 operator/(Int128 a, Int128 b) noexcept {
        assert(b); bool neg = a.IsNegative() ^ b.IsNegative();
        Uint128 ua = a.Abs(), ub = b.Abs();
        return neg ? Int128(- (ua / ub)) : Int128(ua / ub);
    }
    [[nodiscard]] friend Int128 operator%(Int128 a, Int128 b) noexcept {
        assert(b);
        Uint128 ua = a.Abs(), ub = b.Abs();
        Uint128 rem = ua % ub;
        return a.IsNegative() ? Int128(-rem) : Int128(rem);
    }
    [[nodiscard]] friend constexpr Int128 operator-(Int128 a) noexcept { return Int128(-a.u_); }
    [[nodiscard]] friend constexpr Int128 operator<<(Int128 a, int bits) noexcept { return Int128(a.u_ << bits); }
    [[nodiscard]] friend constexpr Int128 operator>>(Int128 a, int bits) noexcept {
        // Arithmetic right shift
        uint64_t hi = a.u_.High64();
        if (bits >= 128) return a.IsNegative() ? Int128(-1) : Int128(0);
        if (bits >= 64) return Int128(Uint128::FromPair(
            a.IsNegative() ? ~0ULL : 0, static_cast<int64_t>(hi) >> (bits - 64)));
        Uint128 shifted = a.u_ >> bits;
        if (!a.IsNegative()) return Int128(shifted);
        // Sign extend
        uint64_t sign_mask = ~0ULL << (64 - bits);
        return Int128(Uint128::FromPair(
            shifted.High64() | sign_mask, shifted.Low64()));
    }
    [[nodiscard]] friend constexpr Int128 operator&(Int128 a, Int128 b) noexcept { return Int128(a.u_ & b.u_); }
    [[nodiscard]] friend constexpr Int128 operator|(Int128 a, Int128 b) noexcept { return Int128(a.u_ | b.u_); }
    [[nodiscard]] friend constexpr Int128 operator^(Int128 a, Int128 b) noexcept { return Int128(a.u_ ^ b.u_); }
    [[nodiscard]] friend constexpr Int128 operator~(Int128 a) noexcept { return Int128(~a.u_); }

    constexpr Int128& operator+=(Int128 o) noexcept { u_ += o.u_; return *this; }
    constexpr Int128& operator-=(Int128 o) noexcept { u_ -= o.u_; return *this; }
    Int128& operator*=(Int128 o) noexcept { *this = *this * o; return *this; }
    Int128& operator/=(Int128 o) noexcept { *this = *this / o; return *this; }
    Int128& operator%=(Int128 o) noexcept { *this = *this % o; return *this; }
    constexpr Int128& operator<<=(int bits) noexcept { u_ <<= bits; return *this; }
    constexpr Int128& operator>>=(int bits) noexcept { *this = *this >> bits; return *this; }
    constexpr Int128& operator&=(Int128 o) noexcept { u_ &= o.u_; return *this; }
    constexpr Int128& operator|=(Int128 o) noexcept { u_ |= o.u_; return *this; }
    constexpr Int128& operator^=(Int128 o) noexcept { u_ ^= o.u_; return *this; }
    constexpr Int128& operator++() noexcept { u_ += Uint128(1); return *this; }
    constexpr Int128  operator++(int) noexcept { Int128 t = *this; ++(*this); return t; }
    constexpr Int128& operator--() noexcept { u_ -= Uint128(1); return *this; }
    constexpr Int128  operator--(int) noexcept { Int128 t = *this; --(*this); return t; }

    // Comparison
    [[nodiscard]] friend constexpr bool operator==(Int128 a, Int128 b) noexcept { return a.u_ == b.u_; }
    [[nodiscard]] friend constexpr bool operator!=(Int128 a, Int128 b) noexcept { return !(a == b); }
    [[nodiscard]] friend constexpr bool operator<(Int128 a, Int128 b) noexcept {
        if (a.IsNegative() != b.IsNegative()) return a.IsNegative();
        return a.u_ < b.u_;
    }
    [[nodiscard]] friend constexpr bool operator<=(Int128 a, Int128 b) noexcept { return !(b < a); }
    [[nodiscard]] friend constexpr bool operator>(Int128 a, Int128 b) noexcept  { return b < a; }
    [[nodiscard]] friend constexpr bool operator>=(Int128 a, Int128 b) noexcept { return !(a < b); }
};

#endif

ZETA_DIAGNOSTIC_POP

// ── Uint128 division helper (fallback) ───────────────────────────────

#if !ZETA_HAS_BUILTIN_INT128
inline std::pair<Uint128, Uint128> Uint128::divide(Uint128 a, Uint128 b) noexcept {
    // Binary long division.
    if (b == Uint128(0)) return {Uint128(0), Uint128(0)}; // shouldn't reach (assert above)
    if (a < b) return {Uint128(0), a};
    if (a == b) return {Uint128(1), Uint128(0)};
    Uint128 q(0), r(0);
    for (int i = 127; i >= 0; --i) {
        r = r << 1;
        r |= Uint128((a >> i) & Uint128(1));
        if (r >= b) {
            r = r - b;
            q = q | (Uint128(1) << i);
        }
    }
    return {q, r};
}
#endif

} // namespace zeta

// ── std::numeric_limits specialisations ──────────────────────────────

namespace std {

template <>
class numeric_limits<zeta::Uint128> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int  digits = 128;
    static constexpr int  digits10 = 38;
    [[nodiscard]] static constexpr zeta::Uint128 min() noexcept { return 0; }
    [[nodiscard]] static constexpr zeta::Uint128 max() noexcept {
        return zeta::Uint128::FromPair(~uint64_t{0}, ~uint64_t{0});
    }
    [[nodiscard]] static constexpr zeta::Uint128 lowest() noexcept { return min(); }
};

template <>
class numeric_limits<zeta::Int128> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int  digits = 127;
    static constexpr int  digits10 = 38;
    [[nodiscard]] static constexpr zeta::Int128 min() noexcept {
        return zeta::Int128::FromPair(int64_t(0x8000000000000000ULL), 0);
    }
    [[nodiscard]] static constexpr zeta::Int128 max() noexcept {
        return zeta::Int128::FromPair(int64_t(0x7FFFFFFFFFFFFFFFULL), ~uint64_t{0});
    }
    [[nodiscard]] static constexpr zeta::Int128 lowest() noexcept { return min(); }
};

} // namespace std

#endif // ZETA_NUMERIC_INT128_H
