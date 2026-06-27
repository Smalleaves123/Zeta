#include "zeta/numeric/int128.h"

#include <catch2/catch_test_macros.hpp>

#include <limits>
#include <type_traits>

using namespace zeta;

// ═══════════════════════════════════════════════════════════════════════
// Uint128
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("uint128: default construction is zero", "[uint128][ctor]") {
    constexpr Uint128 z;
    STATIC_REQUIRE(z == Uint128(0));
    STATIC_REQUIRE(!static_cast<bool>(z));
}

TEST_CASE("uint128: from uint64_t", "[uint128][ctor]") {
    Uint128 x(42);
    REQUIRE(x.Low64() == 42);
    REQUIRE(x.High64() == 0);
}

TEST_CASE("uint128: FromPair", "[uint128][ctor]") {
    Uint128 x = Uint128::FromPair(1, 0xDEADBEEFULL);
    REQUIRE(x.High64() == 1);
    REQUIRE(x.Low64()  == 0xDEADBEEFULL);
}

TEST_CASE("uint128: explicit conversion to uint64_t", "[uint128][conv]") {
    Uint128 x = Uint128::FromPair(0xABCD, 0x1234);
    REQUIRE(static_cast<uint64_t>(x) == 0x1234);
}

// ── Arithmetic ────────────────────────────────────────────────────

TEST_CASE("uint128: addition", "[uint128][arith]") {
    Uint128 a(100), b(200);
    REQUIRE(a + b == Uint128(300));

    // Carry across 64-bit boundary.
    Uint128 c = Uint128::FromPair(0, ~uint64_t{0});         // max low 64
    Uint128 d = c + Uint128(1);
    REQUIRE(d.High64() == 1);
    REQUIRE(d.Low64()  == 0);
}

TEST_CASE("uint128: subtraction", "[uint128][arith]") {
    Uint128 a(300), b(100);
    REQUIRE(a - b == Uint128(200));

    // Borrow across boundary.
    Uint128 c = Uint128::FromPair(1, 0);
    Uint128 d = c - Uint128(1);
    REQUIRE(d.High64() == 0);
    REQUIRE(d.Low64()  == ~uint64_t{0});
}

TEST_CASE("uint128: multiplication", "[uint128][arith]") {
    Uint128 a(1'000'000);
    REQUIRE(a * a == Uint128(1'000'000'000'000ULL));

    // Crosses 64-bit boundary.
    Uint128 b = Uint128::FromPair(0, 1ULL << 63);
    Uint128 c = b * Uint128(2);
    REQUIRE(c.High64() == 1);
    REQUIRE(c.Low64()  == 0);
}

TEST_CASE("uint128: division", "[uint128][arith]") {
    Uint128 a(100);
    REQUIRE(a / Uint128(3) == Uint128(33));
    REQUIRE(a % Uint128(3) == Uint128(1));
}

TEST_CASE("uint128: bitwise operations", "[uint128][arith]") {
    Uint128 a = Uint128::FromPair(0xF0F0, 0x0F0F);
    Uint128 b = Uint128::FromPair(0xFF00, 0x00FF);

    REQUIRE(((a & b).High64()) == (0xF0F0 & 0xFF00));
    REQUIRE(((a & b).Low64())  == (0x0F0F & 0x00FF));
    REQUIRE(((a | b).High64()) == (0xF0F0 | 0xFF00));
    REQUIRE(((a | b).Low64())  == (0x0F0F | 0x00FF));
    REQUIRE(((a ^ b).High64()) == (0xF0F0 ^ 0xFF00));
    REQUIRE(((a ^ b).Low64())  == (0x0F0F ^ 0x00FF));

    Uint128 c = ~a;
    REQUIRE(c.High64() == ~0xF0F0ULL);
    REQUIRE(c.Low64()  == ~0x0F0FULL);
}

TEST_CASE("uint128: shift left", "[uint128][arith]") {
    Uint128 a(1);
    REQUIRE((a << 10) == Uint128(1024));
    REQUIRE((a << 64) == Uint128::FromPair(1, 0));
    REQUIRE((a << 127) == Uint128::FromPair(1ULL << 63, 0));
}

TEST_CASE("uint128: shift right", "[uint128][arith]") {
    Uint128 a = Uint128::FromPair(1, 0);
    REQUIRE((a >> 64) == Uint128(1));
    REQUIRE((a >> 1)  == Uint128::FromPair(0, 1ULL << 63));
}

TEST_CASE("uint128: increment / decrement", "[uint128][arith]") {
    Uint128 a(0);
    ++a; REQUIRE(a == Uint128(1));
    --a; REQUIRE(a == Uint128(0));

    Uint128 b = Uint128::FromPair(0, ~uint64_t{0});
    ++b;
    REQUIRE(b.High64() == 1);
    REQUIRE(b.Low64()  == 0);
}

// ── Comparison ────────────────────────────────────────────────────

TEST_CASE("uint128: comparison operators", "[uint128][cmp]") {
    Uint128 a(10), b(20), c(10);
    REQUIRE(a == c);
    REQUIRE(a != b);
    REQUIRE(a < b);
    REQUIRE(a <= c);
    REQUIRE(b > a);
    REQUIRE(b >= a);
}

// ═══════════════════════════════════════════════════════════════════════
// Int128
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("int128: default zero", "[int128][ctor]") {
    constexpr Int128 z;
    STATIC_REQUIRE(z == Int128(0));
}

TEST_CASE("int128: negative value", "[int128][ctor]") {
    Int128 neg(-42);
    REQUIRE(neg.Low64() == -42);
    REQUIRE(neg < 0);
    REQUIRE(neg.IsNegative());
}

TEST_CASE("int128: FromPair", "[int128][ctor]") {
    Int128 x = Int128::FromPair(-1, 0xDEADBEEFULL);  // -1 << 64 | lo
    REQUIRE(x.High64() == -1);
    REQUIRE(x < 0);
}

TEST_CASE("int128: Abs", "[int128][ctor]") {
    Int128 pos(42);
    Int128 neg(-42);
    REQUIRE(pos.Abs() == Uint128(42));
    REQUIRE(neg.Abs() == Uint128(42));
}

// ── Arithmetic ────────────────────────────────────────────────────

TEST_CASE("int128: addition", "[int128][arith]") {
    REQUIRE(Int128(10) + Int128(20) == Int128(30));
    REQUIRE(Int128(-5) + Int128(5) == Int128(0));
}

TEST_CASE("int128: subtraction", "[int128][arith]") {
    REQUIRE(Int128(100) - Int128(30) == Int128(70));
    REQUIRE(Int128(0) - Int128(1) == Int128(-1));
}

TEST_CASE("int128: multiplication", "[int128][arith]") {
    REQUIRE(Int128(6) * Int128(7) == Int128(42));
    REQUIRE(Int128(-3) * Int128(-4) == Int128(12));
    REQUIRE(Int128(-3) * Int128(4) == Int128(-12));
}

TEST_CASE("int128: division", "[int128][arith]") {
    REQUIRE(Int128(100) / Int128(3) == Int128(33));
    REQUIRE(Int128(-100) / Int128(3) == Int128(-33));
}

TEST_CASE("int128: negation", "[int128][arith]") {
    REQUIRE(-Int128(42) == Int128(-42));
    REQUIRE(-Int128(-42) == Int128(42));
}

TEST_CASE("int128: bitwise operations", "[int128][arith]") {
    Int128 a(0b1010);
    Int128 b(0b1100);
    REQUIRE((a & b) == Int128(0b1000));
    REQUIRE((a | b) == Int128(0b1110));
    REQUIRE((a ^ b) == Int128(0b0110));
    REQUIRE(~Int128(0) == Int128(-1));
}

TEST_CASE("int128: left shift preserves bit pattern for negatives", "[int128][arith]") {
    REQUIRE((Int128(-1) << 1) == Int128(-2));
    REQUIRE((Int128(1) << 127) == Int128::FromPair(std::numeric_limits<int64_t>::min(), 0));
}

// ── Comparison ────────────────────────────────────────────────────

TEST_CASE("int128: comparison with negatives", "[int128][cmp]") {
    Int128 a(-10), b(0), c(5);
    REQUIRE(a < b);
    REQUIRE(b < c);
    REQUIRE(a < c);
    REQUIRE(c > a);
    REQUIRE(b <= b);
}

// ═══════════════════════════════════════════════════════════════════════
// numeric_limits
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("int128: numeric_limits Uint128", "[int128][limits]") {
    using L = std::numeric_limits<Uint128>;
    STATIC_REQUIRE(L::is_specialized);
    STATIC_REQUIRE(!L::is_signed);
    STATIC_REQUIRE(L::is_integer);
    STATIC_REQUIRE(L::min() == Uint128(0));
    REQUIRE(L::max() == Uint128::FromPair(~uint64_t{0}, ~uint64_t{0}));
}

TEST_CASE("int128: numeric_limits Int128", "[int128][limits]") {
    using L = std::numeric_limits<Int128>;
    STATIC_REQUIRE(L::is_specialized);
    STATIC_REQUIRE(L::is_signed);
    STATIC_REQUIRE(L::is_integer);
    REQUIRE(L::min() < Int128(0));
    REQUIRE(L::max() > Int128(0));
}
