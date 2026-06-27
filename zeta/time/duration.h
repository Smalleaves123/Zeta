#ifndef ZETA_TIME_DURATION_H
#define ZETA_TIME_DURATION_H

/// @file   time/duration.h
/// @brief  zeta::Duration — a signed 64-bit nanosecond time interval.
///
/// `zeta::Duration` represents a time span with nanosecond resolution.
/// It is stored as a single `int64_t` and is both arithmetic-safe and
/// constexpr-friendly.  Conversions to/from `std::chrono::duration` are
/// provided for seamless interop.
///
/// Factory functions follow Abseil convention:
///   auto d = zeta::Seconds(3) + zeta::Milliseconds(500);
///
/// Example:
///   zeta::Duration timeout = zeta::Seconds(5);
///   auto ns = timeout.ToNanoseconds();          // 5'000'000'000
///   auto ms = zeta::DurationToChronoMilliseconds(timeout);
///
/// @warning  Saturates at `int64_t` bounds (~292 years) — no overflow.

#include <cassert>
#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// Duration
// ═══════════════════════════════════════════════════════════════════════

class Duration {
    int64_t ns_;

    // Private constructor from raw nanoseconds.
    explicit constexpr Duration(int64_t ns, int) noexcept : ns_(ns) {}

public:
    /// Zero duration.
    constexpr Duration() noexcept : ns_(0) {}

    // ── Named constructors (Abseil-compatible) ──────────────────────

    [[nodiscard]] static constexpr Duration Nanoseconds(int64_t n) noexcept {
        return Duration(n, 0);
    }
    [[nodiscard]] static constexpr Duration Microseconds(int64_t n) noexcept {
        return multiply_check(n, 1'000LL);
    }
    [[nodiscard]] static constexpr Duration Milliseconds(int64_t n) noexcept {
        return multiply_check(n, 1'000'000LL);
    }
    [[nodiscard]] static constexpr Duration Seconds(int64_t n) noexcept {
        return multiply_check(n, 1'000'000'000LL);
    }
    [[nodiscard]] static constexpr Duration Minutes(int64_t n) noexcept {
        return multiply_check(n, 60'000'000'000LL);
    }
    [[nodiscard]] static constexpr Duration Hours(int64_t n) noexcept {
        return multiply_check(n, 3'600'000'000'000LL);
    }

    /// Saturating infinite durations.
    [[nodiscard]] static constexpr Duration Infinite() noexcept {
        return Duration(std::numeric_limits<int64_t>::max(), 0);
    }
    [[nodiscard]] static constexpr Duration NegativeInfinite() noexcept {
        return Duration(std::numeric_limits<int64_t>::min(), 0);
    }

    // ── Chrono interop ─────────────────────────────────────────────

    template <typename Rep, typename Period>
    [[nodiscard]] static constexpr Duration FromChrono(
        std::chrono::duration<Rep, Period> d) noexcept {
        return Duration(
            static_cast<int64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(d).count()),
            0);
    }

    template <typename ToDuration = std::chrono::nanoseconds>
    [[nodiscard]] constexpr auto ToChrono() const noexcept {
        return std::chrono::duration_cast<ToDuration>(
            std::chrono::nanoseconds(ns_));
    }

    // ── Accessors ──────────────────────────────────────────────────

    [[nodiscard]] constexpr int64_t ToNanoseconds()  const noexcept { return ns_; }
    [[nodiscard]] constexpr int64_t ToMicroseconds() const noexcept { return ns_ / 1'000LL; }
    [[nodiscard]] constexpr int64_t ToMilliseconds() const noexcept { return ns_ / 1'000'000LL; }
    [[nodiscard]] constexpr int64_t ToSeconds()      const noexcept { return ns_ / 1'000'000'000LL; }
    [[nodiscard]] constexpr int64_t ToMinutes()      const noexcept { return ns_ / 60'000'000'000LL; }
    [[nodiscard]] constexpr int64_t ToHours()        const noexcept { return ns_ / 3'600'000'000'000LL; }

    /// Fractional seconds as a `double` (may lose precision for large values).
    [[nodiscard]] constexpr double ToDoubleSeconds() const noexcept {
        return static_cast<double>(ns_) / 1'000'000'000.0;
    }
    [[nodiscard]] constexpr double ToDoubleMilliseconds() const noexcept {
        return static_cast<double>(ns_) / 1'000'000.0;
    }

    /// True iff this is exactly zero.
    [[nodiscard]] constexpr bool IsZero() const noexcept { return ns_ == 0; }

    // ── Raw representation ─────────────────────────────────────────

    /// Raw nanosecond count (for serialization).
    [[nodiscard]] static constexpr Duration FromRaw(int64_t ns) noexcept {
        return Duration(ns, 0);
    }
    [[nodiscard]] constexpr int64_t ToRaw() const noexcept { return ns_; }

    // ── Unary operators ────────────────────────────────────────────

    [[nodiscard]] constexpr Duration operator+() const noexcept { return *this; }
    [[nodiscard]] constexpr Duration operator-() const noexcept {
        // Saturate negation of MIN.
        if (ns_ == std::numeric_limits<int64_t>::min())
            return Infinite();
        return Duration(-ns_, 0);
    }

    // ── Compound assignment ────────────────────────────────────────

    constexpr Duration& operator+=(Duration rhs) noexcept {
        *this = add_sat(ns_, rhs.ns_);
        return *this;
    }
    constexpr Duration& operator-=(Duration rhs) noexcept {
        // -INT64_MIN = INT64_MAX + 1 — add one extra to avoid off-by-one.
        if (rhs.ns_ == std::numeric_limits<int64_t>::min()) {
            *this = add_sat(ns_, std::numeric_limits<int64_t>::max());
            if (ns_ < std::numeric_limits<int64_t>::max()) ++ns_;
        } else {
            *this = add_sat(ns_, -rhs.ns_);
        }
        return *this;
    }
    constexpr Duration& operator*=(int64_t factor) noexcept {
        *this = multiply_check(ns_, factor);
        return *this;
    }
    constexpr Duration& operator/=(int64_t divisor) noexcept {
        assert(divisor != 0);
        if (divisor == 0) return *this;
        ns_ /= divisor;
        return *this;
    }

    // ── Arithmetic ─────────────────────────────────────────────────

    [[nodiscard]] friend constexpr Duration operator+(Duration a, Duration b) noexcept {
        return a += b;
    }
    [[nodiscard]] friend constexpr Duration operator-(Duration a, Duration b) noexcept {
        return a -= b;
    }
    [[nodiscard]] friend constexpr Duration operator*(Duration a, int64_t factor) noexcept {
        return a *= factor;
    }
    [[nodiscard]] friend constexpr Duration operator*(int64_t factor, Duration a) noexcept {
        return a * factor;
    }
    [[nodiscard]] friend constexpr Duration operator/(Duration a, int64_t divisor) noexcept {
        return a /= divisor;
    }
    /// Ratio of two durations (truncates toward zero).
    [[nodiscard]] friend constexpr int64_t operator/(Duration a, Duration b) noexcept {
        assert(b.ns_ != 0);
        if (b.ns_ == 0) return 0;
        return a.ns_ / b.ns_;
    }
    /// Remainder of two durations.
    [[nodiscard]] friend constexpr Duration operator%(Duration a, Duration b) noexcept {
        assert(b.ns_ != 0);
        if (b.ns_ == 0) return Duration();
        return Duration(a.ns_ % b.ns_, 0);
    }

    // ── Comparison ─────────────────────────────────────────────────

    [[nodiscard]] friend constexpr bool operator==(Duration a, Duration b) noexcept {
        return a.ns_ == b.ns_;
    }
    [[nodiscard]] friend constexpr bool operator!=(Duration a, Duration b) noexcept {
        return a.ns_ != b.ns_;
    }
    [[nodiscard]] friend constexpr bool operator<(Duration a, Duration b) noexcept {
        return a.ns_ < b.ns_;
    }
    [[nodiscard]] friend constexpr bool operator<=(Duration a, Duration b) noexcept {
        return a.ns_ <= b.ns_;
    }
    [[nodiscard]] friend constexpr bool operator>(Duration a, Duration b) noexcept {
        return a.ns_ > b.ns_;
    }
    [[nodiscard]] friend constexpr bool operator>=(Duration a, Duration b) noexcept {
        return a.ns_ >= b.ns_;
    }

    // ── Arithmetic free functions ──────────────────────────────────

    /// Absolute value.
    [[nodiscard]] friend constexpr Duration Abs(Duration d) noexcept {
        if (d.ns_ >= 0) return d;
        return -d;
    }

    /// Minimum of two durations.
    [[nodiscard]] friend constexpr Duration Min(Duration a, Duration b) noexcept {
        return a < b ? a : b;
    }

    /// Maximum of two durations.
    [[nodiscard]] friend constexpr Duration Max(Duration a, Duration b) noexcept {
        return a > b ? a : b;
    }

private:
    // Saturating signed multiply by a scale factor.
    // If scale is negative, negate both arguments so the overflow check
    // always operates with a positive scale (the product is the same).
    [[nodiscard]] static constexpr Duration multiply_check(int64_t v,
                                                            int64_t scale) noexcept {
        constexpr auto kMax = std::numeric_limits<int64_t>::max();
        constexpr auto kMin = std::numeric_limits<int64_t>::min();
        if (v == 0 || scale == 0) return Duration(0, 0);
        // Normalise sign: keep scale positive, flip v if needed.
        if (scale < 0) {
            // Guard -INT64_MIN overflow in v = -v.
            if (v == kMin) {
                // v = kMin, scale < 0 → product = (-kMin) * (-scale) > 0.
                // Since |v| ≥ 2^63 and |scale| ≥ 1, product always
                // exceeds INT64_MAX, saturating to Infinity.
                return Infinite();
            }
            v = -v;
            scale = -scale;
        }
        if (scale == 1) return Duration(v, 0);
        if (v > 0) {
            if (v > kMax / scale) return Infinite();
        } else if (v < 0) {
            if (v < kMin / scale) return NegativeInfinite();
        }
        return Duration(v * scale, 0);
    }

    // Saturating signed addition.
    [[nodiscard]] static constexpr Duration add_sat(int64_t a, int64_t b) noexcept {
        constexpr auto kMax = std::numeric_limits<int64_t>::max();
        constexpr auto kMin = std::numeric_limits<int64_t>::min();
        int64_t result;
        if (b >= 0) {
            if (a > kMax - b) result = kMax;
            else              result = a + b;
        } else {
            if (a < kMin - b) result = kMin;
            else              result = a + b;
        }
        return Duration(result, 0);
    }
};

// ── Chrono conversion convenience functions ─────────────────────────

template <typename Rep, typename Period>
[[nodiscard]] constexpr Duration ChronoDurationToZeta(
    std::chrono::duration<Rep, Period> d) noexcept {
    return Duration::FromChrono(d);
}

template <typename ToDuration = std::chrono::nanoseconds>
[[nodiscard]] constexpr auto DurationToChrono(Duration d) noexcept {
    return d.ToChrono<ToDuration>();
}

// ── Literal operator (in namespace zeta::literals) ───────────────────

inline namespace literals {
inline namespace duration_literals {

[[nodiscard]] constexpr Duration operator""_ns(
    unsigned long long n) noexcept {
    return Duration::Nanoseconds(static_cast<int64_t>(n));
}
[[nodiscard]] constexpr Duration operator""_us(
    unsigned long long n) noexcept {
    return Duration::Microseconds(static_cast<int64_t>(n));
}
[[nodiscard]] constexpr Duration operator""_ms(
    unsigned long long n) noexcept {
    return Duration::Milliseconds(static_cast<int64_t>(n));
}
[[nodiscard]] constexpr Duration operator""_s(
    unsigned long long n) noexcept {
    return Duration::Seconds(static_cast<int64_t>(n));
}
[[nodiscard]] constexpr Duration operator""_min(
    unsigned long long n) noexcept {
    return Duration::Minutes(static_cast<int64_t>(n));
}
[[nodiscard]] constexpr Duration operator""_h(
    unsigned long long n) noexcept {
    return Duration::Hours(static_cast<int64_t>(n));
}

} // namespace duration_literals
} // namespace literals

} // namespace zeta

#endif // ZETA_TIME_DURATION_H
