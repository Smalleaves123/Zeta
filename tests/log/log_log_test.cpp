#include "zeta/log/log.h"

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
    zeta::log_internal::SetLogSink(&sink);

    {
        zeta::LogMessage msg(zeta::log_internal::LogSeverity::WARNING,
                             "test.cpp", 42);
        msg << "hello " << 123;
        // destructor sends to sink
    }

    REQUIRE(sink.call_count == 1);
    REQUIRE(sink.last_severity == zeta::log_internal::LogSeverity::WARNING);
    REQUIRE(sink.last_message == "hello 123");

    zeta::log_internal::SetLogSink(nullptr);
}

TEST_CASE("log: ZETA_LOG macro compiles", "[log]") {
    // Just verify the macro compiles and doesn't crash.
    // (Output goes to stderr by default — acceptable in tests.)
    ZETA_LOG(INFO) << "test message " << 42;
    SUCCEED();
}

TEST_CASE("log: runtime severity filter", "[log]") {
    TestSink sink;
    zeta::log_internal::SetLogSink(&sink);
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
    zeta::log_internal::SetLogSink(nullptr);
}

TEST_CASE("log: file sink writes formatted records", "[log]") {
    auto path = std::filesystem::temp_directory_path() /
        "zeta_log_file_sink_test.log";
    std::filesystem::remove(path);

    {
        zeta::log_internal::FileLogSink sink(path, 0, false);
        zeta::log_internal::SetLogSink(&sink);
        ZETA_LOG(WARNING) << "file sink message";
        zeta::log_internal::SetLogSink(nullptr);
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
        zeta::log_internal::FileLogSink sink(path, 80, false);
        zeta::log_internal::SetLogSink(&sink);
        ZETA_LOG(INFO) << "first message that is intentionally long";
        ZETA_LOG(INFO) << "second message that should rotate";
        zeta::log_internal::SetLogSink(nullptr);
    }

    REQUIRE(std::filesystem::exists(path));
    REQUIRE(std::filesystem::exists(rotated));

    std::filesystem::remove(path);
    std::filesystem::remove(rotated);
}
