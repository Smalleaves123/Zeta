#ifndef ZETA_META_TYPE_TRAITS_H
#define ZETA_META_TYPE_TRAITS_H

/// @file   meta/type_traits.h
/// @brief  Supplementary type traits beyond `<type_traits>`.
///
/// Provides common trait utilities missing from the standard library:
///   - `is_specialization_of` — detect template instantiations
///   - `remove_cvref` / `remove_cvref_t` — convenience for stripping cv+ref
///   - `is_one_of` — check if a type is among a set of candidates
///   - `copy_const` / `copy_cv` — propagate const/volatile qualifiers
///
/// Example:
///   static_assert(zeta::is_specialization_of<std::vector<int>, std::vector>);
///   static_assert(!zeta::is_specialization_of<int, std::vector>);
///   using U = zeta::remove_cvref_t<const int&>;  // int

#include <cstddef>
#include <type_traits>

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// remove_cvref (convenience — C++20 std::remove_cvref already exists)
// ═══════════════════════════════════════════════════════════════════════

/// Strip top-level cv-qualifiers and reference.
template <typename T>
using remove_cvref = std::remove_cvref<T>;

/// Shorthand for `typename remove_cvref<T>::type`.
template <typename T>
using remove_cvref_t = std::remove_cvref_t<T>;

// ═══════════════════════════════════════════════════════════════════════
// is_specialization_of
// ═══════════════════════════════════════════════════════════════════════

namespace meta_internal {

template <typename T, template <typename...> class Template>
struct is_specialization_of_impl : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of_impl<Template<Args...>, Template> : std::true_type {};

} // namespace meta_internal

/// True iff `T` is an instantiation of the template `Template`.
///
/// Example:
///   static_assert(is_specialization_of<std::vector<int>, std::vector>::value);
///   static_assert(!is_specialization_of<int, std::vector>::value);
template <typename T, template <typename...> class Template>
using is_specialization_of = meta_internal::is_specialization_of_impl<T, Template>;

/// Variable template shorthand for `is_specialization_of`.
template <typename T, template <typename...> class Template>
inline constexpr bool is_specialization_of_v =
    is_specialization_of<T, Template>::value;

// ═══════════════════════════════════════════════════════════════════════
// is_one_of
// ═══════════════════════════════════════════════════════════════════════

/// True iff `T` matches at least one of `Ts...`.
template <typename T, typename... Ts>
struct is_one_of : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

/// Variable template shorthand for `is_one_of`.
template <typename T, typename... Ts>
inline constexpr bool is_one_of_v = is_one_of<T, Ts...>::value;

// ═══════════════════════════════════════════════════════════════════════
// copy_const / copy_cv
// ═══════════════════════════════════════════════════════════════════════

/// Copy `const` from `From` onto `To`.
template <typename From, typename To>
struct copy_const {
    using type = std::conditional_t<std::is_const_v<From>,
                                     std::add_const_t<To>,
                                     std::remove_const_t<To>>;
};
template <typename From, typename To>
using copy_const_t = typename copy_const<From, To>::type;

/// Copy `const` and `volatile` from `From` onto `To`.
template <typename From, typename To>
struct copy_cv {
private:
    using step1 = std::conditional_t<std::is_volatile_v<From>,
                                      std::add_volatile_t<To>,
                                      std::remove_volatile_t<To>>;
public:
    using type = copy_const_t<From, step1>;
};
template <typename From, typename To>
using copy_cv_t = typename copy_cv<From, To>::type;

// ═══════════════════════════════════════════════════════════════════════
// type_identity (C++20 has std::type_identity, provided for convenience)
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
using type_identity = std::type_identity<T>;

template <typename T>
using type_identity_t = std::type_identity_t<T>;

// ═══════════════════════════════════════════════════════════════════════
// conditional_t convenience (already in C++14, provided for completeness)
// ═══════════════════════════════════════════════════════════════════════

template <bool B, typename T, typename F>
using conditional_t = std::conditional_t<B, T, F>;

// ═══════════════════════════════════════════════════════════════════════
// is_detected (detection idiom)
// ═══════════════════════════════════════════════════════════════════════

namespace meta_internal {

template <typename Default, typename AlwaysVoid,
          template <typename...> class Op, typename... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <typename Default, template <typename...> class Op, typename... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

} // namespace meta_internal

/// The detection idiom: detects whether `Op<Args...>` is valid.
///
/// Specialise `detected_t<Op, Args...>` to get the type (or a default).
/// Use `is_detected_v<Op, Args...>` for a boolean check.
template <template <typename...> class Op, typename... Args>
using is_detected =
    typename meta_internal::detector<void, void, Op, Args...>::value_t;

template <template <typename...> class Op, typename... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

template <template <typename...> class Op, typename... Args>
using detected_t =
    typename meta_internal::detector<void, void, Op, Args...>::type;

template <typename Default, template <typename...> class Op, typename... Args>
using detected_or = meta_internal::detector<Default, void, Op, Args...>;

template <typename Default, template <typename...> class Op, typename... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

} // namespace zeta

#endif // ZETA_META_TYPE_TRAITS_H
