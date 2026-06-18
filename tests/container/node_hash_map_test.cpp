#include "zeta/container/node_hash_map.h"
#include "zeta/container/node_hash_set.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

// ═══════════════════════════════════════════════════════════════════════
// node_hash_map
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("node_hash_map: default construction", "[node_hash][map]") {
    zeta::node_hash_map<int, std::string> m;
    REQUIRE(m.empty());
    REQUIRE(m.size() == 0);
}

TEST_CASE("node_hash_map: insert and find", "[node_hash][map]") {
    zeta::node_hash_map<int, std::string> m;
    auto [it, ok] = m.insert({1, "one"});
    REQUIRE(ok);
    REQUIRE(it->first == 1);
    REQUIRE(it->second == "one");

    auto it2 = m.find(1);
    REQUIRE(it2 != m.end());
    REQUIRE(it2->second == "one");
}

TEST_CASE("node_hash_map: insert duplicate", "[node_hash][map]") {
    zeta::node_hash_map<int, std::string> m;
    m.insert({1, "one"});
    auto [it, ok] = m.insert({1, "another"});
    REQUIRE(!ok);
    REQUIRE(it->second == "one");
}

TEST_CASE("node_hash_map: operator[]", "[node_hash][map]") {
    zeta::node_hash_map<int, std::string> m;
    m[1] = "one";
    REQUIRE(m[1] == "one");
    m[1] = "ONE";
    REQUIRE(m[1] == "ONE");
}

TEST_CASE("node_hash_map: contains", "[node_hash][map]") {
    zeta::node_hash_map<int, int> m;
    m.insert({42, 0});
    REQUIRE(m.contains(42));
    REQUIRE(!m.contains(99));
}

TEST_CASE("node_hash_map: erase", "[node_hash][map]") {
    zeta::node_hash_map<int, int> m;
    m.insert({1, 0});
    m.insert({2, 0});
    REQUIRE(m.erase(1) == 1);
    REQUIRE(!m.contains(1));
    REQUIRE(m.erase(99) == 0);
}

TEST_CASE("node_hash_map: large insert", "[node_hash][map]") {
    zeta::node_hash_map<int, int> m;
    constexpr int N = 500;
    for (int i = 0; i < N; ++i) m.insert({i, i * 10});
    REQUIRE(m.size() == N);
    for (int i = 0; i < N; ++i) REQUIRE(m.contains(i));
}

TEST_CASE("node_hash_map: try_emplace", "[node_hash][map]") {
    zeta::node_hash_map<int, std::string> m;
    auto [it, ok] = m.try_emplace(1, "hello");
    REQUIRE(ok);
    REQUIRE(it->second == "hello");

    auto [it2, ok2] = m.try_emplace(1, "world");
    REQUIRE(!ok2);
    REQUIRE(it2->second == "hello");  // unchanged
}

// ═══════════════════════════════════════════════════════════════════════
// node_hash_set
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("node_hash_set: basic operations", "[node_hash][set]") {
    zeta::node_hash_set<int> s;
    auto [it, ok] = s.insert(42);
    REQUIRE(ok);
    REQUIRE(*it == 42);
    REQUIRE(s.contains(42));
    REQUIRE(!s.contains(99));
    REQUIRE(s.erase(42) == 1);
    REQUIRE(s.empty());
}

TEST_CASE("node_hash_set: large insert", "[node_hash][set]") {
    zeta::node_hash_set<int> s;
    for (int i = 0; i < 500; ++i) s.insert(i);
    REQUIRE(s.size() == 500);
    for (int i = 0; i < 500; ++i) REQUIRE(s.contains(i));
}
