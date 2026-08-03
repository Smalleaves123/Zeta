#include "zeta/synchronization/channel.h"

#include <catch2/catch_test_macros.hpp>

#include <future>
#include <memory>
#include <thread>

TEST_CASE("Channel: sends and receives values", "[sync][channel]") {
    zeta::Channel<int> channel;

    REQUIRE(channel.Send(42));
    REQUIRE(channel.Size() == 1);
    REQUIRE(channel.TryReceive().value() == 42);
    REQUIRE(channel.Size() == 0);

    channel.Close();
    REQUIRE_FALSE(channel.Send(7));
    REQUIRE_FALSE(channel.TryReceive().has_value());
}

TEST_CASE("Channel: supports move-only values", "[sync][channel]") {
    zeta::Channel<std::unique_ptr<int>> channel;
    auto value = std::make_unique<int>(42);

    REQUIRE(channel.Send(std::move(value)));
    REQUIRE(value == nullptr);

    auto received = channel.Receive();
    REQUIRE(received.has_value());
    REQUIRE(**received == 42);
}

TEST_CASE("Channel: bounded sends fail without blocking when full",
          "[sync][channel]") {
    zeta::Channel<int> channel(1);

    REQUIRE(channel.TrySend(1));
    REQUIRE_FALSE(channel.TrySend(2));
    REQUIRE(channel.TryReceive().value() == 1);
    REQUIRE(channel.TrySend(2));
}

TEST_CASE("Channel: close wakes receivers and drains queued values",
          "[sync][channel]") {
    zeta::Channel<int> draining(1);
    REQUIRE(draining.TrySend(1));
    draining.Close();
    REQUIRE(draining.Receive().value() == 1);
    REQUIRE_FALSE(draining.Receive().has_value());

    zeta::Channel<int> channel(1);
    std::promise<bool> receiver_returned;
    auto receiver_result = receiver_returned.get_future();

    std::thread receiver([&] {
        receiver_returned.set_value(!channel.Receive().has_value());
    });

    channel.Close();
    receiver.join();
    REQUIRE(receiver_result.get());
    REQUIRE(channel.IsClosed());
    REQUIRE_FALSE(channel.Receive().has_value());

    REQUIRE(channel.TrySend(1) == false);
}
