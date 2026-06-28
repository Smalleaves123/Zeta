#include "flag_header_fixture.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("flags header fixture: mutable state is shared", "[flags][header]") {
    FLAGS_header_port.Set(9000);
    REQUIRE(FLAGS_header_port.Get() == 9000);
    FLAGS_header_port.Set(4321);
}
