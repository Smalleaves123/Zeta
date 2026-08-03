#include "zeta/log/log.h"
#include "zeta/log/formatters.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

// A simple test sink that captures output.
class TestSink : public zeta::log_internal::LogSink {
public:
    void Send(zeta::log_internal::LogSeverity severity,
              const char* file, int line,
              std::string_view message) noexcept override {
        (void)file;
        (void)line;
        last_severity = severity;
        last_message = std::string(message);
        call_count++;
    }
    zeta::log_internal::LogSeverity last_severity =
        zeta::log_internal::LogSeverity::INFO;
    std::string last_message;
    int call_count = 0;
};

class PrefixFormatter : public zeta::log_internal::LogFormatter {
public:
    std::string Format(
        const zeta::log_internal::LogRecordView& record) noexcept override {
        std::ostringstream out;
        out << "PREFIX|" << zeta::log_internal::SeverityName(record.severity)
            << '|' << record.message << '\n';
        return out.str();
    }
};

TEST_CASE("log: severity names", "[log]") {
    REQUIRE(zeta::log_internal::SeverityName(
        zeta::log_internal::LogSeverity::INFO) == "INFO");
    REQUIRE(zeta::log_internal::SeverityName(
        zeta::log_internal::LogSeverity::WARNING) == "WARNING");
    REQUIRE(zeta::log_internal::SeverityName(
        zeta::log_internal::LogSeverity::ERROR) == "ERROR");
    REQUIRE(zeta::log_internal::SeverityName(
        zeta::log_internal::LogSeverity::FATAL) == "FATAL");
}

TEST_CASE("log: LogMessage with custom sink", "[log]") {
    TestSink sink;
    zeta::log_internal::ScopedLogSink scoped_sink(&sink);

    {
        zeta::LogMessage msg(zeta::log_internal::LogSeverity::WARNING,
                             "test.cpp", 42);
        msg << "hello " << 123;
        // destructor sends to sink
    }

    REQUIRE(sink.call_count == 1);
    REQUIRE(sink.last_severity == zeta::log_internal::LogSeverity::WARNING);
    REQUIRE(sink.last_message == "hello 123");
}

TEST_CASE("log: ZETA_LOG macro compiles", "[log]") {
    // Just verify the macro compiles and doesn't crash.
    // (Output goes to stderr by default — acceptable in tests.)
    ZETA_LOG(INFO) << "test message " << 42;
    SUCCEED();
}

TEST_CASE("log: runtime severity filter", "[log]") {
    TestSink sink;
    zeta::log_internal::ScopedLogSink scoped_sink(&sink);
    zeta::log_internal::SetMinLogSeverity(
        zeta::log_internal::LogSeverity::ERROR);

    {
        zeta::LogMessage msg(zeta::log_internal::LogSeverity::INFO,
                             "test.cpp", 7);
        msg << "filtered";
    }
    {
        zeta::LogMessage msg(zeta::log_internal::LogSeverity::ERROR,
                             "test.cpp", 8);
        msg << "kept";
    }

    REQUIRE(sink.call_count == 1);
    REQUIRE(sink.last_message == "kept");
    zeta::log_internal::SetMinLogSeverity(
        zeta::log_internal::LogSeverity::INFO);
}

TEST_CASE("log: file sink writes formatted records", "[log]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_log_file_sink_test.log";
    std::filesystem::remove(path);

    {
        zeta::log_internal::FileLogSink sink(path, 0, false);
        zeta::log_internal::ScopedLogSink scoped_sink(&sink);
        ZETA_LOG(WARNING) << "file sink message";
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content.find("WARNING") != std::string::npos);
    REQUIRE(content.find("file sink message") != std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("log: file sink rotates when max size exceeded", "[log]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_log_rotate_test.log";
    auto rotated = path;
    rotated += ".1";
    std::filesystem::remove(path);
    std::filesystem::remove(rotated);

    {
        zeta::log_internal::FileLogSink sink(path, 80, false, 3);
        zeta::log_internal::ScopedLogSink scoped_sink(&sink);
        ZETA_LOG(INFO) << "first message that is intentionally long";
        ZETA_LOG(INFO) << "second message that should rotate";
        ZETA_LOG(INFO) << "third message that should rotate again";
    }

    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::exists(rotated));
    REQUIRE(std::filesystem::exists(path.string() + ".2"));

    std::filesystem::remove(path);
    std::filesystem::remove(rotated);
    std::filesystem::remove(path.string() + ".2");
}

TEST_CASE("log: scoped sink restores previous sink", "[log]") {
    TestSink sink1;
    TestSink sink2;
    zeta::log_internal::SetLogSink(&sink1);

    {
        zeta::log_internal::ScopedLogSink scoped_sink(&sink2);
        ZETA_LOG(INFO) << "inner";
    }
    ZETA_LOG(INFO) << "outer";

    REQUIRE(sink2.call_count == 1);
    REQUIRE(sink2.last_message == "inner");
    REQUIRE(sink1.call_count == 1);
    REQUIRE(sink1.last_message == "outer");

    zeta::log_internal::SetLogSink(nullptr);
}

TEST_CASE("log: custom formatter is applied", "[log]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_log_formatter_test.log";
    std::filesystem::remove(path);

    PrefixFormatter formatter;
    {
        zeta::log_internal::FileLogSink sink(path, 0, false);
        zeta::log_internal::ScopedLogSink scoped_sink(&sink);
        zeta::log_internal::ScopedLogFormatter scoped_formatter(&formatter);
        ZETA_LOG(ERROR) << "formatted";
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content.find("PREFIX|ERROR|formatted") != std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("log: structured fields are formatted as JSON", "[log][structured]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_log_json_formatter_test.log";
    std::filesystem::remove(path);

    {
        zeta::JsonLogFormatter formatter;
        zeta::log_internal::FileLogSink sink(path, 0, false);
        zeta::log_internal::ScopedLogSink scoped_sink(&sink);
        zeta::log_internal::ScopedLogFormatter scoped_formatter(&formatter);
        zeta::LogMessage(zeta::log_internal::LogSeverity::INFO,
                         "structured.cpp", 18)
            .WithField("request_id", 42)
            .WithField("detail", "quoted \"value\"\n")
            << "request completed";
    }

    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    REQUIRE(content.find("\"severity\":\"INFO\"") != std::string::npos);
    REQUIRE(content.find("\"request_id\":\"42\"") != std::string::npos);
    REQUIRE(content.find("\"detail\":\"quoted \\\"value\\\"\\n\"") !=
            std::string::npos);

    std::filesystem::remove(path);
}

TEST_CASE("log: conditional macro only emits when enabled", "[log]") {
    TestSink sink;
    zeta::log_internal::ScopedLogSink scoped_sink(&sink);

    ZETA_LOG_IF(INFO, false) << "filtered";
    REQUIRE(sink.call_count == 0);

    ZETA_LOG_IF(INFO, true) << "kept";
    REQUIRE(sink.call_count == 1);
    REQUIRE(sink.last_message == "kept");
}
