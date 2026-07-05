#ifndef ZETA_MEMORY_FUNCTION_REF_H
#define ZETA_MEMORY_FUNCTION_REF_H

/// @file   memory/function_ref.h
/// @brief  A lightweight, non-owning, type-erased callable reference.
///
/// `zeta::function_ref<Ret(Args...)>` is to `std::function` what
/// `std::string_view` is to `std::string`:
///   - zero heap allocations
///   - two-pointer footprint (object ptr + thunk ptr)
///   - must not outlive the callable it references
///
/// Inspired by `absl::FunctionRef` and `llvm::function_ref`.

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename Signature>
class function_ref; // intentionally undefined

// ── Primary: Ret(Args...) ─────────────────────────────────────────
template <typename Ret, typename... Args>
class function_ref<Ret(Args...)> {
public:
    using result_type = Ret;

    function_ref() = delete;

    /// Construct from any callable compatible with `Ret(Args...)`.
    /// The callable must outlive this function_ref.
    ///
    /// @warning This constructor only accepts lvalues.  Binding to a
    /// temporary (rvalue) is forbidden at compile time to prevent
    /// dangling references.  Store the callable in a variable first.
    template <
        typename F,
        typename = std::enable_if_t<
            std::is_lvalue_reference_v<F> &&  // reject rvalues (temporaries)
            std::is_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...>>>
    function_ref(F&& f) noexcept {
        callable_ = reinterpret_cast<const void*>(std::addressof(f));
        thunk_ = &invoke<std::remove_reference_t<F>>;
    }

    function_ref(const function_ref&) noexcept = default;
    function_ref& operator=(const function_ref&) noexcept = default;

    Ret operator()(Args... args) const {
        return thunk_(callable_, std::forward<Args>(args)...);
    }

private:
    const void* callable_;
    Ret (*thunk_)(const void*, Args...);

    template <typename F>
    static Ret invoke(const void* obj, Args... args) {
        using Fn = std::remove_reference_t<F>;
        if constexpr (std::is_function_v<Fn>) {
            return (*reinterpret_cast<Fn*>(const_cast<void*>(obj)))(
                std::forward<Args>(args)...);
        } else if constexpr (std::is_const_v<Fn>) {
            return (*reinterpret_cast<const std::remove_const_t<Fn>*>(obj))(
                std::forward<Args>(args)...);
        } else {
            return (*const_cast<Fn*>(reinterpret_cast<const Fn*>(obj)))(
                std::forward<Args>(args)...);
        }
    }
};

// ── noexcept variant: Ret(Args...) noexcept ───────────────────────
template <typename Ret, typename... Args>
class function_ref<Ret(Args...) noexcept> {
public:
    using result_type = Ret;

    function_ref() = delete;

    template <
        typename F,
        typename = std::enable_if_t<
            std::is_lvalue_reference_v<F> &&  // reject rvalues (temporaries)
            std::is_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...> &&
            std::is_nothrow_invocable_r_v<Ret,
                                          std::remove_reference_t<F>&,
                                          Args...>>>
    function_ref(F&& f) noexcept {
        callable_ = reinterpret_cast<const void*>(std::addressof(f));
        thunk_ = &invoke<std::remove_reference_t<F>>;
    }

    function_ref(const function_ref&) noexcept = default;
    function_ref& operator=(const function_ref&) noexcept = default;

    Ret operator()(Args... args) const noexcept {
        return thunk_(callable_, std::forward<Args>(args)...);
    }

private:
    const void* callable_;
    Ret (*thunk_)(const void*, Args...) noexcept;

    template <typename F>
    static Ret invoke(const void* obj, Args... args) noexcept {
        using Fn = std::remove_reference_t<F>;
        if constexpr (std::is_function_v<Fn>) {
            return (*reinterpret_cast<Fn*>(const_cast<void*>(obj)))(
                std::forward<Args>(args)...);
        } else if constexpr (std::is_const_v<Fn>) {
            return (*reinterpret_cast<const std::remove_const_t<Fn>*>(obj))(
                std::forward<Args>(args)...);
        } else {
            return (*const_cast<Fn*>(reinterpret_cast<const Fn*>(obj)))(
                std::forward<Args>(args)...);
        }
    }
};

} // namespace zeta

#endif // ZETA_MEMORY_FUNCTION_REF_H
