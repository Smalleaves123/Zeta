#include "zeta/debugging/assert.h"
#include "zeta/debugging/crash_handler.h"
#include "zeta/debugging/stack_trace.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("debugging: assertions and checks accept valid conditions", "[debugging]") {
    int value = 7;
    ZETA_ASSERT(value == 7);
    ZETA_DCHECK(value == 7);
    ZETA_CHECK(value == 7);
    ZETA_CHECK_MSG(value == 7, "value should be initialized");
    SUCCEED();
}

TEST_CASE("debugging: captures and formats a stack trace", "[debugging]") {
    const zeta::StackTrace trace = zeta::CaptureStackTrace(0, 16);
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(_WIN32)
    REQUIRE_FALSE(trace.empty());
    REQUIRE_FALSE(trace[0].symbol.empty());
    REQUIRE_FALSE(trace.ToString().empty());
#else
    SUCCEED();
#endif
}

TEST_CASE("debugging: null symbolization is stable", "[debugging]") {
    REQUIRE(zeta::Symbolize(nullptr) == "0x0");
}

TEST_CASE("debugging: failure signal handlers install and uninstall", "[debugging]") {
    zeta::InstallFailureSignalHandler();
    zeta::UninstallFailureSignalHandler();
    SUCCEED();
}
