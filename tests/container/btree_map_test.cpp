#include "zeta/container/btree_map.h"
#include "zeta/container/btree_set.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <string>

namespace {

struct ThrowingMoveAssignment {
    static inline bool throw_on_move_assignment = false;

    int value = 0;

    ThrowingMoveAssignment() = default;
    explicit ThrowingMoveAssignment(int v) : value(v) {}
    ThrowingMoveAssignment(const ThrowingMoveAssignment&) = default;
    ThrowingMoveAssignment& operator=(const ThrowingMoveAssignment&) = default;
    ThrowingMoveAssignment(ThrowingMoveAssignment&& other) noexcept
        : value(other.value) {}
    ThrowingMoveAssignment& operator=(ThrowingMoveAssignment&& other) {
        if (throw_on_move_assignment) {
            throw std::runtime_error("move assignment");
        }
        value = other.value;
        return *this;
    }
};

} // namespace

// ═══════════════════════════════════════════════════════════════════════
// btree_map
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("btree_map: default construction", "[btree][map]") {
    zeta::btree_map<int, std::string> m;
    REQUIRE(m.empty());
    REQUIRE(m.size() == 0);
}

TEST_CASE("btree_map: insert and find", "[btree][map]") {
    zeta::btree_map<int, std::string> m;
    auto [it, ok] = m.insert({1, "one"});
    REQUIRE(ok);
    REQUIRE(it->first == 1);
    REQUIRE(it->second == "one");

    auto it2 = m.find(1);
    REQUIRE(it2 != m.end());
    REQUIRE(it2->second == "one");
}

TEST_CASE("btree_map: insert duplicate", "[btree][map]") {
    zeta::btree_map<int, std::string> m;
    m.insert({1, "one"});
    auto [it, ok] = m.insert({1, "another"});
    REQUIRE(!ok);
    REQUIRE(it->second == "one");  // unchanged
}

TEST_CASE("btree_map: operator[]", "[btree][map]") {
    zeta::btree_map<int, std::string> m;
    m[1] = "one";
    REQUIRE(m[1] == "one");
    m[1] = "ONE";
    REQUIRE(m[1] == "ONE");
}

TEST_CASE("btree_map: ordered iteration", "[btree][map]") {
    zeta::btree_map<int, int> m;
    m.insert({3, 30});
    m.insert({1, 10});
    m.insert({2, 20});

    int prev = 0;
    for (auto& [k, v] : m) {
        REQUIRE(k > prev);
        prev = k;
    }
}

TEST_CASE("btree_map: iterator can modify mapped values", "[btree][map]") {
    zeta::btree_map<int, int> m;
    m.insert({1, 10});
    m.insert({2, 20});

    for (auto& [key, value] : m) {
        value += key;
    }

    REQUIRE(m.find(1)->second == 11);
    REQUIRE(m.find(2)->second == 22);
}

TEST_CASE("btree_map: decrementing end yields the last element", "[btree][map]") {
    zeta::btree_map<int, int> m;
    for (int value = 0; value < 100; ++value) {
        m.insert({value, value * 10});
    }

    auto it = m.end();
    --it;
    REQUIRE(it->first == 99);
    REQUIRE(it->second == 990);
}

TEST_CASE("btree_map: contains", "[btree][map]") {
    zeta::btree_map<int, int> m;
    m.insert({42, 0});
    REQUIRE(m.contains(42));
    REQUIRE(!m.contains(99));
}

TEST_CASE("btree_map: erase", "[btree][map]") {
    zeta::btree_map<int, int> m;
    m.insert({1, 0});
    m.insert({2, 0});
    REQUIRE(m.erase(1) == 1);
    REQUIRE(!m.contains(1));
    REQUIRE(m.contains(2));
    REQUIRE(m.erase(99) == 0);
}

TEST_CASE("btree_map: large insert", "[btree][map]") {
    zeta::btree_map<int, int> m;
    constexpr int N = 500;
    for (int i = 0; i < N; ++i) m.insert({i, i * 10});
    REQUIRE(m.size() == N);
    for (int i = 0; i < N; ++i) {
        REQUIRE(m.contains(i));
    }
}

TEST_CASE("btree_map: failed split leaves the map unchanged",
          "[btree][map][exception]") {
    zeta::btree_map<int, ThrowingMoveAssignment> m;
    for (int i = 0; i < 64; ++i) {
        REQUIRE(m.insert({i, ThrowingMoveAssignment(i)}).second);
    }

    ThrowingMoveAssignment::throw_on_move_assignment = true;
    REQUIRE_THROWS_AS(
        m.insert({100, ThrowingMoveAssignment(100)}), std::runtime_error);
    ThrowingMoveAssignment::throw_on_move_assignment = false;

    REQUIRE(m.size() == 64);
    REQUIRE_FALSE(m.contains(100));
    int count = 0;
    for (const auto& item : m) {
        REQUIRE(item.first == count);
        ++count;
    }
    REQUIRE(count == 64);
}

TEST_CASE("btree_map: erase from internal node keeps ordering intact", "[btree][map][erase]") {
    zeta::btree_map<int, int> m;
    constexpr int N = 200;
    for (int i = 0; i < N; ++i) m.insert({i, i});

    REQUIRE(m.erase(100) == 1);
    REQUIRE(!m.contains(100));
    REQUIRE(m.size() == N - 1);
    REQUIRE(m.contains(99));
    REQUIRE(m.contains(101));
    REQUIRE(m.contains(32));
    REQUIRE(m.contains(65));
}

// ═══════════════════════════════════════════════════════════════════════
// btree_set
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("btree_set: basic operations", "[btree][set]") {
    zeta::btree_set<int> s;
    auto [it, ok] = s.insert(42);
    REQUIRE(ok);
    REQUIRE(*it == 42);
    REQUIRE(s.contains(42));
    REQUIRE(!s.contains(99));
    REQUIRE(s.erase(42) == 1);
    REQUIRE(s.empty());
}

TEST_CASE("btree_set: ordered iteration", "[btree][set]") {
    zeta::btree_set<int> s;
    for (int v : {5, 3, 1, 4, 2}) s.insert(v);
    int expected = 1;
    for (int v : s) {
        REQUIRE(v == expected);
        ++expected;
    }
}
