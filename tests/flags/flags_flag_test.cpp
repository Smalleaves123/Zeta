#include "zeta/flags/flag.h"
#include "zeta/flags/help.h"
#include "zeta/flags/parse.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>

// Register test flags (static init — registered via ZETA_FLAG macro)
ZETA_FLAG(int32, test_port, 8080, "Test port number");
ZETA_FLAG(bool, test_verbose, false, "Enable verbose mode");
ZETA_FLAG(string, test_name, "default", "Test name");
ZETA_FLAG(double, test_ratio, 1.5, "Test ratio");
ZETA_FLAG(float, test_timeout, 1.25f, "Request timeout");
ZETA_FLAG_ENV(int32, test_env_port, 7000, "Port from the environment",
             "ZETA_TEST_PORT");

TEST_CASE("flags: default values", "[flags]") {
    REQUIRE(FLAGS_test_port.Get() == 8080);
    REQUIRE(FLAGS_test_verbose.Get() == false);
    REQUIRE(FLAGS_test_name.Get() == "default");
    REQUIRE(FLAGS_test_ratio.Get() == 1.5);
}

TEST_CASE("flags: set and get", "[flags]") {
    FLAGS_test_port.Set(9090);
    REQUIRE(FLAGS_test_port.Get() == 9090);
    FLAGS_test_port.Set(8080); // restore
}

TEST_CASE("flags: Parse bool", "[flags]") {
    zeta::Flag<bool> f("debug", "debug mode", __FILE__, false);
    REQUIRE(f.Parse("true"));
    REQUIRE(f.Get() == true);
    REQUIRE(f.Parse("false"));
    REQUIRE(f.Get() == false);
    REQUIRE(f.Parse("1"));
    REQUIRE(f.Get() == true);
    REQUIRE(f.Parse("0"));
    REQUIRE(f.Get() == false);
    REQUIRE(f.Parse("yes"));
    REQUIRE(f.Get() == true);
    REQUIRE(f.Parse("no"));
    REQUIRE(f.Get() == false);
}

TEST_CASE("flags: Parse int32", "[flags]") {
    zeta::Flag<int32_t> f("count", "count", __FILE__, 0);
    REQUIRE(f.Parse("42"));
    REQUIRE(f.Get() == 42);
    REQUIRE(f.Parse("-100"));
    REQUIRE(f.Get() == -100);
    REQUIRE(!f.Parse("not_a_number"));
}

TEST_CASE("flags: Parse string", "[flags]") {
    zeta::Flag<std::string> f("name", "name", __FILE__, "");
    REQUIRE(f.Parse("hello"));
    REQUIRE(f.Get() == "hello");
}

TEST_CASE("flags: Parse float and typed separated arguments", "[flags][typed]") {
    zeta::Flag<float> timeout("timeout", "timeout", __FILE__, 1.0f);
    REQUIRE(timeout.Parse("2.5"));
    REQUIRE(timeout.Get() == 2.5f);

    char arg0[] = "program";
    char arg1[] = "--test_port";
    char arg2[] = "9191";
    char* argv[] = {arg0, arg1, arg2};
    const auto result = zeta::ParseCommandLineChecked(3, argv);
    REQUIRE(result.ok());
    REQUIRE(FLAGS_test_port.Get() == 9191);
    FLAGS_test_port.Set(8080);
}

TEST_CASE("flags: ParseCommandLine with known flags", "[flags]") {
    char arg0[] = "program";
    char arg1[] = "--test_port=9090";
    char arg2[] = "--test_verbose";
    char arg3[] = "positional_arg";
    char* argv[] = {arg0, arg1, arg2, arg3};
    int argc = 4;

    auto rest = zeta::ParseCommandLine(argc, argv);

    REQUIRE(FLAGS_test_port.Get() == 9090);
    REQUIRE(FLAGS_test_verbose.Get() == true);
    REQUIRE(rest.size() == 1);
    REQUIRE(rest[0] == "positional_arg");

    // Restore defaults.
    FLAGS_test_port.Set(8080);
    FLAGS_test_verbose.Set(false);
}

TEST_CASE("flags: ParseCommandLine supports --noflag for bools", "[flags]") {
    FLAGS_test_verbose.Set(true);

    char arg0[] = "program";
    char arg1[] = "--notest_verbose";
    char* argv[] = {arg0, arg1};
    int argc = 2;

    auto rest = zeta::ParseCommandLine(argc, argv);

    REQUIRE(rest.empty());
    REQUIRE(FLAGS_test_verbose.Get() == false);

    FLAGS_test_verbose.Set(false);
}

TEST_CASE("flags: TypeName returns correct types", "[flags]") {
    zeta::Flag<int32_t> f1("a", "", __FILE__, 0);
    zeta::Flag<bool>     f2("b", "", __FILE__, false);
    zeta::Flag<std::string> f3("c", "", __FILE__, "");
    zeta::Flag<double>   f4("d", "", __FILE__, 0.0);
    zeta::Flag<float>    f5("e", "", __FILE__, 0.0f);

    REQUIRE(f1.TypeName() == "int32");
    REQUIRE(f2.TypeName() == "bool");
    REQUIRE(f3.TypeName() == "string");
    REQUIRE(f4.TypeName() == "double");
    REQUIRE(f5.TypeName() == "float");
}

TEST_CASE("flags: environment values apply before command line", "[flags][env]") {
#if defined(_WIN32)
    _putenv_s("ZETA_TEST_PORT", "7777");
#else
    setenv("ZETA_TEST_PORT", "7777", 1);
#endif
    const auto env_errors = zeta::ApplyEnvironmentVariables();
    REQUIRE(env_errors.empty());
    REQUIRE(FLAGS_test_env_port.Get() == 7777);

    char arg0[] = "program";
    char arg1[] = "--test_env_port=8888";
    char* argv[] = {arg0, arg1};
    const auto result = zeta::ParseCommandLineChecked(2, argv);
    REQUIRE(result.ok());
    REQUIRE(FLAGS_test_env_port.Get() == 8888);

#if defined(_WIN32)
    _putenv_s("ZETA_TEST_PORT", "");
#else
    unsetenv("ZETA_TEST_PORT");
#endif
    FLAGS_test_env_port.Set(7000);
}

TEST_CASE("flags: validators and required configuration are checked", "[flags][validation]") {
    zeta::Flag<int32_t> positive("positive", "positive", __FILE__, 3);
    positive.SetValidator([](const int32_t& value) { return value > 0; },
                          "must be positive");
    REQUIRE(!positive.Parse("0"));
    REQUIRE(positive.Get() == 3);
    REQUIRE(positive.Parse("9"));

    std::string error;
    REQUIRE(positive.Validate(&error));

    zeta::Flag<int32_t> required(
        "required", "required", __FILE__, 0, false,
        zeta::FlagOptions{nullptr, true});
    REQUIRE(!required.Validate(&error));
    REQUIRE(error == "required flag was not set");
    required.Set(1);
    REQUIRE(required.Validate(&error));
}

TEST_CASE("flags: generated help includes type, default, and environment", "[flags][help]") {
    const std::string help = zeta::FormatFlagsHelp("Usage");
    REQUIRE(help.find("Usage:") != std::string::npos);
    REQUIRE(help.find("--test_env_port=<int32>") != std::string::npos);
    REQUIRE(help.find("Port from the environment") != std::string::npos);
    REQUIRE(help.find("[default: 7000]") != std::string::npos);
    REQUIRE(help.find("[env: ZETA_TEST_PORT]") != std::string::npos);
}

TEST_CASE("flags: CurrentValue is safe under concurrent reads", "[flags][thread]") {
    zeta::Flag<int32_t> f("threads", "", __FILE__, 1234);
    constexpr int kThreads = 8;
    constexpr int kIterations = 1000;

    std::atomic<bool> ok{true};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < kIterations; ++j) {
                if (f.CurrentValue() != "1234") ok.store(false, std::memory_order_relaxed);
            }
        });
    }
    for (auto& t : threads) t.join();
    REQUIRE(ok.load(std::memory_order_relaxed));
}
