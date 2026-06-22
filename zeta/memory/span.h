#ifndef ZETA_MEMORY_SPAN_H
#define ZETA_MEMORY_SPAN_H

/// @file   memory/span.h
/// @brief  Non-owning contiguous view over a sequence of elements.
///
/// `zeta::Span<T>` is a lightweight alias to `std::span<T>` for code that
/// wants to stay under the `zeta::` namespace while using the standard
/// contiguous-view abstraction.
///
/// Example:
///   std::vector<int> v = {1, 2, 3};
///   zeta::Span<int> s(v);
///   auto tail = s.subspan(1);

#include <cstddef>
#include <span>

namespace zeta {

template <typename T, std::size_t Extent = std::dynamic_extent>
using Span = std::span<T, Extent>;

} // namespace zeta

#endif // ZETA_MEMORY_SPAN_H
