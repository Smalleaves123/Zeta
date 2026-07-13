#ifndef ZETA_METRICS_METRICS_H
#define ZETA_METRICS_METRICS_H

/// @file   metrics/metrics.h
/// @brief  Small thread-safe metrics primitives for application instrumentation.
///
/// The types in this header are local instruments. They intentionally do not
/// provide a process-wide registry or a wire format; applications can expose
/// snapshots through their preferred metrics backend.

#include "zeta/time/clock.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace zeta {
namespace metrics {

class Counter {
public:
    explicit Counter(std::uint64_t initial = 0) noexcept
        : value_(initial) {}

    void Increment(std::uint64_t delta = 1) noexcept {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }

    [[nodiscard]] std::uint64_t value() const noexcept {
        return value_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::uint64_t> value_;
};

class Gauge {
public:
    explicit Gauge(std::int64_t initial = 0) noexcept
        : value_(initial) {}

    void Set(std::int64_t value) noexcept {
        value_.store(value, std::memory_order_relaxed);
    }

    void Add(std::int64_t delta) noexcept {
        value_.fetch_add(delta, std::memory_order_relaxed);
    }

    [[nodiscard]] std::int64_t value() const noexcept {
        return value_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<std::int64_t> value_;
};

struct HistogramSnapshot {
    // Bucket i contains samples <= upper_bounds[i]. The final bucket is the
    // overflow bucket and contains samples greater than the final bound.
    std::vector<std::int64_t> upper_bounds;
    std::vector<std::uint64_t> bucket_counts;
    std::uint64_t count = 0;
    std::int64_t sum = 0;
};

class Histogram {
public:
    Histogram(std::initializer_list<std::int64_t> upper_bounds)
        : upper_bounds_(upper_bounds),
          bucket_counts_(upper_bounds_.size() + 1, 0) {
        std::sort(upper_bounds_.begin(), upper_bounds_.end());
        upper_bounds_.erase(
            std::unique(upper_bounds_.begin(), upper_bounds_.end()),
            upper_bounds_.end());
        bucket_counts_.assign(upper_bounds_.size() + 1, 0);
    }

    void Observe(std::int64_t sample) {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto bucket = static_cast<std::size_t>(
            std::lower_bound(upper_bounds_.begin(), upper_bounds_.end(), sample) -
            upper_bounds_.begin());
        ++bucket_counts_[bucket];
        ++count_;
        sum_ = SaturatingAdd(sum_, sample);
    }

    [[nodiscard]] HistogramSnapshot Snapshot() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return HistogramSnapshot{
            upper_bounds_, bucket_counts_, count_, sum_};
    }

private:
    [[nodiscard]] static std::int64_t SaturatingAdd(
        std::int64_t left, std::int64_t right) noexcept {
        constexpr auto kMax = std::numeric_limits<std::int64_t>::max();
        constexpr auto kMin = std::numeric_limits<std::int64_t>::min();
        if (right > 0 && left > kMax - right) return kMax;
        if (right < 0 && left < kMin - right) return kMin;
        return left + right;
    }

    mutable std::mutex mutex_;
    std::vector<std::int64_t> upper_bounds_;
    std::vector<std::uint64_t> bucket_counts_;
    std::uint64_t count_ = 0;
    std::int64_t sum_ = 0;
};

/// Observes elapsed monotonic nanoseconds when the scope exits.
class ScopedTimer {
public:
    explicit ScopedTimer(Histogram& histogram) noexcept
        : histogram_(histogram), start_(Clock::Now()) {}

    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

    ~ScopedTimer() {
        histogram_.Observe(Clock::Between(start_, Clock::Now()).ToNanoseconds());
    }

private:
    Histogram& histogram_;
    Clock::time_point start_;
};

} // namespace metrics
} // namespace zeta

#endif // ZETA_METRICS_METRICS_H
