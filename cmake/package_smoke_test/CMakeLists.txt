cmake_minimum_required(VERSION 3.20)
project(zeta_package_smoke_test LANGUAGES CXX)

find_package(zeta REQUIRED CONFIG)

add_executable(zeta_package_smoke_test main.cpp)
target_link_libraries(zeta_package_smoke_test PRIVATE
    zeta::algorithm
    zeta::container
    zeta::crc
    zeta::log
    zeta::memory
    zeta::metrics
    zeta::service
    zeta::strings
    zeta::synchronization
)
