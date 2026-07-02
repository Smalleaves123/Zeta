cmake_minimum_required(VERSION 3.20)
project(zeta VERSION 0.1.0 LANGUAGES CXX)

option(ZETA_ENABLE_WARNINGS_AS_ERRORS
       "Treat compiler warnings as errors for Zeta builds"
       OFF)

# ── C++20 required ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ──────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(ZETA_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
    if(ZETA_ENABLE_WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
endif()

# ── Header-only library (Abseil-style: headers live under zeta/) ─
add_library(zeta INTERFACE)
add_library(zeta::zeta ALIAS zeta)
add_custom_target(zeta_headers)
target_include_directories(zeta INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(zeta INTERFACE cxx_std_20)

# ── Subdirectories ──────────────────────────────────────────────
add_subdirectory(zeta)
get_property(ZETA_EXPORT_TARGETS GLOBAL PROPERTY ZETA_EXPORT_TARGETS)

option(ZETA_BUILD_TESTS "Build test suite" ON)
if(ZETA_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)
    add_subdirectory(tests)
endif()

option(ZETA_BUILD_BENCHMARKS "Build benchmarks" OFF)
if(ZETA_BUILD_BENCHMARKS)
    # We only need benchmark::benchmark for our own targets.
    # Disable the library's bundled gtest path so CMake does not re-enter a
    # nested configure step that is sensitive to policy-version defaults.
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.5
    )
    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()

option(ZETA_BUILD_EXAMPLES "Build example programs" OFF)
if(ZETA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

option(ZETA_BUILD_FUZZERS "Build fuzz harness targets" OFF)
set(ZETA_FUZZ_COMPILE_OPTIONS -fsanitize=address,undefined -fno-omit-frame-pointer)
set(ZETA_FUZZ_LINK_OPTIONS -fsanitize=address,undefined)
if(ZETA_BUILD_FUZZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        message(FATAL_ERROR "Fuzz targets require a Clang- or GCC-compatible compiler")
    endif()
    add_subdirectory(fuzz)
endif()

# ── Install ──────────────────────────────────────────────────────
include(GNUInstallDirs)
# Install all public headers (preserve directory structure under zeta/)
install(DIRECTORY zeta
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h")

# Generate package config files
include(CMakePackageConfigHelpers)
configure_file(
    cmake/zetaConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfig.cmake"
    @ONLY)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(TARGETS zeta ${ZETA_EXPORT_TARGETS} EXPORT zetaTargets)
install(EXPORT zetaTargets
    NAMESPACE zeta::
    FILE zetaTargets.cmake
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zeta")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zeta")
