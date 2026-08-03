#ifndef ZETA_SYNCHRONIZATION_CHANNEL_H
#define ZETA_SYNCHRONIZATION_CHANNEL_H

/// @file   synchronization/channel.h
/// @brief  Thread-safe blocking queue with close semantics.

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <utility>

namespace zeta {

template <typename T>
class Channel {
public:
    explicit Channel(std::size_t capacity = 0) noexcept
        : capacity_(capacity) {}

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    [[nodiscard]] bool Send(T value) {
        std::unique_lock<std::mutex> lock(mutex_);
        not_full_.wait(lock, [this] {
            return closed_ || capacity_ == 0 || queue_.size() < capacity_;
        });
        if (closed_) return false;

        queue_.push_back(std::move(value));
        lock.unlock();
        not_empty_.notify_one();
        return true;
    }

    [[nodiscard]] bool TrySend(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_ || (capacity_ != 0 && queue_.size() >= capacity_)) {
            return false;
        }

        queue_.push_back(std::move(value));
        not_empty_.notify_one();
        return true;
    }

    [[nodiscard]] std::optional<T> Receive() {
        std::unique_lock<std::mutex> lock(mutex_);
        not_empty_.wait(lock, [this] { return closed_ || !queue_.empty(); });
        if (queue_.empty()) return std::nullopt;

        std::optional<T> value(std::in_place, std::move(queue_.front()));
        queue_.pop_front();
        lock.unlock();
        not_full_.notify_one();
        return value;
    }

    [[nodiscard]] std::optional<T> TryReceive() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return std::nullopt;

        std::optional<T> value(std::in_place, std::move(queue_.front()));
        queue_.pop_front();
        not_full_.notify_one();
        return value;
    }

    void Close() noexcept {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) return;
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    [[nodiscard]] bool IsClosed() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    [[nodiscard]] std::size_t Size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    const std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T> queue_;
    bool closed_ = false;
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_CHANNEL_H
