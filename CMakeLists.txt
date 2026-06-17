cmake_minimum_required(VERSION 3.20)
project(zeta VERSION 0.1.0 LANGUAGES CXX)

# ── C++20 required ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Compiler flags ──────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# ── Header-only library (Abseil-style: headers live under zeta/) ─
add_library(zeta INTERFACE)
target_include_directories(zeta INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(zeta INTERFACE cxx_std_20)

# ── Subdirectories ──────────────────────────────────────────────
add_subdirectory(zeta)

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
    include(FetchContent)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.8.5
    )
    FetchContent_MakeAvailable(googlebenchmark)
    add_subdirectory(benchmarks)
endif()

# ── Install ──────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install all public headers (preserve directory structure under zeta/)
install(DIRECTORY zeta
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h")

# Generate package config files
set(ZETA_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}")
configure_package_config_file(
    cmake/zetaConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zeta"
    PATH_VARS ZETA_INCLUDE_DIR)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/zetaConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zeta")
