#ifndef ZETA_FUNCTIONAL_OVERLOAD_H
#define ZETA_FUNCTIONAL_OVERLOAD_H

/// @file   functional/overload.h
/// @brief  Combine several callables into one overloaded function object.

namespace zeta {

/// A convenient visitor for `std::visit` and other overload-based dispatch.
template <typename... Functions>
struct Overload : Functions... {
    using Functions::operator()...;
};

template <typename... Functions>
Overload(Functions...) -> Overload<Functions...>;

} // namespace zeta

#endif // ZETA_FUNCTIONAL_OVERLOAD_H
