#ifndef ZETA_LOG_RECORD_H
#define ZETA_LOG_RECORD_H

/// @file   log/record.h
/// @brief  Structured log record data shared by formatters and sinks.

#include <span>
#include <string>
#include <string_view>

#include "zeta/log/internal/severity.h"

namespace zeta {

/// An owned key/value pair attached to a log record.
struct LogField {
    std::string key;
    std::string value;
};

/// A non-owning view of one complete log record.
struct LogRecordView {
    log_internal::LogSeverity severity;
    const char* file;
    int line;
    std::string_view message;
    std::span<const LogField> fields{};
};

} // namespace zeta

namespace zeta::log_internal {
using LogRecordView = ::zeta::LogRecordView;
} // namespace zeta::log_internal

#endif // ZETA_LOG_RECORD_H
