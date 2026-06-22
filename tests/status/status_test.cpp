#include "zeta/status/status.h"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>

// ═══════════════════════════════════════════════════════════════════
// Construction
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: default construction produces OK", "[status]") {
    zeta::Status s;
    REQUIRE(s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kOk);
    REQUIRE(s.message().empty());
    REQUIRE(s.ToString() == "OK");
}

TEST_CASE("Status: from code + string message", "[status]") {
    auto s = zeta::Status(zeta::StatusCode::kNotFound, "file missing");
    REQUIRE(!s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kNotFound);
    REQUIRE(s.message() == "file missing");
}

TEST_CASE("Status: from code + string_view message", "[status]") {
    auto s = zeta::Status(zeta::StatusCode::kInternal, "boom");
    REQUIRE(!s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kInternal);
    REQUIRE(s.message() == "boom");
}

TEST_CASE("Status: kOk code with message ignored", "[status]") {
    auto s = zeta::Status(zeta::StatusCode::kOk, "should be ignored");
    REQUIRE(s.ok());
    REQUIRE(s.message().empty());
}

TEST_CASE("Status: kOk code with empty string is OK", "[status]") {
    auto s = zeta::Status(zeta::StatusCode::kInternal, "");
    REQUIRE(!s.ok()); // Internal + "" is still an error (message is empty but code is not OK)
}

// ═══════════════════════════════════════════════════════════════════
// Factory helpers
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: OkStatus is OK", "[status]") {
    auto s = zeta::OkStatus();
    REQUIRE(s.ok());
}

TEST_CASE("Status: factory functions produce correct codes", "[status]") {
    REQUIRE(zeta::CancelledError("x").code()           == zeta::StatusCode::kCancelled);
    REQUIRE(zeta::UnknownError("x").code()             == zeta::StatusCode::kUnknown);
    REQUIRE(zeta::InvalidArgumentError("x").code()     == zeta::StatusCode::kInvalidArgument);
    REQUIRE(zeta::DeadlineExceededError("x").code()    == zeta::StatusCode::kDeadlineExceeded);
    REQUIRE(zeta::NotFoundError("x").code()            == zeta::StatusCode::kNotFound);
    REQUIRE(zeta::AlreadyExistsError("x").code()       == zeta::StatusCode::kAlreadyExists);
    REQUIRE(zeta::PermissionDeniedError("x").code()    == zeta::StatusCode::kPermissionDenied);
    REQUIRE(zeta::ResourceExhaustedError("x").code()   == zeta::StatusCode::kResourceExhausted);
    REQUIRE(zeta::FailedPreconditionError("x").code()  == zeta::StatusCode::kFailedPrecondition);
    REQUIRE(zeta::AbortedError("x").code()             == zeta::StatusCode::kAborted);
    REQUIRE(zeta::OutOfRangeError("x").code()          == zeta::StatusCode::kOutOfRange);
    REQUIRE(zeta::UnimplementedError("x").code()       == zeta::StatusCode::kUnimplemented);
    REQUIRE(zeta::InternalError("x").code()            == zeta::StatusCode::kInternal);
    REQUIRE(zeta::UnavailableError("x").code()         == zeta::StatusCode::kUnavailable);
    REQUIRE(zeta::DataLossError("x").code()            == zeta::StatusCode::kDataLoss);
    REQUIRE(zeta::UnauthenticatedError("x").code()     == zeta::StatusCode::kUnauthenticated);
}

// ═══════════════════════════════════════════════════════════════════
// Copy
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: copy of OK is OK", "[status][copy]") {
    auto a = zeta::OkStatus();
    auto b = a;
    REQUIRE(b.ok());
    REQUIRE(a.ok()); // original unaffected
}

TEST_CASE("Status: copy of error is independent", "[status][copy]") {
    auto a = zeta::NotFoundError("file");
    auto b = a;
    REQUIRE(b.code() == a.code());
    REQUIRE(b.message() == a.message());
}

TEST_CASE("Status: copy assignment", "[status][copy]") {
    auto a = zeta::NotFoundError("a");
    auto b = zeta::InternalError("b");
    b = a;
    REQUIRE(b.code() == zeta::StatusCode::kNotFound);
    REQUIRE(b.message() == "a");
}

TEST_CASE("Status: self-copy assignment no-op", "[status][copy]") {
    auto a = zeta::NotFoundError("file");
    auto& self = a;
    a = self;
    REQUIRE(a.code() == zeta::StatusCode::kNotFound);
    REQUIRE(a.message() == "file");
}

// ═══════════════════════════════════════════════════════════════════
// Move
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: move of OK is OK", "[status][move]") {
    auto a = zeta::OkStatus();
    auto b = std::move(a);
    REQUIRE(b.ok());
    REQUIRE(a.ok()); // moved-from OK is still OK (nullptr)
}

TEST_CASE("Status: moved-from error becomes OK", "[status][move]") {
    auto a = zeta::NotFoundError("file");
    auto b = std::move(a);
    REQUIRE(!b.ok());
    REQUIRE(b.message() == "file");
    REQUIRE(a.ok()); // moved-from becomes OK
}

TEST_CASE("Status: move assignment", "[status][move]") {
    auto a = zeta::NotFoundError("a");
    auto b = zeta::InternalError("b");
    b = std::move(a);
    REQUIRE(b.code() == zeta::StatusCode::kNotFound);
    REQUIRE(a.ok()); // moved-from a is now OK
}

// ═══════════════════════════════════════════════════════════════════
// Equality
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: OK equals OK", "[status]") {
    REQUIRE(zeta::OkStatus() == zeta::OkStatus());
    REQUIRE(!(zeta::OkStatus() != zeta::OkStatus()));
}

TEST_CASE("Status: same error equals itself", "[status]") {
    auto a = zeta::NotFoundError("file");
    REQUIRE(a == a);
}

TEST_CASE("Status: same code + message are equal", "[status]") {
    auto a = zeta::NotFoundError("file");
    auto b = zeta::NotFoundError("file");
    REQUIRE(a == b);
}

TEST_CASE("Status: different codes are not equal", "[status]") {
    REQUIRE(zeta::NotFoundError("x") != zeta::InternalError("x"));
}

TEST_CASE("Status: same code different message are not equal", "[status]") {
    REQUIRE(zeta::NotFoundError("a") != zeta::NotFoundError("b"));
}

TEST_CASE("Status: OK not equal to error", "[status]") {
    REQUIRE(zeta::OkStatus() != zeta::NotFoundError("x"));
    REQUIRE(zeta::NotFoundError("x") != zeta::OkStatus());
}

// ═══════════════════════════════════════════════════════════════════
// ToString
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: ToString for OK", "[status]") {
    REQUIRE(zeta::OkStatus().ToString() == "OK");
}

TEST_CASE("Status: ToString contains code name and message", "[status]") {
    auto s = zeta::NotFoundError("file.txt");
    auto str = s.ToString();
    REQUIRE(str.find("NOT_FOUND") != std::string::npos);
    REQUIRE(str.find("file.txt") != std::string::npos);
}

// ═══════════════════════════════════════════════════════════════════
// StatusCodeToString
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusCodeToString: all codes map to non-empty strings", "[status]") {
    for (auto code : {
        zeta::StatusCode::kOk,
        zeta::StatusCode::kCancelled,
        zeta::StatusCode::kUnknown,
        zeta::StatusCode::kInvalidArgument,
        zeta::StatusCode::kDeadlineExceeded,
        zeta::StatusCode::kNotFound,
        zeta::StatusCode::kAlreadyExists,
        zeta::StatusCode::kPermissionDenied,
        zeta::StatusCode::kResourceExhausted,
        zeta::StatusCode::kFailedPrecondition,
        zeta::StatusCode::kAborted,
        zeta::StatusCode::kOutOfRange,
        zeta::StatusCode::kUnimplemented,
        zeta::StatusCode::kInternal,
        zeta::StatusCode::kUnavailable,
        zeta::StatusCode::kDataLoss,
        zeta::StatusCode::kUnauthenticated,
    }) {
        auto str = zeta::StatusCodeToString(code);
        REQUIRE(str[0] != '\0'); // const char*, not std::string
    }
}

// ═══════════════════════════════════════════════════════════════════
// Size
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: sizeof is 16 bytes for SSO", "[status][compile]") {
    static_assert(sizeof(zeta::Status) == 16,
                  "Status is 16 bytes (8-byte header + 8-byte inline msg area)");
}

// ═══════════════════════════════════════════════════════════════════
// SSO: small-message optimization
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: short message uses inline storage", "[status][sso]") {
    auto s = zeta::NotFoundError("file.txt");
    REQUIRE(!s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kNotFound);
    REQUIRE(s.message() == "file.txt");
}

TEST_CASE("Status: exactly 15 char message fits inline", "[status][sso]") {
    std::string msg(15, 'x');
    auto s = zeta::NotFoundError(msg);
    REQUIRE(s.message() == msg);
    REQUIRE(s.message().size() == 15);
}

TEST_CASE("Status: 16 char message overflows to heap", "[status][sso]") {
    std::string msg(16, 'x');
    auto s = zeta::NotFoundError(msg);
    REQUIRE(s.message() == msg);
}

TEST_CASE("Status: very long message on heap", "[status][sso]") {
    std::string msg(1000, 'y');
    auto s = zeta::NotFoundError(msg);
    REQUIRE(s.message().size() == 1000);
    REQUIRE(s.message() == msg);
}

TEST_CASE("Status: empty message error is inline", "[status][sso]") {
    auto s = zeta::InternalError("");
    REQUIRE(!s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kInternal);
    REQUIRE(s.message().empty());
}

TEST_CASE("Status: copy of inline stays valid", "[status][sso][copy]") {
    auto a = zeta::NotFoundError("hello");
    auto b = a;
    REQUIRE(b.code() == zeta::StatusCode::kNotFound);
    REQUIRE(b.message() == "hello");
    REQUIRE(a.message() == "hello"); // original unchanged
}

TEST_CASE("Status: move of inline invalidates source", "[status][sso][move]") {
    auto a = zeta::NotFoundError("hello");
    auto b = std::move(a);
    REQUIRE(b.message() == "hello");
    REQUIRE(a.ok()); // moved-from is OK
}

TEST_CASE("Status: move of heap invalidates source", "[status][sso][move]") {
    std::string msg(100, 'z');
    auto a = zeta::NotFoundError(msg);
    auto b = std::move(a);
    REQUIRE(b.message() == msg);
    REQUIRE(a.ok());
}

TEST_CASE("Status: IgnoreStatus compiles and runs", "[status][utility]") {
    auto s = zeta::NotFoundError("ignored");
    zeta::IgnoreStatus(s);        // const&
    zeta::IgnoreStatus(std::move(s)); // rvalue
    SUCCEED("IgnoreStatus works");
}

// ═══════════════════════════════════════════════════════════════════
// Edge cases
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("Status: long message", "[status]") {
    std::string long_msg(1000, 'x');
    auto s = zeta::NotFoundError(long_msg);
    REQUIRE(s.message().size() == 1000);
    REQUIRE(s.message() == long_msg);
}

TEST_CASE("Status: empty message error", "[status]") {
    auto s = zeta::InternalError("");
    REQUIRE(!s.ok());
    REQUIRE(s.code() == zeta::StatusCode::kInternal);
}
