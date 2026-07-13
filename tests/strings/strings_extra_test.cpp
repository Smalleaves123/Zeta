#include "zeta/strings/ascii.h"
#include "zeta/strings/escaping.h"
#include "zeta/strings/match.h"
#include "zeta/strings/substitute.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("strings: ASCII predicates and case helpers", "[strings][ascii]") {
    REQUIRE(zeta::AsciiIsAlpha('Z'));
    REQUIRE(zeta::AsciiIsDigit('7'));
    REQUIRE(zeta::AsciiIsAlnum('_') == false);
    REQUIRE(zeta::AsciiIsSpace('\n'));
    REQUIRE(zeta::AsciiToLower('Q') == 'q');
    REQUIRE(zeta::AsciiToUpper('q') == 'Q');
    REQUIRE(zeta::AsciiStrToLower("Hello 123") == "hello 123");
}

TEST_CASE("strings: wildcard matching", "[strings][match]") {
    REQUIRE(zeta::StrMatch("service/api/v1", "service/*"));
    REQUIRE(zeta::StrMatch("abc", "a?c"));
    REQUIRE(zeta::StrMatch("abc", "*b*"));
    REQUIRE(!zeta::StrMatch("abc", "a*d"));
    REQUIRE(zeta::StrMatchIgnoreCase("HelloWorld", "h*WORLD"));
}

TEST_CASE("strings: C escaping round-trips", "[strings][escaping]") {
    const std::string original("line\nquote=\" byte=\x01");
    const auto escaped = zeta::CEscape(original);
    REQUIRE(escaped == "line\\nquote=\\\" byte=\\x01");

    const auto restored = zeta::CUnescape(escaped);
    REQUIRE(restored.ok());
    REQUIRE(restored.value() == original);
    REQUIRE(!zeta::CUnescape("bad\\q").ok());
}

TEST_CASE("strings: positional substitution", "[strings][substitute]") {
    REQUIRE(zeta::StrSubstitute("$1/$0 ($$)", "name", 42) ==
            "42/name ($)");
    REQUIRE(zeta::StrSubstitute("literal", 1) == "literal");
    REQUIRE(zeta::StrSubstitute("$9", "x") == "$9");
}
