#include "zeta/memory/function_ref.h"

#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <string>
#include <type_traits>

// ═══════════════════════════════════════════════════════════════════
// Construction from various callable sources
// ═══════════════════════════════════════════════════════════════════

static int add_one(int x) { return x + 1; }

TEST_CASE("function_ref: free function", "[function_ref]") {
    zeta::function_ref<int(int)> ref = add_one;
    REQUIRE(ref(0) == 1);
    REQUIRE(ref(41) == 42);
}

TEST_CASE("function_ref: lambda with capture", "[function_ref]") {
    int factor = 10;
    auto mul = [factor](int x) { return x * factor; };
    zeta::function_ref<int(int)> ref = mul;
    REQUIRE(ref(5) == 50);
}

struct Doubler {
    int operator()(int x) const { return x * 2; }
};

TEST_CASE("function_ref: function object", "[function_ref]") {
    Doubler d;
    zeta::function_ref<int(int)> ref = d;
    REQUIRE(ref(21) == 42);
}

TEST_CASE("function_ref: wrapping std::function", "[function_ref]") {
    std::function<int(int)> f = [](int x) { return x * x; };
    zeta::function_ref<int(int)> ref = f;
    REQUIRE(ref(9) == 81);
}

TEST_CASE("function_ref: function pointer", "[function_ref]") {
    zeta::function_ref<int(int)> ref = add_one;
    REQUIRE(ref(5) == 6);
}

// ═══════════════════════════════════════════════════════════════════
// Return and argument types
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("function_ref: void return", "[function_ref]") {
    int side = 0;
    auto inc = [&side] { ++side; };
    zeta::function_ref<void()> ref = inc;
    ref();
    REQUIRE(side == 1);
}

TEST_CASE("function_ref: noexcept specialization", "[function_ref]") {
    auto safe_double = [](int x) noexcept -> int { return x * 2; };
    zeta::function_ref<int(int) noexcept> ref = safe_double;
    static_assert(noexcept(ref(1)));
    REQUIRE(ref(10) == 20);
}

TEST_CASE("function_ref: multiple arguments", "[function_ref]") {
    auto sum = [](int a, int b, int c) { return a + b + c; };
    zeta::function_ref<int(int, int, int)> ref = sum;
    REQUIRE(ref(1, 2, 3) == 6);
}

TEST_CASE("function_ref: reference argument", "[function_ref]") {
    auto append_bang = [](std::string& s) { s += "!"; };
    zeta::function_ref<void(std::string&)> ref = append_bang;
    std::string msg = "hello";
    ref(msg);
    REQUIRE(msg == "hello!");
}

TEST_CASE("function_ref: zero arguments", "[function_ref]") {
    auto forty_two = [] { return 42; };
    zeta::function_ref<int()> ref = forty_two;
    REQUIRE(ref() == 42);
}

// ═══════════════════════════════════════════════════════════════════
// Copy semantics
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("function_ref: copy construction", "[function_ref][copy]") {
    auto fn = [](int x) { return x * 3; };
    zeta::function_ref<int(int)> ref1 = fn;
    zeta::function_ref<int(int)> ref2 = ref1; // copy
    REQUIRE(ref1(2) == 6);
    REQUIRE(ref2(2) == 6);
}

TEST_CASE("function_ref: copy assignment", "[function_ref][copy]") {
    auto fn1 = [](int x) { return x * 2; };
    auto fn2 = [](int x) { return x * 3; };
    zeta::function_ref<int(int)> ref1 = fn1;
    zeta::function_ref<int(int)> ref2 = fn2;
    ref2 = ref1; // copy assignment
    REQUIRE(ref2(5) == 10);
}

TEST_CASE("function_ref: multiple calls consistent", "[function_ref][property]") {
    auto triple = [](int x) { return x * 3; };
    zeta::function_ref<int(int)> ref = triple;
    for (int i = 0; i < 10; ++i) {
        REQUIRE(ref(i * 2) == i * 6);
    }
}

TEST_CASE("function_ref: invocation through const ref", "[function_ref]") {
    auto square = [](int x) { return x * x; };
    const zeta::function_ref<int(int)> ref = square;
    REQUIRE(ref(5) == 25);
}

// ═══════════════════════════════════════════════════════════════════
// Compile-time checks
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("function_ref: sizeof is two pointers", "[function_ref][compile]") {
    static_assert(sizeof(zeta::function_ref<int(int)>) == 2 * sizeof(void*));
    static_assert(sizeof(zeta::function_ref<int(int) noexcept>) == 2 * sizeof(void*));
}

TEST_CASE("function_ref: default constructor is deleted", "[function_ref][compile]") {
    static_assert(!std::is_default_constructible_v<zeta::function_ref<int(int)>>);
}

// ═══════════════════════════════════════════════════════════════════
// noexcept variant: must reject non-noexcept callables
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("function_ref: noexcept rejects non-noexcept", "[function_ref][compile]") {
    // A non-noexcept lambda should NOT be constructible into noexcept function_ref
    auto can_throw = [](int x) -> int { return x; }; // NOT noexcept
    bool constexpr is_constructible =
        std::is_constructible_v<zeta::function_ref<int(int) noexcept>, decltype(can_throw)>;
    REQUIRE(!is_constructible);
}

TEST_CASE("function_ref: noexcept propagates", "[function_ref][compile]") {
    auto safe = [](int x) noexcept -> int { return x; };
    zeta::function_ref<int(int) noexcept> ref = safe;
    static_assert(noexcept(ref(42)));
}

// ═══════════════════════════════════════════════════════════════════
// Property: call equivalence
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("function_ref: rvalue lambda is rejected at compile time", "[function_ref][compile]") {
    // Binding to a temporary must not compile.
    auto constexpr constructible =
        std::is_constructible_v<zeta::function_ref<int()>, decltype([] { return 42; })>;
    REQUIRE(!constructible);
}

TEST_CASE("function_ref: call equals direct call", "[function_ref][property]") {
    auto fn = [](int a, int b) { return a + b; };
    zeta::function_ref<int(int, int)> ref = fn;
    for (int a = -5; a <= 5; ++a) {
        for (int b = -5; b <= 5; ++b) {
            REQUIRE(ref(a, b) == fn(a, b));
        }
    }
}
