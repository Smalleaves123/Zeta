#ifndef ZETA_LOG_INTERNAL_SINK_H
#define ZETA_LOG_INTERNAL_SINK_H

#include <cstdio>
#include <string_view>

#include "zeta/log/internal/severity.h"

namespace zeta {
namespace log_internal {

// ═══════════════════════════════════════════════════════════════════════
// LogSink
// ═══════════════════════════════════════════════════════════════════════

class LogSink {
public:
    LogSink() = default;
    virtual ~LogSink() = default;

    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;

    virtual void Send(LogSeverity severity, const char* file, int line,
                      std::string_view message) noexcept {
        std::fprintf(stderr, "[%s] %s:%d: %.*s\n",
                     SeverityName(severity).data(),
                     file, line,
                     static_cast<int>(message.size()),
                     message.data());
    }
};

/// Shared mutable pointer + default sink.  Both SetLogSink and ActiveSink
/// read/write this same state.
struct SinkState {
    LogSink  default_instance;
    LogSink* custom = nullptr;
};
[[nodiscard]] inline SinkState& SinkStateInstance() noexcept {
    static SinkState state;
    return state;
}

inline void SetLogSink(LogSink* sink) noexcept {
    SinkStateInstance().custom = sink;
}

[[nodiscard]] inline LogSink* ActiveSink() noexcept {
    auto& state = SinkStateInstance();
    return state.custom ? state.custom : &state.default_instance;
}

} // namespace log_internal
} // namespace zeta

#endif // ZETA_LOG_INTERNAL_SINK_H
