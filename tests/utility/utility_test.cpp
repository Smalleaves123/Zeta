#include "zeta/utility/utility.h"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

// ═══════════════════════════════════════════════════════════════════════
// in_place
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("utility: in_place tag type", "[utility][in_place]") {
    // Just verify it compiles and is constexpr.
    constexpr auto tag = zeta::in_place;
    STATIC_REQUIRE(std::is_same_v<decltype(tag), const zeta::in_place_t>);
}

// ═══════════════════════════════════════════════════════════════════════
// as_const
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("utility: as_const — lvalue becomes const lvalue", "[utility][as_const]") {
    int x = 42;
    const int& ref = zeta::as_const(x);
    REQUIRE(ref == 42);
    STATIC_REQUIRE(std::is_same_v<decltype(ref), const int&>);

    // Verify it's the same object.
    REQUIRE(&ref == &x);
}

TEST_CASE("utility: as_const — lvalue becomes const", "[utility][as_const]") {
    int x = 1;
    auto& ref = zeta::as_const(x);
    REQUIRE(ref == 1);
    STATIC_REQUIRE(std::is_same_v<decltype(ref), const int&>);
    REQUIRE(&ref == &x);  // same object
}

TEST_CASE("utility: as_const — const lvalue stays const", "[utility][as_const]") {
    const int x = 99;
    const int& ref = zeta::as_const(x);
    REQUIRE(ref == 99);
    STATIC_REQUIRE(std::is_same_v<decltype(ref), const int&>);
}

// ═══════════════════════════════════════════════════════════════════════
// ignore
// ═══════════════════════════════════════════════════════════════════════

int side_effect = 0;
int get_side_effect() { return ++side_effect; }

TEST_CASE("utility: ignore suppresses values", "[utility][ignore]") {
    side_effect = 0;
    zeta::ignore = get_side_effect();   // should call get_side_effect()
    REQUIRE(side_effect == 1);

    zeta::ignore = 42;                  // literal
    zeta::ignore = 3.14;                // float
    zeta::ignore = std::string("hi");   // temporary
    SUCCEED();
}
