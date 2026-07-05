#include "zeta/container/fixed_array.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

struct ThrowingItem {
    static inline int live_count = 0;
    static inline int copy_calls = 0;
    static inline int copy_throw_after = -1;

    int value = 0;

    ThrowingItem() { ++live_count; }
    explicit ThrowingItem(int v) : value(v) { ++live_count; }

    ThrowingItem(const ThrowingItem& other) : value(other.value) {
        if (copy_throw_after >= 0 && copy_calls++ >= copy_throw_after) {
            throw std::runtime_error("copy");
        }
        ++live_count;
    }

    ThrowingItem(ThrowingItem&& other) noexcept : value(other.value) {
        ++live_count;
        other.value = -1;
    }

    ThrowingItem& operator=(const ThrowingItem&) = default;
    ThrowingItem& operator=(ThrowingItem&&) = default;

    ~ThrowingItem() { --live_count; }

    static void Reset() {
        live_count = 0;
        copy_calls = 0;
        copy_throw_after = -1;
    }
};

} // namespace

TEST_CASE("FixedArray: default construction is empty", "[fixed_array]") {
    zeta::FixedArray<int, 4> values;
    REQUIRE(values.empty());
    REQUIRE(values.size() == 0);
}

TEST_CASE("FixedArray: size constructor default-initializes elements", "[fixed_array]") {
    zeta::FixedArray<int, 4> values(3);
    REQUIRE(values.size() == 3);
    REQUIRE(values[0] == 0);
    REQUIRE(values[2] == 0);
}

TEST_CASE("FixedArray: fill constructor copies a value", "[fixed_array]") {
    zeta::FixedArray<std::string, 4> values(3, "hi");
    REQUIRE(values.size() == 3);
    REQUIRE(values.front() == "hi");
    REQUIRE(values.back() == "hi");
}

TEST_CASE("FixedArray: initializer_list construction", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3, 4};
    REQUIRE(values.size() == 4);
    REQUIRE(values[1] == 2);
    REQUIRE(values[3] == 4);
}

TEST_CASE("FixedArray: at throws out of range", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3};
    REQUIRE(values.at(2) == 3);
    REQUIRE_THROWS_AS(values.at(3), std::out_of_range);
}

TEST_CASE("FixedArray: iteration works", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3, 4};
    int sum = std::accumulate(values.begin(), values.end(), 0);
    REQUIRE(sum == 10);
}

TEST_CASE("FixedArray: reverse iteration works", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3};
    std::vector<int> reversed(values.rbegin(), values.rend());
    REQUIRE(reversed == std::vector<int>{3, 2, 1});
}

TEST_CASE("FixedArray: copy construction preserves values", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3};
    zeta::FixedArray<int, 4> copy(values);
    REQUIRE(copy == values);
}

TEST_CASE("FixedArray: copy assignment preserves values", "[fixed_array]") {
    zeta::FixedArray<int, 4> values = {1, 2, 3};
    zeta::FixedArray<int, 4> copy;
    copy = values;
    REQUIRE(copy == values);
}

TEST_CASE("FixedArray: move construction supports move-only types", "[fixed_array][move]") {
    zeta::FixedArray<std::unique_ptr<int>, 2> values(3);
    values[0] = std::make_unique<int>(1);
    values[1] = std::make_unique<int>(2);
    values[2] = std::make_unique<int>(42);

    zeta::FixedArray<std::unique_ptr<int>, 2> moved(std::move(values));
    REQUIRE(moved.size() == 3);
    REQUIRE(*moved[2] == 42);
}

TEST_CASE("FixedArray: fill updates all elements", "[fixed_array]") {
    zeta::FixedArray<int, 4> values(3, 1);
    values.fill(7);
    REQUIRE(values[0] == 7);
    REQUIRE(values[1] == 7);
    REQUIRE(values[2] == 7);
}

TEST_CASE("FixedArray: swap exchanges contents", "[fixed_array]") {
    zeta::FixedArray<int, 4> a = {1, 2};
    zeta::FixedArray<int, 4> b = {3, 4, 5};
    swap(a, b);
    REQUIRE(a == zeta::FixedArray<int, 4>({3, 4, 5}));
    REQUIRE(b == zeta::FixedArray<int, 4>({1, 2}));
}

TEST_CASE("FixedArray: move-only types are not copyable", "[fixed_array][compile]") {
    static_assert(!std::is_copy_constructible_v<zeta::FixedArray<std::unique_ptr<int>, 4>>);
    static_assert(!std::is_copy_assignable_v<zeta::FixedArray<std::unique_ptr<int>, 4>>);
}

TEST_CASE("FixedArray: construction cleans up on copy throw", "[fixed_array][exception]") {
    ThrowingItem::Reset();
    ThrowingItem::copy_throw_after = 1;
    REQUIRE_THROWS_AS((zeta::FixedArray<ThrowingItem, 2>(3, ThrowingItem(7))), std::runtime_error);
    REQUIRE(ThrowingItem::live_count == 0);
}
