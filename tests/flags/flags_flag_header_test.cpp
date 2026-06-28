#include "flag_header_fixture.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("flags header fixture: default value is visible", "[flags][header]") {
    REQUIRE(FLAGS_header_port.Get() == 4321);
    REQUIRE(FLAGS_header_port.TypeName() == "int32");
}
