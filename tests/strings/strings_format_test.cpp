#include "zeta/strings/format.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("format: no placeholders", "[strings][format]") {
    auto s = zeta::Format("hello world");
    REQUIRE(s == "hello world");
}

TEST_CASE("format: single auto placeholder", "[strings][format]") {
    auto s = zeta::Format("hello {}", "world");
    REQUIRE(s == "hello world");
}

TEST_CASE("format: multiple auto placeholders", "[strings][format]") {
    auto s = zeta::Format("x={}, y={}", 10, 20);
    REQUIRE(s == "x=10, y=20");
}

TEST_CASE("format: mixed string and int args", "[strings][format]") {
    auto s = zeta::Format("{}: {}", "port", 8080);
    REQUIRE(s == "port: 8080");
}

TEST_CASE("format: literal braces with {{ and }}", "[strings][format]") {
    auto s = zeta::Format("{{{} }}", 42);
    REQUIRE(s == "{42 }");
}

TEST_CASE("format: positional placeholders", "[strings][format]") {
    auto s = zeta::Format("{1} {0}", "world", "hello");
    REQUIRE(s == "hello world");
}

TEST_CASE("format: malformed placeholder is literal", "[strings][format][edge]") {
    auto s = zeta::Format("a{b");
    REQUIRE(s == "a{b");
}

TEST_CASE("format: malformed placeholder does not consume auto index", "[strings][format][edge]") {
    auto s = zeta::Format("a{b {} {}", "x", "y");
    REQUIRE(s == "a{b x y");
}

TEST_CASE("format: empty format string", "[strings][format]") {
    auto s = zeta::Format("");
    REQUIRE(s == "");
}

TEST_CASE("format: format with string arg types", "[strings][format]") {
    std::string name = "Alice";
    auto s = zeta::Format("Hello, {}", name);
    REQUIRE(s == "Hello, Alice");
}
