#include "zeta/container/flat_hash_map.h"

#include <catch2/catch_test_macros.hpp>
#include <random>
#include <unordered_map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std::literals;

// ═══════════════════════════════════════════════════════════════════
// Helper: transparent hash / equal for heterogeneous lookup tests
// ═══════════════════════════════════════════════════════════════════
struct TransparentHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& s) const noexcept {
        return std::hash<std::string>{}(s);
    }
};
struct TransparentEq {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

struct CollisionHash {
    size_t operator()(int value) const noexcept {
        return static_cast<size_t>(value & 0x0F);
    }
};

// ═══════════════════════════════════════════════════════════════════
// flat_hash_map tests
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("flat_hash_map: default construction", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    REQUIRE(m.empty());
}

TEST_CASE("flat_hash_map: insert and lookup", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    auto [it, ok] = m.insert({1, "one"});
    REQUIRE(ok);
    REQUIRE(it->first == 1);
    REQUIRE(it->second == "one");
    REQUIRE(m.contains(1));
    REQUIRE(!m.contains(2));
}

TEST_CASE("flat_hash_map: operator[]", "[map]") {
    zeta::flat_hash_map<std::string, int> m;
    m["hello"] = 42;
    REQUIRE(m["hello"] == 42);
    m["world"] = 99;
    REQUIRE(m["world"] == 99);
    REQUIRE(m["new"] == 0);
}

TEST_CASE("flat_hash_map: at()", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    m.insert({1, "one"});
    REQUIRE(m.at(1) == "one");
    REQUIRE_THROWS_AS(m.at(2), std::out_of_range);
}

TEST_CASE("flat_hash_map: const at()", "[map]") {
    const zeta::flat_hash_map<int, std::string> m = {{1, "one"}};
    REQUIRE(m.at(1) == "one");
    REQUIRE_THROWS_AS(m.at(2), std::out_of_range);
}

TEST_CASE("flat_hash_map: insert duplicate", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    m.insert({1, "one"});
    auto [it, ok] = m.insert({1, "ONE"});
    REQUIRE(!ok);
    REQUIRE(it->second == "one");
}

TEST_CASE("flat_hash_map: try_emplace", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    auto [it1, ok1] = m.try_emplace(1, "one");
    REQUIRE(ok1);
    REQUIRE(it1->second == "one");

    auto [it2, ok2] = m.try_emplace(1, "ONE");
    REQUIRE(!ok2);
    REQUIRE(it2->second == "one");
    REQUIRE(m.size() == 1);
}

TEST_CASE("flat_hash_map: emplace", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    auto [it, ok] = m.emplace(1, "one");
    REQUIRE(ok);
    REQUIRE(it->second == "one");
}

TEST_CASE("flat_hash_map: insert_or_assign", "[map]") {
    zeta::flat_hash_map<int, std::string> m;
    auto [it1, ok1] = m.insert_or_assign(1, "one");
    REQUIRE(ok1);

    auto [it2, ok2] = m.insert_or_assign(1, "ONE");
    REQUIRE(!ok2);
    REQUIRE(it2->second == "ONE");
    REQUIRE(m.size() == 1);
}

TEST_CASE("flat_hash_map: erase", "[map]") {
    zeta::flat_hash_map<int, std::string> m = {{1, "one"}, {2, "two"}};
    REQUIRE(m.erase(1) == 1);
    REQUIRE(!m.contains(1));
    REQUIRE(m.size() == 1);
}

TEST_CASE("flat_hash_map: erase by iterator", "[map]") {
    zeta::flat_hash_map<int, std::string> m = {{1, "one"}, {2, "two"}};
    auto it = m.find(1);
    REQUIRE(it != m.end());
    auto next = m.erase(it);
    REQUIRE(m.size() == 1);
    REQUIRE(!m.contains(1));
    (void)next;
}

TEST_CASE("flat_hash_map: erase from empty", "[map][edge]") {
    zeta::flat_hash_map<int, std::string> m;
    REQUIRE(m.erase(1) == 0);
}

TEST_CASE("flat_hash_map: lookup survives tombstones in probe chain", "[map][erase][collision]") {
    zeta::flat_hash_map<int, int, CollisionHash> m;
    m.insert({1, 10});
    m.insert({17, 20});
    m.insert({33, 30});

    REQUIRE(m.erase(1) == 1);
    REQUIRE(m.contains(17));
    REQUIRE(m.contains(33));
    REQUIRE(m.at(17) == 20);
    REQUIRE(m.at(33) == 30);
}

TEST_CASE("flat_hash_map: iteration", "[map]") {
    zeta::flat_hash_map<int, int> m = {{10, 100}, {20, 200}};
    int sum = 0;
    for (const auto& [k, v] : m) sum += k + v;
    REQUIRE(sum == 330);
}

TEST_CASE("flat_hash_map: const iteration", "[map]") {
    const zeta::flat_hash_map<int, int> m = {{10, 100}, {20, 200}};
    int sum = 0;
    for (const auto& [k, v] : m) sum += k + v;
    REQUIRE(sum == 330);
}

TEST_CASE("flat_hash_map: cbegin / cend", "[map]") {
    zeta::flat_hash_map<int, int> m = {{1, 10}, {2, 20}};
    auto it = m.cbegin();
    auto end = m.cend();
    REQUIRE(std::distance(it, end) == static_cast<decltype(std::distance(it, end))>(m.size()));
}

TEST_CASE("flat_hash_map: large insert", "[map]") {
    zeta::flat_hash_map<int, int> m;
    constexpr int N = 1000;
    for (int i = 0; i < N; ++i) m.insert({i, i * i});
    REQUIRE(m.size() == N);
    for (int i = 0; i < N; ++i) REQUIRE(m.at(i) == i * i);
}

TEST_CASE("flat_hash_map: count", "[map]") {
    const zeta::flat_hash_map<int, int> m = {{1, 10}};
    REQUIRE(m.count(1) == 1);
    REQUIRE(m.count(2) == 0);
}

TEST_CASE("flat_hash_map: copy", "[map]") {
    zeta::flat_hash_map<int, int> a = {{1, 10}, {2, 20}};
    auto b = a;
    REQUIRE(b.size() == 2);
    REQUIRE(a == b);
}

TEST_CASE("flat_hash_map: move", "[map]") {
    zeta::flat_hash_map<int, int> a = {{1, 10}};
    auto b = std::move(a);
    REQUIRE(b.at(1) == 10);
    REQUIRE(a.empty());
}

TEST_CASE("flat_hash_map: swap", "[map]") {
    zeta::flat_hash_map<int, std::string> a = {{1, "one"}};
    zeta::flat_hash_map<int, std::string> b = {{2, "two"}, {3, "three"}};
    a.swap(b);
    REQUIRE(a.size() == 2);
    REQUIRE(b.size() == 1);
    REQUIRE(a.contains(2));
    REQUIRE(b.contains(1));
}

TEST_CASE("flat_hash_map: reserve", "[map]") {
    zeta::flat_hash_map<int, int> m;
    m.reserve(100);
    REQUIRE(m.capacity() >= 100);
    REQUIRE(m.size() == 0);
}

TEST_CASE("flat_hash_map: rehash", "[map]") {
    zeta::flat_hash_map<int, std::string> m = {{1, "one"}, {2, "two"}};
    size_t old_cap = m.capacity();
    m.rehash(old_cap * 2);
    REQUIRE(m.capacity() >= old_cap * 2);
    REQUIRE(m.size() == 2);
    REQUIRE(m.at(1) == "one");
}

TEST_CASE("flat_hash_map: operator[] with move key", "[map]") {
    zeta::flat_hash_map<std::string, int> m;
    std::string key = "hello";
    m[std::move(key)] = 42;
    REQUIRE(m["hello"] == 42);
}

TEST_CASE("flat_hash_map: heterogeneous lookup", "[map][hetero]") {
    zeta::flat_hash_map<std::string, int, TransparentHash, TransparentEq> m;
    m.insert({"hello", 42});
    m.insert({"world", 99});

    REQUIRE(m.contains("hello"sv));
    REQUIRE(m.contains("world"sv));
    REQUIRE(!m.contains("nope"sv));

    auto it = m.find("hello"sv);
    REQUIRE(it != m.end());
    REQUIRE(it->second == 42);

    REQUIRE(m.count("hello"sv) == 1);
    REQUIRE(m.erase("world"sv) == 1);
}

TEST_CASE("flat_hash_map: heterogeneous at()", "[map][hetero]") {
    zeta::flat_hash_map<std::string, int, TransparentHash, TransparentEq> m;
    m.insert({"hello", 42});
    m.insert({"world", 99});

    // at() with string_view — no std::string temporary
    REQUIRE(m.at("hello"sv) == 42);
    REQUIRE(m.at("world"sv) == 99);
    REQUIRE_THROWS_AS(m.at("nope"sv), std::out_of_range);

    // const at()
    const auto& cm = m;
    REQUIRE(cm.at("hello"sv) == 42);
    REQUIRE_THROWS_AS(cm.at("nope"sv), std::out_of_range);
}

TEST_CASE("flat_hash_map: move-only mapped type", "[map]") {
    zeta::flat_hash_map<int, std::unique_ptr<int>> m;
    m.try_emplace(1, std::make_unique<int>(42));
    REQUIRE(*m.at(1) == 42);

    m.insert_or_assign(1, std::make_unique<int>(99));
    REQUIRE(*m.at(1) == 99);
}

TEST_CASE("flat_hash_map: rvalue insert", "[map]") {
    zeta::flat_hash_map<std::string, int> m;
    m.insert(std::make_pair("hello", 42));
    REQUIRE(m.at("hello") == 42);
}

TEST_CASE("flat_hash_map: randomized parity with std::unordered_map", "[map][stress]") {
    zeta::flat_hash_map<int, int, CollisionHash> actual;
    std::unordered_map<int, int, CollisionHash> expected;
    std::mt19937 rng(20260622);

    for (int step = 0; step < 5000; ++step) {
        const int key = static_cast<int>(rng() % 128);
        switch (rng() % 5) {
        case 0: {
            const int value = static_cast<int>(rng() % 100000);
            const auto [it1, ok1] = actual.insert({key, value});
            const auto [it2, ok2] = expected.insert({key, value});
            REQUIRE(ok1 == ok2);
            REQUIRE(it1->second == it2->second);
            break;
        }
        case 1: {
            const int value = static_cast<int>(rng() % 100000);
            const bool existed = expected.find(key) != expected.end();
            const auto [it, inserted] = actual.insert_or_assign(key, value);
            expected.insert_or_assign(key, value);
            REQUIRE(it->second == expected.at(key));
            REQUIRE(inserted == !existed);
            break;
        }
        case 2: {
            REQUIRE(actual.erase(key) == expected.erase(key));
            break;
        }
        case 3: {
            if (rng() & 1U) {
                actual.reserve((rng() % 256) + 1);
            } else {
                actual.rehash((rng() % 256) + 1);
            }
            break;
        }
        default: {
            const auto actual_it = actual.find(key);
            const auto expected_it = expected.find(key);
            REQUIRE((actual_it == actual.end()) == (expected_it == expected.end()));
            if (expected_it != expected.end()) {
                REQUIRE(actual_it->second == expected_it->second);
            }
            break;
        }
        }

        REQUIRE(actual.size() == expected.size());
        for (const auto& [expected_key, expected_value] : expected) {
            auto it = actual.find(expected_key);
            REQUIRE(it != actual.end());
            REQUIRE(it->second == expected_value);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════
// Stress tests
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("flat_hash_map: large insert (100k)", "[map][stress]") {
    zeta::flat_hash_map<int, int> m;
    constexpr int N = 100000;
    for (int i = 0; i < N; ++i) m.insert({i, -i});
    REQUIRE(m.size() == N);
    REQUIRE(m.at(42) == -42);
    REQUIRE(m.at(N - 1) == -(N - 1));
    for (int i = 0; i < N; ++i) REQUIRE(m.contains(i));
}

TEST_CASE("flat_hash_map: self-assignment", "[map][edge]") {
    zeta::flat_hash_map<int, int> m = {{1, 10}, {2, 20}};
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    m = m;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
    REQUIRE(m.size() == 2);
    REQUIRE(m.at(1) == 10);
}

// ═══════════════════════════════════════════════════════════════════
// const correctness
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("flat_hash_map: const contains/count after mutations", "[map][const]") {
    zeta::flat_hash_map<int, int> m = {{1, 10}, {2, 20}};
    m.erase(1);
    const auto& cm = m;
    REQUIRE(!cm.contains(1));
    REQUIRE(cm.contains(2));
    REQUIRE(cm.count(1) == 0);
    REQUIRE(cm.count(2) == 1);
}
