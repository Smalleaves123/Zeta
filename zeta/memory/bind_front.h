#ifndef ZETA_MEMORY_BIND_FRONT_H
#define ZETA_MEMORY_BIND_FRONT_H

/// @file   memory/bind_front.h
/// @brief  Lightweight front-binding helper for callables.

#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace zeta {

namespace detail {

template <typename T>
using BindFrontStore = std::unwrap_ref_decay_t<T>;

template <typename Self, std::size_t I>
decltype(auto) BindFrontGet(Self&& self) {
    return std::get<I>(std::forward<Self>(self).bound_);
}

template <typename Self, typename... CallArgs, std::size_t... I>
decltype(auto) BindFrontInvoke(Self&& self,
                               std::index_sequence<I...>,
                               CallArgs&&... call_args) {
    return std::invoke(std::forward<Self>(self).func_,
                       BindFrontGet<Self, I>(std::forward<Self>(self))...,
                       std::forward<CallArgs>(call_args)...);
}

template <typename F, typename... BoundArgs>
class BindFrontClosure {
public:
    template <typename Fn, typename... Args>
    explicit BindFrontClosure(Fn&& fn, Args&&... args)
        : func_(std::forward<Fn>(fn))
        , bound_(std::forward<Args>(args)...) {}

    template <typename... CallArgs>
    decltype(auto) operator()(CallArgs&&... call_args) & {
        return detail::BindFrontInvoke(*this,
                                       std::index_sequence_for<BoundArgs...>{},
                                       std::forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    decltype(auto) operator()(CallArgs&&... call_args) const& {
        return detail::BindFrontInvoke(*this,
                                       std::index_sequence_for<BoundArgs...>{},
                                       std::forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    decltype(auto) operator()(CallArgs&&... call_args) && {
        return detail::BindFrontInvoke(std::move(*this),
                                       std::index_sequence_for<BoundArgs...>{},
                                       std::forward<CallArgs>(call_args)...);
    }

    template <typename... CallArgs>
    decltype(auto) operator()(CallArgs&&... call_args) const&& {
        return detail::BindFrontInvoke(std::move(*this),
                                       std::index_sequence_for<BoundArgs...>{},
                                       std::forward<CallArgs>(call_args)...);
    }

private:
    F func_;
    std::tuple<BoundArgs...> bound_;

    template <typename Self, std::size_t I>
    friend decltype(auto) BindFrontGet(Self&& self);

    template <typename Self, typename... CallArgs2, std::size_t... I>
    friend decltype(auto) BindFrontInvoke(Self&& self,
                                          std::index_sequence<I...>,
                                          CallArgs2&&... call_args);
};

} // namespace detail

template <typename F, typename... BoundArgs>
auto bind_front(F&& f, BoundArgs&&... bound_args) {
    using Closure = detail::BindFrontClosure<
        detail::BindFrontStore<F>,
        detail::BindFrontStore<BoundArgs>...>;
    return Closure(std::forward<F>(f), std::forward<BoundArgs>(bound_args)...);
}

} // namespace zeta

#endif // ZETA_MEMORY_BIND_FRONT_H
