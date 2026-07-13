#include "zeta/crc/crc32c.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>

TEST_CASE("crc32c: standard vectors", "[crc]") {
    REQUIRE(zeta::ComputeCrc32c("") == 0x00000000U);
    REQUIRE(zeta::ComputeCrc32c("a") == 0xC1D04330U);
    REQUIRE(zeta::ComputeCrc32c("123456789") == 0xE3069283U);
}

TEST_CASE("crc32c: streaming extension matches one-shot", "[crc]") {
    zeta::Crc32cAccumulator accumulator;
    accumulator.Update("1234");
    accumulator.Update("56789");

    REQUIRE(accumulator.Finalize() == zeta::ComputeCrc32c("123456789"));
    REQUIRE(zeta::ExtendCrc32c(zeta::ComputeCrc32c("1234"), "56789") ==
            zeta::ComputeCrc32c("123456789"));
}

TEST_CASE("crc32c: binary data supports embedded zeroes", "[crc]") {
    constexpr std::uint8_t bytes[] = {0, 1, 2, 3, 0xFF};
    REQUIRE(zeta::ComputeCrc32c(bytes, sizeof(bytes)) == 0x4EC27548U);
}
