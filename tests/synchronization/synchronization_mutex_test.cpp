#include "zeta/synchronization/mutex.h"

#include <catch2/catch_test_macros.hpp>

#include <thread>
#include <vector>

TEST_CASE("mutex: Lock/Unlock", "[sync][mutex]") {
    zeta::Mutex mu;
    int counter = 0;
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < 1000; ++i) {
                mu.Lock();
                ++counter;
                mu.Unlock();
            }
        });
    }
    for (auto& t : threads) t.join();
    REQUIRE(counter == 4000);
}

TEST_CASE("mutex: MutexLock RAII", "[sync][mutex]") {
    zeta::Mutex mu;
    {
        zeta::MutexLock lock(&mu);
        REQUIRE(mu.TryLock() == false);  // already held
    }
    REQUIRE(mu.TryLock() == true);
    mu.Unlock();
}

TEST_CASE("mutex: Reader/Writer lock", "[sync][mutex]") {
    zeta::Mutex mu;
    mu.ReaderLock();
    REQUIRE(mu.ReaderTryLock() == true);  // multiple readers OK
    mu.ReaderUnlock();
    mu.ReaderUnlock();

    mu.WriterLock();
    REQUIRE(mu.ReaderTryLock() == false);  // writer excludes readers
    mu.WriterUnlock();
}

TEST_CASE("mutex: ReaderMutexLock RAII", "[sync][mutex]") {
    zeta::Mutex mu;
    {
        zeta::ReaderMutexLock lock(&mu);
        REQUIRE(mu.ReaderTryLock());  // shared lock
        mu.ReaderUnlock();
    }
}

TEST_CASE("mutex: WriterMutexLock RAII", "[sync][mutex]") {
    zeta::Mutex mu;
    int val = 0;
    std::thread t([&] {
        zeta::WriterMutexLock lock(&mu);
        val = 42;
    });
    t.join();
    REQUIRE(val == 42);
}
