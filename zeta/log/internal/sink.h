#ifndef ZETA_LOG_INTERNAL_SINK_H
#define ZETA_LOG_INTERNAL_SINK_H

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include "zeta/log/record.h"
#include "zeta/log/internal/severity.h"
#include "zeta/time/timestamp.h"

namespace zeta {
namespace log_internal {

// ═══════════════════════════════════════════════════════════════════════
// Formatting
// ═══════════════════════════════════════════════════════════════════════

[[nodiscard]] inline std::string FormatTimestamp() {
    return zeta::FormatNow();
}

class LogFormatter {
public:
    LogFormatter() = default;
    virtual ~LogFormatter() = default;

    LogFormatter(const LogFormatter&) = delete;
    LogFormatter& operator=(const LogFormatter&) = delete;

    [[nodiscard]] virtual std::string Format(
        const LogRecordView& record) {
        std::ostringstream out;
        out << '[' << FormatTimestamp() << "] ["
            << SeverityName(record.severity) << "] [tid="
            << std::this_thread::get_id() << "] "
            << record.file << ':' << record.line << ": "
            << record.message;
        for (const auto& field : record.fields) {
            out << " [" << field.key << '=' << field.value << ']';
        }
        out << '\n';
        return out.str();
    }
};

struct FormatterState {
    std::shared_mutex mutex;
    LogFormatter default_instance;
    LogFormatter* custom = nullptr;
};
[[nodiscard]] inline FormatterState& FormatterStateInstance() noexcept {
    static FormatterState state;
    return state;
}

[[nodiscard]] inline LogFormatter* ExchangeLogFormatter(
    LogFormatter* formatter) {
    auto& state = FormatterStateInstance();
    std::unique_lock<std::shared_mutex> lock(state.mutex);
    LogFormatter* previous = state.custom;
    state.custom = formatter;
    return previous;
}

inline void SetLogFormatter(LogFormatter* formatter) {
    (void)ExchangeLogFormatter(formatter);
}

[[nodiscard]] inline LogFormatter* ActiveFormatter() {
    auto& state = FormatterStateInstance();
    std::shared_lock<std::shared_mutex> lock(state.mutex);
    return state.custom ? state.custom : &state.default_instance;
}

[[nodiscard]] inline std::string FormatLogRecord(const LogRecordView& record) {
    auto& state = FormatterStateInstance();
    std::shared_lock<std::shared_mutex> lock(state.mutex);
    LogFormatter* formatter = state.custom ? state.custom : &state.default_instance;
    return formatter->Format(record);
}

class ScopedLogFormatter {
public:
    explicit ScopedLogFormatter(LogFormatter* formatter)
        : previous_(ExchangeLogFormatter(formatter)) {}

    ScopedLogFormatter(const ScopedLogFormatter&) = delete;
    ScopedLogFormatter& operator=(const ScopedLogFormatter&) = delete;

    ~ScopedLogFormatter() { SetLogFormatter(previous_); }

private:
    LogFormatter* previous_;
};

// ═══════════════════════════════════════════════════════════════════════
// LogSink
// ═══════════════════════════════════════════════════════════════════════

class LogSink {
public:
    LogSink() = default;
    virtual ~LogSink() = default;

    LogSink(const LogSink&) = delete;
    LogSink& operator=(const LogSink&) = delete;

    virtual void Send(const LogRecordView& record) {
        std::string formatted = FormatLogRecord(record);
        std::fwrite(formatted.data(), 1, formatted.size(), stderr);
        std::fflush(stderr);
    }

    /// Legacy overload retained for sinks implemented before structured fields.
    virtual void Send(LogSeverity severity, const char* file, int line,
                      std::string_view message) {
        Send(LogRecordView{severity, file, line, message});
    }
};

class FileLogSink : public LogSink {
public:
    explicit FileLogSink(std::filesystem::path path,
                         std::size_t max_bytes = 0,
                         bool append = true,
                         std::size_t max_files = 2)
        : path_(std::move(path)),
          max_bytes_(max_bytes),
          append_(append),
          max_files_(max_files < 1 ? 1 : max_files) {
        OpenFile(append_);
    }

    ~FileLogSink() override {
        if (file_ != nullptr) {
            std::fclose(file_);
        }
    }

    void Send(const LogRecordView& record) override {
        std::lock_guard<std::mutex> lock(mu_);
        if (file_ == nullptr) return;

        std::string formatted = FormatLogRecord(record);
        if (max_bytes_ > 0 && current_size_ + formatted.size() > max_bytes_) {
            Rotate();
            if (file_ == nullptr) return;
        }

        std::fwrite(formatted.data(), 1, formatted.size(), file_);
        std::fflush(file_);
        current_size_ += formatted.size();
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
        if (max_files_ > 1) {
            for (std::size_t i = max_files_ - 1; i > 0; --i) {
                auto older = RotatedPath(i);
                auto newer = RotatedPath(i + 1);
                if (i == max_files_ - 1) {
                    std::filesystem::remove(newer, ec);
                }
                if (std::filesystem::exists(older, ec)) {
                    std::filesystem::rename(older, newer, ec);
                }
            }
            if (std::filesystem::exists(path_, ec)) {
                std::filesystem::rename(path_, RotatedPath(1), ec);
            }
        } else {
            std::filesystem::remove(path_, ec);
        }
        OpenFile(false);
    }

    [[nodiscard]] std::filesystem::path RotatedPath(std::size_t index) const {
        auto rotated = path_;
        rotated += ".";
        rotated += std::to_string(index);
        return rotated;
    }

    std::filesystem::path path_;
    std::size_t max_bytes_ = 0;
    bool append_ = true;
    std::size_t max_files_ = 2;
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

/// Shared mutable pointer + default sink.  Logging holds a shared lock while
/// dispatching, so a scoped/custom sink cannot be replaced mid-dispatch.
struct SinkState {
    std::shared_mutex mutex;
    LogSink  default_instance;
    LogSink* custom = nullptr;
};
[[nodiscard]] inline SinkState& SinkStateInstance() noexcept {
    static SinkState state;
    return state;
}

[[nodiscard]] inline LogSink* ActiveSink() {
    auto& state = SinkStateInstance();
    std::shared_lock<std::shared_mutex> lock(state.mutex);
    return state.custom ? state.custom : &state.default_instance;
}

[[nodiscard]] inline LogSink* ExchangeLogSink(LogSink* sink) {
    auto& state = SinkStateInstance();
    std::unique_lock<std::shared_mutex> lock(state.mutex);
    LogSink* previous = state.custom;
    state.custom = sink;
    return previous;
}

inline void SetLogSink(LogSink* sink) {
    (void)ExchangeLogSink(sink);
}

inline void SendToActiveSink(const LogRecordView& record) {
    auto& state = SinkStateInstance();
    std::shared_lock<std::shared_mutex> lock(state.mutex);
    LogSink* sink = state.custom ? state.custom : &state.default_instance;
    sink->Send(record);
}

inline void SendToActiveSink(LogSeverity severity, const char* file, int line,
                             std::string_view message) {
    auto& state = SinkStateInstance();
    std::shared_lock<std::shared_mutex> lock(state.mutex);
    LogSink* sink = state.custom ? state.custom : &state.default_instance;
    sink->Send(severity, file, line, message);
}

class ScopedLogSink {
public:
    explicit ScopedLogSink(LogSink* sink)
        : previous_(ExchangeLogSink(sink)) {}

    ScopedLogSink(const ScopedLogSink&) = delete;
    ScopedLogSink& operator=(const ScopedLogSink&) = delete;

    ~ScopedLogSink() { SetLogSink(previous_); }

private:
    LogSink* previous_;
};

} // namespace log_internal
} // namespace zeta

#endif // ZETA_LOG_INTERNAL_SINK_H
