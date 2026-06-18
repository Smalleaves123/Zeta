#include "zeta/meta/type_traits.h"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <type_traits>
#include <vector>

// ═══════════════════════════════════════════════════════════════════════
// is_specialization_of
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("meta: is_specialization_of — positive cases", "[meta][specialization]") {
    STATIC_REQUIRE(zeta::is_specialization_of<std::vector<int>, std::vector>::value);
    STATIC_REQUIRE(zeta::is_specialization_of<std::map<int, std::string>, std::map>::value);
    STATIC_REQUIRE(zeta::is_specialization_of_v<std::vector<int>, std::vector>);
}

TEST_CASE("meta: is_specialization_of — negative cases", "[meta][specialization]") {
    STATIC_REQUIRE(!zeta::is_specialization_of<int, std::vector>::value);
    STATIC_REQUIRE(!zeta::is_specialization_of<std::string, std::vector>::value);
    STATIC_REQUIRE(!zeta::is_specialization_of_v<int, std::vector>);
    STATIC_REQUIRE(!zeta::is_specialization_of_v<std::vector<int>, std::map>);
}

TEST_CASE("meta: is_specialization_of — non-type template params rejected", "[meta][specialization]") {
    // This should not match std::array (which takes a non-type NTTP).
    STATIC_REQUIRE(!zeta::is_specialization_of_v<std::array<int, 3>, std::vector>);
}

// ═══════════════════════════════════════════════════════════════════════
// is_one_of
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("meta: is_one_of", "[meta][one_of]") {
    STATIC_REQUIRE(zeta::is_one_of_v<int, int>);
    STATIC_REQUIRE(zeta::is_one_of_v<int, float, int, double>);
    STATIC_REQUIRE(!zeta::is_one_of_v<int, float, double, char>);
    STATIC_REQUIRE(!zeta::is_one_of_v<int>);
}

// ═══════════════════════════════════════════════════════════════════════
// copy_const / copy_cv
// ═══════════════════════════════════════════════════════════════════════

TEST_CASE("meta: copy_const", "[meta][copy_const]") {
    STATIC_REQUIRE(std::is_same_v<zeta::copy_const_t<const int, double>, const double>);
    STATIC_REQUIRE(std::is_same_v<zeta::copy_const_t<int, double>, double>);
    STATIC_REQUIRE(std::is_same_v<zeta::copy_const_t<const int, int>, const int>);
}

TEST_CASE("meta: copy_cv", "[meta][copy_cv]") {
    STATIC_REQUIRE(std::is_same_v<zeta::copy_cv_t<const int, double>, const double>);
    STATIC_REQUIRE(std::is_same_v<zeta::copy_cv_t<volatile int, double>, volatile double>);
    STATIC_REQUIRE(std::is_same_v<zeta::copy_cv_t<const volatile int, double>, const volatile double>);
}

// ═══════════════════════════════════════════════════════════════════════
// is_detected
// ═══════════════════════════════════════════════════════════════════════

template <typename T>
using has_size_type = typename T::size_type;

template <typename T>
using has_nonexistent = typename T::nonexistent_type;

TEST_CASE("meta: is_detected — detection idiom", "[meta][detection]") {
    STATIC_REQUIRE(zeta::is_detected_v<has_size_type, std::vector<int>>);
    STATIC_REQUIRE(!zeta::is_detected_v<has_nonexistent, std::vector<int>>);
    STATIC_REQUIRE(!zeta::is_detected_v<has_size_type, int>);
}

TEST_CASE("meta: detected_t — type retrieval", "[meta][detection]") {
    STATIC_REQUIRE(std::is_same_v<
        zeta::detected_t<has_size_type, std::vector<int>>,
        std::vector<int>::size_type>);
}

TEST_CASE("meta: detected_or — default fallback", "[meta][detection]") {
    STATIC_REQUIRE(std::is_same_v<
        zeta::detected_or_t<void, has_nonexistent, std::vector<int>>,
        void>);
}
