#ifndef ZETA_STATUS_STATUS_MACROS_H
#define ZETA_STATUS_STATUS_MACROS_H

/// @file   status/status_macros.h
/// @brief  Convenience macros for Status / StatusOr propagation.

#include "zeta/status/status_builder.h"
#include "zeta/status/statusor.h"

#include <utility>

#define ZETA_STATUS_PRIVATE_CONCAT_INNER_(x, y) x##y
#define ZETA_STATUS_PRIVATE_CONCAT_(x, y) ZETA_STATUS_PRIVATE_CONCAT_INNER_(x, y)
#define ZETA_STATUS_PRIVATE_UNIQUE_(x) ZETA_STATUS_PRIVATE_CONCAT_(x, __LINE__)

#define ZETA_RETURN_IF_ERROR(expr)                                              \
    for (::zeta::Status ZETA_STATUS_PRIVATE_UNIQUE_(_status_) = (expr);         \
         !ZETA_STATUS_PRIVATE_UNIQUE_(_status_).ok();)                          \
        return ::zeta::StatusBuilder(                                           \
            std::move(ZETA_STATUS_PRIVATE_UNIQUE_(_status_)))

#define ZETA_ASSIGN_OR_RETURN(lhs, expr)                                        \
    auto ZETA_STATUS_PRIVATE_UNIQUE_(_status_or_) = (expr);                     \
    if (!ZETA_STATUS_PRIVATE_UNIQUE_(_status_or_).ok())                         \
        return std::move(ZETA_STATUS_PRIVATE_UNIQUE_(_status_or_)).status();    \
    lhs = std::move(*ZETA_STATUS_PRIVATE_UNIQUE_(_status_or_))

#endif // ZETA_STATUS_STATUS_MACROS_H
