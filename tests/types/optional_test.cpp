#include "zeta/types/optional.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("Optional: default is empty", "[types][optional]") {
    zeta::Optional<int> opt;
    REQUIRE(!opt.has_value());
    REQUIRE(!opt);
}

TEST_CASE("Optional: construct from value", "[types][optional]") {
    zeta::Optional<std::string> opt("hello");
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == "hello");
}

TEST_CASE("Optional: in_place construction", "[types][optional]") {
    zeta::Optional<std::string> opt(zeta::in_place, 3, 'x');
    REQUIRE(opt.has_value());
    REQUIRE(opt.value() == "xxx");
}

TEST_CASE("Optional: value_or returns fallback", "[types][optional]") {
    zeta::Optional<int> empty;
    zeta::Optional<int> full(7);
    REQUIRE(empty.value_or(3) == 3);
    REQUIRE(full.value_or(3) == 7);
}

TEST_CASE("Optional: Map transforms a value", "[types][optional][map]") {
    zeta::Optional<int> opt(21);
    auto mapped = opt.Map([](int v) { return v * 2; });
    REQUIRE(mapped.has_value());
    REQUIRE(mapped.value() == 42);
}

TEST_CASE("Optional: Map keeps empty state", "[types][optional][map]") {
    zeta::Optional<int> opt;
    auto mapped = opt.Map([](int v) { return v * 2; });
    REQUIRE(!mapped.has_value());
}

TEST_CASE("Optional: AndThen chains optional-producing work", "[types][optional][and_then]") {
    zeta::Optional<int> opt(10);
    auto chained = opt.AndThen([](int v) {
        return zeta::Optional<std::string>(std::to_string(v + 1));
    });
    REQUIRE(chained.has_value());
    REQUIRE(chained.value() == "11");
}

TEST_CASE("Optional: OrElse recovers from empty state", "[types][optional][or_else]") {
    zeta::Optional<int> opt;
    auto recovered = opt.OrElse([] { return 8; });
    REQUIRE(recovered.has_value());
    REQUIRE(recovered.value() == 8);
}
