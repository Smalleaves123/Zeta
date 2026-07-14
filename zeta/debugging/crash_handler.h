#ifndef ZETA_DEBUGGING_CRASH_HANDLER_H
#define ZETA_DEBUGGING_CRASH_HANDLER_H

/// @file   debugging/crash_handler.h
/// @brief  Best-effort failure signal handler with raw stack output.

#include <csignal>
#include <cstddef>
#include <cstdlib>

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <execinfo.h>
#include <unistd.h>
#define ZETA_DEBUGGING_HAS_SIGNAL_BACKTRACE 1
#else
#define ZETA_DEBUGGING_HAS_SIGNAL_BACKTRACE 0
#endif

namespace zeta {

struct FailureSignalHandlerOptions {
    int output_fd = 2; // STDERR_FILENO on POSIX platforms.
};

namespace debugging_internal {

inline volatile std::sig_atomic_t& SignalHandlerActive() noexcept {
    static volatile std::sig_atomic_t active = 0;
    return active;
}

inline int& SignalOutputFd() noexcept {
    static int fd = 2;
    return fd;
}

inline const char* SignalName(int signal_number) noexcept {
    switch (signal_number) {
        case SIGABRT: return "SIGABRT";
        case SIGFPE:  return "SIGFPE";
        case SIGILL:  return "SIGILL";
        case SIGSEGV: return "SIGSEGV";
#ifdef SIGBUS
        case SIGBUS:  return "SIGBUS";
#endif
        default:      return "SIGNAL";
    }
}

#if ZETA_DEBUGGING_HAS_SIGNAL_BACKTRACE
inline void WriteLiteral(const char* text) noexcept {
    const char* cursor = text;
    while (*cursor != '\0') ++cursor;
    (void)::write(SignalOutputFd(), text,
                  static_cast<std::size_t>(cursor - text));
}
#endif

inline void FailureSignalHandler(int signal_number) noexcept {
    if (SignalHandlerActive() != 0) std::_Exit(128 + signal_number);
    SignalHandlerActive() = 1;

#if ZETA_DEBUGGING_HAS_SIGNAL_BACKTRACE
    WriteLiteral("\nZeta fatal signal: ");
    WriteLiteral(SignalName(signal_number));
    WriteLiteral("\nStack trace:\n");
    void* frames[64];
    const int count = ::backtrace(frames, 64);
    ::backtrace_symbols_fd(frames, count, SignalOutputFd());
#endif

    std::signal(signal_number, SIG_DFL);
    std::raise(signal_number);
}

} // namespace debugging_internal

/// Installs handlers for common fatal signals. Safe to call more than once.
inline void InstallFailureSignalHandler(
    FailureSignalHandlerOptions options = {}) noexcept {
    debugging_internal::SignalOutputFd() = options.output_fd;
    std::signal(SIGABRT, debugging_internal::FailureSignalHandler);
    std::signal(SIGFPE, debugging_internal::FailureSignalHandler);
    std::signal(SIGILL, debugging_internal::FailureSignalHandler);
    std::signal(SIGSEGV, debugging_internal::FailureSignalHandler);
#ifdef SIGBUS
    std::signal(SIGBUS, debugging_internal::FailureSignalHandler);
#endif
}

/// Restores default handling for signals managed by InstallFailureSignalHandler.
inline void UninstallFailureSignalHandler() noexcept {
    std::signal(SIGABRT, SIG_DFL);
    std::signal(SIGFPE, SIG_DFL);
    std::signal(SIGILL, SIG_DFL);
    std::signal(SIGSEGV, SIG_DFL);
#ifdef SIGBUS
    std::signal(SIGBUS, SIG_DFL);
#endif
}

} // namespace zeta

#undef ZETA_DEBUGGING_HAS_SIGNAL_BACKTRACE

#endif // ZETA_DEBUGGING_CRASH_HANDLER_H
