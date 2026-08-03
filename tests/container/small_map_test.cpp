#include "zeta/container/small_map.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("SmallMap: uses map-like operations across inline capacity",
          "[container][small_map]") {
    zeta::SmallMap<std::string, int, 2> values;

    values["one"] = 1;
    REQUIRE(values.try_emplace("two", 2).second);
    REQUIRE(values.try_emplace("two", 20).second == false);
    REQUIRE(values.insert_or_assign("three", 3).second);
    REQUIRE(values.size() == 3);
    REQUIRE(values.contains("one"));
    REQUIRE(values.at("three") == 3);

    REQUIRE(values.erase("two"));
    REQUIRE_FALSE(values.contains("two"));
    REQUIRE_FALSE(values.erase("missing"));
    values.clear();
    REQUIRE(values.empty());
}
