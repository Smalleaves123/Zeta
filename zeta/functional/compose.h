#ifndef ZETA_FUNCTIONAL_COMPOSE_H
#define ZETA_FUNCTIONAL_COMPOSE_H

/// @file   functional/compose.h
/// @brief  Compose callables from right to left.

#include <functional>
#include <type_traits>
#include <utility>

namespace zeta {
namespace functional_internal {

template <typename F, typename G>
class ComposeClosure {
public:
    template <typename Fn, typename Gn>
    constexpr ComposeClosure(Fn&& first, Gn&& second)
        : first_(std::forward<Fn>(first)), second_(std::forward<Gn>(second)) {}

    template <typename Self, typename... Args>
    static decltype(auto) Invoke(Self&& self, Args&&... args) {
        return std::invoke(
            std::forward<Self>(self).first_,
            std::invoke(std::forward<Self>(self).second_,
                        std::forward<Args>(args)...));
    }

    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... args) & {
        return Invoke(*this, std::forward<Args>(args)...);
    }

    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... args) const& {
        return Invoke(*this, std::forward<Args>(args)...);
    }

    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... args) && {
        return Invoke(std::move(*this), std::forward<Args>(args)...);
    }

    template <typename... Args>
    constexpr decltype(auto) operator()(Args&&... args) const&& {
        return Invoke(std::move(*this), std::forward<Args>(args)...);
    }

private:
    F first_;
    G second_;
};

} // namespace functional_internal

/// Returns a callable equivalent to `first(second(args...))`.
template <typename F, typename G>
constexpr auto compose(F&& first, G&& second) {
    using Closure = functional_internal::ComposeClosure<
        std::decay_t<F>, std::decay_t<G>>;
    return Closure(std::forward<F>(first), std::forward<G>(second));
}

/// Compose more than two callables from right to left.
template <typename F, typename G, typename... Rest>
constexpr auto compose(F&& first, G&& second, Rest&&... rest) {
    return compose(std::forward<F>(first),
                   compose(std::forward<G>(second),
                           std::forward<Rest>(rest)...));
}

} // namespace zeta

#endif // ZETA_FUNCTIONAL_COMPOSE_H
