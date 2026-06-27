#include "zeta/status/result.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>
#include <utility>

TEST_CASE("Result: alias matches StatusOr", "[result]") {
    static_assert(std::is_same_v<zeta::Result<int>, zeta::StatusOr<int>>);
    zeta::Result<int> r(42);
    REQUIRE(r.ok());
    REQUIRE(r.value() == 42);
}

TEST_CASE("Result: value_or returns held value", "[result]") {
    zeta::Result<std::string> r(std::string("hello"));
    REQUIRE(r.value_or("fallback") == "hello");
}

TEST_CASE("Result: value_or returns fallback on error", "[result]") {
    zeta::Result<std::string> r(zeta::InternalError("fail"));
    REQUIRE(r.value_or("fallback") == "fallback");
}

TEST_CASE("Result: move-aware value_or", "[result]") {
    zeta::Result<std::string> r(std::string("hello"));
    REQUIRE(std::move(r).value_or(std::string("fallback")) == "hello");
}

TEST_CASE("Result: void alias compiles", "[result]") {
    zeta::Result<void> r;
    REQUIRE(r.ok());
    r.value_or();
}

TEST_CASE("Result: supports functional composition", "[result][compose]") {
    zeta::Result<int> r(10);
    auto mapped = r.Map([](int v) { return v + 5; });
    REQUIRE(mapped.ok());
    REQUIRE(mapped.value() == 15);

    auto chained = mapped.AndThen([](int v) {
        return zeta::Result<std::string>(std::to_string(v));
    });
    REQUIRE(chained.ok());
    REQUIRE(chained.value() == "15");
}
