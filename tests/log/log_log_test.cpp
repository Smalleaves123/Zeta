#include "zeta/log/log.h"

#include <catch2/catch_test_macros.hpp>

#include <sstream>

// A simple test sink that captures output.
class TestSink : public zeta::log_internal::LogSink {
public:
    void Send(zeta::log_internal::LogSeverity severity,
              const char* file, int line,
              std::string_view message) noexcept override {
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
