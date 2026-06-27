#include "zeta/types/any.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <typeinfo>

TEST_CASE("Any: default state is empty", "[types][any]") {
    zeta::Any value;
    REQUIRE(!value.has_value());
    REQUIRE(!value);
}

TEST_CASE("Any: construct from a value", "[types][any]") {
    zeta::Any value = std::string("hello");
    REQUIRE(value.has_value());
    REQUIRE(value.type() == typeid(std::string));
    REQUIRE(value.get<std::string>() == "hello");
}

TEST_CASE("Any: in_place_type construction", "[types][any]") {
    zeta::Any value(std::in_place_type<std::string>, 3, 'x');
    REQUIRE(value.has_value());
    REQUIRE(value.get<std::string>() == "xxx");
}

TEST_CASE("Any: get_if observes the stored type", "[types][any][get_if]") {
    zeta::Any value = 42;
    REQUIRE(zeta::AnyCast<int>(&value) != nullptr);
    REQUIRE(zeta::AnyCast<std::string>(&value) == nullptr);
}

TEST_CASE("Any: emplace replaces the current value", "[types][any][emplace]") {
    zeta::Any value = 7;
    value.emplace<std::string>(4, 'z');
    REQUIRE(value.has_value());
    REQUIRE(value.get<std::string>() == "zzzz");
}

TEST_CASE("Any: reset clears the payload", "[types][any][reset]") {
    zeta::Any value = 7;
    REQUIRE(value.has_value());
    value.reset();
    REQUIRE(!value.has_value());
}

TEST_CASE("Any: swap exchanges payloads", "[types][any][swap]") {
    zeta::Any a = 7;
    zeta::Any b = std::string("hello");
    a.swap(b);
    REQUIRE(a.get<std::string>() == "hello");
    REQUIRE(b.get<int>() == 7);
}
