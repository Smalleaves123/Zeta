#include "zeta/synchronization/once.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

TEST_CASE("once: calls function exactly once", "[sync][once]") {
    zeta::OnceFlag flag;
    int count = 0;

    std::vector<std::thread> threads;
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&] {
            zeta::CallOnce(flag, [&] { ++count; });
        });
    }
    for (auto& t : threads) t.join();

    REQUIRE(count == 1);
}

TEST_CASE("once: already called returns quickly", "[sync][once]") {
    zeta::OnceFlag flag;
    zeta::CallOnce(flag, [] {});

    // Second call should be a no-op (fast path).
    int count = 0;
    zeta::CallOnce(flag, [&] { ++count; });
    REQUIRE(count == 0);
}

TEST_CASE("once: two separate flags", "[sync][once]") {
    zeta::OnceFlag a;
    zeta::OnceFlag b;
    int count = 0;

    zeta::CallOnce(a, [&] { ++count; });
    REQUIRE(count == 1);

    zeta::CallOnce(b, [&] { ++count; });
    REQUIRE(count == 2);
}
