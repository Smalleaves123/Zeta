#ifndef ZETA_STATUS_RESULT_H
#define ZETA_STATUS_RESULT_H

/// @file   status/result.h
/// @brief  Result alias for `StatusOr`.
///
/// `zeta::Result<T>` is the public-facing name for `StatusOr<T>` when a
/// result-oriented API reads better than an error-or-value API.
///
/// Example:
///   zeta::Result<int> ParseInt(std::string_view s);

#include "zeta/status/statusor.h"

namespace zeta {

template <typename T>
using Result = StatusOr<T>;

} // namespace zeta

#endif // ZETA_STATUS_RESULT_H
