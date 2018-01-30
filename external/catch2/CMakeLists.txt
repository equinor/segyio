cmake_minimum_required(VERSION 2.8.12)
project(catch2 CXX)

# Dummy source file added because INTERFACE type
# library is not available in CMake 2.8.12
add_library(catch2 dummy.cpp)
target_include_directories(catch2 SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
