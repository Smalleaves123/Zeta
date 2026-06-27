#include "zeta/base/as_const.h"
#include "zeta/base/ignore.h"
#include "zeta/base/in_place.h"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

TEST_CASE("base: in_place is a constexpr tag", "[base][in_place]") {
    constexpr auto tag = zeta::in_place;
    STATIC_REQUIRE(std::is_same_v<decltype(tag), const zeta::in_place_t>);
}

TEST_CASE("base: as_const returns const lvalue reference", "[base][as_const]") {
    int value = 7;
    const int& ref = zeta::as_const(value);
    REQUIRE(ref == 7);
    REQUIRE(&ref == &value);
}

TEST_CASE("base: ignore accepts arbitrary values", "[base][ignore]") {
    zeta::ignore = 42;
    zeta::ignore = "discard me";
    zeta::ignore = 3.14;
    SUCCEED();
}
