#ifndef ZETA_STATUS_STATUSOR_H
#define ZETA_STATUS_STATUSOR_H

/// @file   status/statusor.h
/// @brief  Result type: either a value or an error Status.
///
/// `zeta::StatusOr<T>` is a discriminated union holding either a value of
/// type `T` or a `Status` error.  Uses manual aligned storage — zero
/// overhead over the two members.  No exceptions, no RTTI, no std::optional.
///
/// Example:
///   zeta::StatusOr<int> Parse(std::string_view s) {
///     if (s.empty()) return zeta::InvalidArgumentError("empty");
///     return 42;
///   }
///
///   auto r = Parse("42");
///   if (!r.ok()) return std::move(r).status();
///   Use(*r);

#include "zeta/status/status.h"

#include <concepts>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename T>
class StatusOr;

// ── StatusOr<void> ───────────────────────────────────────────────────

/// Specialization for void: holds only a Status.
template <>
class StatusOr<void> {
    Status status_;

public:
    using value_type = void;

    /// Constructs an OK status (success, no value).
    StatusOr() noexcept : status_(OkStatus()) {}

    /// Construction from a Status — the error path.
    explicit StatusOr(Status status) noexcept : status_(std::move(status)) {}

    // ── Copy / Move ─────────────────────────────────────────────

    StatusOr(const StatusOr&) = default;
    StatusOr& operator=(const StatusOr&) = default;
    StatusOr(StatusOr&&) noexcept = default;
    StatusOr& operator=(StatusOr&&) noexcept = default;
    ~StatusOr() = default;

    // ── Observers ────────────────────────────────────────────────

    [[nodiscard]] bool ok() const noexcept { return status_.ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }

    [[nodiscard]] bool has_value() const noexcept { return ok(); }

    // ── Functional composition ───────────────────────────────────

    template <typename F>
        requires std::invocable<F&>
    [[nodiscard]] auto Map(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(status_);
            } else {
                return StatusOr<Return>(status_);
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f));
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(std::invoke(std::forward<F>(f)));
        }
    }

    template <typename F>
        requires std::invocable<F&>
    [[nodiscard]] auto Map(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(std::move(status_));
            } else {
                return StatusOr<Return>(std::move(status_));
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f));
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(std::invoke(std::forward<F>(f)));
        }
    }

    template <typename F>
        requires std::invocable<F&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&>>;
        if (!ok()) return Return(status_);
        return std::invoke(std::forward<F>(f));
    }

    template <typename F>
        requires std::invocable<F&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&>>;
        if (!ok()) return Return(std::move(status_));
        return std::invoke(std::forward<F>(f));
    }

    template <typename F>
        requires std::invocable<F&, const Status&>
    [[nodiscard]] StatusOr<void> OrElse(F&& f) const& {
        if (ok()) return *this;
        return StatusOr<void>(std::invoke(std::forward<F>(f), status_));
    }

    template <typename F>
        requires std::invocable<F&, Status>
    [[nodiscard]] StatusOr<void> OrElse(F&& f) && {
        if (ok()) return std::move(*this);
        return StatusOr<void>(std::invoke(std::forward<F>(f), std::move(status_)));
    }

    [[nodiscard]] const Status& status() const& noexcept { return status_; }
    [[nodiscard]] Status       status() &&      noexcept { return std::move(status_); }

    void value_or() const noexcept { }

    /// @pre ok() == true
    void value() const { assert(ok()); }

    /// @pre ok() == true
    void operator*() const { assert(ok()); }
};

// ── StatusOr<T> ──────────────────────────────────────────────────────

/// Holds either a `T` (success) or a `Status` (error).
///
/// ## Storage
///
/// Uses aligned raw storage + placement new — no std::optional overhead.
/// `sizeof(StatusOr<T>)` = max(alignof(T), 8) + 16 (Status) + bool + padding.
///
/// ## Requirements on T
///
///   - Destructible (always required by the language)
///   - Move-constructible (required for move construction)
///   - Copy-constructible iff you want to copy StatusOr<T>
template <typename T>
class StatusOr {
    static_assert(!std::is_same_v<std::remove_cv_t<T>, Status>,
                  "StatusOr<Status> is not allowed");
    static_assert(!std::is_same_v<std::remove_cv_t<T>, StatusOr<T>>,
                  "StatusOr<StatusOr<T>> is not allowed");
    static_assert(!std::is_reference_v<T>,
                  "StatusOr<T&> is not allowed");

    // ── Data ─────────────────────────────────────────────────────
    // Order: status first (stable address), then value storage.
    Status                              status_;
    bool                                has_value_ = false;
    alignas(T) unsigned char            storage_[sizeof(T)];

    // ── Value access helpers ─────────────────────────────────────
    T*       value_ptr()       noexcept { return std::launder(reinterpret_cast<T*>(storage_)); }
    const T* value_ptr() const noexcept { return std::launder(reinterpret_cast<const T*>(storage_)); }

    void construct_value(T&& v) noexcept(std::is_nothrow_move_constructible_v<T>) {
        ::new (storage_) T(std::move(v));
        has_value_ = true;
    }

    void destroy_value() noexcept {
        if (has_value_) {
            value_ptr()->~T();
            has_value_ = false;
        }
    }

    void copy_from(const StatusOr& other) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        if (other.has_value_) {
            ::new (storage_) T(*other.value_ptr());
            has_value_ = true;
        }
    }

public:
    using value_type = T;

    // ── Construction ─────────────────────────────────────────────

    StatusOr() = delete;

    /// Implicit construction from a value — the success path.
    /// NOLINTNEXTLINE
    StatusOr(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        : status_(OkStatus()) {
        construct_value(std::move(value));
    }

    /// Construction from a Status — the error path.
    /// An OK status is normalized into an internal error so `StatusOr<T>`
    /// never reaches the invalid state "ok() == true but no value".
    /// NOLINTNEXTLINE(google-explicit-constructor)
    StatusOr(Status status) noexcept
        : status_(status.ok()
                      ? InternalError("StatusOr<T> requires a value")
                      : std::move(status)) {}

    // ── Copy (only valid if T is copyable) ───────────────────────

    StatusOr(const StatusOr& other)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        : status_(other.status_) {
        copy_from(other);
    }

    StatusOr& operator=(const StatusOr& other)
        noexcept(std::is_nothrow_copy_constructible_v<T> &&
                 std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_swappable_v<T>) {
        if (this != &other) {
            StatusOr tmp(other);
            swap(tmp);
        }
        return *this;
    }

    // ── Move ─────────────────────────────────────────────────────

    StatusOr(StatusOr&& other)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        : status_(std::move(other.status_)) {
        if (other.has_value_) {
            construct_value(std::move(*other.value_ptr()));
            other.destroy_value();
        }
    }

    StatusOr& operator=(StatusOr&& other)
        noexcept(std::is_nothrow_move_constructible_v<T> &&
                 std::is_nothrow_swappable_v<T>) {
        if (this != &other) {
            StatusOr tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    ~StatusOr() { destroy_value(); }

    // ── Observers ────────────────────────────────────────────────

    /// True if this holds a value (success).
    [[nodiscard]] bool ok() const noexcept { return status_.ok(); }

    /// Equivalent to ok().
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }

    void swap(StatusOr& other) noexcept(
        std::is_nothrow_swappable_v<T> &&
        std::is_nothrow_move_constructible_v<T>) {
        using std::swap;
        if (this == &other) return;

        if (has_value_ && other.has_value_) {
            swap(*value_ptr(), *other.value_ptr());
            return;
        }

        if (has_value_ && !other.has_value_) {
            Status other_status = std::move(other.status_);
            T tmp(std::move(*value_ptr()));
            other.construct_value(std::move(tmp));
            destroy_value();
            status_ = std::move(other_status);
            return;
        }

        if (!has_value_ && other.has_value_) {
            other.swap(*this);
            return;
        }

        swap(status_, other.status_);
    }

    // ── Functional composition ───────────────────────────────────

    /// Applies `f` to the held value and wraps the result in `StatusOr`.
    template <typename F>
        requires std::invocable<F&, T&>
    [[nodiscard]] auto Map(F&& f) & {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(status_);
            } else {
                return StatusOr<Return>(status_);
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f), *value_ptr());
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(std::invoke(std::forward<F>(f), *value_ptr()));
        }
    }

    template <typename F>
        requires std::invocable<F&, const T&>
    [[nodiscard]] auto Map(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(status_);
            } else {
                return StatusOr<Return>(status_);
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f), *value_ptr());
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(std::invoke(std::forward<F>(f), *value_ptr()));
        }
    }

    template <typename F>
        requires std::invocable<F&, T&&>
    [[nodiscard]] auto Map(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(std::move(status_));
            } else {
                return StatusOr<Return>(std::move(status_));
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f), std::move(*value_ptr()));
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(
                std::invoke(std::forward<F>(f), std::move(*value_ptr())));
        }
    }

    template <typename F>
        requires std::invocable<F&, const T&&>
    [[nodiscard]] auto Map(F&& f) const&& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>;
        if (!ok()) {
            if constexpr (std::is_void_v<Return>) {
                return StatusOr<void>(std::move(status_));
            } else {
                return StatusOr<Return>(std::move(status_));
            }
        }

        if constexpr (std::is_void_v<Return>) {
            std::invoke(std::forward<F>(f), std::move(*value_ptr()));
            return StatusOr<void>();
        } else {
            return StatusOr<Return>(
                std::invoke(std::forward<F>(f), std::move(*value_ptr())));
        }
    }

    /// Chains another StatusOr-producing computation.
    template <typename F>
        requires std::invocable<F&, T&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, T&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) & {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&>>;
        if (!ok()) return Return(status_);
        return std::invoke(std::forward<F>(f), *value_ptr());
    }

    template <typename F>
        requires std::invocable<F&, const T&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, const T&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        if (!ok()) return Return(status_);
        return std::invoke(std::forward<F>(f), *value_ptr());
    }

    template <typename F>
        requires std::invocable<F&, T&&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, T&&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&&>>;
        if (!ok()) return Return(std::move(status_));
        return std::invoke(std::forward<F>(f), std::move(*value_ptr()));
    }

    template <typename F>
        requires std::invocable<F&, const T&&> &&
                 std::constructible_from<std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>, Status>
    [[nodiscard]] auto AndThen(F&& f) const&& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>;
        if (!ok()) return Return(std::move(status_));
        return std::invoke(std::forward<F>(f), std::move(*value_ptr()));
    }

    /// Recovers from an error and keeps the same StatusOr type.
    template <typename F>
        requires std::invocable<F&, const Status&>
    [[nodiscard]] StatusOr OrElse(F&& f) const& {
        if (ok()) return *this;
        return StatusOr(std::invoke(std::forward<F>(f), status_));
    }

    template <typename F>
        requires std::invocable<F&, Status>
    [[nodiscard]] StatusOr OrElse(F&& f) && {
        if (ok()) return std::move(*this);
        return StatusOr(std::invoke(std::forward<F>(f), std::move(status_)));
    }

    /// Returns the held value if ok(), otherwise returns `default_value`.
    template <typename U>
        requires std::is_copy_constructible_v<T>
    [[nodiscard]] T value_or(U&& default_value) const& {
        return ok() ? *value_ptr()
                    : static_cast<T>(std::forward<U>(default_value));
    }

    /// Move-aware overload for rvalues.
    [[nodiscard]] T value_or(T default_value) && {
        return ok() ? std::move(*value_ptr()) : std::move(default_value);
    }

    /// Returns the status.  OkStatus() when ok() is true.
    [[nodiscard]] const Status& status() const& noexcept { return status_; }
    [[nodiscard]] Status       status() &&      noexcept { return std::move(status_); }

    // ── Value access ─────────────────────────────────────────────

    /// Returns a reference to the held value.
    /// @pre ok() == true
    [[nodiscard]] T& value() & {
        assert(ok());
        return *value_ptr();
    }

    /// @pre ok() == true
    [[nodiscard]] const T& value() const& {
        assert(ok());
        return *value_ptr();
    }

    /// @pre ok() == true
    /// After this call the StatusOr is in a moved-from state — do not
    /// call ok() or value() again.  The destructor still runs and
    /// properly cleans up the moved-from value.
    [[nodiscard]] T&& value() && {
        assert(ok());
        return std::move(*value_ptr());
    }

    /// @pre ok() == true
    [[nodiscard]] const T&& value() const&& {
        assert(ok());
        return std::move(*value_ptr());
    }

    // ── Pointer-like access ──────────────────────────────────────

    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T&& operator*() && { return std::move(*this).value(); }
    [[nodiscard]] const T&& operator*() const&& { return std::move(*this).value(); }

    /// @pre ok() == true
    [[nodiscard]] T* operator->() {
        assert(ok());
        return value_ptr();
    }

    /// @pre ok() == true
    [[nodiscard]] const T* operator->() const {
        assert(ok());
        return value_ptr();
    }
};

// ── Deduction guides ─────────────────────────────────────────────────

template <typename T>
StatusOr(T) -> StatusOr<T>;

} // namespace zeta

#endif // ZETA_STATUS_STATUSOR_H
