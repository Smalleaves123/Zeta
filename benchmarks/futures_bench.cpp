#include "zeta/futures/coroutine.h"

#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <utility>
#include <vector>

namespace {

zeta::ThreadPoolExecutor benchmark_thread_pool(1);
zeta::ScheduledExecutor benchmark_scheduler;

zeta::Coroutine<int> AddOneCoroutine(zeta::Future<int> input) {
    auto result = co_await std::move(input);
    if (!result.ok()) co_return result.status();
    co_return result.value() + 1;
}

template <typename T>
void CompletePromise(zeta::Promise<T>& promise, T value) {
    benchmark::DoNotOptimize(promise.SetValue(std::move(value)));
}

static void BM_FuturePromiseGet(benchmark::State& state) {
    for (auto _ : state) {
        auto [promise, future] = zeta::makePromiseContract<int>();
        CompletePromise(promise, 42);
        auto result = std::move(future).Get();
        benchmark::DoNotOptimize(result.value());
    }
}

static void BM_StdFuturePromiseGet(benchmark::State& state) {
    for (auto _ : state) {
        std::promise<int> promise;
        auto future = promise.get_future();
        promise.set_value(42);
        benchmark::DoNotOptimize(future.get());
    }
}

static void BM_FutureThenInline(benchmark::State& state) {
    for (auto _ : state) {
        auto [promise, future] = zeta::makePromiseContract<int>();
        auto next = std::move(future).Then([](int value) {
            return value + 1;
        });
        CompletePromise(promise, 41);
        auto result = std::move(next).Get();
        benchmark::DoNotOptimize(result.value());
    }
}

static void BM_FutureThenThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        auto [promise, future] = zeta::makePromiseContract<int>();
        auto next = std::move(future).Via(benchmark_thread_pool).Then([](int value) {
            return value + 1;
        });
        CompletePromise(promise, 41);
        auto result = std::move(next).Get();
        benchmark::DoNotOptimize(result.value());
    }
}

static void BM_CollectAll(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<zeta::Promise<int>> promises;
        std::vector<zeta::Future<int>> futures;
        promises.reserve(count);
        futures.reserve(count);
        for (std::size_t index = 0; index != count; ++index) {
            auto [promise, future] = zeta::makePromiseContract<int>();
            promises.push_back(std::move(promise));
            futures.push_back(std::move(future));
        }

        auto grouped = zeta::collectAll(std::move(futures));
        for (std::size_t index = 0; index != count; ++index) {
            benchmark::DoNotOptimize(promises[index].SetValue(
                static_cast<int>(index)));
        }
        auto result = std::move(grouped).Get();
        benchmark::DoNotOptimize(result.value().size());
    }
}

static void BM_CollectAny(benchmark::State& state) {
    const auto count = static_cast<std::size_t>(state.range(0));
    for (auto _ : state) {
        std::vector<zeta::Promise<int>> promises;
        std::vector<zeta::Future<int>> futures;
        promises.reserve(count);
        futures.reserve(count);
        for (std::size_t index = 0; index != count; ++index) {
            auto [promise, future] = zeta::makePromiseContract<int>();
            promises.push_back(std::move(promise));
            futures.push_back(std::move(future));
        }

        auto grouped = zeta::collectAny(std::move(futures));
        const auto winner = count / 2;
        benchmark::DoNotOptimize(promises[winner].SetValue(1));
        auto result = std::move(grouped).Get();
        benchmark::DoNotOptimize(result.value().first);

        // Complete the losers so their continuations are drained before the
        // next benchmark iteration begins.
        for (std::size_t index = 0; index != count; ++index) {
            if (index != winner) {
                benchmark::DoNotOptimize(promises[index].SetValue(0));
            }
        }
    }
}

static void BM_CancellationRegistration(benchmark::State& state) {
    for (auto _ : state) {
        zeta::CancellationSource source;
        auto registration = source.GetToken().Register([] {});
        registration.Reset();
    }
}

static void BM_ScheduledExecutor(benchmark::State& state) {
    for (auto _ : state) {
        auto [promise, future] = zeta::makePromiseContract<void>();
        auto shared_promise = std::make_shared<zeta::Promise<void>>(
            std::move(promise));
        benchmark_scheduler.ScheduleAfter(
            std::chrono::steady_clock::duration::zero(),
            [shared_promise]() mutable {
                benchmark::DoNotOptimize(shared_promise->SetValue());
            });
        benchmark::DoNotOptimize(std::move(future).Get().ok());
    }
}

static void BM_CoroutineAwait(benchmark::State& state) {
    for (auto _ : state) {
        auto [promise, future] = zeta::makePromiseContract<int>();
        auto output = AddOneCoroutine(std::move(future)).GetFuture();
        CompletePromise(promise, 41);
        auto result = std::move(output).Get();
        benchmark::DoNotOptimize(result.value());
    }
}

} // namespace

BENCHMARK(BM_FuturePromiseGet);
BENCHMARK(BM_StdFuturePromiseGet);
BENCHMARK(BM_FutureThenInline);
BENCHMARK(BM_FutureThenThreadPool);
BENCHMARK(BM_CollectAll)->Arg(1)->Arg(8)->Arg(64);
BENCHMARK(BM_CollectAny)->Arg(1)->Arg(8)->Arg(64);
BENCHMARK(BM_CancellationRegistration);
BENCHMARK(BM_ScheduledExecutor);
BENCHMARK(BM_CoroutineAwait);

BENCHMARK_MAIN();
