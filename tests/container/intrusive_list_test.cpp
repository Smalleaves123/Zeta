#include "zeta/container/intrusive_list.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

namespace {

struct Item {
    zeta::IntrusiveListHook hook;
    int value;
};

using ItemList = zeta::IntrusiveList<Item, &Item::hook>;

} // namespace

TEST_CASE("IntrusiveList: links existing objects without ownership",
          "[container][intrusive_list]") {
    Item first{{}, 1};
    Item second{{}, 2};
    Item third{{}, 3};
    ItemList list;

    REQUIRE(list.push_back(first));
    REQUIRE(list.push_back(second));
    REQUIRE(list.push_front(third));
    REQUIRE_FALSE(list.push_back(first));
    REQUIRE(list.size() == 3);
    REQUIRE(list.front().value == 3);
    REQUIRE(list.back().value == 2);

    std::vector<int> values;
    for (const Item& item : list) values.push_back(item.value);
    REQUIRE(values == std::vector<int>{3, 1, 2});

    REQUIRE(list.erase(first));
    REQUIRE(!first.hook.IsLinked());
    REQUIRE(list.size() == 2);
    list.clear();
    REQUIRE(list.empty());
    REQUIRE(!second.hook.IsLinked());
    REQUIRE(!third.hook.IsLinked());
}
