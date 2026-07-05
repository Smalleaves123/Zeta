#include "zeta/synchronization/synchronized.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <thread>
#include <vector>

TEST_CASE("Synchronized: read and write access work", "[sync][synchronized]") {
    zeta::Synchronized<std::vector<int>> data;

    {
        auto w = data.wlock();
        w->push_back(1);
        w->push_back(2);
    }

    auto r = data.rlock();
    REQUIRE(r->size() == 2);
    REQUIRE((*r)[0] == 1);
    REQUIRE((*r)[1] == 2);
}

TEST_CASE("Synchronized: WithWLock and WithRLock", "[sync][synchronized]") {
    zeta::Synchronized<std::string> text(std::in_place, "hello");

    text.WithWLock([](std::string& s) {
        s += " world";
    });

    auto len = text.WithRLock([](const std::string& s) {
        return s.size();
    });

    REQUIRE(len == 11);
}

TEST_CASE("Synchronized: concurrent increments are serialized", "[sync][synchronized][thread]") {
    zeta::Synchronized<int> counter(0);
    constexpr int kThreads = 4;
    constexpr int kIterations = 1000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&] {
            for (int j = 0; j < kIterations; ++j) {
                auto w = counter.wlock();
                ++(*w);
            }
        });
    }
    for (auto& t : threads) t.join();

    auto r = counter.rlock();
    REQUIRE(*r == kThreads * kIterations);
}

TEST_CASE("Synchronized: supports non-default-constructible types", "[sync][synchronized][compile]") {
    struct NonDefault {
        explicit NonDefault(int v) : value(v) {}
        int value;
    };

    zeta::Synchronized<NonDefault> item(std::in_place, 7);
    REQUIRE(item.rlock()->value == 7);
}
