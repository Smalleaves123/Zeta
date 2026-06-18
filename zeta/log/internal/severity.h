#ifndef ZETA_LOG_INTERNAL_SEVERITY_H
#define ZETA_LOG_INTERNAL_SEVERITY_H

/// @file   log/internal/severity.h
/// @brief  Log severity levels and compile-time filtering.

#include <cstdint>
#include <string_view>

namespace zeta {
namespace log_internal {

// ═══════════════════════════════════════════════════════════════════════
// LogSeverity
// ═══════════════════════════════════════════════════════════════════════

enum class LogSeverity : int8_t {
    INFO    = 0,
    WARNING = 1,
    ERROR   = 2,
    FATAL   = 3,
};

/// Compile-time minimum severity.  Messages below this level are
/// completely eliminated (zero runtime cost).  Override with:
///   #define ZETA_MIN_LOG_LEVEL zeta::log_internal::LogSeverity::WARNING
#ifndef ZETA_MIN_LOG_LEVEL
#define ZETA_MIN_LOG_LEVEL ::zeta::log_internal::LogSeverity::INFO
#endif

[[nodiscard]] constexpr std::string_view SeverityName(LogSeverity s) noexcept {
    switch (s) {
        case LogSeverity::INFO:    return "INFO";
        case LogSeverity::WARNING: return "WARNING";
        case LogSeverity::ERROR:   return "ERROR";
        case LogSeverity::FATAL:   return "FATAL";
    }
    return "UNKNOWN";
}

} // namespace log_internal
} // namespace zeta

#endif // ZETA_LOG_INTERNAL_SEVERITY_H
