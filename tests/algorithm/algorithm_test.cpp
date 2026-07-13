#include "zeta/algorithm/algorithm.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <iterator>
#include <numeric>
#include <vector>

TEST_CASE("algorithm: search and predicates accept containers", "[algorithm]") {
    const std::array<int, 5> values{1, 2, 3, 4, 5};

    REQUIRE(zeta::c_contains(values, 3));
    REQUIRE(!zeta::c_contains(values, 9));
    REQUIRE(zeta::c_count_if(values, [](int value) { return value % 2 == 1; }) == 3);
    REQUIRE(zeta::c_all_of(values, [](int value) { return value > 0; }));
    REQUIRE(zeta::c_any_of(values, [](int value) { return value == 4; }));
    REQUIRE(zeta::c_none_of(values, [](int value) { return value < 0; }));
}

TEST_CASE("algorithm: sorting and bounds work without begin/end spelling",
          "[algorithm]") {
    std::vector<int> values{5, 1, 4, 2, 3};
    zeta::c_sort(values);

    REQUIRE(zeta::c_is_sorted(values));
    REQUIRE(zeta::c_binary_search(values, 4));
    REQUIRE(*zeta::c_lower_bound(values, 3) == 3);
    REQUIRE(*zeta::c_upper_bound(values, 3) == 4);
}

TEST_CASE("algorithm: copy, transform, accumulate, and iota compose",
          "[algorithm]") {
    std::array<int, 4> values{};
    zeta::c_iota(values, 1);

    std::vector<int> doubled;
    doubled.resize(values.size());
    zeta::c_transform(values, doubled.begin(), [](int value) { return value * 2; });

    REQUIRE(zeta::c_equal(doubled, std::vector<int>{2, 4, 6, 8}));
    REQUIRE(zeta::c_accumulate(doubled, 0) == 20);

    std::vector<int> copied;
    zeta::c_copy_if(doubled, std::back_inserter(copied),
                    [](int value) { return value >= 4; });
    REQUIRE(copied == std::vector<int>{4, 6, 8});
}
