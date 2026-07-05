#ifndef ZETA_TYPES_OPTIONAL_H
#define ZETA_TYPES_OPTIONAL_H

/// @file   types/optional.h
/// @brief  Lightweight optional value container with monadic helpers.
///
/// `zeta::Optional<T>` is an Abseil-style convenience wrapper over
/// `std::optional<T>` with a small set of composition helpers:
///   - `Map`    transform a present value
///   - `AndThen` chain another Optional-returning operation
///   - `OrElse` recover with a fallback value
///
/// Example:
///   zeta::Optional<int> ParseInt(std::string_view s);
///   auto out = ParseInt("42")
///       .Map([](int v) { return v + 1; })
///       .OrElse([] { return 0; });

#include <cassert>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

#include "zeta/base/in_place.h"

namespace zeta {

template <typename T>
class Optional {
    static_assert(!std::is_void_v<T>, "Optional<void> is not supported");

public:
    using value_type = T;

    Optional() noexcept = default;
    Optional(std::nullopt_t) noexcept : value_(std::nullopt) {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    Optional(const T& value) : value_(value) {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    Optional(T&& value) : value_(std::move(value)) {}

    template <typename... Args>
    explicit Optional(in_place_t, Args&&... args)
        : value_(std::in_place, std::forward<Args>(args)...) {}

    Optional(const Optional&) = default;
    Optional(Optional&&) noexcept = default;
    Optional& operator=(const Optional&) = default;
    Optional& operator=(Optional&&) noexcept = default;

    [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] T& value() & {
        assert(has_value());
        return *value_;
    }

    [[nodiscard]] const T& value() const& {
        assert(has_value());
        return *value_;
    }

    [[nodiscard]] T&& value() && {
        assert(has_value());
        return std::move(*value_);
    }

    [[nodiscard]] const T&& value() const&& {
        assert(has_value());
        return std::move(*value_);
    }

    [[nodiscard]] T& operator*() & { return value(); }
    [[nodiscard]] const T& operator*() const& { return value(); }
    [[nodiscard]] T&& operator*() && { return std::move(*this).value(); }
    [[nodiscard]] const T&& operator*() const&& { return std::move(*this).value(); }

    [[nodiscard]] T* operator->() {
        assert(has_value());
        return &*value_;
    }

    [[nodiscard]] const T* operator->() const {
        assert(has_value());
        return &*value_;
    }

    void reset() noexcept { value_.reset(); }

    template <typename... Args>
    T& emplace(Args&&... args) {
        return value_.emplace(std::forward<Args>(args)...);
    }

    template <typename U>
        requires std::is_copy_constructible_v<T>
    [[nodiscard]] T value_or(U&& default_value) const& {
        return has_value() ? *value_
                           : static_cast<T>(std::forward<U>(default_value));
    }

    [[nodiscard]] T value_or(T default_value) && {
        return has_value() ? std::move(*value_) : std::move(default_value);
    }

    template <typename F>
        requires std::invocable<F&, T&>
    [[nodiscard]] auto Map(F&& f) & {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&>>;
        static_assert(!std::is_void_v<Return>, "Optional::Map requires a value return");
        if (!has_value()) return Optional<Return>(std::nullopt);
        return Optional<Return>(std::invoke(std::forward<F>(f), *value_));
    }

    template <typename F>
        requires std::invocable<F&, const T&>
    [[nodiscard]] auto Map(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        static_assert(!std::is_void_v<Return>, "Optional::Map requires a value return");
        if (!has_value()) return Optional<Return>(std::nullopt);
        return Optional<Return>(std::invoke(std::forward<F>(f), *value_));
    }

    template <typename F>
        requires std::invocable<F&, T&&>
    [[nodiscard]] auto Map(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&&>>;
        static_assert(!std::is_void_v<Return>, "Optional::Map requires a value return");
        if (!has_value()) return Optional<Return>(std::nullopt);
        return Optional<Return>(
            std::invoke(std::forward<F>(f), std::move(*value_)));
    }

    template <typename F>
        requires std::invocable<F&, const T&&>
    [[nodiscard]] auto Map(F&& f) const&& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>;
        static_assert(!std::is_void_v<Return>, "Optional::Map requires a value return");
        if (!has_value()) return Optional<Return>(std::nullopt);
        return Optional<Return>(
            std::invoke(std::forward<F>(f), std::move(*value_)));
    }

    template <typename F>
        requires std::invocable<F&, T&> &&
                 (!std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, T&>>>) &&
                 std::is_constructible_v<std::remove_cvref_t<std::invoke_result_t<F&, T&>>, std::nullopt_t>
    [[nodiscard]] auto AndThen(F&& f) & {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&>>;
        if (!has_value()) return Return(std::nullopt);
        return std::invoke(std::forward<F>(f), *value_);
    }

    template <typename F>
        requires std::invocable<F&, T&> &&
                 std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, T&>>>
    auto AndThen(F&&) & = delete;

    template <typename F>
        requires std::invocable<F&, const T&> &&
                 (!std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&>>>) &&
                 std::is_constructible_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&>>, std::nullopt_t>
    [[nodiscard]] auto AndThen(F&& f) const& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&>>;
        if (!has_value()) return Return(std::nullopt);
        return std::invoke(std::forward<F>(f), *value_);
    }

    template <typename F>
        requires std::invocable<F&, const T&> &&
                 std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&>>>
    auto AndThen(F&&) const& = delete;

    template <typename F>
        requires std::invocable<F&, T&&> &&
                 (!std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, T&&>>>) &&
                 std::is_constructible_v<std::remove_cvref_t<std::invoke_result_t<F&, T&&>>, std::nullopt_t>
    [[nodiscard]] auto AndThen(F&& f) && {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, T&&>>;
        if (!has_value()) return Return(std::nullopt);
        return std::invoke(std::forward<F>(f), std::move(*value_));
    }

    template <typename F>
        requires std::invocable<F&, T&&> &&
                 std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, T&&>>>
    auto AndThen(F&&) && = delete;

    template <typename F>
        requires std::invocable<F&, const T&&> &&
                 (!std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>>) &&
                 std::is_constructible_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>, std::nullopt_t>
    [[nodiscard]] auto AndThen(F&& f) const&& {
        using Return = std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>;
        if (!has_value()) return Return(std::nullopt);
        return std::invoke(std::forward<F>(f), std::move(*value_));
    }

    template <typename F>
        requires std::invocable<F&, const T&&> &&
                 std::is_void_v<std::remove_cvref_t<std::invoke_result_t<F&, const T&&>>>
    auto AndThen(F&&) const&& = delete;

    template <typename F>
        requires std::invocable<F&>
    [[nodiscard]] Optional OrElse(F&& f) const& {
        if (has_value()) return *this;
        return Optional(std::invoke(std::forward<F>(f)));
    }

    template <typename F>
        requires std::invocable<F&>
    [[nodiscard]] Optional OrElse(F&& f) && {
        if (has_value()) return std::move(*this);
        return Optional(std::invoke(std::forward<F>(f)));
    }

private:
    std::optional<T> value_;
};

template <typename T>
Optional(T) -> Optional<T>;

} // namespace zeta

#endif // ZETA_TYPES_OPTIONAL_H
