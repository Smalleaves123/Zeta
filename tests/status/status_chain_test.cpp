#include "zeta/status/status_chain.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("StatusChain: adds context without changing the code", "[status][chain]") {
    auto chain = zeta::ChainStatus(zeta::InternalError("write failed"));
    chain.AddContext("saving profile").AddContext("request=42");

    REQUIRE(chain.status().code() == zeta::StatusCode::kInternal);
    REQUIRE(chain.status().message() == "write failed: saving profile: request=42");
}

TEST_CASE("StatusChain: preserves causes and matches nested codes", "[status][chain]") {
    auto chain = zeta::ChainStatus(zeta::UnavailableError("service unavailable"));
    chain.CausedBy(zeta::DeadlineExceededError("database timeout"));
    chain.CausedBy(zeta::NotFoundError("replica metadata"));

    REQUIRE(chain.causes().size() == 2);
    REQUIRE(chain.ContainsCode(zeta::StatusCode::kUnavailable));
    REQUIRE(chain.ContainsCode(zeta::StatusCode::kDeadlineExceeded));
    REQUIRE(chain.ContainsCode(zeta::StatusCode::kNotFound));
    REQUIRE(!chain.ContainsCode(zeta::StatusCode::kPermissionDenied));
}

TEST_CASE("StatusChain: flattens for StatusOr propagation", "[status][chain]") {
    auto chain = zeta::ChainStatus(zeta::InternalError("request failed"))
                     .AddContext("loading settings")
                     .CausedBy(zeta::InvalidArgumentError("bad port"));
    const zeta::Status flattened = chain.ToStatus();

    REQUIRE(flattened.code() == zeta::StatusCode::kInternal);
    REQUIRE(flattened.message().find("loading settings") != std::string::npos);
    REQUIRE(flattened.message().find("INVALID_ARGUMENT: bad port") !=
            std::string::npos);
}

TEST_CASE("StatusChain: OK chains remain OK", "[status][chain]") {
    auto chain = zeta::ChainStatus(zeta::OkStatus())
                     .AddContext("ignored")
                     .CausedBy(zeta::NotFoundError("ignored cause"));
    REQUIRE(chain.ok());
    REQUIRE(chain.ToStatus().ok());
    REQUIRE(chain.ToString() == "OK");
}
