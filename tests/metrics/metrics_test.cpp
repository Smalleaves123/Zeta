#include "zeta/metrics/metrics.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <thread>
#include <vector>

TEST_CASE("metrics: counter and gauge are thread-safe", "[metrics]") {
    zeta::metrics::Counter counter;
    zeta::metrics::Gauge gauge(10);
    std::vector<std::thread> workers;

    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&] {
            for (int j = 0; j < 1000; ++j) {
                counter.Increment();
                gauge.Add(1);
            }
        });
    }
    for (auto& worker : workers) worker.join();

    REQUIRE(counter.value() == 4000);
    REQUIRE(gauge.value() == 4010);

    gauge.Set(-7);
    REQUIRE(gauge.value() == -7);
}

TEST_CASE("metrics: histogram sorts bounds and records overflow", "[metrics]") {
    zeta::metrics::Histogram histogram({100, 10, 0, 10});

    histogram.Observe(-1);
    histogram.Observe(0);
    histogram.Observe(5);
    histogram.Observe(10);
    histogram.Observe(11);
    histogram.Observe(100);
    histogram.Observe(101);

    const auto snapshot = histogram.Snapshot();
    REQUIRE(snapshot.upper_bounds == std::vector<std::int64_t>{0, 10, 100});
    REQUIRE(snapshot.bucket_counts ==
            std::vector<std::uint64_t>{2, 2, 2, 1});
    REQUIRE(snapshot.count == 7);
    REQUIRE(snapshot.sum == 226);
}

TEST_CASE("metrics: scoped timer observes one sample", "[metrics][time]") {
    zeta::metrics::Histogram histogram({0});
    {
        zeta::metrics::ScopedTimer timer(histogram);
        std::this_thread::yield();
    }

    const auto snapshot = histogram.Snapshot();
    REQUIRE(snapshot.count == 1);
    REQUIRE(snapshot.bucket_counts.size() == 2);
}
