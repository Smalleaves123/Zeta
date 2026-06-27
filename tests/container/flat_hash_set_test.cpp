#include "zeta/container/flat_hash_set.h"
#include "zeta/container/internal/raw_hash_set.h"

#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::literals;

// ═══════════════════════════════════════════════════════════════════
// raw_hash_set internal unit tests (SIMD / Swiss Table primitives)
// ═══════════════════════════════════════════════════════════════════

namespace ci = zeta::container_internal;

TEST_CASE("raw_hash_set: Ctrl enum values", "[raw_hash_set][internal]") {
    static_assert(static_cast<int8_t>(ci::kEmpty)   == -128);
    static_assert(static_cast<int8_t>(ci::kDeleted) == -2);
    static_assert(ci::kSentinel == ci::kDeleted);
}

TEST_CASE("raw_hash_set: H2 hash truncation", "[raw_hash_set][internal]") {
    // H2 extracts bits [57..63] & 0x7F (7 bits, range [0, 127])
    REQUIRE(ci::H2(0) == 0);
    REQUIRE(ci::H2(0x7FULL << 57) == 0x7F);
    REQUIRE(ci::H2(0xFFULL << 57) == 0x7F); // top bit masked off
    REQUIRE(ci::H2(0x01ULL << 57) == 0x01);
    REQUIRE(ci::H2(0xDEADBEEF) == 0);        // low bits don't affect
}

TEST_CASE("raw_hash_set: next_power_of_2", "[raw_hash_set][internal]") {
    // Uses a private static method — tested via the public rehash/reserve API
    // which calls next_power_of_2 internally. We verify the observable result.
    zeta::flat_hash_set<int> s;
    REQUIRE(s.capacity() == 0);

    s.rehash(0);        // next_power_of_2(0) == 1
    REQUIRE(s.capacity() >= 1);

    s.rehash(3);        // next_power_of_2(3) == 4
    REQUIRE(s.capacity() >= 4);

    s.rehash(15);       // next_power_of_2(15) == 16
    REQUIRE(s.capacity() >= 16);
}

TEST_CASE("raw_hash_set: MapPolicy key extraction", "[raw_hash_set][internal]") {
    using MP = ci::MapPolicy<int, std::string>;
    std::pair<const int, std::string> p = {42, "hello"};
    const int& k = MP::get_key(p);
    REQUIRE(k == 42);
    REQUIRE(&k == &p.first);
}

TEST_CASE("raw_hash_set: SetPolicy key extraction", "[raw_hash_set][internal]") {
    using SP = ci::SetPolicy<int>;
    int v = 99;
    const int& k = SP::get_key(v);
    REQUIRE(k == 99);
    REQUIRE(&k == &v);
}

// ═══════════════════════════════════════════════════════════════════
// flat_hash_set tests
// ═══════════════════════════════════════════════════════════════════

// ── Construction ──────────────────────────────────────────────────

TEST_CASE("flat_hash_set: default construction", "[set]") {
    zeta::flat_hash_set<int> s;
    REQUIRE(s.empty());
    REQUIRE(s.size() == 0);
}

TEST_CASE("flat_hash_set: initializer_list construction", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    REQUIRE(s.size() == 3);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
}

// ── Insert / contains / emplace ───────────────────────────────────

TEST_CASE("flat_hash_set: insert and contains", "[set]") {
    zeta::flat_hash_set<int> s;
    auto [it, ok] = s.insert(42);
    REQUIRE(ok);
    REQUIRE(*it == 42);
    REQUIRE(s.contains(42));
    REQUIRE(!s.contains(99));
    REQUIRE(s.size() == 1);
}

TEST_CASE("flat_hash_set: insert duplicate", "[set]") {
    zeta::flat_hash_set<int> s;
    s.insert(10);
    auto [it, ok] = s.insert(10);
    REQUIRE(!ok);
    REQUIRE(*it == 10);
    REQUIRE(s.size() == 1);
}

TEST_CASE("flat_hash_set: emplace", "[set]") {
    zeta::flat_hash_set<std::string> s;
    auto [it, ok] = s.emplace(3, 'x');
    REQUIRE(ok);
    REQUIRE(*it == "xxx");
    REQUIRE(s.contains("xxx"));
}

TEST_CASE("flat_hash_set: rvalue insert", "[set]") {
    zeta::flat_hash_set<std::string> s;
    std::string key = "hello";
    s.insert(std::move(key));
    REQUIRE(s.contains("hello"));
}

// ── Erase ─────────────────────────────────────────────────────────

TEST_CASE("flat_hash_set: erase by key", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    REQUIRE(s.erase(2) == 1);
    REQUIRE(!s.contains(2));
    REQUIRE(s.size() == 2);
    REQUIRE(s.erase(99) == 0);
}

TEST_CASE("flat_hash_set: erase by iterator", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2};
    auto it = s.find(1);
    REQUIRE(it != s.end());
    auto next = s.erase(it);
    REQUIRE(s.size() == 1);
    REQUIRE(!s.contains(1));
    (void)next;
}

TEST_CASE("flat_hash_set: erase from empty", "[set][edge]") {
    zeta::flat_hash_set<int> s;
    REQUIRE(s.erase(1) == 0);
}

// ── Iteration ─────────────────────────────────────────────────────

TEST_CASE("flat_hash_set: iteration", "[set]") {
    zeta::flat_hash_set<int> s = {3, 1, 2};
    int sum = 0;
    for (int v : s) sum += v;
    REQUIRE(sum == 6);
}

TEST_CASE("flat_hash_set: const iteration", "[set]") {
    const zeta::flat_hash_set<int> s = {1, 2, 3};
    int sum = 0;
    for (const int& v : s) sum += v;
    REQUIRE(sum == 6);
}

TEST_CASE("flat_hash_set: cbegin / cend", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    auto it = s.cbegin();
    auto end = s.cend();
    REQUIRE(std::distance(it, end) == static_cast<decltype(std::distance(it, end))>(s.size()));
}

TEST_CASE("flat_hash_set: const find and contains", "[set]") {
    const zeta::flat_hash_set<int> s = {1, 2, 3};
    REQUIRE(s.contains(2));
    REQUIRE(s.find(2) != s.end());
    REQUIRE(*s.find(2) == 2);
}

// ── Capacity ──────────────────────────────────────────────────────

TEST_CASE("flat_hash_set: large insert", "[set]") {
    zeta::flat_hash_set<int> s;
    constexpr int N = 1000;
    for (int i = 0; i < N; ++i) s.insert(i);
    REQUIRE(s.size() == N);
    for (int i = 0; i < N; ++i) REQUIRE(s.contains(i));
}

TEST_CASE("flat_hash_set: reserve", "[set]") {
    zeta::flat_hash_set<int> s;
    s.reserve(100);
    REQUIRE(s.capacity() >= 100);
    REQUIRE(s.empty());
}

TEST_CASE("flat_hash_set: rehash", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    size_t old_cap = s.capacity();
    s.rehash(old_cap * 2);
    REQUIRE(s.capacity() >= old_cap * 2);
    REQUIRE(s.size() == 3);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
}

TEST_CASE("flat_hash_set: count", "[set]") {
    const zeta::flat_hash_set<int> s = {1, 2};
    REQUIRE(s.count(1) == 1);
    REQUIRE(s.count(3) == 0);
}

// ── Copy / move / swap ────────────────────────────────────────────

TEST_CASE("flat_hash_set: copy", "[set]") {
    zeta::flat_hash_set<int> a = {1, 2, 3};
    auto b = a;
    REQUIRE(b.size() == 3);
    REQUIRE(a == b);
}

TEST_CASE("flat_hash_set: move", "[set]") {
    zeta::flat_hash_set<int> a = {1, 2, 3};
    auto b = std::move(a);
    REQUIRE(b.size() == 3);
    REQUIRE(a.empty());
}

TEST_CASE("flat_hash_set: clear", "[set]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    s.clear();
    REQUIRE(s.empty());
}

TEST_CASE("flat_hash_set: swap", "[set]") {
    zeta::flat_hash_set<int> a = {1, 2};
    zeta::flat_hash_set<int> b = {3, 4, 5};
    a.swap(b);
    REQUIRE(a.size() == 3);
    REQUIRE(b.size() == 2);
    REQUIRE(a.contains(3));
    REQUIRE(b.contains(1));
}

TEST_CASE("flat_hash_set: self-swap", "[set][edge]") {
    zeta::flat_hash_set<int> s = {1, 2, 3};
    s.swap(s);
    REQUIRE(s.size() == 3);
    REQUIRE(s.contains(1));
    REQUIRE(s.contains(2));
    REQUIRE(s.contains(3));
}

TEST_CASE("flat_hash_set: equality", "[set]") {
    zeta::flat_hash_set<int> a = {1, 2, 3};
    zeta::flat_hash_set<int> b = {1, 2, 3};
    zeta::flat_hash_set<int> c = {1, 2};
    REQUIRE(a == b);
    REQUIRE(a != c);
}

// ── Heterogeneous lookup ──────────────────────────────────────────

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

TEST_CASE("flat_hash_set: heterogeneous lookup", "[set][hetero]") {
    zeta::flat_hash_set<std::string, TransparentHash, TransparentEq> s;
    s.insert("hello");
    s.insert("world");

    REQUIRE(s.contains("hello"sv));
    REQUIRE(s.contains("world"sv));
    REQUIRE(!s.contains("missing"sv));

    auto it = s.find("hello"sv);
    REQUIRE(it != s.end());
    REQUIRE(*it == "hello");

    REQUIRE(s.erase("world"sv) == 1);
    REQUIRE(s.count("hello"sv) == 1);
    REQUIRE(s.count("world"sv) == 0);
}

// ── Stress / edge ─────────────────────────────────────────────────

struct CollisionKey {
    int value;
    bool operator==(const CollisionKey& o) const { return value == o.value; }
};
struct CollisionHash {
    size_t operator()(const CollisionKey&) const noexcept { return 0; }
};

struct RehashThrowingItem {
    int value{0};

    static inline int live_count = 0;
    static inline int move_calls = 0;
    static inline int move_throw_after = -1;

    RehashThrowingItem() = default;
    explicit RehashThrowingItem(int v) : value(v) { ++live_count; }
    RehashThrowingItem(const RehashThrowingItem&) = delete;
    RehashThrowingItem(RehashThrowingItem&& other) noexcept(false)
        : value(other.value) {
        if (move_throw_after >= 0 && move_calls++ >= move_throw_after) {
            throw std::runtime_error("move");
        }
        ++live_count;
    }
    RehashThrowingItem& operator=(const RehashThrowingItem&) = delete;
    RehashThrowingItem& operator=(RehashThrowingItem&&) = default;
    ~RehashThrowingItem() { --live_count; }

    static void Reset() {
        live_count = 0;
        move_calls = 0;
        move_throw_after = -1;
    }

    bool operator==(const RehashThrowingItem& other) const {
        return value == other.value;
    }
};

struct RehashThrowingHash {
    size_t operator()(const RehashThrowingItem& item) const noexcept {
        return std::hash<int>{}(item.value);
    }
};

TEST_CASE("flat_hash_set: collision-heavy keys", "[set][stress]") {
    zeta::flat_hash_set<CollisionKey, CollisionHash> s;
    constexpr int N = 500;
    for (int i = 0; i < N; ++i) s.insert({i});
    REQUIRE(s.size() == N);
    for (int i = 0; i < N; ++i) REQUIRE(s.contains({i}));
}

TEST_CASE("flat_hash_set: interleaved insert and erase (tombstone)", "[set][stress]") {
    zeta::flat_hash_set<int> s;
    for (int round = 0; round < 10; ++round) {
        for (int i = 0; i < 100; ++i) s.insert(i);
        for (int i = 0; i < 100; i += 2) s.erase(i);
        for (int i = 0; i < 100; i += 2) s.insert(i + 100);
    }
    for (int i = 0; i < 100; i += 2)  REQUIRE(!s.contains(i));
    for (int i = 1; i < 100; i += 2)  REQUIRE(s.contains(i));
    for (int i = 0; i < 100; i += 2)  REQUIRE(s.contains(i + 100));
}

TEST_CASE("flat_hash_set: rehash rolls back on move throw", "[set][exception]") {
    RehashThrowingItem::Reset();
    zeta::flat_hash_set<RehashThrowingItem, RehashThrowingHash> s;
    s.emplace(1);
    s.emplace(2);
    s.emplace(3);

    const size_t old_capacity = s.capacity();
    RehashThrowingItem::move_calls = 0;
    RehashThrowingItem::move_throw_after = 1;

    REQUIRE_THROWS_AS(s.rehash(old_capacity * 2), std::runtime_error);

    REQUIRE(s.size() == 3);
    REQUIRE(RehashThrowingItem::live_count == 3);

    std::vector<int> values;
    values.reserve(s.size());
    for (const auto& item : s) values.push_back(item.value);
    std::sort(values.begin(), values.end());
    REQUIRE(values == std::vector<int>{1, 2, 3});
}
