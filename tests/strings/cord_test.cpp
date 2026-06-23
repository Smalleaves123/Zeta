#include "zeta/strings/cord.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

TEST_CASE("Cord: default construction", "[cord]") {
    zeta::Cord cord;
    REQUIRE(cord.empty());
    REQUIRE(cord.size() == 0);
    REQUIRE(cord.chunk_count() == 0);
    REQUIRE(cord.Flatten().empty());
}

TEST_CASE("Cord: append string_view chunks", "[cord]") {
    zeta::Cord cord;
    cord.Append("hello");
    cord.Append(", ");
    cord.Append("world");

    REQUIRE(cord.size() == 12);
    REQUIRE(cord.chunk_count() == 3);
    REQUIRE(cord.Flatten() == "hello, world");
}

TEST_CASE("Cord: append moved string", "[cord]") {
    std::string payload = "payload";
    zeta::Cord cord;
    cord.Append(std::move(payload));

    REQUIRE(cord.size() == 7);
    REQUIRE(cord.chunk_count() == 1);
    REQUIRE(cord.Flatten() == "payload");
}

TEST_CASE("Cord: append char", "[cord]") {
    zeta::Cord cord("ab");
    cord.Append('c');
    REQUIRE(cord.Flatten() == "abc");
}

TEST_CASE("Cord: append other cord by copy", "[cord]") {
    zeta::Cord left("alpha");
    zeta::Cord right("beta");

    left.Append(right);

    REQUIRE(left.Flatten() == "alphabeta");
    REQUIRE(right.Flatten() == "beta");
}

TEST_CASE("Cord: append other cord by move", "[cord]") {
    zeta::Cord left("alpha");
    zeta::Cord right("beta");

    left.Append(std::move(right));

    REQUIRE(left.Flatten() == "alphabeta");
    REQUIRE(right.empty());
}

TEST_CASE("Cord: AppendTo appends onto existing string", "[cord]") {
    zeta::Cord cord("hello");
    cord.Append(", ");
    cord.Append("world");

    std::string out = "prefix:";
    cord.AppendTo(&out);

    REQUIRE(out == "prefix:hello, world");
}

TEST_CASE("Cord: ForEachChunk preserves chunk boundaries", "[cord]") {
    zeta::Cord cord;
    cord.Append("ab");
    cord.Append("cd");
    cord.Append("ef");

    std::vector<std::string> chunks;
    cord.ForEachChunk([&](std::string_view chunk) {
        chunks.emplace_back(chunk);
    });

    REQUIRE(chunks == std::vector<std::string>{"ab", "cd", "ef"});
}

TEST_CASE("Cord: operator plus-equals overloads", "[cord]") {
    zeta::Cord cord("ab");
    cord += "cd";
    cord += 'e';

    zeta::Cord suffix("fg");
    cord += suffix;

    REQUIRE(cord.Flatten() == "abcdefg");
}

TEST_CASE("Cord: equality compares flattened content", "[cord]") {
    zeta::Cord a;
    a.Append("ab");
    a.Append("cd");

    zeta::Cord b("abcd");
    REQUIRE(a == b);
    REQUIRE(!(a != b));
}

TEST_CASE("Cord: RemovePrefix trims from the front", "[cord]") {
    zeta::Cord cord;
    cord.Append("alpha");
    cord.Append("beta");
    cord.Append("gamma");

    cord.RemovePrefix(7);

    REQUIRE(cord.size() == 7);
    REQUIRE(cord.Flatten() == "tagamma");
}

TEST_CASE("Cord: RemovePrefix clears when oversized", "[cord]") {
    zeta::Cord cord("hello");
    cord.RemovePrefix(99);
    REQUIRE(cord.empty());
}

TEST_CASE("Cord: RemoveSuffix trims from the back", "[cord]") {
    zeta::Cord cord;
    cord.Append("alpha");
    cord.Append("beta");
    cord.Append("gamma");

    cord.RemoveSuffix(6);

    REQUIRE(cord.size() == 8);
    REQUIRE(cord.Flatten() == "alphabet");
}

TEST_CASE("Cord: RemoveSuffix clears when oversized", "[cord]") {
    zeta::Cord cord("hello");
    cord.RemoveSuffix(99);
    REQUIRE(cord.empty());
}

TEST_CASE("Cord: prefix and suffix removal can be combined", "[cord]") {
    zeta::Cord cord("::hello world::");
    cord.RemovePrefix(2);
    cord.RemoveSuffix(2);
    REQUIRE(cord.Flatten() == "hello world");
}

TEST_CASE("Cord: Subcord extracts middle range", "[cord]") {
    zeta::Cord cord;
    cord.Append("alpha");
    cord.Append("beta");
    cord.Append("gamma");

    zeta::Cord sub = cord.Subcord(3, 6);
    REQUIRE(sub.Flatten() == "habeta");
}

TEST_CASE("Cord: Subcord to end", "[cord]") {
    zeta::Cord cord("hello world");
    REQUIRE(cord.Subcord(6).Flatten() == "world");
}

TEST_CASE("Cord: Subcord out of range is empty", "[cord]") {
    zeta::Cord cord("hello");
    REQUIRE(cord.Subcord(99).empty());
    REQUIRE(cord.Subcord(1, 0).empty());
}

TEST_CASE("Cord: StartsWith across chunk boundaries", "[cord]") {
    zeta::Cord cord;
    cord.Append("alpha");
    cord.Append("beta");
    cord.Append("gamma");

    REQUIRE(cord.StartsWith("alph"));
    REQUIRE(cord.StartsWith("alphabeta"));
    REQUIRE(cord.StartsWith("alphabetagamma"));
    REQUIRE(!cord.StartsWith("beta"));
    REQUIRE(!cord.StartsWith("alphabetagammaz"));
}

TEST_CASE("Cord: EndsWith across chunk boundaries", "[cord]") {
    zeta::Cord cord;
    cord.Append("alpha");
    cord.Append("beta");
    cord.Append("gamma");

    REQUIRE(cord.EndsWith("gamma"));
    REQUIRE(cord.EndsWith("betagamma"));
    REQUIRE(cord.EndsWith("alphabetagamma"));
    REQUIRE(!cord.EndsWith("alpha"));
    REQUIRE(!cord.EndsWith("zalphabetagamma"));
}
