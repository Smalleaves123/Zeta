#include "zeta/time/duration.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <limits>

using namespace zeta;
using namespace zeta::duration_literals;

// ═══════════════════════════════════════════════════════════════════════
// Construction / factories
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: default construction is zero", "[duration][ctor]") {
    constexpr Duration d;
    STATIC_REQUIRE(d.IsZero());
    STATIC_REQUIRE(d.ToNanoseconds() == 0);
}

TEST_CASE("duration: factory functions", "[duration][ctor]") {
    STATIC_REQUIRE(Duration::Nanoseconds(1).ToNanoseconds() == 1);
    STATIC_REQUIRE(Duration::Microseconds(1).ToNanoseconds() == 1'000LL);
    STATIC_REQUIRE(Duration::Milliseconds(1).ToNanoseconds() == 1'000'000LL);
    STATIC_REQUIRE(Duration::Seconds(1).ToNanoseconds()      == 1'000'000'000LL);
    STATIC_REQUIRE(Duration::Minutes(1).ToNanoseconds()      == 60'000'000'000LL);
    STATIC_REQUIRE(Duration::Hours(1).ToNanoseconds()        == 3'600'000'000'000LL);
}

TEST_CASE("duration: literal operators", "[duration][ctor]") {
    STATIC_REQUIRE((1_ns).ToNanoseconds()  == 1LL);
    STATIC_REQUIRE((1_us).ToNanoseconds()  == 1'000LL);
    STATIC_REQUIRE((1_ms).ToNanoseconds()  == 1'000'000LL);
    STATIC_REQUIRE((1_s).ToNanoseconds()   == 1'000'000'000LL);
    STATIC_REQUIRE((1_min).ToNanoseconds() == 60'000'000'000LL);
    STATIC_REQUIRE((1_h).ToNanoseconds()   == 3'600'000'000'000LL);
}

// ═══════════════════════════════════════════════════════════════════════
// Accessors
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: To* accessors round-trip", "[duration][access]") {
    Duration d = Duration::Seconds(1);
    REQUIRE(d.ToNanoseconds()  == 1'000'000'000LL);
    REQUIRE(d.ToMicroseconds() == 1'000'000LL);
    REQUIRE(d.ToMilliseconds() == 1'000LL);
    REQUIRE(d.ToSeconds()      == 1LL);
}

TEST_CASE("duration: ToDouble* accessors", "[duration][access]") {
    Duration d = Duration::Milliseconds(1'500);
    REQUIRE(d.ToDoubleSeconds() == 1.5);
    REQUIRE(d.ToDoubleMilliseconds() == 1'500.0);
}

TEST_CASE("duration: FromRaw / ToRaw round-trip", "[duration][access]") {
    constexpr Duration d = Duration::FromRaw(123456789LL);
    STATIC_REQUIRE(d.ToRaw() == 123456789LL);
}

// ═══════════════════════════════════════════════════════════════════════
// Arithmetic
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: addition", "[duration][arith]") {
    REQUIRE((Duration::Seconds(2) + Duration::Milliseconds(500)).ToMilliseconds() == 2'500LL);
    Duration d = Duration::Seconds(1);
    d += Duration::Milliseconds(300);
    REQUIRE(d.ToMilliseconds() == 1'300LL);
}

TEST_CASE("duration: subtraction", "[duration][arith]") {
    REQUIRE((Duration::Seconds(3) - Duration::Seconds(1)).ToSeconds() == 2LL);
    Duration d = Duration::Seconds(5);
    d -= Duration::Milliseconds(200);
    REQUIRE(d.ToMilliseconds() == 4'800LL);
}

TEST_CASE("duration: scalar multiply", "[duration][arith]") {
    REQUIRE((Duration::Seconds(3) * 2).ToSeconds() == 6LL);
    REQUIRE((4 * Duration::Milliseconds(250)).ToMilliseconds() == 1'000LL);
}

TEST_CASE("duration: scalar divide", "[duration][arith]") {
    REQUIRE((Duration::Seconds(10) / 2).ToSeconds() == 5LL);
}

TEST_CASE("duration: duration ratio", "[duration][arith]") {
    REQUIRE(Duration::Seconds(10) / Duration::Milliseconds(100) == 100);
}

TEST_CASE("duration: duration modulo", "[duration][arith]") {
    Duration rem = Duration::Nanoseconds(10) % Duration::Nanoseconds(3);
    REQUIRE(rem.ToNanoseconds() == 1);
}

TEST_CASE("duration: negation", "[duration][arith]") {
    Duration d = Duration::Seconds(5);
    REQUIRE((-d).ToSeconds() == -5);
    REQUIRE((- (-d)) == d);
}

// ═══════════════════════════════════════════════════════════════════════
// Free functions
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: Abs", "[duration][free]") {
    REQUIRE(Abs(Duration::Seconds(-5)) == Duration::Seconds(5));
    REQUIRE(Abs(Duration::Seconds(3))  == Duration::Seconds(3));
}

TEST_CASE("duration: Min / Max", "[duration][free]") {
    REQUIRE(Min(Duration::Seconds(1), Duration::Seconds(2)) == Duration::Seconds(1));
    REQUIRE(Max(Duration::Seconds(1), Duration::Seconds(2)) == Duration::Seconds(2));
}

// ═══════════════════════════════════════════════════════════════════════
// Comparison
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: comparison operators", "[duration][cmp]") {
    Duration a = Duration::Seconds(1);
    Duration b = Duration::Milliseconds(1'000);
    Duration c = Duration::Seconds(2);

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a < c);
    REQUIRE(a <= b);
    REQUIRE(c > a);
    REQUIRE(c >= b);
}

// ═══════════════════════════════════════════════════════════════════════
// Chrono interop
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: to/from chrono", "[duration][chrono]") {
    Duration d = Duration::Seconds(1);
    auto chrono_ns = d.ToChrono<std::chrono::nanoseconds>();
    REQUIRE(chrono_ns.count() == 1'000'000'000);

    auto chrono_ms = d.ToChrono<std::chrono::milliseconds>();
    REQUIRE(chrono_ms.count() == 1'000);

    Duration d2 = Duration::FromChrono(std::chrono::microseconds(500));
    REQUIRE(d2.ToMicroseconds() == 500);
}

TEST_CASE("duration: chrono convenience functions", "[duration][chrono]") {
    Duration d = ChronoDurationToZeta(std::chrono::milliseconds(42));
    REQUIRE(d.ToMilliseconds() == 42);

    auto chrono_us = DurationToChrono<std::chrono::microseconds>(Duration::Milliseconds(1));
    REQUIRE(chrono_us.count() == 1'000);
}

// ═══════════════════════════════════════════════════════════════════════
// Saturation / extremes
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: Infinite / NegativeInfinite", "[duration][extreme]") {
    REQUIRE(Duration::Infinite().ToNanoseconds() == std::numeric_limits<int64_t>::max());
    REQUIRE(Duration::NegativeInfinite().ToNanoseconds() == std::numeric_limits<int64_t>::min());
}

TEST_CASE("duration: add_sat clamps at max", "[duration][extreme]") {
    Duration d = Duration::Infinite();
    Duration d2 = d + Duration::Nanoseconds(1);
    REQUIRE(d2 == Duration::Infinite());  // still clamped
}

TEST_CASE("duration: negation of min", "[duration][extreme]") {
    Duration d = Duration::NegativeInfinite();
    Duration neg = -d;
    REQUIRE(neg == Duration::Infinite());
}

TEST_CASE("duration: multiply saturates", "[duration][extreme]") {
    // Hours(max) should saturate
    Duration d = Duration::Hours(std::numeric_limits<int64_t>::max());
    REQUIRE(d == Duration::Infinite());

    // Negative hours(max) → negative infinite
    Duration nd = Duration::Hours(std::numeric_limits<int64_t>::min());
    REQUIRE(nd == Duration::NegativeInfinite());
}

// ═══════════════════════════════════════════════════════════════════════
// Constexpr
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("duration: constexpr evaluation", "[duration][constexpr]") {
    constexpr Duration d = Duration::Seconds(3) + Duration::Milliseconds(500);
    STATIC_REQUIRE(d.ToMilliseconds() == 3'500);
    STATIC_REQUIRE(d.ToDoubleSeconds() == 3.5);
    STATIC_REQUIRE(d > Duration::Seconds(1));
    STATIC_REQUIRE(Abs(-d) == d);
}
