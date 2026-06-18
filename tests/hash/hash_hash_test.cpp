#include "zeta/hash/hash.h"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <string_view>
#include <vector>

using namespace zeta;

// ═══════════════════════════════════════════════════════════════════════
// HashState — one-shot
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("hash: Combined produces non-zero hash", "[hash][combined]") {
    uint64_t h = HashState::Combined(0, 42);
    REQUIRE(h != 0);
    REQUIRE(h != 42);
}

TEST_CASE("hash: CombinedBytes produces non-zero hash", "[hash][combined]") {
    const char* s = "hello";
    uint64_t h = HashState::CombinedBytes(s, std::strlen(s));
    REQUIRE(h != 0);
}

TEST_CASE("hash: Combined is deterministic", "[hash][combined]") {
    uint64_t h1 = HashState::Combined(0, 1, 2, 3);
    uint64_t h2 = HashState::Combined(0, 1, 2, 3);
    REQUIRE(h1 == h2);
}

TEST_CASE("hash: different seed → different hash", "[hash][combined]") {
    uint64_t h1 = HashState::Combined(0, 42);
    uint64_t h2 = HashState::Combined(12345, 42);
    // Different seeds should (with high probability) give different results.
    REQUIRE(h1 != h2);
}

TEST_CASE("hash: different values → different hash", "[hash][combined]") {
    uint64_t h1 = HashState::Combined(0, 42);
    uint64_t h2 = HashState::Combined(0, 43);
    REQUIRE(h1 != h2);
}

// ═══════════════════════════════════════════════════════════════════════
// HashState — incremental (Feed / Finalize)
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("hash: incremental equals one-shot", "[hash][incremental]") {
    const int data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    size_t len = sizeof(data);

    // Incremental.
    HashState hs(0);
    hs.Feed(data, len);
    uint64_t h_inc = hs.Finalize();

    // One-shot.
    uint64_t h_one = HashState::CombinedBytes(data, len);

    REQUIRE(h_inc == h_one);
}

TEST_CASE("hash: incremental across multiple Feed calls", "[hash][incremental]") {
    const char part1[] = "hello ";
    const char part2[] = "world";

    HashState hs(0);
    hs.Feed(part1, std::strlen(part1));
    hs.Feed(part2, std::strlen(part2));
    uint64_t h_inc = hs.Finalize();

    const char* full = "hello world";
    uint64_t h_one = HashState::CombinedBytes(full, std::strlen(full));

    REQUIRE(h_inc == h_one);
}

TEST_CASE("hash: incremental with small chunks (< 32 bytes)", "[hash][incremental]") {
    HashState hs(0);
    for (int i = 0; i < 100; ++i) {
        hs.Feed(&i, 1);  // feed 1 byte at a time
    }
    uint64_t h = hs.Finalize();
    REQUIRE(h != 0);
}

// ═══════════════════════════════════════════════════════════════════════
// Hash<T> specializations
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("hash: Hash<std::string>", "[hash][string]") {
    Hash<std::string> h;
    REQUIRE(h("hello") != 0);
    REQUIRE(h("hello") == h("hello"));               // deterministic
    REQUIRE(h("hello") != h("world"));               // different strings
}

TEST_CASE("hash: Hash<std::string_view>", "[hash][string]") {
    Hash<std::string_view> h;
    std::string_view a = "abc";
    std::string_view b = "xyz";
    REQUIRE(h(a) == h(a));
    REQUIRE(h(a) != h(b));
}

TEST_CASE("hash: Hash<const char*>", "[hash][string]") {
    Hash<const char*> h;
    const char* a = "hello";
    const char* b = "world";
    REQUIRE(h(a) == h(a));
    REQUIRE(h(a) != h(b));
}

TEST_CASE("hash: Hash<int> falls back to std::hash<int>", "[hash][builtin]") {
    Hash<int> h;
    REQUIRE(h(42) == std::hash<int>{}(42));
}

// ═══════════════════════════════════════════════════════════════════════
// Free functions
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("hash: hash_combine", "[hash][free]") {
    uint64_t h = hash_combine(0, 1, 2, 3);
    REQUIRE(h != 0);

    // Deterministic.
    REQUIRE(hash_combine(0, 1, 2, 3) == h);

    // Different order → different hash.
    REQUIRE(hash_combine(0, 3, 2, 1) != h);
}

TEST_CASE("hash: hash_range", "[hash][free]") {
    std::vector<int> v1 = {1, 2, 3};
    std::vector<int> v2 = {1, 2, 3};
    std::vector<int> v3 = {1, 2, 4};

    uint64_t h1 = hash_range(v1.begin(), v1.end());
    uint64_t h2 = hash_range(v2.begin(), v2.end());
    uint64_t h3 = hash_range(v3.begin(), v3.end());

    REQUIRE(h1 == h2);
    REQUIRE(h1 != h3);
    REQUIRE(h1 != 0);
}

TEST_CASE("hash: hash_bytes", "[hash][free]") {
    const char* data = "some bytes to hash";
    uint64_t h1 = hash_bytes(data, std::strlen(data));
    uint64_t h2 = hash_bytes(data, std::strlen(data));
    REQUIRE(h1 == h2);
    REQUIRE(h1 != 0);
}

TEST_CASE("hash: hash_bytes with seed", "[hash][free]") {
    const char* data = "hello";
    uint64_t h1 = hash_bytes(data, 5, 0);
    uint64_t h2 = hash_bytes(data, 5, 42);
    REQUIRE(h1 != h2);
}

// ═══════════════════════════════════════════════════════════════════════
// Quality checks (avalanche / distribution)
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("hash: avalanche — small changes flip many bits", "[hash][quality]") {
    uint64_t h1 = HashState::Combined(0, 0);
    uint64_t h2 = HashState::Combined(0, 1);
    // h1 and h2 should differ in many bit positions.
    uint64_t diff = h1 ^ h2;
    int bit_count = 0;
    while (diff) { bit_count += (diff & 1); diff >>= 1; }
    // A good hash should differ in at least 20 out of 64 bits for 1-bit input change.
    REQUIRE(bit_count >= 20);
}

TEST_CASE("hash: empty input produces non-zero hash", "[hash][quality]") {
    HashState hs(0);
    uint64_t h = hs.Finalize();
    REQUIRE(h != 0);
}
