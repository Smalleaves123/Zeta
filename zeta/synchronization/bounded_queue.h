#ifndef ZETA_SYNCHRONIZATION_BOUNDED_QUEUE_H
#define ZETA_SYNCHRONIZATION_BOUNDED_QUEUE_H

/// @file   synchronization/bounded_queue.h
/// @brief  Mutex-protected non-blocking queue with a fixed capacity.

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>

namespace zeta {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("bounded queue capacity must not be zero");
        }
    }

    BoundedQueue(const BoundedQueue&) = delete;
    BoundedQueue& operator=(const BoundedQueue&) = delete;
    BoundedQueue(BoundedQueue&&) = delete;
    BoundedQueue& operator=(BoundedQueue&&) = delete;

    [[nodiscard]] bool TryPush(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.size() == capacity_) return false;
        queue_.push_back(std::move(value));
        return true;
    }

    [[nodiscard]] std::optional<T> TryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;
        std::optional<T> value(std::in_place, std::move(queue_.front()));
        queue_.pop_front();
        return value;
    }

    void Clear() noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    [[nodiscard]] std::size_t Capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] bool Empty() const noexcept {
        return Size() == 0;
    }

    [[nodiscard]] bool Full() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size() == capacity_;
    }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::deque<T> queue_;
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_BOUNDED_QUEUE_H
