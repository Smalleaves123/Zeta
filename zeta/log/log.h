#ifndef ZETA_LOG_LOG_H
#define ZETA_LOG_LOG_H

/// @file   log/log.h
/// @brief  Structured logging — `ZETA_LOG(INFO) << "message";`
///
/// Log levels: INFO, WARNING, ERROR, FATAL.
/// FATAL messages call `std::abort()` after flushing.
///
/// Compile-time filtering:
///   #define ZETA_MIN_LOG_LEVEL zeta::log_internal::LogSeverity::WARNING
/// Messages below this level are entirely eliminated (zero runtime cost).
///
/// Example:
///   ZETA_LOG(INFO) << "server listening on port " << port;
///   ZETA_LOG(ERROR) << "connection refused: " << err;

#include <sstream>
#include <span>
#include <string_view>
#include <vector>

#include "zeta/log/internal/severity.h"
#include "zeta/log/internal/sink.h"
#include "zeta/log/record.h"

namespace zeta {

// ═══════════════════════════════════════════════════════════════════════
// LogMessage — accumulates a single log record
// ═══════════════════════════════════════════════════════════════════════

class LogMessage {
public:
    LogMessage(log_internal::LogSeverity severity,
               const char* file, int line) noexcept
        : severity_(severity), file_(file), line_(line) {}

    LogMessage(const LogMessage&) = delete;
    LogMessage& operator=(const LogMessage&) = delete;

    ~LogMessage() {
        if (!log_internal::ShouldLog(severity_)) {
            if (severity_ == log_internal::LogSeverity::FATAL) {
                std::abort();
            }
            return;
        }
        std::string msg = stream_.str();
        if (fields_.empty()) {
            log_internal::ActiveSink()->Send(severity_, file_, line_, msg);
        } else {
            log_internal::ActiveSink()->Send(
                LogRecordView{severity_, file_, line_, msg, fields_});
        }
        if (severity_ == log_internal::LogSeverity::FATAL) {
            std::abort();
        }
    }

    /// Stream insertion — the primary API.
    template <typename T>
    LogMessage& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    /// Add an owned structured key/value field to this record.
    template <typename T>
    LogMessage& WithField(std::string_view key, const T& value) {
        std::ostringstream field_stream;
        field_stream << value;
        fields_.push_back(
            LogField{std::string(key), std::move(field_stream).str()});
        return *this;
    }

private:
    log_internal::LogSeverity severity_;
    const char*               file_;
    int                       line_;
    std::ostringstream        stream_;
    std::vector<LogField>     fields_;
};

} // namespace zeta

// ═══════════════════════════════════════════════════════════════════════
// ZETA_LOG macro
// ═══════════════════════════════════════════════════════════════════════

#define ZETA_LOG(level)                                                    \
    if (::zeta::log_internal::LogSeverity::level < ZETA_MIN_LOG_LEVEL)    \
        ;                                                                 \
    else                                                                  \
        ::zeta::LogMessage(::zeta::log_internal::LogSeverity::level,      \
                           __FILE__, __LINE__)

#define ZETA_LOG_IF(level, condition)                                      \
    if (!(condition))                                                       \
        ;                                                                  \
    else                                                                   \
        ZETA_LOG(level)

#endif // ZETA_LOG_LOG_H
