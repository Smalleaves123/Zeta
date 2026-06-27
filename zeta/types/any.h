#ifndef ZETA_TYPES_ANY_H
#define ZETA_TYPES_ANY_H

/// @file   types/any.h
/// @brief  Type-erased container for a single dynamically typed value.
///
/// `zeta::Any` is a small Abseil-style wrapper over `std::any` that keeps
/// the common operations close to the object:
///   - default / in-place construction
///   - `has_value()` / `reset()`
///   - `get_if<T>()` and `AnyCast<T>()` pointer accessors
///
/// Example:
///   zeta::Any value = std::string("hello");
///   if (auto* s = zeta::AnyCast<std::string>(&value)) {
///     std::puts(s->c_str());
///   }

#include <any>
#include <cassert>
#include <concepts>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace zeta {

class Any {
public:
    using value_type = std::any;

    Any() noexcept = default;
    Any(const Any&) = default;
    Any(Any&&) noexcept = default;
    Any& operator=(const Any&) = default;
    Any& operator=(Any&&) noexcept = default;

    template <typename T>
        requires (!std::is_same_v<std::remove_cvref_t<T>, Any>) &&
                 std::constructible_from<value_type, T&&>
    // NOLINTNEXTLINE(google-explicit-constructor)
    Any(T&& value) : value_(std::forward<T>(value)) {}

    template <typename T, typename... Args>
    explicit Any(std::in_place_type_t<T>, Args&&... args)
        : value_(std::in_place_type<T>, std::forward<Args>(args)...) {}

    [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] const std::type_info& type() const noexcept {
        return value_.type();
    }

    void reset() noexcept { value_.reset(); }

    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        return value_.emplace<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    [[nodiscard]] T* get_if() noexcept {
        return std::any_cast<T>(&value_);
    }

    template <typename T>
    [[nodiscard]] const T* get_if() const noexcept {
        return std::any_cast<T>(&value_);
    }

    template <typename T>
    [[nodiscard]] T& get() & {
        auto* ptr = get_if<T>();
        assert(ptr != nullptr);
        return *ptr;
    }

    template <typename T>
    [[nodiscard]] const T& get() const& {
        auto* ptr = get_if<T>();
        assert(ptr != nullptr);
        return *ptr;
    }

    template <typename T>
    [[nodiscard]] T&& get() && {
        auto* ptr = get_if<T>();
        assert(ptr != nullptr);
        return std::move(*ptr);
    }

    template <typename T>
    [[nodiscard]] const T&& get() const&& {
        auto* ptr = get_if<T>();
        assert(ptr != nullptr);
        return std::move(*ptr);
    }

    void swap(Any& other) noexcept { value_.swap(other.value_); }

private:
    std::any value_;

    template <typename T>
    friend T* AnyCast(Any* any) noexcept;

    template <typename T>
    friend const T* AnyCast(const Any* any) noexcept;
};

template <typename T>
[[nodiscard]] T* AnyCast(Any* any) noexcept {
    return any == nullptr ? nullptr : std::any_cast<T>(&any->value_);
}

template <typename T>
[[nodiscard]] const T* AnyCast(const Any* any) noexcept {
    return any == nullptr ? nullptr : std::any_cast<T>(&any->value_);
}

template <typename T>
[[nodiscard]] T* AnyCast(Any& any) noexcept {
    return AnyCast<T>(&any);
}

template <typename T>
[[nodiscard]] const T* AnyCast(const Any& any) noexcept {
    return AnyCast<T>(&any);
}

} // namespace zeta

#endif // ZETA_TYPES_ANY_H
