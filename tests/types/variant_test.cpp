#include "zeta/types/variant.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <type_traits>

TEST_CASE("Variant: default constructs first alternative", "[types][variant]") {
    zeta::Variant<int, std::string> v;
    REQUIRE(v.index() == 0);
    REQUIRE(v.holds_alternative<int>());
    REQUIRE(v.get<0>() == 0);
}

TEST_CASE("Variant: construct from value", "[types][variant]") {
    zeta::Variant<int, std::string> v("hello");
    REQUIRE(v.index() == 1);
    REQUIRE(v.holds_alternative<std::string>());
    REQUIRE(v.get<std::string>() == "hello");
}

TEST_CASE("Variant: in_place_type construction", "[types][variant]") {
    zeta::Variant<int, std::string> v(std::in_place_type<std::string>, 3, 'x');
    REQUIRE(v.holds_alternative<std::string>());
    REQUIRE(v.get<std::string>() == "xxx");
}

TEST_CASE("Variant: in_place_index construction", "[types][variant]") {
    zeta::Variant<int, std::string> v(std::in_place_index<0>, 42);
    REQUIRE(v.holds_alternative<int>());
    REQUIRE(v.get<0>() == 42);
}

TEST_CASE("Variant: visit dispatches to the active alternative", "[types][variant][visit]") {
    zeta::Variant<int, std::string> v("ab");
    auto text = v.Visit([](const auto& value) -> std::string {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_same_v<T, int>) {
            return std::to_string(value);
        } else {
            return value;
        }
    });
    REQUIRE(text == "ab");
}

TEST_CASE("Variant: emplace switches the active alternative", "[types][variant][emplace]") {
    zeta::Variant<int, std::string> v(7);
    REQUIRE(v.holds_alternative<int>());
    v.emplace<std::string>(4, 'z');
    REQUIRE(v.holds_alternative<std::string>());
    REQUIRE(v.get<std::string>() == "zzzz");
}

TEST_CASE("Variant: get_if observes the active alternative", "[types][variant][get_if]") {
    zeta::Variant<int, std::string> v(9);
    REQUIRE(v.get_if<int>() != nullptr);
    REQUIRE(v.get_if<std::string>() == nullptr);
}
