#include "zeta/memory/span.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

TEST_CASE("span: construct from array", "[span]") {
    int data[] = {1, 2, 3, 4};
    zeta::Span<int> s(data);

    REQUIRE(s.size() == 4);
    REQUIRE(s.front() == 1);
    REQUIRE(s.back() == 4);
}

TEST_CASE("span: construct from vector", "[span]") {
    std::vector<int> v = {10, 20, 30};
    zeta::Span<int> s(v);

    REQUIRE(s.size() == 3);
    REQUIRE(s[1] == 20);
    s[2] = 99;
    REQUIRE(v[2] == 99);
}

TEST_CASE("span: const view", "[span]") {
    std::array<int, 3> a = {7, 8, 9};
    zeta::Span<const int> s(a);

    REQUIRE(s.size() == 3);
    REQUIRE(s[0] == 7);
}

TEST_CASE("span: subspan operations", "[span]") {
    std::array<int, 5> a = {1, 2, 3, 4, 5};
    zeta::Span<int> s(a);

    auto mid = s.subspan(1, 3);
    REQUIRE(mid.size() == 3);
    REQUIRE(mid[0] == 2);
    REQUIRE(mid[2] == 4);

    auto first_two = s.first<2>();
    auto last_two = s.last<2>();
    REQUIRE(first_two[0] == 1);
    REQUIRE(first_two[1] == 2);
    REQUIRE(last_two[0] == 4);
    REQUIRE(last_two[1] == 5);
}

TEST_CASE("span: empty span", "[span]") {
    zeta::Span<int> s;
    REQUIRE(s.empty());
    REQUIRE(s.data() == nullptr);
}
