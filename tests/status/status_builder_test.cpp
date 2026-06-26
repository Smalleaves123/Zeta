#include "zeta/status/status_builder.h"
#include "zeta/status/status_macros.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

zeta::Status AlwaysFail() {
    return zeta::NotFoundError("config");
}

zeta::StatusOr<int> ParseNumber(bool ok) {
    if (!ok) return zeta::InvalidArgumentError("bad number");
    return 7;
}

zeta::Status MacroReturnIfErrorPlain(bool ok) {
    if (ok) return zeta::OkStatus();
    ZETA_RETURN_IF_ERROR(AlwaysFail());
    return zeta::OkStatus();
}

zeta::Status MacroReturnIfErrorAnnotated(bool ok) {
    if (ok) return zeta::OkStatus();
    ZETA_RETURN_IF_ERROR(AlwaysFail()) << "while loading startup settings";
    return zeta::OkStatus();
}

zeta::StatusOr<int> MacroAssignOrReturn(bool ok) {
    ZETA_ASSIGN_OR_RETURN(auto value, ParseNumber(ok));
    return value + 1;
}

zeta::StatusOr<std::string> MacroAssignOrReturnToExisting(bool ok) {
    std::string text = "prefix";
    ZETA_ASSIGN_OR_RETURN(auto value, ParseNumber(ok));
    text += std::to_string(value);
    return text;
}

} // namespace

TEST_CASE("Status predicates identify codes", "[status][builder]") {
    REQUIRE(zeta::IsNotFound(zeta::NotFoundError("x")));
    REQUIRE(zeta::IsInvalidArgument(zeta::InvalidArgumentError("x")));
    REQUIRE(zeta::IsUnavailable(zeta::UnavailableError("x")));
    REQUIRE(!zeta::IsNotFound(zeta::InternalError("x")));
}

TEST_CASE("AppendMessage appends context", "[status][builder]") {
    auto status = zeta::AppendMessage(zeta::NotFoundError("config"), "while loading");
    REQUIRE(status.code() == zeta::StatusCode::kNotFound);
    REQUIRE(status.message() == "config: while loading");
}

TEST_CASE("AppendMessage is no-op on OK", "[status][builder]") {
    auto status = zeta::AppendMessage(zeta::OkStatus(), "ignored");
    REQUIRE(status.ok());
}

TEST_CASE("PrependMessage prepends context", "[status][builder]") {
    auto status = zeta::PrependMessage(zeta::NotFoundError("config"), "startup");
    REQUIRE(status.message() == "startup: config");
}

TEST_CASE("Annotate aliases append semantics", "[status][builder]") {
    auto status = zeta::Annotate(zeta::InternalError("write failed"), "db");
    REQUIRE(status.message() == "write failed: db");
}

TEST_CASE("StatusBuilder appends streamed text by default", "[status][builder]") {
    zeta::Status status =
        zeta::StatusBuilder(zeta::NotFoundError("config")) << "while loading " << 7;
    REQUIRE(status.code() == zeta::StatusCode::kNotFound);
    REQUIRE(status.message() == "config: while loading 7");
}

TEST_CASE("StatusBuilder can prepend", "[status][builder]") {
    zeta::Status status = zeta::StatusBuilder(zeta::NotFoundError("config"))
                              .SetPrepend()
                          << "startup";
    REQUIRE(status.message() == "startup: config");
}

TEST_CASE("StatusBuilder can customize separator", "[status][builder]") {
    zeta::Status status = zeta::StatusBuilder(zeta::NotFoundError("config"))
                              .SetSeparator(" | ")
                          << "startup";
    REQUIRE(status.message() == "config | startup");
}

TEST_CASE("StatusBuilder leaves OK untouched", "[status][builder]") {
    zeta::Status status = zeta::StatusBuilder(zeta::OkStatus()) << "ignored";
    REQUIRE(status.ok());
}

TEST_CASE("ZETA_RETURN_IF_ERROR returns plain status", "[status][macros]") {
    REQUIRE(MacroReturnIfErrorPlain(true).ok());
    auto failed = MacroReturnIfErrorPlain(false);
    REQUIRE(zeta::IsNotFound(failed));
    REQUIRE(failed.message() == "config");
}

TEST_CASE("ZETA_RETURN_IF_ERROR returns annotated status", "[status][macros]") {
    auto failed = MacroReturnIfErrorAnnotated(false);
    REQUIRE(zeta::IsNotFound(failed));
    REQUIRE(failed.message() == "config: while loading startup settings");
}

TEST_CASE("ZETA_ASSIGN_OR_RETURN supports declaration", "[status][macros]") {
    auto result = MacroAssignOrReturn(true);
    REQUIRE(result.ok());
    REQUIRE(*result == 8);
}

TEST_CASE("ZETA_ASSIGN_OR_RETURN propagates error", "[status][macros]") {
    auto result = MacroAssignOrReturn(false);
    REQUIRE(!result.ok());
    REQUIRE(zeta::IsInvalidArgument(result.status()));
}

TEST_CASE("ZETA_ASSIGN_OR_RETURN supports local declarations", "[status][macros]") {
    auto result = MacroAssignOrReturnToExisting(true);
    REQUIRE(result.ok());
    REQUIRE(*result == "prefix7");
}
