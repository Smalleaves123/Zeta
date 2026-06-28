#include "zeta/flags/flag.h"
#include "zeta/flags/parse.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>
#include <string>

// Register test flags (static init — registered via ZETA_FLAG macro)
ZETA_FLAG(int32, test_port, 8080, "Test port number");
ZETA_FLAG(bool, test_verbose, false, "Enable verbose mode");
ZETA_FLAG(string, test_name, "default", "Test name");
ZETA_FLAG(double, test_ratio, 1.5, "Test ratio");

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

TEST_CASE("flags: TypeName returns correct types", "[flags]") {
    zeta::Flag<int32_t> f1("a", "", __FILE__, 0);
    zeta::Flag<bool>     f2("b", "", __FILE__, false);
    zeta::Flag<std::string> f3("c", "", __FILE__, "");
    zeta::Flag<double>   f4("d", "", __FILE__, 0.0);

    REQUIRE(f1.TypeName() == "int32");
    REQUIRE(f2.TypeName() == "bool");
    REQUIRE(f3.TypeName() == "string");
    REQUIRE(f4.TypeName() == "double");
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
