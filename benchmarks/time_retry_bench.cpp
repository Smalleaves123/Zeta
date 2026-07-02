#include "zeta/time/retry.h"

#include <benchmark/benchmark.h>

static void BM_TimeRetryPolicy(benchmark::State& state) {
    for (auto _ : state) {
        zeta::RetryPolicy policy(
            zeta::ExponentialBackoff(zeta::Duration::Milliseconds(5),
                                     zeta::Duration::Milliseconds(80)),
            16,
            zeta::Deadline::After(zeta::Duration::Milliseconds(250)));

        std::size_t attempts = 0;
        while (policy.CanRetry()) {
            benchmark::DoNotOptimize(policy.NextDelay());
            policy.Advance();
            ++attempts;
        }

        benchmark::DoNotOptimize(attempts);
    }
}

BENCHMARK(BM_TimeRetryPolicy)->Iterations(10000);
BENCHMARK_MAIN();
