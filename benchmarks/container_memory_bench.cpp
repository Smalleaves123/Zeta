#include "zeta/container/inlined_vector.h"
#include "zeta/container/small_map.h"
#include "zeta/memory/byte_buffer.h"
#include "zeta/memory/memory_pool.h"
#include "zeta/memory/object_pool.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct PoolValue {
    explicit PoolValue(std::uint64_t value) : value(value) {}

    std::uint64_t value;
};

struct Block64 {
    std::byte bytes[64];
};

static void BM_StdVectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<std::uint64_t> values;
        for (std::int64_t index = 0; index != state.range(0); ++index) {
            values.push_back(static_cast<std::uint64_t>(index));
        }
        benchmark::DoNotOptimize(values.data());
    }
}

static void BM_InlinedVectorPushBack(benchmark::State& state) {
    for (auto _ : state) {
        zeta::InlinedVector<std::uint64_t, 8> values;
        for (std::int64_t index = 0; index != state.range(0); ++index) {
            values.push_back(static_cast<std::uint64_t>(index));
        }
        benchmark::DoNotOptimize(values.data());
    }
}

static void BM_SmallMapInsertFind(benchmark::State& state) {
    for (auto _ : state) {
        zeta::SmallMap<int, int, 8> values;
        for (int index = 0; index != state.range(0); ++index) {
            values.try_emplace(index, index * 2);
        }

        int sum = 0;
        for (int index = 0; index != state.range(0); ++index) {
            sum += values.at(index);
        }
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_StdUnorderedMapInsertFind(benchmark::State& state) {
    for (auto _ : state) {
        std::unordered_map<int, int> values;
        for (int index = 0; index != state.range(0); ++index) {
            values.emplace(index, index * 2);
        }

        int sum = 0;
        for (int index = 0; index != state.range(0); ++index) {
            sum += values.at(index);
        }
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_MemoryPoolRoundTrip(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    zeta::MemoryPool pool(64, alignof(std::max_align_t), count);
    pool.Reserve(count);
    std::vector<void*> blocks(count);

    for (auto _ : state) {
        for (void*& block : blocks) block = pool.Allocate();
        benchmark::DoNotOptimize(blocks.data());
        for (void* block : blocks) pool.Deallocate(block);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_OperatorNewRoundTrip(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    std::vector<Block64*> blocks(count);

    for (auto _ : state) {
        for (Block64*& block : blocks) block = new Block64;
        benchmark::DoNotOptimize(blocks.data());
        for (Block64* block : blocks) delete block;
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_ObjectPoolRoundTrip(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    zeta::ObjectPool<PoolValue> pool(count);
    pool.Reserve(count);
    std::vector<PoolValue*> objects(count);

    for (auto _ : state) {
        for (std::size_t index = 0; index != count; ++index) {
            objects[index] = pool.Create(index);
        }
        benchmark::DoNotOptimize(objects.data());
        for (PoolValue* object : objects) pool.Destroy(object);
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_NewDeleteObjectRoundTrip(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    std::vector<PoolValue*> objects(count);

    for (auto _ : state) {
        for (std::size_t index = 0; index != count; ++index) {
            objects[index] = new PoolValue(index);
        }
        benchmark::DoNotOptimize(objects.data());
        for (PoolValue* object : objects) delete object;
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

static void BM_ByteBufferAppend(benchmark::State& state) {
    const auto chunk_size = static_cast<std::size_t>(state.range(0));
    const std::string chunk(chunk_size, 'x');
    const auto total_size = chunk_size * 16;

    for (auto _ : state) {
        zeta::ByteBuffer buffer(total_size);
        for (int index = 0; index != 16; ++index) buffer.Append(chunk);
        benchmark::DoNotOptimize(buffer.ReadableBytes().data());
        benchmark::DoNotOptimize(buffer.Size());
    }
    state.SetBytesProcessed(state.iterations() * total_size);
}

static void BM_ByteBufferAppendConsume(benchmark::State& state) {
    const auto chunk_size = static_cast<std::size_t>(state.range(0));
    const std::string chunk(chunk_size, 'x');
    zeta::ByteBuffer buffer(chunk_size * 16);

    for (auto _ : state) {
        buffer.Append(chunk);
        buffer.Append(chunk);
        benchmark::DoNotOptimize(buffer.ReadableBytes().data());
        benchmark::DoNotOptimize(buffer.Consume(chunk_size));
        buffer.Append(chunk);  // Exercises compaction after the prefix read.
        benchmark::DoNotOptimize(buffer.Consume(chunk_size * 2));
    }
    state.SetBytesProcessed(state.iterations() * chunk_size * 3);
}

}  // namespace

BENCHMARK(BM_StdVectorPushBack)->Arg(4)->Arg(8)->Arg(32)->Arg(128);
BENCHMARK(BM_InlinedVectorPushBack)->Arg(4)->Arg(8)->Arg(32)->Arg(128);
BENCHMARK(BM_SmallMapInsertFind)->Arg(4)->Arg(8)->Arg(32);
BENCHMARK(BM_StdUnorderedMapInsertFind)->Arg(4)->Arg(8)->Arg(32);
BENCHMARK(BM_MemoryPoolRoundTrip)->Arg(8)->Arg(64)->Arg(512);
BENCHMARK(BM_OperatorNewRoundTrip)->Arg(8)->Arg(64)->Arg(512);
BENCHMARK(BM_ObjectPoolRoundTrip)->Arg(8)->Arg(64)->Arg(512);
BENCHMARK(BM_NewDeleteObjectRoundTrip)->Arg(8)->Arg(64)->Arg(512);
BENCHMARK(BM_ByteBufferAppend)->Arg(64)->Arg(1024);
BENCHMARK(BM_ByteBufferAppendConsume)->Arg(64)->Arg(1024);

BENCHMARK_MAIN();
