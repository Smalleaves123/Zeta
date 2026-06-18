#include "zeta/cleanup/cleanup.h"

#include <catch2/catch_test_macros.hpp>

#include <functional>
#include <memory>
#include <type_traits>

// ═══════════════════════════════════════════════════════════════════════
// Basic behaviour
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("cleanup: invokes callable on destruction", "[cleanup][basic]") {
    int count = 0;
    {
        auto guard = zeta::MakeCleanup([&] { ++count; });
        REQUIRE(count == 0);
        REQUIRE(guard.IsActive());
    }
    REQUIRE(count == 1);
}

TEST_CASE("cleanup: manual Invoke", "[cleanup][basic]") {
    int count = 0;
    {
        auto guard = zeta::MakeCleanup([&] { ++count; });
        REQUIRE(guard.IsActive());
        guard.Invoke();
        REQUIRE(!guard.IsActive());
        REQUIRE(count == 1);
    }
    // No second invocation.
    REQUIRE(count == 1);
}

TEST_CASE("cleanup: Cancel dismisses guard", "[cleanup][basic]") {
    int count = 0;
    {
        auto guard = zeta::MakeCleanup([&] { ++count; });
        guard.Cancel();
        REQUIRE(!guard.IsActive());
    }
    REQUIRE(count == 0);
}

TEST_CASE("cleanup: double Cancel is safe", "[cleanup][basic]") {
    int count = 0;
    auto guard = zeta::MakeCleanup([&] { ++count; });
    guard.Cancel();
    guard.Cancel(); // no-op
    REQUIRE(!guard.IsActive());
    guard.Invoke();  // also no-op
    REQUIRE(count == 0);
}

// ═══════════════════════════════════════════════════════════════════════
// Move semantics
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("cleanup: move construction transfers responsibility", "[cleanup][move]") {
    int count = 0;
    {
        auto g1 = zeta::MakeCleanup([&] { ++count; });
        REQUIRE(g1.IsActive());
        auto g2 = std::move(g1);
        REQUIRE(!g1.IsActive());  // moved-from is inactive
        REQUIRE(g2.IsActive());
    }
    REQUIRE(count == 1);
}

TEST_CASE("cleanup: move assignment", "[cleanup][move]") {
    int a = 0, b = 0;
    {
        // Use std::function to get the same concrete type for both guards.
        zeta::Cleanup<std::function<void()>> g1([&a] { ++a; });
        zeta::Cleanup<std::function<void()>> g2([&b] { ++b; });
        REQUIRE(g1.IsActive());
        REQUIRE(g2.IsActive());
        g2 = std::move(g1);
        // g1's callable should now be in g2; old g2's callable was invoked.
        REQUIRE(b == 1);
        REQUIRE(!g1.IsActive());
    }
    // g2 destroyed → a incremented.
    REQUIRE(a == 1);
}

// ═══════════════════════════════════════════════════════════════════════
// Type-erasure
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("cleanup: stores stateful lambda", "[cleanup][types]") {
    int val = 0;
    auto adder = zeta::MakeCleanup([x = 5, &val] { val += x; });
    // Let it destruct.
    {
        auto g = std::move(adder);
    }
    REQUIRE(val == 5);
}

TEST_CASE("cleanup: stores std::function", "[cleanup][types]") {
    int count = 0;
    std::function<void()> fn = [&] { ++count; };
    {
        auto guard = zeta::MakeCleanup(std::move(fn));
        REQUIRE(guard.IsActive());
    }
    REQUIRE(count == 1);
}

// ═══════════════════════════════════════════════════════════════════════
// noexcept propagation
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("cleanup: noexcept move for noexcept callables", "[cleanup][noexcept]") {
    auto g = zeta::MakeCleanup([] {});  // lambda is noexcept-invocable
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<decltype(g)>);
}

TEST_CASE("cleanup: IsActive reflects state", "[cleanup][query]") {
    auto g = zeta::MakeCleanup([] {});
    REQUIRE(g.IsActive());
    auto g2 = std::move(g);
    REQUIRE(!g.IsActive());
    REQUIRE(g2.IsActive());
    g2.Cancel();
    REQUIRE(!g2.IsActive());
}
