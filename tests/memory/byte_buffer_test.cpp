#include "zeta/memory/byte_buffer.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <string_view>

TEST_CASE("ByteBuffer: appends text and binary data", "[memory][byte_buffer]") {
    zeta::ByteBuffer buffer;
    buffer.Reserve(16);
    buffer.Append("hello");

    const std::array<std::byte, 3> binary{
        std::byte{0x00}, std::byte{0x7f}, std::byte{0xff}};
    buffer.Append(binary);

    REQUIRE(buffer.Size() == 8);
    REQUIRE(!buffer.Empty());
    REQUIRE(buffer.ReadableBytes()[0] == std::byte{'h'});
    REQUIRE(buffer.ReadableBytes()[5] == std::byte{0x00});
    REQUIRE(buffer.ReadableBytes()[7] == std::byte{0xff});
}

TEST_CASE("ByteBuffer: consumes a prefix and compacts on append",
          "[memory][byte_buffer]") {
    zeta::ByteBuffer buffer;
    buffer.Append("prefix-payload");

    REQUIRE(buffer.Consume(7));
    REQUIRE(buffer.Size() == 7);
    REQUIRE(std::string_view(
                reinterpret_cast<const char*>(buffer.ReadableBytes().data()),
                buffer.Size()) == "payload");

    buffer.Append("-tail");
    REQUIRE(std::string_view(
                reinterpret_cast<const char*>(buffer.ReadableBytes().data()),
                buffer.Size()) == "payload-tail");
}

TEST_CASE("ByteBuffer: rejects consuming beyond readable data",
          "[memory][byte_buffer]") {
    zeta::ByteBuffer buffer;
    buffer.Append("data");

    REQUIRE_FALSE(buffer.Consume(5));
    REQUIRE(buffer.Size() == 4);
    REQUIRE(buffer.Consume(4));
    REQUIRE(buffer.Empty());

    buffer.Clear();
    REQUIRE(buffer.Empty());
}
