#include "zeta/strings/str_join.h"

#include <catch2/catch_test_macros.hpp>
#include <array>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Basic
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: empty range", "[str_join]") {
    std::vector<std::string> v;
    REQUIRE(zeta::StrJoin(v, ", ") == "");
}

TEST_CASE("StrJoin: single element", "[str_join]") {
    std::vector<std::string> v = {"hello"};
    REQUIRE(zeta::StrJoin(v, ", ") == "hello");
}

TEST_CASE("StrJoin: multiple elements", "[str_join]") {
    std::vector<std::string> v = {"a", "b", "c"};
    REQUIRE(zeta::StrJoin(v, ", ") == "a, b, c");
}

TEST_CASE("StrJoin: ints", "[str_join]") {
    std::vector<int> v = {1, 2, 3};
    REQUIRE(zeta::StrJoin(v, ", ") == "1, 2, 3");
}

TEST_CASE("StrJoin: empty delimiter", "[str_join]") {
    std::vector<std::string> v = {"a", "b", "c"};
    REQUIRE(zeta::StrJoin(v, "") == "abc");
}

TEST_CASE("StrJoin: custom delimiter", "[str_join]") {
    std::vector<std::string> v = {"x", "y", "z"};
    REQUIRE(zeta::StrJoin(v, "::") == "x::y::z");
}

// ═══════════════════════════════════════════════════════════════════
// Element type coverage
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: string_view elements", "[str_join][types]") {
    std::vector<std::string_view> v = {"a", "b", "c"};
    REQUIRE(zeta::StrJoin(v, ",") == "a,b,c");
}

TEST_CASE("StrJoin: const char* elements", "[str_join][types]") {
    std::vector<const char*> v = {"x", "y", "z"};
    REQUIRE(zeta::StrJoin(v, "") == "xyz");
}

TEST_CASE("StrJoin: char elements", "[str_join][types]") {
    std::vector<char> v = {'a', 'b', 'c'};
    REQUIRE(zeta::StrJoin(v, ",") == "a,b,c");
}

TEST_CASE("StrJoin: double elements", "[str_join][types]") {
    std::vector<double> v = {1.5, 2.5};
    auto r = zeta::StrJoin(v, ",");
    REQUIRE(r.find("1.5") != std::string::npos);
    REQUIRE(r.find("2.5") != std::string::npos);
}

TEST_CASE("StrJoin: unsigned elements", "[str_join][types]") {
    std::vector<unsigned> v = {1, 2, 3};
    REQUIRE(zeta::StrJoin(v, ",") == "1,2,3");
}

// ═══════════════════════════════════════════════════════════════════
// Range type coverage
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: C-array", "[str_join][range]") {
    int arr[] = {1, 2, 3};
    REQUIRE(zeta::StrJoin(arr, ",") == "1,2,3");
}

TEST_CASE("StrJoin: std::array", "[str_join][range]") {
    std::array<int, 4> a = {1, 2, 3, 4};
    REQUIRE(zeta::StrJoin(a, "-") == "1-2-3-4");
}

TEST_CASE("StrJoin: std::list", "[str_join][range]") {
    std::list<std::string> l = {"a", "b", "c"};
    REQUIRE(zeta::StrJoin(l, " ") == "a b c");
}

TEST_CASE("StrJoin: std::set", "[str_join][range]") {
    std::set<int> s = {3, 1, 2};
    // set is sorted, so 1,2,3
    REQUIRE(zeta::StrJoin(s, ",") == "1,2,3");
}

// ═══════════════════════════════════════════════════════════════════
// Edge cases
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: empty initializer_list", "[str_join][edge]") {
    REQUIRE(zeta::StrJoin(std::initializer_list<int>{}, ",") == "");
}

TEST_CASE("StrJoin: single char delimiter", "[str_join][edge]") {
    std::vector<std::string> v = {"hello", "world"};
    REQUIRE(zeta::StrJoin(v, ",") == "hello,world");
}

TEST_CASE("StrJoin: delimiter longer than result", "[str_join][edge]") {
    std::vector<std::string> v = {"a"};
    REQUIRE(zeta::StrJoin(v, "very long delimiter") == "a");
}

// ═══════════════════════════════════════════════════════════════════
// initializer_list overload
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: initializer_list", "[str_join]") {
    REQUIRE(zeta::StrJoin({"a", "b", "c"}, "-") == "a-b-c");
}

// ═══════════════════════════════════════════════════════════════════
// Property-based invariants
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrJoin: empty range yields empty string", "[str_join][property]") {
    std::vector<int> empty;
    REQUIRE(zeta::StrJoin(empty, ",") == "");
    REQUIRE(zeta::StrJoin(empty, "") == "");
    REQUIRE(zeta::StrJoin(empty, "xyz") == "");
}

TEST_CASE("StrJoin: single element yields element itself", "[str_join][property]") {
    std::vector<int> v = {42};
    REQUIRE(zeta::StrJoin(v, "anything") == "42");
    REQUIRE(zeta::StrJoin(v, "") == "42");
}

TEST_CASE("StrJoin: delimiter count invariant", "[str_join][property]") {
    std::vector<std::string> v = {"a", "b", "c", "d"};
    std::string result = zeta::StrJoin(v, "||");
    // delimiter appears (n-1) times
    size_t count = 0;
    for (size_t pos = 0; (pos = result.find("||", pos)) != std::string::npos; pos += 2) {
        ++count;
    }
    REQUIRE(count == v.size() - 1);
}

TEST_CASE("StrJoin: first and last element boundaries", "[str_join][property]") {
    std::vector<std::string> v = {"alpha", "beta", "omega"};
    std::string result = zeta::StrJoin(v, ":");
    REQUIRE(result.starts_with("alpha"));
    REQUIRE(result.ends_with("omega"));
}
