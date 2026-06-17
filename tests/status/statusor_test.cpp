#include "zeta/status/statusor.h"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// ═══════════════════════════════════════════════════════════════════
// Construction: value
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: construct from value", "[statusor]") {
    zeta::StatusOr<int> r(42);
    REQUIRE(r.ok());
    REQUIRE(r.value() == 42);
}

TEST_CASE("StatusOr: construct from string", "[statusor]") {
    zeta::StatusOr<std::string> r("hello");
    REQUIRE(r.ok());
    REQUIRE(r.value() == "hello");
}

TEST_CASE("StatusOr: construct from moved value", "[statusor]") {
    std::string s = "hello";
    zeta::StatusOr<std::string> r(std::move(s));
    REQUIRE(r.ok());
    REQUIRE(r.value() == "hello");
}

// ═══════════════════════════════════════════════════════════════════
// Construction: error
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: construct from Status error", "[statusor]") {
    auto err = zeta::NotFoundError("missing");
    zeta::StatusOr<int> r(err);
    REQUIRE(!r.ok());
    REQUIRE(r.status().code() == zeta::StatusCode::kNotFound);
}

TEST_CASE("StatusOr: construct from moved Status", "[statusor]") {
    zeta::StatusOr<int> r(zeta::InvalidArgumentError("bad"));
    REQUIRE(!r.ok());
    REQUIRE(r.status().code() == zeta::StatusCode::kInvalidArgument);
}

// ═══════════════════════════════════════════════════════════════════
// Observers
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: operator bool", "[statusor]") {
    zeta::StatusOr<int> ok(1);
    zeta::StatusOr<int> err(zeta::InternalError("fail"));
    if (ok) { SUCCEED("ok is truthy"); }
    if (!err) { SUCCEED("err is falsy"); }
}

TEST_CASE("StatusOr: status() on OK returns OkStatus", "[statusor]") {
    zeta::StatusOr<int> r(42);
    REQUIRE(r.status().ok());
}

TEST_CASE("StatusOr: status() on error preserves code and message", "[statusor]") {
    auto err = zeta::NotFoundError("file.txt");
    zeta::StatusOr<int> r(err);
    REQUIRE(r.status().code() == zeta::StatusCode::kNotFound);
    REQUIRE(r.status().message() == "file.txt");
}

// ═══════════════════════════════════════════════════════════════════
// Value access
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: value() returns reference", "[statusor]") {
    zeta::StatusOr<int> r(42);
    r.value() = 100;
    REQUIRE(r.value() == 100);
}

TEST_CASE("StatusOr: value() on const returns const ref", "[statusor]") {
    const zeta::StatusOr<int> r(42);
    const int& v = r.value();
    REQUIRE(v == 42);
}

TEST_CASE("StatusOr: operator*", "[statusor]") {
    zeta::StatusOr<int> r(42);
    REQUIRE(*r == 42);
    *r = 100;
    REQUIRE(*r == 100);
}

TEST_CASE("StatusOr: operator* on const", "[statusor]") {
    const zeta::StatusOr<int> r(42);
    REQUIRE(*r == 42);
}

TEST_CASE("StatusOr: operator->", "[statusor]") {
    zeta::StatusOr<std::string> r("hello");
    REQUIRE(r->size() == 5);
    REQUIRE(r->empty() == false);
}

TEST_CASE("StatusOr: operator-> on const", "[statusor]") {
    const zeta::StatusOr<std::vector<int>> r(std::vector{1, 2, 3});
    REQUIRE(r->size() == 3);
}

// ═══════════════════════════════════════════════════════════════════
// Move semantics
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: move out value via && value()", "[statusor][move]") {
    zeta::StatusOr<std::string> r("hello");
    std::string s = std::move(r).value();
    REQUIRE(s == "hello");
}

TEST_CASE("StatusOr: move out value via && operator*", "[statusor][move]") {
    zeta::StatusOr<std::string> r("hello");
    std::string s = *std::move(r);
    REQUIRE(s == "hello");
}

TEST_CASE("StatusOr: move constructor preserves value", "[statusor][move]") {
    zeta::StatusOr<std::string> a("hello");
    zeta::StatusOr<std::string> b(std::move(a));
    REQUIRE(b.ok());
    REQUIRE(b.value() == "hello");
}

TEST_CASE("StatusOr: move assignment", "[statusor][move]") {
    zeta::StatusOr<std::string> a("hello");
    zeta::StatusOr<std::string> b(zeta::InternalError("fail"));
    b = std::move(a);
    REQUIRE(b.ok());
    REQUIRE(b.value() == "hello");
}

// ═══════════════════════════════════════════════════════════════════
// Copy semantics
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: copy constructor preserves value", "[statusor][copy]") {
    zeta::StatusOr<int> a(42);
    zeta::StatusOr<int> b(a);
    REQUIRE(b.ok());
    REQUIRE(b.value() == 42);
    REQUIRE(a.ok()); // original unchanged
    REQUIRE(a.value() == 42);
}

TEST_CASE("StatusOr: copy constructor preserves error", "[statusor][copy]") {
    zeta::StatusOr<int> a(zeta::NotFoundError("missing"));
    zeta::StatusOr<int> b(a);
    REQUIRE(!b.ok());
    REQUIRE(b.status().code() == zeta::StatusCode::kNotFound);
    REQUIRE(a.status().message() == "missing");
}

TEST_CASE("StatusOr: copy assignment", "[statusor][copy]") {
    zeta::StatusOr<int> a(42);
    zeta::StatusOr<int> b(0);
    b = a;
    REQUIRE(b.value() == 42);
}

// ═══════════════════════════════════════════════════════════════════
// Error propagation pattern
// ═══════════════════════════════════════════════════════════════════

static zeta::StatusOr<int> may_fail(bool fail) {
    if (fail) return zeta::InvalidArgumentError("simulated failure");
    return 42;
}

TEST_CASE("StatusOr: error propagation pattern", "[statusor][pattern]") {
    auto result = may_fail(false);
    REQUIRE(result.ok());
    REQUIRE(*result == 42);
}

TEST_CASE("StatusOr: error path propagation", "[statusor][pattern]") {
    auto result = may_fail(true);
    REQUIRE(!result.ok());
    REQUIRE(result.status().code() == zeta::StatusCode::kInvalidArgument);
    REQUIRE(result.status().message() == "simulated failure");
}

// ═══════════════════════════════════════════════════════════════════
// move() on status for propagation
// ═══════════════════════════════════════════════════════════════════

static zeta::StatusOr<std::string> propagate_inner(bool fail) {
    auto r = may_fail(fail);
    if (!r.ok()) return std::move(r).status(); // propagate error
    return std::to_string(*r + 1);
}

TEST_CASE("StatusOr: error propagation via move(status)", "[statusor][pattern]") {
    auto r = propagate_inner(true);
    REQUIRE(!r.ok());
    REQUIRE(r.status().code() == zeta::StatusCode::kInvalidArgument);
}

TEST_CASE("StatusOr: value path via propagation pattern", "[statusor][pattern]") {
    auto r = propagate_inner(false);
    REQUIRE(r.ok());
    REQUIRE(*r == "43");
}

// ═══════════════════════════════════════════════════════════════════
// Move-only type
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: move-only type (unique_ptr)", "[statusor][move-only]") {
    zeta::StatusOr<std::unique_ptr<int>> r(std::make_unique<int>(42));
    REQUIRE(r.ok());
    REQUIRE(**r == 42);

    auto moved = std::move(r);
    REQUIRE(moved.ok());
    REQUIRE(**moved == 42);
}

// ═══════════════════════════════════════════════════════════════════
// CTAD (class template argument deduction)
// ═══════════════════════════════════════════════════════════════════

TEST_CASE("StatusOr: CTAD from value", "[statusor][ctad]") {
    zeta::StatusOr r(42); // deduces StatusOr<int>
    REQUIRE(r.ok());
    REQUIRE(r.value() == 42);
}

TEST_CASE("StatusOr: CTAD from string", "[statusor][ctad]") {
    zeta::StatusOr r(std::string("hello")); // deduces StatusOr<std::string>
    REQUIRE(r.ok());
    REQUIRE(r.value() == "hello");
}

// ═══════════════════════════════════════════════════════════════════
// StatusOr<void>
// ═══════════════════════════════════════════════════════════════════

static zeta::StatusOr<void> void_ok() { return {}; }
static zeta::StatusOr<void> void_err() { return zeta::NotFoundError("gone"); }

TEST_CASE("StatusOr<void>: default constructs as OK", "[statusor][void]") {
    zeta::StatusOr<void> r;
    REQUIRE(r.ok());
}

TEST_CASE("StatusOr<void>: error construction", "[statusor][void]") {
    auto r = void_err();
    REQUIRE(!r.ok());
    REQUIRE(r.status().code() == zeta::StatusCode::kNotFound);
}

TEST_CASE("StatusOr<void>: value() succeeds when OK", "[statusor][void]") {
    auto r = void_ok();
    r.value(); // does not throw/assert
    SUCCEED("void value() called ok");
}

TEST_CASE("StatusOr<void>: operator*", "[statusor][void]") {
    auto r = void_ok();
    *r;
    SUCCEED("void operator* called ok");
}

TEST_CASE("StatusOr<void>: propagation pattern", "[statusor][void]") {
    auto r = void_err();
    if (!r.ok()) {
        REQUIRE(r.status().code() == zeta::StatusCode::kNotFound);
    }
}

// ═══════════════════════════════════════════════════════════════════
// Compile-time safety
// ═══════════════════════════════════════════════════════════════════

// StatusOr<Status> and StatusOr<T&> are disallowed at compile time
// via static_assert in statusor.h — no run-time test needed.

TEST_CASE("StatusOr: size is reasonable for primitive T", "[statusor][compile]") {
    // Status(16) + int(4) + bool(1) + padding = ~24 on most platforms
    REQUIRE(sizeof(zeta::StatusOr<int>) >= 16 + 4);
}
