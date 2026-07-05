#include "zeta/memory/any_invocable.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace {

struct MoveOnlyFunctor {
    explicit MoveOnlyFunctor(std::unique_ptr<int> value)
        : value(std::move(value)) {}

    MoveOnlyFunctor(const MoveOnlyFunctor&) = delete;
    MoveOnlyFunctor& operator=(const MoveOnlyFunctor&) = delete;
    MoveOnlyFunctor(MoveOnlyFunctor&&) noexcept = default;
    MoveOnlyFunctor& operator=(MoveOnlyFunctor&&) noexcept = default;

    int operator()(int x) const { return *value + x; }

    std::unique_ptr<int> value;
};

} // namespace

TEST_CASE("AnyInvocable: stores and invokes move-only functor", "[any_invocable]") {
    zeta::AnyInvocable<int(int)> fn(MoveOnlyFunctor(std::make_unique<int>(40)));
    REQUIRE(fn(2) == 42);
}

TEST_CASE("AnyInvocable: move construction transfers ownership", "[any_invocable][move]") {
    zeta::AnyInvocable<int(int)> first = [offset = std::make_unique<int>(10)](int x) {
        return x + *offset;
    };

    zeta::AnyInvocable<int(int)> second(std::move(first));
    REQUIRE(!first);
    REQUIRE(second(32) == 42);
}

TEST_CASE("AnyInvocable: move assignment replaces target", "[any_invocable][move]") {
    zeta::AnyInvocable<int()> lhs = [] { return 1; };
    zeta::AnyInvocable<int()> rhs = [] { return 42; };

    lhs = std::move(rhs);

    REQUIRE(!rhs);
    REQUIRE(lhs() == 42);
}

TEST_CASE("AnyInvocable: reset clears callable", "[any_invocable]") {
    zeta::AnyInvocable<void()> fn = [] {};
    REQUIRE(static_cast<bool>(fn));
    fn.Reset();
    REQUIRE(!fn);
}

TEST_CASE("AnyInvocable: in_place construction works", "[any_invocable]") {
    zeta::AnyInvocable<int(int)> fn(std::in_place_type<MoveOnlyFunctor>,
                                    std::make_unique<int>(20));
    REQUIRE(fn(22) == 42);
}

TEST_CASE("AnyInvocable: noexcept specialization accepts safe callable", "[any_invocable][noexcept]") {
    zeta::AnyInvocable<int(int) noexcept> fn = [](int x) noexcept { return x * 2; };
    static_assert(noexcept(fn(1)));
    REQUIRE(fn(21) == 42);
}

TEST_CASE("AnyInvocable: noexcept specialization rejects throwing callable", "[any_invocable][compile]") {
    auto maybe_throw = [](int x) { return x; };
    REQUIRE(!std::is_constructible_v<
        zeta::AnyInvocable<int(int) noexcept>,
        decltype(maybe_throw)>);
}

TEST_CASE("AnyInvocable: move-only semantics are enforced", "[any_invocable][compile]") {
    static_assert(!std::is_copy_constructible_v<zeta::AnyInvocable<int()>>);
    static_assert(!std::is_copy_assignable_v<zeta::AnyInvocable<int()>>);
    static_assert(std::is_move_constructible_v<zeta::AnyInvocable<int()>>);
}

TEST_CASE("AnyInvocable: large callable spills beyond inline storage", "[any_invocable]") {
    struct LargeCallable {
        char payload[128]{};
        int operator()(int x) const { return x + 1; }
    };

    zeta::AnyInvocable<int(int)> fn = LargeCallable{};
    REQUIRE(fn(41) == 42);
}
