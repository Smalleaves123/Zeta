#include "zeta/strings/str_utils.h"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

using namespace std::literals;

// ═══════════════════════════════════════════════════════════════════
// Predicates
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrContains", "[str_utils]") {
    REQUIRE(zeta::StrContains("hello world", "lo wo"));
    REQUIRE(zeta::StrContains("hello", "h"));
    REQUIRE(zeta::StrContains("hello", "o"));
    REQUIRE(!zeta::StrContains("hello", "x"));
    REQUIRE(zeta::StrContains("", ""));
    REQUIRE(!zeta::StrContains("", "x"));
}

TEST_CASE("StrContains: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StrContains("hello", "") == true);   // empty needle
    REQUIRE(zeta::StrContains("", "") == true);          // both empty
    REQUIRE(zeta::StrContains("ab", "abc") == false);   // needle longer than text
}

TEST_CASE("StartsWith", "[str_utils]") {
    REQUIRE(zeta::StartsWith("hello world", "hello"));
    REQUIRE(zeta::StartsWith("abc", ""));
    REQUIRE(!zeta::StartsWith("abc", "abcd"));
    REQUIRE(!zeta::StartsWith("abc", "bc"));
    REQUIRE(zeta::StartsWith("", ""));
}

TEST_CASE("StartsWith: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StartsWith("", "") == true);
    REQUIRE(zeta::StartsWith("", "a") == false);
    REQUIRE(zeta::StartsWith("exact", "exact") == true);
}

TEST_CASE("EndsWith", "[str_utils]") {
    REQUIRE(zeta::EndsWith("hello world", "world"));
    REQUIRE(zeta::EndsWith("abc", ""));
    REQUIRE(!zeta::EndsWith("abc", "xabc"));
    REQUIRE(!zeta::EndsWith("abc", "ab"));
    REQUIRE(zeta::EndsWith("", ""));
}

TEST_CASE("EndsWith: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::EndsWith("", "") == true);
    REQUIRE(zeta::EndsWith("", "a") == false);
    REQUIRE(zeta::EndsWith("exact", "exact") == true);
}

TEST_CASE("StartsWithIgnoreCase", "[str_utils]") {
    REQUIRE(zeta::StartsWithIgnoreCase("Hello", "hello"));
    REQUIRE(zeta::StartsWithIgnoreCase("HELLO", "hello"));
    REQUIRE(!zeta::StartsWithIgnoreCase("hello", "HELLOX"));
}

TEST_CASE("StartsWithIgnoreCase: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StartsWithIgnoreCase("", "") == true);
    REQUIRE(zeta::StartsWithIgnoreCase("ABC", "abc") == true);
    REQUIRE(zeta::StartsWithIgnoreCase("abc", "") == true);
    REQUIRE(zeta::StartsWithIgnoreCase("", "x") == false);
}

TEST_CASE("EndsWithIgnoreCase", "[str_utils]") {
    REQUIRE(zeta::EndsWithIgnoreCase("Hello", "LO"));
    REQUIRE(zeta::EndsWithIgnoreCase("HELLO", "lo"));
    REQUIRE(!zeta::EndsWithIgnoreCase("hello", "XLO"));
}

TEST_CASE("EndsWithIgnoreCase: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::EndsWithIgnoreCase("", "") == true);
    REQUIRE(zeta::EndsWithIgnoreCase("ABC", "BC") == true);
    REQUIRE(zeta::EndsWithIgnoreCase("ABC", "bc") == true);
}

// ═══════════════════════════════════════════════════════════════════
// Stripping
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StripPrefix", "[str_utils]") {
    REQUIRE(zeta::StripPrefix("--flag", "--") == "flag");
    REQUIRE(zeta::StripPrefix("hello", "hel") == "lo");
    REQUIRE(zeta::StripPrefix("abc", "xyz") == "abc"); // unchanged
}

TEST_CASE("StripPrefix: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StripPrefix("abc", "abc") == "");     // full match
    REQUIRE(zeta::StripPrefix("abc", "") == "abc");     // empty prefix
    REQUIRE(zeta::StripPrefix("", "") == "");           // both empty
    REQUIRE(zeta::StripPrefix("", "a") == "");          // empty text
}

TEST_CASE("StripSuffix", "[str_utils]") {
    REQUIRE(zeta::StripSuffix("file.txt", ".txt") == "file");
    REQUIRE(zeta::StripSuffix("hello", "lo") == "hel");
    REQUIRE(zeta::StripSuffix("abc", "xyz") == "abc");
}

TEST_CASE("StripSuffix: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StripSuffix("abc", "abc") == "");     // full match
    REQUIRE(zeta::StripSuffix("abc", "") == "abc");     // empty suffix
    REQUIRE(zeta::StripSuffix("", "") == "");           // both empty
    REQUIRE(zeta::StripSuffix("", "a") == "");          // empty text
}

TEST_CASE("StripAsciiWhitespace", "[str_utils]") {
    REQUIRE(zeta::StripAsciiWhitespace("  hello  ") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("\t\n hello \r\n") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("hello") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("   ") == "");
}

TEST_CASE("StripAsciiWhitespace: edge and individual chars", "[str_utils][edge]") {
    // Each ASCII whitespace character individually
    REQUIRE(zeta::StripAsciiWhitespace(" hello") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("hello ") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("\thello") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("hello\n") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("\rhello") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("hello\v") == "hello");
    REQUIRE(zeta::StripAsciiWhitespace("\fhello") == "hello");

    // Nothing but whitespace in the middle
    REQUIRE(zeta::StripAsciiWhitespace("a  b") == "a  b");

    // Only whitespace variants
    REQUIRE(zeta::StripAsciiWhitespace("\t\n\r\v\f ") == "");
}

TEST_CASE("StripAsciiWhitespace: idempotent", "[str_utils][property]") {
    // Applying twice yields same result.
    // Note: keep the intermediate string alive — StripAsciiWhitespace
    // returns string_view, so a temporary std::string would leave a
    // dangling view (caught by AddressSanitizer).
    auto r1 = zeta::StripAsciiWhitespace("  hello world  ");
    std::string s(r1);
    auto r2 = zeta::StripAsciiWhitespace(s);
    REQUIRE(r1 == r2);
}

// ── Untested in original ──────────────────────────────────────────
TEST_CASE("StripLeadingAsciiWhitespace", "[str_utils]") {
    REQUIRE(zeta::StripLeadingAsciiWhitespace("  hello  ") == "hello  ");
    REQUIRE(zeta::StripLeadingAsciiWhitespace("\thello") == "hello");
    REQUIRE(zeta::StripLeadingAsciiWhitespace("hello") == "hello");
    REQUIRE(zeta::StripLeadingAsciiWhitespace("   ") == "");
    REQUIRE(zeta::StripLeadingAsciiWhitespace("a b") == "a b");
}

TEST_CASE("StripTrailingAsciiWhitespace", "[str_utils]") {
    REQUIRE(zeta::StripTrailingAsciiWhitespace("  hello  ") == "  hello");
    REQUIRE(zeta::StripTrailingAsciiWhitespace("hello\t") == "hello");
    REQUIRE(zeta::StripTrailingAsciiWhitespace("hello") == "hello");
    REQUIRE(zeta::StripTrailingAsciiWhitespace("   ") == "");
    REQUIRE(zeta::StripTrailingAsciiWhitespace("a b") == "a b");
}

// ═══════════════════════════════════════════════════════════════════
// Case conversion
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("AsciiStrToLower / Upper", "[str_utils]") {
    REQUIRE(zeta::AsciiStrToLower("Hello World!") == "hello world!");
    REQUIRE(zeta::AsciiStrToUpper("Hello World!") == "HELLO WORLD!");
    REQUIRE(zeta::AsciiStrToLower("123") == "123");
}

TEST_CASE("AsciiStrToLower: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::AsciiStrToLower("") == "");
    REQUIRE(zeta::AsciiStrToLower("ALREADY") == "already");
    REQUIRE(zeta::AsciiStrToLower("123!@#") == "123!@#");
    REQUIRE(zeta::AsciiStrToLower("Mixed") == "mixed");
}

TEST_CASE("AsciiStrToUpper: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::AsciiStrToUpper("") == "");
    REQUIRE(zeta::AsciiStrToUpper("already") == "ALREADY");
    REQUIRE(zeta::AsciiStrToUpper("123!@#") == "123!@#");
    REQUIRE(zeta::AsciiStrToUpper("Mixed") == "MIXED");
}

TEST_CASE("AsciiStrToLower round-trip invariant", "[str_utils][property]") {
    // AsciiStrToLower is idempotent
    std::string original = "Hello World! 123";
    std::string once  = zeta::AsciiStrToLower(original);
    std::string twice = zeta::AsciiStrToLower(once);
    REQUIRE(once == twice);
}

TEST_CASE("AsciiStrToUpper round-trip invariant", "[str_utils][property]") {
    // AsciiStrToUpper is idempotent
    std::string original = "Hello World! 123";
    std::string once  = zeta::AsciiStrToUpper(original);
    std::string twice = zeta::AsciiStrToUpper(once);
    REQUIRE(once == twice);
}

// ═══════════════════════════════════════════════════════════════════
// Replacement
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrReplaceFirst", "[str_utils]") {
    REQUIRE(zeta::StrReplaceFirst("hello world", "world", "there") == "hello there");
    REQUIRE(zeta::StrReplaceFirst("a,b,c", ",", "|") == "a|b,c");
    REQUIRE(zeta::StrReplaceFirst("abc", "xyz", "123") == "abc");
}

TEST_CASE("StrReplaceFirst: edge cases", "[str_utils][edge]") {
    // Empty from — always matches at position 0
    // (behavior is implementation-defined, but should not crash)
    auto r = zeta::StrReplaceFirst("hello", "", "x");
    REQUIRE(r.find("x") == 0);

    REQUIRE(zeta::StrReplaceFirst("abc", "abc", "") == "");  // full replace with empty
    REQUIRE(zeta::StrReplaceFirst("abc", "abc", "abc") == "abc"); // no-op
    REQUIRE(zeta::StrReplaceFirst("abc", "x", "y") == "abc");     // no match
}

TEST_CASE("StrReplaceAll", "[str_utils]") {
    REQUIRE(zeta::StrReplaceAll("a,b,c", ",", "|") == "a|b|c");
    REQUIRE(zeta::StrReplaceAll("hello world world", "world", "there") == "hello there there");
    REQUIRE(zeta::StrReplaceAll("abc", "xyz", "123") == "abc");
    REQUIRE(zeta::StrReplaceAll("aaa", "a", "aa") == "aaaaaa");
}

TEST_CASE("StrReplaceAll: edge cases", "[str_utils][edge]") {
    REQUIRE(zeta::StrReplaceAll("abc", "", "x") == "abc");   // empty from (special-cased)
    REQUIRE(zeta::StrReplaceAll("abc", "abc", "") == "");    // full replace with empty
    REQUIRE(zeta::StrReplaceAll("abc", "abc", "abc") == "abc");     // no-op
    REQUIRE(zeta::StrReplaceAll("abc", "x", "y") == "abc");         // no match
    REQUIRE(zeta::StrReplaceAll("a", "a", "xyz") == "xyz");         // single element
    REQUIRE(zeta::StrReplaceAll("aa", "a", "a") == "aa");           // no-growth replace
}

TEST_CASE("StrReplaceAll idempotent when from==to", "[str_utils][property]") {
    std::string s = "hello world foo bar";
    REQUIRE(zeta::StrReplaceAll(s, "o", "o") == s);
    REQUIRE(zeta::StrReplaceAll(s, "foo", "foo") == s);
    REQUIRE(zeta::StrReplaceAll(s, "xyz", "xyz") == s);
}
