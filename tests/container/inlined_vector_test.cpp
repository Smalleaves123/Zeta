#include "zeta/container/inlined_vector.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Construction and basic access
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: default construction", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v;
    REQUIRE(v.empty());
    REQUIRE(v.size() == 0);
    REQUIRE(v.capacity() >= 4);
}

TEST_CASE("InlinedVector: push_back inline", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);
    REQUIRE(v.capacity() == 4); // still inline
}

TEST_CASE("InlinedVector: push_back overflow to heap", "[inlined_vector]") {
    zeta::InlinedVector<int, 2> v;
    v.push_back(1);
    v.push_back(2);
    REQUIRE(v.capacity() == 2);
    v.push_back(3); // triggers heap allocation
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);
    REQUIRE(v.capacity() >= 3);
}

TEST_CASE("InlinedVector: push_back rvalue", "[inlined_vector]") {
    zeta::InlinedVector<std::string, 4> v;
    std::string s = "hello";
    v.push_back(std::move(s));
    REQUIRE(v[0] == "hello");
}

TEST_CASE("InlinedVector: initializer_list", "[inlined_vector]") {
    zeta::InlinedVector<int, 8> v = {1, 2, 3, 4, 5};
    REQUIRE(v.size() == 5);
    REQUIRE(v[2] == 3);
}

TEST_CASE("InlinedVector: emplace_back", "[inlined_vector]") {
    zeta::InlinedVector<std::string, 4> v;
    v.emplace_back(3, 'x');
    REQUIRE(v[0] == "xxx");
}

TEST_CASE("InlinedVector: pop_back", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.pop_back();
    REQUIRE(v.size() == 2);
    REQUIRE(v.back() == 2);
}

TEST_CASE("InlinedVector: pop_back single element", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {42};
    v.pop_back();
    REQUIRE(v.empty());
}

TEST_CASE("InlinedVector: front/back", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {10, 20, 30};
    REQUIRE(v.front() == 10);
    REQUIRE(v.back() == 30);
}

TEST_CASE("InlinedVector: data()", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    REQUIRE(v.data() != nullptr);
    REQUIRE(v.data()[0] == 1);
    REQUIRE(v.data()[2] == 3);
}

TEST_CASE("InlinedVector: data() after heap transition", "[inlined_vector]") {
    zeta::InlinedVector<int, 2> v = {1, 2, 3, 4}; // on heap
    REQUIRE(v.data() != nullptr);
    REQUIRE(v.data()[0] == 1);
    REQUIRE(v.data()[3] == 4);
}

TEST_CASE("InlinedVector: at()", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    REQUIRE(v.at(1) == 2);
    REQUIRE_THROWS_AS(v.at(5), std::out_of_range);
}

// ═══════════════════════════════════════════════════════════════════
// Capacity management
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: reserve", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v;
    v.reserve(100);
    REQUIRE(v.capacity() >= 100);
    REQUIRE(v.size() == 0);
}

TEST_CASE("InlinedVector: reserve less than current", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v;
    v.reserve(100);
    size_t cap = v.capacity();
    v.reserve(10); // should be no-op
    REQUIRE(v.capacity() == cap);
}

TEST_CASE("InlinedVector: resize", "[inlined_vector]") {
    zeta::InlinedVector<int, 8> v = {1, 2, 3};
    v.resize(5, 99);
    REQUIRE(v.size() == 5);
    REQUIRE(v[3] == 99);
    REQUIRE(v[4] == 99);
    v.resize(2);
    REQUIRE(v.size() == 2);
}

TEST_CASE("InlinedVector: resize to zero", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.resize(0);
    REQUIRE(v.empty());
}

TEST_CASE("InlinedVector: clear", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.clear();
    REQUIRE(v.empty());
}

TEST_CASE("InlinedVector: clear idempotent", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.clear();
    v.clear();
    REQUIRE(v.empty());
}

TEST_CASE("InlinedVector: shrink_to_fit inline", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3}; // inline
    v.shrink_to_fit();
    REQUIRE(v.size() == 3);
    REQUIRE(v.capacity() >= 4); // stays inline
}

TEST_CASE("InlinedVector: shrink_to_fit heap to inline", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v;
    for (int i = 0; i < 10; ++i) v.push_back(i); // on heap
    REQUIRE(v.capacity() > 4);
    for (int i = 0; i < 6; ++i) v.pop_back(); // back to 4 elements — fits inline
    v.shrink_to_fit();
    // After shrink_to_fit, size ≤ InlinedCapacity → should be back inline
    REQUIRE(v.size() == 4);
    REQUIRE(v[0] == 0);
    REQUIRE(v[3] == 3);
}

// ═══════════════════════════════════════════════════════════════════
// Modifiers
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: insert", "[inlined_vector]") {
    zeta::InlinedVector<int, 8> v = {1, 2, 4};
    v.insert(v.begin() + 2, 3);
    REQUIRE(v.size() == 4);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);
    REQUIRE(v[3] == 4);
}

TEST_CASE("InlinedVector: insert at beginning", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 8> v = {2, 3};
    v.insert(v.begin(), 1);
    REQUIRE(v == zeta::InlinedVector<int, 8>({1, 2, 3}));
}

TEST_CASE("InlinedVector: insert at end", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 8> v = {1, 2};
    v.insert(v.end(), 3);
    REQUIRE(v == zeta::InlinedVector<int, 8>({1, 2, 3}));
}

TEST_CASE("InlinedVector: insert rvalue", "[inlined_vector]") {
    zeta::InlinedVector<std::string, 4> v = {"a", "c"};
    std::string b = "b";
    v.insert(v.begin() + 1, std::move(b));
    REQUIRE(v[0] == "a");
    REQUIRE(v[1] == "b");
    REQUIRE(v[2] == "c");
}

TEST_CASE("InlinedVector: erase", "[inlined_vector]") {
    zeta::InlinedVector<int, 8> v = {1, 2, 3, 4};
    auto it = v.erase(v.begin() + 1);
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 3);
    REQUIRE(*it == 3);
}

TEST_CASE("InlinedVector: erase first element", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.erase(v.begin());
    REQUIRE(v[0] == 2);
    REQUIRE(v.size() == 2);
}

TEST_CASE("InlinedVector: erase last element", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.erase(v.end() - 1);
    REQUIRE(v.size() == 2);
    REQUIRE(v.back() == 2);
}

TEST_CASE("InlinedVector: erase single element", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {42};
    v.erase(v.begin());
    REQUIRE(v.empty());
}

// ═══════════════════════════════════════════════════════════════════
// Iterators
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: iteration", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    int sum = 0;
    for (int x : v) sum += x;
    REQUIRE(sum == 6);
}

TEST_CASE("InlinedVector: rbegin/rend", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    auto it = v.rbegin();
    REQUIRE(*it++ == 3);
    REQUIRE(*it++ == 2);
    REQUIRE(*it++ == 1);
    REQUIRE(it == v.rend());
}

TEST_CASE("InlinedVector: cbegin/cend", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    auto it = v.cbegin();
    REQUIRE(*it == 1);
    size_t count = 0;
    for (auto cit = v.cbegin(); cit != v.cend(); ++cit) ++count;
    REQUIRE(count == v.size());
}

// ═══════════════════════════════════════════════════════════════════
// Copy / move / assignment
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: copy", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2, 3};
    auto b = a;
    REQUIRE(b.size() == 3);
    REQUIRE(a == b);
}

TEST_CASE("InlinedVector: copy assignment", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2, 3};
    zeta::InlinedVector<int, 4> b;
    b = a;
    REQUIRE(b.size() == 3);
    REQUIRE(b[0] == 1);
    REQUIRE(b[2] == 3);
}

TEST_CASE("InlinedVector: move", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2, 3};
    auto b = std::move(a);
    REQUIRE(b.size() == 3);
    REQUIRE(a.empty());
}

TEST_CASE("InlinedVector: move assignment", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2, 3};
    zeta::InlinedVector<int, 4> b;
    b = std::move(a);
    REQUIRE(b.size() == 3);
}

TEST_CASE("InlinedVector: heap copy", "[inlined_vector]") {
    zeta::InlinedVector<int, 2> a = {1, 2, 3, 4}; // on heap
    auto b = a;
    REQUIRE(b.size() == 4);
    REQUIRE(b[0] == 1);
    REQUIRE(b[3] == 4);
}

TEST_CASE("InlinedVector: swap", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2};
    zeta::InlinedVector<int, 4> b = {3, 4, 5};
    a.swap(b);
    REQUIRE(a.size() == 3);
    REQUIRE(b.size() == 2);
    REQUIRE(a[0] == 3);
    REQUIRE(b[0] == 1);
}

TEST_CASE("InlinedVector: self-swap", "[inlined_vector][edge]") {
    zeta::InlinedVector<int, 4> v = {1, 2, 3};
    v.swap(v);
    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[2] == 3);
}

TEST_CASE("InlinedVector: heap move", "[inlined_vector]") {
    zeta::InlinedVector<int, 2> a = {1, 2, 3, 4, 5};
    REQUIRE(a.capacity() > 2);
    auto b = std::move(a);
    REQUIRE(b.size() == 5);
    REQUIRE(a.empty());
}

TEST_CASE("InlinedVector: move-only type", "[inlined_vector]") {
    zeta::InlinedVector<std::unique_ptr<int>, 4> v;
    v.push_back(std::make_unique<int>(42));
    REQUIRE(*v[0] == 42);
    v.emplace_back(std::make_unique<int>(99));
    REQUIRE(*v[1] == 99);
    REQUIRE(v.size() == 2);
}

TEST_CASE("InlinedVector: operator==", "[inlined_vector]") {
    zeta::InlinedVector<int, 4> a = {1, 2, 3};
    zeta::InlinedVector<int, 4> b = {1, 2, 3};
    zeta::InlinedVector<int, 4> c = {1, 2};
    REQUIRE(a == b);
    REQUIRE(a != c);
}

// ═══════════════════════════════════════════════════════════════════
// Property-based invariants
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("InlinedVector: size <= capacity invariant", "[inlined_vector][property]") {
    zeta::InlinedVector<int, 4> v;
    REQUIRE(v.size() <= v.capacity());
    for (int i = 0; i < 20; ++i) {
        v.push_back(i);
        REQUIRE(v.size() <= v.capacity());
    }
    for (int i = 0; i < 10; ++i) {
        v.pop_back();
        REQUIRE(v.size() <= v.capacity());
    }
}

TEST_CASE("InlinedVector: push_back preserves previous elements", "[inlined_vector][property]") {
    zeta::InlinedVector<int, 4> v;
    for (int i = 0; i < 10; ++i) {
        v.push_back(i * 10);
        for (int j = 0; j <= i; ++j) {
            REQUIRE(v[j] == j * 10);
        }
    }
}

TEST_CASE("InlinedVector: clear leaves capacity unchanged", "[inlined_vector][property]") {
    zeta::InlinedVector<int, 4> v;
    v.reserve(100);
    v.push_back(1);
    v.push_back(2);
    size_t cap = v.capacity();
    v.clear();
    REQUIRE(v.empty());
    REQUIRE(v.capacity() == cap);
}

TEST_CASE("InlinedVector: data() and operator[] equivalence", "[inlined_vector][property]") {
    zeta::InlinedVector<int, 4> v = {10, 20, 30, 40};
    for (size_t i = 0; i < v.size(); ++i) {
        REQUIRE(v[i] == v.data()[i]);
    }
}
