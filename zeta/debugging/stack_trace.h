#ifndef ZETA_DEBUGGING_STACK_TRACE_H
#define ZETA_DEBUGGING_STACK_TRACE_H

/// @file   debugging/stack_trace.h
/// @brief  Portable stack capture and best-effort symbolization.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#define ZETA_DEBUGGING_HAS_NATIVE_STACK_TRACE 1
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#define ZETA_DEBUGGING_HAS_NATIVE_STACK_TRACE 1
#else
#define ZETA_DEBUGGING_HAS_NATIVE_STACK_TRACE 0
#endif

namespace zeta {

/// A captured instruction address and its best-effort symbol name.
struct StackFrame {
    std::uintptr_t address = 0;
    std::string symbol;
};

namespace debugging_internal {

inline std::string FormatAddress(std::uintptr_t address) {
    std::ostringstream out;
    out << "0x" << std::hex << address;
    return out.str();
}

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
inline std::string Demangle(const char* name) {
    if (name == nullptr) return {};
    int status = 0;
    char* demangled = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status != 0 || demangled == nullptr) return name;
    std::string result(demangled);
    std::free(demangled);
    return result;
}
#endif

} // namespace debugging_internal

/// Converts an instruction address to a human-readable symbol where possible.
inline std::string Symbolize(std::uintptr_t address) {
    if (address == 0) return "0x0";

#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
    Dl_info info{};
    if (dladdr(reinterpret_cast<void*>(address), &info) != 0) {
        std::string symbol = debugging_internal::Demangle(info.dli_sname);
        if (symbol.empty()) {
            symbol = info.dli_fname != nullptr ? info.dli_fname : "unknown";
        }
        if (info.dli_saddr != nullptr) {
            const auto symbol_address = reinterpret_cast<std::uintptr_t>(info.dli_saddr);
            if (address >= symbol_address) {
                symbol += "+" + debugging_internal::FormatAddress(
                    address - symbol_address);
            }
        }
        return symbol;
    }
#endif

    return debugging_internal::FormatAddress(address);
}

inline std::string Symbolize(const void* address) {
    return Symbolize(reinterpret_cast<std::uintptr_t>(address));
}

/// Owns a captured sequence of stack frames.
class StackTrace {
public:
    StackTrace() = default;

    /// Captures the current call stack. `skip` omits caller frames.
    [[nodiscard]] static StackTrace Capture(std::size_t skip = 0,
                                            std::size_t max_depth = 64) {
        StackTrace trace;
        if (max_depth == 0) return trace;

        constexpr std::size_t kMaxFrames = 256;
        const std::size_t depth = std::min(max_depth, kMaxFrames);

#if defined(_WIN32)
        void* addresses[kMaxFrames];
        const auto count = CaptureStackBackTrace(
            static_cast<DWORD>(skip + 1), static_cast<DWORD>(depth),
            addresses, nullptr);
        for (USHORT i = 0; i < count; ++i) {
            const auto address = reinterpret_cast<std::uintptr_t>(addresses[i]);
            trace.frames_.push_back({address, Symbolize(address)});
        }
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
        void* addresses[kMaxFrames + 1];
        const int count = backtrace(addresses, static_cast<int>(depth + 1));
        const std::size_t begin = std::min<std::size_t>(
            static_cast<std::size_t>(count), skip + 1);
        for (std::size_t i = begin; i < static_cast<std::size_t>(count); ++i) {
            const auto address = reinterpret_cast<std::uintptr_t>(addresses[i]);
            trace.frames_.push_back({address, Symbolize(address)});
        }
#else
        (void)skip;
#endif
        return trace;
    }

    [[nodiscard]] std::size_t size() const noexcept { return frames_.size(); }
    [[nodiscard]] bool empty() const noexcept { return frames_.empty(); }
    [[nodiscard]] const StackFrame& operator[](std::size_t index) const {
        return frames_[index];
    }
    [[nodiscard]] const std::vector<StackFrame>& frames() const noexcept {
        return frames_;
    }

    /// Formats the trace as one `#N address symbol` entry per line.
    [[nodiscard]] std::string ToString() const {
        std::ostringstream out;
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            out << '#' << i << ' ' << debugging_internal::FormatAddress(
                frames_[i].address) << ' ' << frames_[i].symbol << '\n';
        }
        return out.str();
    }

private:
    std::vector<StackFrame> frames_;
};

[[nodiscard]] inline StackTrace CaptureStackTrace(std::size_t skip = 0,
                                                   std::size_t max_depth = 64) {
    return StackTrace::Capture(skip + 1, max_depth);
}

} // namespace zeta

#undef ZETA_DEBUGGING_HAS_NATIVE_STACK_TRACE

#endif // ZETA_DEBUGGING_STACK_TRACE_H
