#ifndef ZETA_LOG_INTERNAL_SINK_H
#define ZETA_LOG_INTERNAL_SINK_H

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "zeta/log/internal/severity.h"

namespace zeta {
namespace log_internal {

// ═══════════════════════════════════════════════════════════════════════
// Formatting helpers
// ═══════════════════════════════════════════════════════════════════════

[[nodiscard]] inline std::string FormatTimestamp() {
    using clock = std::chrono::system_clock;
    auto now = clock::now();
    auto time = clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif

    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);

    std::ostringstream out;
    out << buf << '.';
    if (ms.count() < 100) out << '0';
    if (ms.count() < 10) out << '0';
    out << ms.count();
    return out.str();
}

[[nodiscard]] inline std::string FormatLogRecord(
    LogSeverity severity, const char* file, int line,
    std::string_view message) {
    std::ostringstream out;
    out << '[' << FormatTimestamp() << "] ["
        << SeverityName(severity) << "] [tid="
        << std::this_thread::get_id() << "] "
        << file << ':' << line << ": " << message << '\n';
    return out.str();
}

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
        std::string record = FormatLogRecord(severity, file, line, message);
        std::fwrite(record.data(), 1, record.size(), stderr);
        std::fflush(stderr);
    }
};

class FileLogSink : public LogSink {
public:
    explicit FileLogSink(std::filesystem::path path,
                         std::size_t max_bytes = 0,
                         bool append = true)
        : path_(std::move(path)), max_bytes_(max_bytes), append_(append) {
        OpenFile(append_);
    }

    ~FileLogSink() override {
        if (file_ != nullptr) {
            std::fclose(file_);
        }
    }

    void Send(LogSeverity severity, const char* file, int line,
              std::string_view message) noexcept override {
        std::lock_guard<std::mutex> lock(mu_);
        if (file_ == nullptr) return;

        std::string record = FormatLogRecord(severity, file, line, message);
        if (max_bytes_ > 0 && current_size_ + record.size() > max_bytes_) {
            Rotate();
            if (file_ == nullptr) return;
        }

        std::fwrite(record.data(), 1, record.size(), file_);
        std::fflush(file_);
        current_size_ += record.size();
    }

private:
    void OpenFile(bool append) {
        const char* mode = append ? "ab" : "wb";
        file_ = std::fopen(path_.string().c_str(), mode);
        if (file_ == nullptr) return;

        if (append) {
            std::error_code ec;
            current_size_ = std::filesystem::exists(path_, ec)
                ? static_cast<std::size_t>(std::filesystem::file_size(path_, ec))
                : 0;
        } else {
            current_size_ = 0;
        }
    }

    void Rotate() {
        if (file_ != nullptr) {
            std::fclose(file_);
            file_ = nullptr;
        }

        std::error_code ec;
        auto rotated = path_;
        rotated += ".1";
        std::filesystem::remove(rotated, ec);
        std::filesystem::rename(path_, rotated, ec);
        OpenFile(false);
    }

    std::filesystem::path path_;
    std::size_t max_bytes_ = 0;
    bool append_ = true;
    std::size_t current_size_ = 0;
    std::FILE* file_ = nullptr;
    std::mutex mu_;
};

[[nodiscard]] inline std::atomic<int>& MinLogSeverityStorage() noexcept {
    static std::atomic<int> level{
        static_cast<int>(ZETA_MIN_LOG_LEVEL)};
    return level;
}

inline void SetMinLogSeverity(LogSeverity severity) noexcept {
    MinLogSeverityStorage().store(static_cast<int>(severity),
                                  std::memory_order_relaxed);
}

[[nodiscard]] inline LogSeverity MinLogSeverity() noexcept {
    return static_cast<LogSeverity>(
        MinLogSeverityStorage().load(std::memory_order_relaxed));
}

[[nodiscard]] inline bool ShouldLog(LogSeverity severity) noexcept {
    return severity == LogSeverity::FATAL ||
           static_cast<int>(severity) >=
               static_cast<int>(MinLogSeverity());
}

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
