#include "zeta/memory/bind_front.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

int Sum3(int a, int b, int c) {
    return a + b + c;
}

struct Accumulator {
    int Add(int base, int extra) const {
        return seed + base + extra;
    }

    int seed = 0;
};

} // namespace

TEST_CASE("bind_front: binds leading arguments of free function", "[bind_front]") {
    auto add = zeta::bind_front(Sum3, 10, 20);
    REQUIRE(add(12) == 42);
}

TEST_CASE("bind_front: supports member function pointers", "[bind_front]") {
    Accumulator acc{20};
    auto bound = zeta::bind_front(&Accumulator::Add, &acc, 10);
    REQUIRE(bound(12) == 42);
}

TEST_CASE("bind_front: supports move-only bound state", "[bind_front][move]") {
    auto bound = zeta::bind_front(
        [](std::unique_ptr<int> value, int extra) { return *value + extra; },
        std::make_unique<int>(40));

    REQUIRE(std::move(bound)(2) == 42);
}

TEST_CASE("bind_front: preserves std::ref bindings", "[bind_front]") {
    std::string text = "hello";
    auto append = zeta::bind_front(
        [](std::string& target, std::string_view suffix) { target += suffix; },
        std::ref(text));

    append("!");
    REQUIRE(text == "hello!");
}

TEST_CASE("bind_front: works with mutable lambdas", "[bind_front]") {
    auto counter = zeta::bind_front(
        [count = 40](int delta) mutable {
            count += delta;
            return count;
        });

    REQUIRE(counter(1) == 41);
    REQUIRE(counter(1) == 42);
}
