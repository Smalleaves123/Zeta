#include "zeta/memory/memory_pool.h"
#include "zeta/memory/object_pool.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

namespace {

struct PooledValue {
    explicit PooledValue(int value) : value(value) {}
    ~PooledValue() { ++destructions; }

    int value;
    static inline int destructions = 0;
};

} // namespace

TEST_CASE("MemoryPool: reuses aligned fixed-size blocks", "[memory][pool]") {
    zeta::MemoryPool pool(sizeof(std::uint64_t), alignof(std::uint64_t), 2);
    pool.Reserve(3);

    REQUIRE(pool.Capacity() == 4);
    REQUIRE(pool.Available() == 4);

    void* first = pool.Allocate();
    void* second = pool.Allocate();
    REQUIRE(reinterpret_cast<std::uintptr_t>(first) % alignof(std::uint64_t) == 0);
    REQUIRE(reinterpret_cast<std::uintptr_t>(second) % alignof(std::uint64_t) == 0);
    REQUIRE(pool.Size() == 2);

    pool.Deallocate(first);
    REQUIRE(pool.Available() == 3);
    REQUIRE(pool.Allocate() == first);

    pool.Deallocate(second);
    pool.Deallocate(first);
    REQUIRE(pool.Size() == 0);
}

TEST_CASE("ObjectPool: constructs and destroys typed values",
          "[memory][pool]") {
    PooledValue::destructions = 0;
    zeta::ObjectPool<PooledValue> pool(2);
    pool.Reserve(3);

    auto* first = pool.Create(7);
    auto* second = pool.Create(9);
    REQUIRE(first->value == 7);
    REQUIRE(second->value == 9);
    REQUIRE(pool.Size() == 2);

    pool.Destroy(first);
    REQUIRE(PooledValue::destructions == 1);
    REQUIRE(pool.Size() == 1);
    pool.Destroy(second);
    REQUIRE(PooledValue::destructions == 2);
    REQUIRE(pool.Size() == 0);
}

TEST_CASE("ObjectPool: destroys live values when the pool is destroyed",
          "[memory][pool]") {
    PooledValue::destructions = 0;
    {
        zeta::ObjectPool<PooledValue> pool(2);
        (void)pool.Create(7);
        (void)pool.Create(9);
        REQUIRE(pool.Size() == 2);
    }
    REQUIRE(PooledValue::destructions == 2);
}
