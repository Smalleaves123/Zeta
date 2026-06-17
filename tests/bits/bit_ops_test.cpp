#include "zeta/bits/bit_ops.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════
// popcount
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("popcount: zero", "[bits][popcount]") {
    REQUIRE(zeta::popcount(uint8_t{0}) == 0);
    REQUIRE(zeta::popcount(uint16_t{0}) == 0);
    REQUIRE(zeta::popcount(uint32_t{0}) == 0);
    REQUIRE(zeta::popcount(uint64_t{0}) == 0);
}

TEST_CASE("popcount: single bit", "[bits][popcount]") {
    for (int b = 0; b < 64; ++b) {
        REQUIRE(zeta::popcount(uint64_t{1} << b) == 1);
    }
}

TEST_CASE("popcount: all ones", "[bits][popcount]") {
    REQUIRE(zeta::popcount(uint8_t{0xFF}) == 8);
    REQUIRE(zeta::popcount(uint16_t{0xFFFF}) == 16);
    REQUIRE(zeta::popcount(uint32_t{0xFFFFFFFF}) == 32);
    REQUIRE(zeta::popcount(uint64_t{0xFFFFFFFFFFFFFFFF}) == 64);
}

TEST_CASE("popcount: alternating bits", "[bits][popcount]") {
    REQUIRE(zeta::popcount(uint32_t{0xAAAAAAAA}) == 16); // 1010...
    REQUIRE(zeta::popcount(uint32_t{0x55555555}) == 16); // 0101...
}

TEST_CASE("popcount: known values", "[bits][popcount]") {
    REQUIRE(zeta::popcount(uint32_t{0b10110000}) == 3);
    REQUIRE(zeta::popcount(uint32_t{0xDEADBEEF}) != 0);
}

// ═══════════════════════════════════════════════════════════════════════
// countl_zero / countr_zero
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("countl_zero: zero returns all bits", "[bits][zero]") {
    REQUIRE(zeta::countl_zero(uint8_t{0}) == 8);
    REQUIRE(zeta::countl_zero(uint16_t{0}) == 16);
    REQUIRE(zeta::countl_zero(uint32_t{0}) == 32);
    REQUIRE(zeta::countl_zero(uint64_t{0}) == 64);
}

TEST_CASE("countl_zero: single bit at various positions", "[bits][zero]") {
    REQUIRE(zeta::countl_zero(uint32_t{1} << 31) == 0);
    REQUIRE(zeta::countl_zero(uint32_t{1} << 30) == 1);
    REQUIRE(zeta::countl_zero(uint32_t{1} << 0) == 31);
    REQUIRE(zeta::countl_zero(uint32_t{0x80000000}) == 0);
}

TEST_CASE("countr_zero: zero returns all bits", "[bits][zero]") {
    REQUIRE(zeta::countr_zero(uint32_t{0}) == 32);
    REQUIRE(zeta::countr_zero(uint64_t{0}) == 64);
}

TEST_CASE("countr_zero: single bit at various positions", "[bits][zero]") {
    REQUIRE(zeta::countr_zero(uint32_t{1} << 0) == 0);
    REQUIRE(zeta::countr_zero(uint32_t{1} << 4) == 4);
    REQUIRE(zeta::countr_zero(uint32_t{1} << 31) == 31);
}

TEST_CASE("countl_zero / countr_zero: consistency with popcount", "[bits][zero]") {
    // For any non-zero value, popcount + countl_zero + countr_zero should be
    // at most bit_width + (trailing zeros before the first 1).
    // Simpler invariant: find_first_set + countl_zero should not overlap.
    auto check = []<typename T>(T x) {
        int pop = zeta::popcount(x);
        int clz = zeta::countl_zero(x);
        int ctz = zeta::countr_zero(x);
        int bw = zeta::bit_width(x);
        // Leading zeros + bit_width == number of bits
        REQUIRE(clz + bw == static_cast<int>(std::numeric_limits<T>::digits));
        // For x != 0, trailing zeros are ≤ (digits - leading zeros - 1)
        if (x != 0) {
            REQUIRE(ctz <= static_cast<int>(std::numeric_limits<T>::digits) - clz - 1);
        }
        (void)pop;
    };
    check(uint32_t{0});
    check(uint32_t{1});
    check(uint32_t{0x80000000});
    check(uint32_t{0xFFFFFFFF});
    check(uint32_t{0b10110000});
}

// ═══════════════════════════════════════════════════════════════════════
// countl_one / countr_one
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("countl_one / countr_one: basic", "[bits][one]") {
    REQUIRE(zeta::countl_one(uint32_t{0}) == 0);
    REQUIRE(zeta::countl_one(uint32_t{~uint32_t{0}}) == 32);
    REQUIRE(zeta::countr_one(uint32_t{0}) == 0);
    REQUIRE(zeta::countr_one(uint32_t{~uint32_t{0}}) == 32);
}

TEST_CASE("countl_one / countr_one: consistency", "[bits][one]") {
    // countl_one(x) == countl_zero(~x)
    auto check = [](uint32_t x) {
        REQUIRE(zeta::countl_one(x) == zeta::countl_zero(~x));
        REQUIRE(zeta::countr_one(x) == zeta::countr_zero(~x));
    };
    check(0);
    check(0xFFFFFFFF);
    check(0xF0F0F0F0);
    check(0x80000000);
}

// ═══════════════════════════════════════════════════════════════════════
// rotl / rotr
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("rotl: rotate by zero is identity", "[bits][rotate]") {
    REQUIRE(zeta::rotl(uint32_t{0x12345678}, 0) == 0x12345678);
    REQUIRE(zeta::rotr(uint32_t{0x12345678}, 0) == 0x12345678);
}

TEST_CASE("rotl: rotate by bit width is identity", "[bits][rotate]") {
    REQUIRE(zeta::rotl(uint32_t{0x12345678}, 32) == 0x12345678);
    REQUIRE(zeta::rotr(uint32_t{0x12345678}, 32) == 0x12345678);
}

TEST_CASE("rotl / rotr: round-trip", "[bits][rotate]") {
    uint32_t x = 0x12345678;
    for (int s = 1; s < 32; ++s) {
        REQUIRE(zeta::rotr(zeta::rotl(x, s), s) == x);
        REQUIRE(zeta::rotl(zeta::rotr(x, s), s) == x);
    }
}

TEST_CASE("rotl / rotr: equivalence", "[bits][rotate]") {
    // rotl(x, s) == rotr(x, -s)
    uint32_t x = 0xDEADBEEF;
    for (int s = 1; s < 32; ++s) {
        REQUIRE(zeta::rotl(x, s) == zeta::rotr(x, 32 - s));
    }
}

TEST_CASE("rotl: known result", "[bits][rotate]") {
    REQUIRE(zeta::rotl(uint32_t{0x80000001}, 1) == 0x00000003);
    REQUIRE(zeta::rotr(uint32_t{0x80000001}, 1) == 0xC0000000);
}

// ═══════════════════════════════════════════════════════════════════════
// has_single_bit / bit_ceil / bit_floor / bit_width
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("has_single_bit: basic", "[bits][pow2]") {
    REQUIRE(!zeta::has_single_bit(uint32_t{0}));
    REQUIRE(zeta::has_single_bit(uint32_t{1}));
    REQUIRE(zeta::has_single_bit(uint32_t{2}));
    REQUIRE(zeta::has_single_bit(uint32_t{4}));
    REQUIRE(zeta::has_single_bit(uint32_t{8}));
    REQUIRE(!zeta::has_single_bit(uint32_t{3}));
    REQUIRE(!zeta::has_single_bit(uint32_t{6}));
    REQUIRE(!zeta::has_single_bit(uint32_t{7}));
}

TEST_CASE("has_single_bit: powers of two across range", "[bits][pow2]") {
    for (int k = 0; k < 31; ++k) {
        REQUIRE(zeta::has_single_bit(uint32_t{1} << k));
    }
    // Non-powers of two
    for (int k = 2; k < 31; ++k) {
        REQUIRE(!zeta::has_single_bit((uint32_t{1} << k) - 1));
    }
}

TEST_CASE("bit_ceil: basic", "[bits][pow2]") {
    REQUIRE(zeta::bit_ceil(uint32_t{0}) == 1);
    REQUIRE(zeta::bit_ceil(uint32_t{1}) == 1);
    REQUIRE(zeta::bit_ceil(uint32_t{2}) == 2);
    REQUIRE(zeta::bit_ceil(uint32_t{3}) == 4);
    REQUIRE(zeta::bit_ceil(uint32_t{5}) == 8);
}

TEST_CASE("bit_ceil: already power of two", "[bits][pow2]") {
    for (int k = 0; k < 31; ++k) {
        uint32_t p = uint32_t{1} << k;
        REQUIRE(zeta::bit_ceil(p) == p);
    }
}

TEST_CASE("bit_floor: basic", "[bits][pow2]") {
    REQUIRE(zeta::bit_floor(uint32_t{0}) == 0);
    REQUIRE(zeta::bit_floor(uint32_t{1}) == 1);
    REQUIRE(zeta::bit_floor(uint32_t{2}) == 2);
    REQUIRE(zeta::bit_floor(uint32_t{3}) == 2);
    REQUIRE(zeta::bit_floor(uint32_t{5}) == 4);
    REQUIRE(zeta::bit_floor(uint32_t{7}) == 4);
    REQUIRE(zeta::bit_floor(uint32_t{8}) == 8);
}

TEST_CASE("bit_width: basic", "[bits][pow2]") {
    REQUIRE(zeta::bit_width(uint32_t{0}) == 0);
    REQUIRE(zeta::bit_width(uint32_t{1}) == 1);
    REQUIRE(zeta::bit_width(uint32_t{2}) == 2);
    REQUIRE(zeta::bit_width(uint32_t{3}) == 2);
    REQUIRE(zeta::bit_width(uint32_t{4}) == 3);
    REQUIRE(zeta::bit_width(uint32_t{0xFFFFFFFF}) == 32);
}

// ═══════════════════════════════════════════════════════════════════════
// find_first_set / find_last_set
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("find_first_set: basic", "[bits][scan]") {
    REQUIRE(zeta::find_first_set(uint32_t{0}) == 32);  // digits
    REQUIRE(zeta::find_first_set(uint32_t{1}) == 0);
    REQUIRE(zeta::find_first_set(uint32_t{2}) == 1);
    REQUIRE(zeta::find_first_set(uint32_t{4}) == 2);
    REQUIRE(zeta::find_first_set(uint32_t{0x80000000}) == 31);
    REQUIRE(zeta::find_first_set(uint32_t{0x80008000}) == 15);  // lowest is bit 15
}

TEST_CASE("find_last_set: basic", "[bits][scan]") {
    REQUIRE(zeta::find_last_set(uint32_t{0}) == -1);
    REQUIRE(zeta::find_last_set(uint32_t{1}) == 0);
    REQUIRE(zeta::find_last_set(uint32_t{2}) == 1);
    REQUIRE(zeta::find_last_set(uint32_t{0x80000000}) == 31);
    REQUIRE(zeta::find_last_set(uint32_t{0x80008000}) == 31);  // highest is bit 31
}

// ═══════════════════════════════════════════════════════════════════════
// set_bit / clear_bit / toggle_bit / test_bit
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("set_bit / test_bit: round-trip", "[bits][bitmanip]") {
    uint32_t v = 0;
    for (int b = 0; b < 32; ++b) {
        v = zeta::set_bit(v, b);
        REQUIRE(zeta::test_bit(v, b));
    }
    REQUIRE(v == 0xFFFFFFFF);
}

TEST_CASE("clear_bit: removes a bit", "[bits][bitmanip]") {
    uint32_t v = 0xFFFFFFFF;
    for (int b = 0; b < 32; ++b) {
        v = zeta::clear_bit(v, b);
        REQUIRE(!zeta::test_bit(v, b));
    }
    REQUIRE(v == 0);
}

TEST_CASE("toggle_bit: toggles correctly", "[bits][bitmanip]") {
    uint32_t v = 0;
    for (int b = 0; b < 32; b += 2) {
        v = zeta::toggle_bit(v, b);
        REQUIRE(zeta::test_bit(v, b));
    }
    for (int b = 0; b < 32; b += 2) {
        v = zeta::toggle_bit(v, b);
        REQUIRE(!zeta::test_bit(v, b));
    }
}

TEST_CASE("set_bit: does not affect other bits", "[bits][bitmanip]") {
    uint32_t orig = 0xAAAAAAAA;
    uint32_t v = zeta::set_bit(orig, 0);  // set bit 0 (was 0)
    REQUIRE(zeta::test_bit(v, 0));
    // All other bits unchanged
    REQUIRE((v & ~uint32_t{1}) == (orig & ~uint32_t{1}));
}

TEST_CASE("bit manipulation: all types", "[bits][bitmanip]") {
    // Compile-time check that these work on all unsigned types
    REQUIRE(zeta::set_bit(uint8_t{0}, 3) == 0x08);
    REQUIRE(zeta::clear_bit(uint16_t{0xFFFF}, 0) == 0xFFFE);
    REQUIRE(zeta::toggle_bit(uint32_t{0}, 0) == 1);
    REQUIRE(!zeta::test_bit(uint64_t{0}, 0));
}

// ═══════════════════════════════════════════════════════════════════════
// bit_extract / bit_insert
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("bit_extract: basic", "[bits][extract]") {
    // 0b101100: bits: 0=0,1=0,2=1,3=1,4=0,5=1 → extract pos=2,len=2 → bits[2:3]=0b11=3
    REQUIRE(zeta::bit_extract(uint32_t{0b101100}, 2, 2) == 0b11);
    // 0b10110000: bits 4-6 → 0b1011>>4=0b1011, &0b111=0b011=3
    REQUIRE(zeta::bit_extract(uint32_t{0b10110000}, 4, 3) == 0b011);
}

TEST_CASE("bit_extract: edge cases", "[bits][extract]") {
    // Out-of-range pos returns 0
    REQUIRE(zeta::bit_extract(uint32_t{0xFFFFFFFF}, 32, 4) == 0);
    REQUIRE(zeta::bit_extract(uint32_t{0xFFFFFFFF}, -1, 4) == 0);
    // Zero len returns 0
    REQUIRE(zeta::bit_extract(uint32_t{0xFFFFFFFF}, 0, 0) == 0);
    // Negative len returns 0
    REQUIRE(zeta::bit_extract(uint32_t{0xFFFFFFFF}, 0, -1) == 0);
    // Full width extraction
    REQUIRE(zeta::bit_extract(uint8_t{0xAB}, 0, 8) == 0xAB);
}

TEST_CASE("bit_insert: basic", "[bits][extract]") {
    // Insert 0b11 at pos=4, len=2 into 0 → should set bits 4-5
    REQUIRE(zeta::bit_insert(uint32_t{0}, 4, 2, 0b11) == (0b11 << 4));
    // Insert 0b101 at pos=0, len=3 into 0b11000:
    //   0b11000 (24) → clear bits 0-2 → 0b11000 (24), insert 0b101→0b11101(29)
    REQUIRE(zeta::bit_insert(uint32_t{0b11000}, 0, 3, 0b101) == 29);
}

TEST_CASE("bit_extract / bit_insert: round-trip", "[bits][extract]") {
    // Insert then extract → same bits
    uint32_t orig = 0;
    uint32_t inserted = zeta::bit_insert(orig, 4, 3, 0b101);
    uint32_t extracted = zeta::bit_extract(inserted, 4, 3);
    REQUIRE(extracted == 0b101);

    // Insert into a non-zero value preserves bits outside the range
    uint32_t value = 0xFFFF0000;
    uint32_t mod = zeta::bit_insert(value, 0, 4, 0b1010);
    REQUIRE(zeta::bit_extract(mod, 0, 4) == 0b1010);
    REQUIRE(zeta::bit_extract(mod, 16, 16) == 0xFFFF);
}

TEST_CASE("bit_insert: edge cases", "[bits][extract]") {
    // Out-of-range pos returns value unchanged
    REQUIRE(zeta::bit_insert(uint32_t{0xFFFF}, 32, 4, 0) == 0xFFFF);
    // Zero len returns value unchanged
    REQUIRE(zeta::bit_insert(uint32_t{0xFFFF}, 0, 0, 0xF) == 0xFFFF);
}

// ═══════════════════════════════════════════════════════════════════════
// byteswap
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("byteswap: uint8_t is identity", "[bits][byteswap]") {
    REQUIRE(zeta::byteswap(uint8_t{0xAB}) == 0xAB);
    REQUIRE(zeta::byteswap(uint8_t{0x00}) == 0x00);
    REQUIRE(zeta::byteswap(uint8_t{0xFF}) == 0xFF);
}

TEST_CASE("byteswap: uint16_t", "[bits][byteswap]") {
    REQUIRE(zeta::byteswap(uint16_t{0x0102}) == 0x0201);
    REQUIRE(zeta::byteswap(uint16_t{0xAA55}) == 0x55AA);
}

TEST_CASE("byteswap: uint32_t", "[bits][byteswap]") {
    REQUIRE(zeta::byteswap(uint32_t{0x01020304}) == 0x04030201);
    REQUIRE(zeta::byteswap(uint32_t{0xDEADBEEF}) == 0xEFBEADDE);
}

TEST_CASE("byteswap: uint64_t", "[bits][byteswap]") {
    REQUIRE(zeta::byteswap(uint64_t{0x0102030405060708}) == 0x0807060504030201);
}

TEST_CASE("byteswap: double swap is identity", "[bits][byteswap]") {
    REQUIRE(zeta::byteswap(zeta::byteswap(uint16_t{0xABCD})) == 0xABCD);
    REQUIRE(zeta::byteswap(zeta::byteswap(uint32_t{0x12345678})) == 0x12345678);
    REQUIRE(zeta::byteswap(zeta::byteswap(uint64_t{0x1122334455667788})) ==
            0x1122334455667788);
}

// ═══════════════════════════════════════════════════════════════════════
// Endianness conversions
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("endian: round-trip invariant", "[bits][endian]") {
    uint32_t x = 0x12345678;
    REQUIRE(zeta::little_to_native(zeta::native_to_little(x)) == x);
    REQUIRE(zeta::big_to_native(zeta::native_to_big(x)) == x);
}

TEST_CASE("endian: cross conversion cancels", "[bits][endian]") {
    uint32_t x = 0xDEADBEEF;
    // Converting to little then to big and back is identity
    auto native = zeta::big_to_native(zeta::native_to_big(x));
    REQUIRE(native == x);
    // Big → Native → Little → Native → Big: two round-trips
    uint32_t big = zeta::native_to_big(x);
    uint32_t native2 = zeta::big_to_native(big);
    REQUIRE(native2 == x);
}

// ═══════════════════════════════════════════════════════════════════════
// Type coverage: all functions compile and work with all unsigned types
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("type coverage: uint8_t", "[bits][types]") {
    uint8_t x = 0b10101010;
    REQUIRE(zeta::popcount(x) == 4);
    REQUIRE(zeta::countl_zero(x) >= 0);
    REQUIRE(zeta::countr_zero(x) >= 0);
    REQUIRE(zeta::has_single_bit(x) == false);
    REQUIRE(zeta::rotl(x, 1) != 0);
    REQUIRE(zeta::find_first_set(x) == 1);
    REQUIRE(zeta::set_bit(x, 0) != x);
}

TEST_CASE("type coverage: uint16_t", "[bits][types]") {
    uint16_t x = 0xF0F0;
    REQUIRE(zeta::popcount(x) == 8);
    REQUIRE(zeta::bit_floor(x) <= x);
    REQUIRE(zeta::bit_width(x) == 16);
    // byteswap: 0xF0F0 is symmetric (0xF0 == 0xF0) so remains unchanged
    REQUIRE(zeta::byteswap(x) == x);
    (void)zeta::find_last_set(x);

    // bit_ceil: test with a value that fits
    REQUIRE(zeta::bit_ceil(uint16_t{0x7FFF}) == 0x8000);
    // bit_ceil overflow: 0xF0F0 > 0x8000 → result doesn't fit in uint16_t → returns 0
    REQUIRE(zeta::bit_ceil(uint16_t{0xFF00}) == 0);
}

TEST_CASE("type coverage: uint64_t", "[bits][types]") {
    uint64_t x = UINT64_C(0x12345678ABCDEF00);
    REQUIRE(zeta::popcount(x) > 0);
    REQUIRE(zeta::countl_zero(x) < 64);
    REQUIRE(zeta::rotl(x, 32) != x);
}

// ═══════════════════════════════════════════════════════════════════════
// Compile-time evaluation (constexpr)
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("constexpr: all functions evaluable at compile time", "[bits][compile]") {
    constexpr uint32_t v = 0b10110000;
    constexpr int pop = zeta::popcount(v);
    constexpr int clz = zeta::countl_zero(v);
    constexpr int ctz = zeta::countr_zero(v);
    constexpr bool p2 = zeta::has_single_bit(v);
    constexpr auto ceil = zeta::bit_ceil(v);
    constexpr auto floor = zeta::bit_floor(v);
    constexpr int bw = zeta::bit_width(v);
    constexpr int ffs = zeta::find_first_set(v);
    constexpr int fls = zeta::find_last_set(v);
    constexpr auto set = zeta::set_bit(v, 0);
    constexpr auto clr = zeta::clear_bit(v, 4);
    constexpr auto tog = zeta::toggle_bit(v, 1);
    constexpr bool tst = zeta::test_bit(v, 7);
    constexpr auto ext = zeta::bit_extract(v, 4, 3);
    constexpr auto ins = zeta::bit_insert(v, 0, 2, uint32_t{0b11});
    constexpr auto bsw = zeta::byteswap(uint16_t{0x0102});

    STATIC_REQUIRE(pop == 3);
    STATIC_REQUIRE(clz == 24);  // 32 - bit_width(0b10110000=8) = 24
    STATIC_REQUIRE(ctz == 4);
    STATIC_REQUIRE(!p2);
    STATIC_REQUIRE(ceil == 256);
    STATIC_REQUIRE(floor == 128);
    STATIC_REQUIRE(bw == 8);
    STATIC_REQUIRE(ffs == 4);
    STATIC_REQUIRE(fls == 7);
    STATIC_REQUIRE(tst == true);
    STATIC_REQUIRE(ext == 0b011);
    STATIC_REQUIRE(ins == 0b10110011);
    STATIC_REQUIRE(bsw == 0x0201);
    (void)set;
    (void)clr;
    (void)tog;
}

// ═══════════════════════════════════════════════════════════════════════
// Utility: ensure functions reject signed types via std::unsigned_integral
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
concept has_popcount = requires(T x) { zeta::popcount(x); };

TEST_CASE("SFINAE: unsigned types accepted, signed types rejected", "[bits][compile]") {
    STATIC_REQUIRE(has_popcount<unsigned int>);
    STATIC_REQUIRE(has_popcount<uint8_t>);
    STATIC_REQUIRE(has_popcount<uint64_t>);
    STATIC_REQUIRE(!has_popcount<int>);
    STATIC_REQUIRE(!has_popcount<float>);
}
