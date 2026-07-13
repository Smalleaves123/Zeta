#include "zeta/base/as_const.h"
#include "zeta/base/attributes.h"
#include "zeta/base/config.h"
#include "zeta/base/ignore.h"
#include "zeta/base/in_place.h"
#include "zeta/base/macros.h"
#include "zeta/base/optimization.h"
#include "zeta/base/thread_annotations.h"

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

TEST_CASE("base: macros provide common portability helpers", "[base][macros]") {
    int value = 5;
    REQUIRE(ZETA_LIKELY(value == 5));
    REQUIRE(!ZETA_UNLIKELY(value == 5 && false));
    ZETA_UNUSED(value);
    SUCCEED();
}

TEST_CASE("base: config exposes the active platform", "[base][config]") {
    STATIC_REQUIRE(ZETA_CXX_STANDARD >= 202002L);
    STATIC_REQUIRE(ZETA_COMPILER_CLANG || ZETA_COMPILER_GCC || ZETA_COMPILER_MSVC);
    STATIC_REQUIRE(ZETA_OS_MACOS || ZETA_OS_LINUX || ZETA_OS_WINDOWS);
}

TEST_CASE("base: optimization and thread annotations compile portably",
          "[base][annotations]") {
    int value = 5;
    ZETA_ASSUME(value == 5);
    ZETA_PREFETCH(&value);
    REQUIRE(value == 5);

    struct Annotated {
        int mu = 0;
        int value ZETA_GUARDED_BY(mu);
    } item{0, 7};
    REQUIRE(item.value == 7);
}
