#ifndef ZETA_SYNCHRONIZATION_SYNCHRONIZED_H
#define ZETA_SYNCHRONIZATION_SYNCHRONIZED_H

/// @file   synchronization/synchronized.h
/// @brief  Mutex-protected value wrapper for shared mutable state.
///
/// `zeta::Synchronized<T>` is a small Folly-style helper that bundles a
/// value with `zeta::Mutex` and exposes scoped read/write accessors.
/// It is useful when a single shared object needs explicit synchronization
/// without manually threading a mutex through every call site.
///
/// Example:
///   zeta::Synchronized<std::vector<int>> data;
///   {
///       auto w = data.wlock();
///       w->push_back(42);
///   }
///   auto r = data.rlock();
///   std::cout << r->size();

#include "zeta/synchronization/mutex.h"

#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T, typename MutexType = Mutex>
class Synchronized {
public:
    using value_type = T;
    using mutex_type = MutexType;

    Synchronized()
        requires std::default_initializable<T>
        : value_(std::in_place) {}

    template <typename... Args>
    explicit Synchronized(std::in_place_t, Args&&... args)
        : value_(std::in_place, std::forward<Args>(args)...) {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    Synchronized(T value) : value_(std::move(value)) {}

    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;

    Synchronized(Synchronized&&) = delete;
    Synchronized& operator=(Synchronized&&) = delete;

    class ReadAccess {
    public:
        ReadAccess(const ReadAccess&) = delete;
        ReadAccess& operator=(const ReadAccess&) = delete;

        ReadAccess(ReadAccess&& other) noexcept
            : parent_(std::exchange(other.parent_, nullptr)) {}

        ~ReadAccess() {
            if (parent_ != nullptr) {
                parent_->mutex_.ReaderUnlock();
            }
        }

        [[nodiscard]] const T* operator->() const noexcept { return &*parent_->value_; }
        [[nodiscard]] const T& operator*() const noexcept { return *parent_->value_; }
        [[nodiscard]] const T& get() const noexcept { return *parent_->value_; }

    private:
        friend class Synchronized;

        explicit ReadAccess(const Synchronized* parent) noexcept
            : parent_(parent) {
            parent_->mutex_.ReaderLock();
        }

        const Synchronized* parent_ = nullptr;
    };

    class WriteAccess {
    public:
        WriteAccess(const WriteAccess&) = delete;
        WriteAccess& operator=(const WriteAccess&) = delete;

        WriteAccess(WriteAccess&& other) noexcept
            : parent_(std::exchange(other.parent_, nullptr)) {}

        ~WriteAccess() {
            if (parent_ != nullptr) {
                parent_->mutex_.Unlock();
            }
        }

        [[nodiscard]] T* operator->() noexcept { return &*parent_->value_; }
        [[nodiscard]] T& operator*() noexcept { return *parent_->value_; }
        [[nodiscard]] T& get() noexcept { return *parent_->value_; }
        [[nodiscard]] const T* operator->() const noexcept { return &*parent_->value_; }
        [[nodiscard]] const T& operator*() const noexcept { return *parent_->value_; }
        [[nodiscard]] const T& get() const noexcept { return *parent_->value_; }

    private:
        friend class Synchronized;

        explicit WriteAccess(Synchronized* parent) noexcept
            : parent_(parent) {
            parent_->mutex_.Lock();
        }

        Synchronized* parent_ = nullptr;
    };

    [[nodiscard]] ReadAccess rlock() const noexcept {
        return ReadAccess(this);
    }

    [[nodiscard]] WriteAccess wlock() noexcept {
        return WriteAccess(this);
    }

    template <typename F>
    decltype(auto) WithRLock(F&& f) const {
        auto access = rlock();
        return std::invoke(std::forward<F>(f), access.get());
    }

    template <typename F>
    decltype(auto) WithWLock(F&& f) {
        auto access = wlock();
        return std::invoke(std::forward<F>(f), access.get());
    }

private:
    mutable MutexType mutex_;
    std::optional<T> value_;
};

} // namespace zeta

#endif // ZETA_SYNCHRONIZATION_SYNCHRONIZED_H
