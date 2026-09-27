#include "zeta/container/flat_hash_map.h"

#include <benchmark/benchmark.h>

#include <cstddef>

static void BM_FlatHashMap_InsertFind(benchmark::State& state) {
    for (auto _ : state) {
        zeta::flat_hash_map<int, int> values;
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

static void BM_FlatHashMap_ReserveInsertFind(benchmark::State& state) {
    for (auto _ : state) {
        zeta::flat_hash_map<int, int> values;
        values.reserve(static_cast<std::size_t>(state.range(0)));
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

static void BM_FlatHashMap_EraseInsertChurn(benchmark::State& state) {
    const int count = state.range(0);
    for (auto _ : state) {
        zeta::flat_hash_map<int, int> values;
        values.reserve(static_cast<std::size_t>(count) * 2);
        for (int index = 0; index != count; ++index) {
            values.emplace(index, index);
        }

        for (int round = 0; round != 4; ++round) {
            for (int index = 0; index != count; ++index) {
                values.erase(index);
            }
            for (int index = 0; index != count; ++index) {
                values.emplace(index, index + round);
            }
        }

        benchmark::DoNotOptimize(values.size());
        benchmark::DoNotOptimize(values.at(count / 2));
    }
}

BENCHMARK(BM_FlatHashMap_InsertFind)->Arg(128)->Arg(1024)->Arg(4096);
BENCHMARK(BM_FlatHashMap_ReserveInsertFind)
    ->Arg(128)
    ->Arg(1024)
    ->Arg(4096);
BENCHMARK(BM_FlatHashMap_EraseInsertChurn)->Arg(128)->Arg(1024)->Arg(4096);

BENCHMARK_MAIN();
