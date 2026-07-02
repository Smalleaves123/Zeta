#include "zeta/container/flat_hash_map.h"
#include "zeta/container/node_hash_map.h"

#include <benchmark/benchmark.h>

#include <cstdint>

static void BM_FlatHashMap_InsertFind(benchmark::State& state) {
    for (auto _ : state) {
        zeta::flat_hash_map<int, int> m;
        for (int i = 0; i < state.range(0); ++i) {
            m.insert({i, i * 2});
        }
        int sum = 0;
        for (int i = 0; i < state.range(0); ++i) {
            sum += m.at(i);
        }
        benchmark::DoNotOptimize(sum);
    }
}

static void BM_NodeHashMap_InsertFind(benchmark::State& state) {
    for (auto _ : state) {
        zeta::node_hash_map<int, int> m;
        for (int i = 0; i < state.range(0); ++i) {
            m.insert({i, i * 2});
        }
        int sum = 0;
        for (int i = 0; i < state.range(0); ++i) {
            sum += m.at(i);
        }
        benchmark::DoNotOptimize(sum);
    }
}

BENCHMARK(BM_FlatHashMap_InsertFind)->Arg(128)->Arg(1024)->Arg(4096);
BENCHMARK(BM_NodeHashMap_InsertFind)->Arg(128)->Arg(1024)->Arg(4096);

BENCHMARK_MAIN();
