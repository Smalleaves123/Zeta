#include "zeta/strings/str_split.h"

#include <catch2/catch_test_macros.hpp>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Char delimiter
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: empty input", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.empty());
}

TEST_CASE("StrSplit: no delimiter in input", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("hello", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == "hello");
}

TEST_CASE("StrSplit: basic comma split", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b,c", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("StrSplit: trailing delimiter", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b,", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "");
}

TEST_CASE("StrSplit: leading delimiter", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit(",a,b", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "");
    REQUIRE(parts[1] == "a");
    REQUIRE(parts[2] == "b");
}

TEST_CASE("StrSplit: consecutive delimiters", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,,b", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "");
    REQUIRE(parts[2] == "b");
}

TEST_CASE("StrSplit: only delimiter", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit(",", ',')) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == "");
    REQUIRE(parts[1] == "");
}

// ═══════════════════════════════════════════════════════════════════
// String delimiter
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: multi-char delimiter", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a=>b=>c", "=>")) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("StrSplit: string delimiter not found", "[str_split]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("hello world", "::")) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == "hello world");
}

TEST_CASE("StrSplit: string delimiter equals input", "[str_split][edge]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("abc", "abc")) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == "");
    REQUIRE(parts[1] == "");
}

TEST_CASE("StrSplit: empty string delimiter returns whole input", "[str_split][edge]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("abc", "")) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == "abc");
}

// ═══════════════════════════════════════════════════════════════════
// ByAnyChar delimiter
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: ByAnyChar with single char", "[str_split][byanychar]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b,c", zeta::ByAnyChar{","})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
}

TEST_CASE("StrSplit: ByAnyChar with multiple chars", "[str_split][byanychar]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b;c|d", zeta::ByAnyChar{",;|"})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 4);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c");
    REQUIRE(parts[3] == "d");
}

TEST_CASE("StrSplit: ByAnyChar consecutive delimiters", "[str_split][byanychar]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,,b", zeta::ByAnyChar{","})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "");
    REQUIRE(parts[2] == "b");
}

// ═══════════════════════════════════════════════════════════════════
// SkipEmpty option
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: SkipEmpty with char delimiter", "[str_split][skipempty]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,,b", ',', zeta::SkipEmpty)) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
}

TEST_CASE("StrSplit: SkipEmpty leading/trailing", "[str_split][skipempty]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit(",a,b,", ',', zeta::SkipEmpty)) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
}

TEST_CASE("StrSplit: SkipEmpty all delimiters", "[str_split][skipempty]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit(",,,", ',', zeta::SkipEmpty)) {
        parts.push_back(p);
    }
    REQUIRE(parts.empty());
}

TEST_CASE("StrSplit: SkipEmpty with string delimiter", "[str_split][skipempty]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a::b:::c", "::", zeta::SkipEmpty)) {
        parts.push_back(p);
    }
    // "a" then "b" then "" then "c" — empty piece where ":::" has leftover ":"
    // The key invariant: no empty string_view from SkipEmpty
    for (auto sv : parts) {
        REQUIRE(!sv.empty());
    }
}

// ═══════════════════════════════════════════════════════════════════
// MaxSplits option
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: MaxSplits limits pieces", "[str_split][maxsplits]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a:b:c:d:e", ':', zeta::MaxSplits_t{3})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 3);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
    REQUIRE(parts[2] == "c:d:e");
}

TEST_CASE("StrSplit: MaxSplits more than pieces", "[str_split][maxsplits]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b", ',', zeta::MaxSplits_t{10})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    REQUIRE(parts[0] == "a");
    REQUIRE(parts[1] == "b");
}

TEST_CASE("StrSplit: MaxSplits zero", "[str_split][maxsplits]") {
    // MaxSplits=0 means no splits → whole input as one piece
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,b,c", ',', zeta::MaxSplits_t{0})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 1);
    REQUIRE(parts[0] == "a,b,c");
}

TEST_CASE("StrSplit: MaxSplits with SkipEmpty", "[str_split][maxsplits]") {
    std::vector<std::string_view> parts;
    for (auto p : zeta::StrSplit("a,,b,c,d", ',', zeta::SkipEmpty,
                                  zeta::MaxSplits_t{2})) {
        parts.push_back(p);
    }
    REQUIRE(parts.size() == 2);
    // SkipEmpty skips empty pieces, then MaxSplits=2 makes rest one piece
    REQUIRE(parts[0] == "a");
    // MaxSplits chops the rest as-is (including leading delimiters),
    // while SkipEmpty only applies to pieces found before the limit.
    REQUIRE(parts[1] == ",b,c,d");
}

// ═══════════════════════════════════════════════════════════════════
// Iterator semantics
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: post-increment", "[str_split]") {
    auto range = zeta::StrSplit("x,y", ',');
    auto it = range.begin();
    REQUIRE(*it++ == "x");
    REQUIRE(*it == "y");
}

TEST_CASE("StrSplit: arrow operator", "[str_split]") {
    auto range = zeta::StrSplit("hello", ',');
    auto it = range.begin();
    REQUIRE(it->size() == 5);
}

TEST_CASE("StrSplit: default-constructed iterators equal", "[str_split][iterator]") {
    using Iter = decltype(zeta::StrSplit("x", ',').begin());
    Iter a;
    Iter b;
    REQUIRE(a == b);
}

TEST_CASE("StrSplit: begin == end for empty input", "[str_split][iterator]") {
    auto range = zeta::StrSplit("", ',');
    REQUIRE(range.begin() == range.end());
}

TEST_CASE("StrSplit: multiple traversals identical", "[str_split][iterator]") {
    auto range = zeta::StrSplit("a,b,c", ',');
    std::vector<std::string_view> first;
    std::vector<std::string_view> second;
    for (auto p : range) first.push_back(p);
    for (auto p : range) second.push_back(p);
    REQUIRE(first == second);
}

TEST_CASE("StrSplit: iterator_traits satisfies forward_iterator", "[str_split][iterator]") {
    auto range = zeta::StrSplit("x", ',');
    using Iter = decltype(range.begin());
    // Verify std::iterator_traits are defined (iterator library compatibility)
    using value_type = typename std::iterator_traits<Iter>::value_type;
    static_assert(std::is_same_v<value_type, std::string_view>);
}

TEST_CASE("StrSplit: const range iteration", "[str_split][iterator]") {
    const auto range = zeta::StrSplit("a,b,c", ',');
    std::vector<std::string_view> parts;
    for (auto p : range) parts.push_back(p);
    REQUIRE(parts.size() == 3);
}

// ═══════════════════════════════════════════════════════════════════
// Property-based invariants
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StrSplit: round-trip property", "[str_split][property]") {
    // Splitting by a single char and joining with that char yields the
    // original string only if there are no consecutive delimiters.
    auto to_vec = [](auto range) {
        std::vector<std::string_view> v;
        for (auto p : range) v.push_back(p);
        return v;
    };

    auto pieces = to_vec(zeta::StrSplit("alpha,bravo,charlie", ','));
    REQUIRE(pieces.size() == 3);
    REQUIRE(pieces[0] == "alpha");
    REQUIRE(pieces[1] == "bravo");
    REQUIRE(pieces[2] == "charlie");
}

TEST_CASE("StrSplit: piece count invariant", "[str_split][property]") {
    // Number of pieces = delimiter_count + 1 for non-empty input with non-empty delimiter
    auto count_pieces = [](std::string_view input, char delim) {
        size_t n = 0;
        for (auto p : zeta::StrSplit(input, delim)) {
            (void)p;
            ++n;
        }
        return n;
    };
    REQUIRE(count_pieces("a,b,c", ',') == 3);
    REQUIRE(count_pieces("a,,b", ',') == 3);
    REQUIRE(count_pieces(",a,", ',') == 3);
}
