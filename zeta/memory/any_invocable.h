#ifndef ZETA_MEMORY_ANY_INVOCABLE_H
#define ZETA_MEMORY_ANY_INVOCABLE_H

/// @file   memory/any_invocable.h
/// @brief  Move-only owning callable wrapper with small-buffer optimization.

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace zeta {

template <typename Signature>
class AnyInvocable;

namespace detail {

constexpr std::size_t kAnyInvocableStorageSize = sizeof(void*) * 3;
constexpr std::size_t kAnyInvocableStorageAlign = alignof(std::max_align_t);

template <typename Ret, typename... Args>
struct AnyInvocableOps {
    using InvokeFn = Ret (*)(const void*, Args&&...);
    using DestroyFn = void (*)(void*) noexcept;
    using MoveFn = void (*)(void*, void*) noexcept;
    InvokeFn invoke;
    DestroyFn destroy;
    MoveFn move;
};

template <typename Ret, typename... Args>
struct AnyInvocableNoexceptOps {
    using InvokeFn = Ret (*)(const void*, Args&&...) noexcept;
    using DestroyFn = void (*)(void*) noexcept;
    using MoveFn = void (*)(void*, void*) noexcept;
    InvokeFn invoke;
    DestroyFn destroy;
    MoveFn move;
};

template <typename T>
inline constexpr bool FitsAnyInvocableStorage =
    sizeof(T) <= kAnyInvocableStorageSize &&
    alignof(T) <= kAnyInvocableStorageAlign &&
    std::is_nothrow_move_constructible_v<T>;

template <typename T, typename Ret, typename... Args>
Ret InvokeInline(const void* storage, Args&&... args) {
    return std::invoke(*const_cast<T*>(reinterpret_cast<const T*>(storage)),
                       std::forward<Args>(args)...);
}

template <typename T, typename Ret, typename... Args>
Ret InvokeHeap(const void* storage, Args&&... args) {
    return std::invoke(**reinterpret_cast<T* const*>(const_cast<void*>(storage)),
                       std::forward<Args>(args)...);
}

template <typename T, typename Ret, typename... Args>
Ret InvokeInlineNoexcept(const void* storage, Args&&... args) noexcept {
    return std::invoke(*const_cast<T*>(reinterpret_cast<const T*>(storage)),
                       std::forward<Args>(args)...);
}

template <typename T, typename Ret, typename... Args>
Ret InvokeHeapNoexcept(const void* storage, Args&&... args) noexcept {
    return std::invoke(**reinterpret_cast<T* const*>(const_cast<void*>(storage)),
                       std::forward<Args>(args)...);
}

template <typename T>
void DestroyInline(void* storage) noexcept {
    reinterpret_cast<T*>(storage)->~T();
}

template <typename T>
void DestroyHeap(void* storage) noexcept {
    delete *reinterpret_cast<T**>(storage);
}

template <typename T>
void MoveInline(void* from, void* to) noexcept {
    ::new (to) T(std::move(*reinterpret_cast<T*>(from)));
    reinterpret_cast<T*>(from)->~T();
}

template <typename T>
void MoveHeap(void* from, void* to) noexcept {
    *reinterpret_cast<T**>(to) = *reinterpret_cast<T**>(from);
    *reinterpret_cast<T**>(from) = nullptr;
}

template <typename T, typename Ret, typename... Args>
struct SelectAnyInvocableOps {
    using Ops = AnyInvocableOps<Ret, Args...>;

    static constexpr Ops value = [] {
        if constexpr (FitsAnyInvocableStorage<T>) {
            return Ops{&InvokeInline<T, Ret, Args...>,
                       &DestroyInline<T>,
                       &MoveInline<T>};
        } else {
            return Ops{&InvokeHeap<T, Ret, Args...>,
                       &DestroyHeap<T>,
                       &MoveHeap<T>};
        }
    }();
};

template <typename T, typename Ret, typename... Args>
struct SelectAnyInvocableNoexceptOps {
    using Ops = AnyInvocableNoexceptOps<Ret, Args...>;

    static constexpr Ops value = [] {
        if constexpr (FitsAnyInvocableStorage<T>) {
            return Ops{&InvokeInlineNoexcept<T, Ret, Args...>,
                       &DestroyInline<T>,
                       &MoveInline<T>};
        } else {
            return Ops{&InvokeHeapNoexcept<T, Ret, Args...>,
                       &DestroyHeap<T>,
                       &MoveHeap<T>};
        }
    }();
};

} // namespace detail

template <typename Ret, typename... Args>
class AnyInvocable<Ret(Args...)> {
public:
    using result_type = Ret;

    AnyInvocable() noexcept = default;
    AnyInvocable(std::nullptr_t) noexcept {}

    template <typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, AnyInvocable>) &&
                 std::is_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...>
    AnyInvocable(F&& f) {
        Emplace<std::decay_t<F>>(std::forward<F>(f));
    }

    template <typename F, typename... CtorArgs>
        requires std::is_constructible_v<F, CtorArgs...> &&
                 std::is_invocable_r_v<Ret, F&, Args...>
    explicit AnyInvocable(std::in_place_type_t<F>, CtorArgs&&... args) {
        Emplace<F>(std::forward<CtorArgs>(args)...);
    }

    AnyInvocable(const AnyInvocable&) = delete;
    AnyInvocable& operator=(const AnyInvocable&) = delete;

    AnyInvocable(AnyInvocable&& other) noexcept {
        MoveFrom(std::move(other));
    }

    AnyInvocable& operator=(AnyInvocable&& other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    AnyInvocable& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    template <typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, AnyInvocable>) &&
                 std::is_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...>
    AnyInvocable& operator=(F&& f) {
        Reset();
        Emplace<std::decay_t<F>>(std::forward<F>(f));
        return *this;
    }

    ~AnyInvocable() noexcept {
        Reset();
    }

    explicit operator bool() const noexcept { return invoke_ != nullptr; }

    Ret operator()(Args... args) const {
        assert(invoke_ != nullptr);
        return invoke_(StoragePtr(), std::forward<Args>(args)...);
    }

    void Reset() noexcept {
        if (destroy_ != nullptr) {
            destroy_(StoragePtr());
        }
        invoke_ = nullptr;
        destroy_ = nullptr;
        move_ = nullptr;
    }

private:
    using Ops = detail::AnyInvocableOps<Ret, Args...>;
    using Storage = std::aligned_storage_t<
        detail::kAnyInvocableStorageSize,
        detail::kAnyInvocableStorageAlign>;

    template <typename F, typename... CtorArgs>
    void Emplace(CtorArgs&&... args) {
        constexpr Ops ops = detail::SelectAnyInvocableOps<F, Ret, Args...>::value;
        if constexpr (detail::FitsAnyInvocableStorage<F>) {
            ::new (StoragePtr()) F(std::forward<CtorArgs>(args)...);
        } else {
            *reinterpret_cast<F**>(StoragePtr()) = new F(std::forward<CtorArgs>(args)...);
        }
        invoke_ = ops.invoke;
        destroy_ = ops.destroy;
        move_ = ops.move;
    }

    void MoveFrom(AnyInvocable&& other) noexcept {
        if (!other) {
            return;
        }
        other.move_(other.StoragePtr(), StoragePtr());
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        move_ = other.move_;
        other.invoke_ = nullptr;
        other.destroy_ = nullptr;
        other.move_ = nullptr;
    }

    void* StoragePtr() noexcept { return &storage_; }
    const void* StoragePtr() const noexcept { return &storage_; }

    Storage storage_{};
    typename Ops::InvokeFn invoke_ = nullptr;
    typename Ops::DestroyFn destroy_ = nullptr;
    typename Ops::MoveFn move_ = nullptr;
};

template <typename Ret, typename... Args>
class AnyInvocable<Ret(Args...) noexcept> {
public:
    using result_type = Ret;

    AnyInvocable() noexcept = default;
    AnyInvocable(std::nullptr_t) noexcept {}

    template <typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, AnyInvocable>) &&
                 std::is_nothrow_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...>
    AnyInvocable(F&& f) {
        Emplace<std::decay_t<F>>(std::forward<F>(f));
    }

    template <typename F, typename... CtorArgs>
        requires std::is_constructible_v<F, CtorArgs...> &&
                 std::is_nothrow_invocable_r_v<Ret, F&, Args...>
    explicit AnyInvocable(std::in_place_type_t<F>, CtorArgs&&... args) {
        Emplace<F>(std::forward<CtorArgs>(args)...);
    }

    AnyInvocable(const AnyInvocable&) = delete;
    AnyInvocable& operator=(const AnyInvocable&) = delete;

    AnyInvocable(AnyInvocable&& other) noexcept {
        MoveFrom(std::move(other));
    }

    AnyInvocable& operator=(AnyInvocable&& other) noexcept {
        if (this != &other) {
            Reset();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    AnyInvocable& operator=(std::nullptr_t) noexcept {
        Reset();
        return *this;
    }

    template <typename F>
        requires (!std::is_same_v<std::remove_cvref_t<F>, AnyInvocable>) &&
                 std::is_nothrow_invocable_r_v<Ret, std::remove_reference_t<F>&, Args...>
    AnyInvocable& operator=(F&& f) {
        Reset();
        Emplace<std::decay_t<F>>(std::forward<F>(f));
        return *this;
    }

    ~AnyInvocable() noexcept {
        Reset();
    }

    explicit operator bool() const noexcept { return invoke_ != nullptr; }

    Ret operator()(Args... args) const noexcept {
        assert(invoke_ != nullptr);
        return invoke_(StoragePtr(), std::forward<Args>(args)...);
    }

    void Reset() noexcept {
        if (destroy_ != nullptr) {
            destroy_(StoragePtr());
        }
        invoke_ = nullptr;
        destroy_ = nullptr;
        move_ = nullptr;
    }

private:
    using Ops = detail::AnyInvocableNoexceptOps<Ret, Args...>;
    using Storage = std::aligned_storage_t<
        detail::kAnyInvocableStorageSize,
        detail::kAnyInvocableStorageAlign>;

    template <typename F, typename... CtorArgs>
    void Emplace(CtorArgs&&... args) {
        constexpr Ops ops = detail::SelectAnyInvocableNoexceptOps<F, Ret, Args...>::value;
        if constexpr (detail::FitsAnyInvocableStorage<F>) {
            ::new (StoragePtr()) F(std::forward<CtorArgs>(args)...);
        } else {
            *reinterpret_cast<F**>(StoragePtr()) = new F(std::forward<CtorArgs>(args)...);
        }
        invoke_ = ops.invoke;
        destroy_ = ops.destroy;
        move_ = ops.move;
    }

    void MoveFrom(AnyInvocable&& other) noexcept {
        if (!other) {
            return;
        }
        other.move_(other.StoragePtr(), StoragePtr());
        invoke_ = other.invoke_;
        destroy_ = other.destroy_;
        move_ = other.move_;
        other.invoke_ = nullptr;
        other.destroy_ = nullptr;
        other.move_ = nullptr;
    }

    void* StoragePtr() noexcept { return &storage_; }
    const void* StoragePtr() const noexcept { return &storage_; }

    Storage storage_{};
    typename Ops::InvokeFn invoke_ = nullptr;
    typename Ops::DestroyFn destroy_ = nullptr;
    typename Ops::MoveFn move_ = nullptr;
};

} // namespace zeta

#endif // ZETA_MEMORY_ANY_INVOCABLE_H
