#ifndef ZETA_FUNCTIONAL_PIPE_H
#define ZETA_FUNCTIONAL_PIPE_H

/// @file   functional/pipe.h
/// @brief  Apply a sequence of callables from left to right.

#include <functional>
#include <type_traits>
#include <utility>

namespace zeta {

/// Applies each callable to the previous result from left to right.
template <typename Value>
constexpr auto pipe(Value&& value) -> std::decay_t<Value> {
    return std::forward<Value>(value);
}

template <typename Value, typename Function, typename... Rest>
constexpr decltype(auto) pipe(Value&& value, Function&& function,
                               Rest&&... rest) {
    return pipe(std::invoke(std::forward<Function>(function),
                            std::forward<Value>(value)),
                std::forward<Rest>(rest)...);
}

} // namespace zeta

#endif // ZETA_FUNCTIONAL_PIPE_H
