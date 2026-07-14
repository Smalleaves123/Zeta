#include "zeta/functional/compose.h"
#include "zeta/functional/overload.h"
#include "zeta/functional/pipe.h"
#include "zeta/memory/any_invocable.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>

TEST_CASE("functional: overload dispatches variant alternatives", "[functional]") {
    const std::variant<int, std::string> value = std::string("zeta");
    const auto result = std::visit(
        zeta::Overload{
            [](int number) { return std::to_string(number); },
            [](const std::string& text) { return text + "!"; },
        },
        value);

    REQUIRE(result == "zeta!");
}

TEST_CASE("functional: compose applies functions right to left", "[functional]") {
    const auto add_one = [](int value) { return value + 1; };
    const auto double_value = [](int value) { return value * 2; };
    const auto format = [](int value) { return std::to_string(value); };

    const auto composed = zeta::compose(format, double_value, add_one);
    REQUIRE(composed(4) == "10");
}

TEST_CASE("functional: pipe applies functions left to right", "[functional]") {
    const auto result = zeta::pipe(
        4,
        [](int value) { return value + 1; },
        [](int value) { return value * 2; },
        [](int value) { return std::to_string(value); });

    REQUIRE(result == "10");
}

TEST_CASE("functional: composition supports move-only callables", "[functional]") {
    auto add = zeta::AnyInvocable<int(int)>([](int value) { return value + 3; });
    auto multiply = zeta::AnyInvocable<int(int)>([](int value) { return value * 2; });
    auto composed = zeta::compose(std::move(multiply), std::move(add));

    REQUIRE(composed(5) == 16);
}
