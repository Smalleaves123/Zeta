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
#include <string_view>

#include "zeta/log/internal/severity.h"
#include "zeta/log/internal/sink.h"

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
        log_internal::ActiveSink()->Send(severity_, file_, line_, msg);
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

private:
    log_internal::LogSeverity severity_;
    const char*               file_;
    int                       line_;
    std::ostringstream        stream_;
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

#endif // ZETA_LOG_LOG_H
