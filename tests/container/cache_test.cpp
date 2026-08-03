#include "zeta/container/lfu_cache.h"
#include "zeta/container/lru_cache.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("LruCache: recent access controls eviction", "[container][cache]") {
    zeta::LruCache<std::string, int> cache(2);

    REQUIRE(cache.Put("a", 1));
    REQUIRE(cache.Put("b", 2));
    REQUIRE(cache.Find("a") != nullptr);
    REQUIRE(cache.Put("c", 3));
    REQUIRE(cache.Find("a") != nullptr);
    REQUIRE(cache.Find("b") == nullptr);
    REQUIRE(cache.Find("c") != nullptr);

    REQUIRE_FALSE(cache.Put("a", 4));
    REQUIRE(*cache.Find("a") == 4);
}

TEST_CASE("LfuCache: frequency controls eviction with recency ties",
          "[container][cache]") {
    zeta::LfuCache<std::string, int> cache(2);

    REQUIRE(cache.Put("a", 1));
    REQUIRE(cache.Put("b", 2));
    REQUIRE(cache.Find("a") != nullptr);
    REQUIRE(cache.Put("c", 3));
    REQUIRE(cache.Find("a") != nullptr);
    REQUIRE(cache.Find("b") == nullptr);
    REQUIRE(cache.Find("c") != nullptr);

    REQUIRE(cache.Erase("a"));
    REQUIRE(cache.size() == 1);
    REQUIRE_FALSE(cache.Erase("missing"));

    zeta::LfuCache<std::string, int> recomputed(2);
    REQUIRE(recomputed.Put("a", 1));
    REQUIRE(recomputed.Put("b", 2));
    REQUIRE(recomputed.Find("a") != nullptr);
    REQUIRE(recomputed.Erase("b"));
    REQUIRE(recomputed.Put("c", 3));
    REQUIRE(recomputed.Put("d", 4));
    REQUIRE(recomputed.Find("a") != nullptr);
    REQUIRE(recomputed.Find("c") == nullptr);
    REQUIRE(recomputed.Find("d") != nullptr);
}

TEST_CASE("Caches: support non-copyable values through Find",
          "[container][cache]") {
    zeta::LruCache<int, std::string> cache(1);
    REQUIRE(cache.Put(7, "value"));
    std::string* value = cache.Find(7);
    REQUIRE(value != nullptr);
    *value = "updated";
    REQUIRE(*cache.Find(7) == "updated");
}
