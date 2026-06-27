#ifndef ZETA_TYPES_VARIANT_H
#define ZETA_TYPES_VARIANT_H

/// @file   types/variant.h
/// @brief  Lightweight tagged union with visit helpers.
///
/// `zeta::Variant<Ts...>` wraps `std::variant<Ts...>` and exposes the
/// standard operations with a small amount of convenience around visiting
/// and in-place construction.
///
/// Example:
///   zeta::Variant<int, std::string> value("hello");
///   auto text = value.Visit([](const auto& v) -> std::string {
///     if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>) {
///       return std::to_string(v);
///     } else {
///       return v;
///     }
///   });

#include <cstddef>
#include <concepts>
#include <type_traits>
#include <utility>
#include <variant>

namespace zeta {

template <typename... Ts>
class Variant {
public:
    using value_type = std::variant<Ts...>;

    Variant() = default;
    Variant(const Variant&) = default;
    Variant(Variant&&) noexcept = default;
    Variant& operator=(const Variant&) = default;
    Variant& operator=(Variant&&) noexcept = default;

    template <typename T>
        requires (!std::is_same_v<std::remove_cvref_t<T>, Variant>) &&
                 std::constructible_from<value_type, T&&>
    // NOLINTNEXTLINE(google-explicit-constructor)
    Variant(T&& value) : value_(std::forward<T>(value)) {}

    template <typename T, typename... Args>
    explicit Variant(std::in_place_type_t<T>, Args&&... args)
        : value_(std::in_place_type<T>, std::forward<Args>(args)...) {}

    template <std::size_t I, typename... Args>
    explicit Variant(std::in_place_index_t<I>, Args&&... args)
        : value_(std::in_place_index<I>, std::forward<Args>(args)...) {}

    [[nodiscard]] bool valueless_by_exception() const noexcept {
        return value_.valueless_by_exception();
    }

    [[nodiscard]] std::size_t index() const noexcept { return value_.index(); }

    template <typename T>
    [[nodiscard]] bool holds_alternative() const noexcept {
        return std::holds_alternative<T>(value_);
    }

    template <std::size_t I>
    [[nodiscard]] decltype(auto) get() & {
        return std::get<I>(value_);
    }

    template <std::size_t I>
    [[nodiscard]] decltype(auto) get() const& {
        return std::get<I>(value_);
    }

    template <std::size_t I>
    [[nodiscard]] decltype(auto) get() && {
        return std::get<I>(std::move(value_));
    }

    template <std::size_t I>
    [[nodiscard]] decltype(auto) get() const&& {
        return std::get<I>(std::move(value_));
    }

    template <typename T>
    [[nodiscard]] decltype(auto) get() & {
        return std::get<T>(value_);
    }

    template <typename T>
    [[nodiscard]] decltype(auto) get() const& {
        return std::get<T>(value_);
    }

    template <typename T>
    [[nodiscard]] decltype(auto) get() && {
        return std::get<T>(std::move(value_));
    }

    template <typename T>
    [[nodiscard]] decltype(auto) get() const&& {
        return std::get<T>(std::move(value_));
    }

    template <typename T>
    [[nodiscard]] T* get_if() noexcept {
        return std::get_if<T>(&value_);
    }

    template <typename T>
    [[nodiscard]] const T* get_if() const noexcept {
        return std::get_if<T>(&value_);
    }

    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        return value_.template emplace<T>(std::forward<Args>(args)...);
    }

    template <std::size_t I, typename... Args>
    decltype(auto) emplace(Args&&... args) {
        return value_.template emplace<I>(std::forward<Args>(args)...);
    }

    template <typename Visitor>
    [[nodiscard]] decltype(auto) Visit(Visitor&& visitor) & {
        return std::visit(std::forward<Visitor>(visitor), value_);
    }

    template <typename Visitor>
    [[nodiscard]] decltype(auto) Visit(Visitor&& visitor) const& {
        return std::visit(std::forward<Visitor>(visitor), value_);
    }

    template <typename Visitor>
    [[nodiscard]] decltype(auto) Visit(Visitor&& visitor) && {
        return std::visit(std::forward<Visitor>(visitor), std::move(value_));
    }

    template <typename Visitor>
    [[nodiscard]] decltype(auto) Visit(Visitor&& visitor) const&& {
        return std::visit(std::forward<Visitor>(visitor), std::move(value_));
    }

    void swap(Variant& other) noexcept(noexcept(value_.swap(other.value_))) {
        value_.swap(other.value_);
    }

private:
    value_type value_;
};

} // namespace zeta

#endif // ZETA_TYPES_VARIANT_H
