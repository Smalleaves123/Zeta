cmake_minimum_required(VERSION 3.20)
project(zeta_package_smoke_test LANGUAGES CXX)

find_package(zeta REQUIRED CONFIG)

add_executable(zeta_package_smoke_test main.cpp)
target_link_libraries(zeta_package_smoke_test PRIVATE
    zeta::container
    zeta::metrics
    zeta::strings
)
