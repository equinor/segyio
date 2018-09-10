cmake_minimum_required(VERSION 2.8.12)
project(catch2 CXX)

# Dummy source file added because INTERFACE type
# library is not available in CMake 2.8.12
# it's STATIC, because MSVC would otherwise not generate a .lib file, making
# "linking" (for header path) fail later
#
# TODO: when cmake minimum version is bumped to 3.x series, replace with
# an INTERFACE library
add_library(catch2 STATIC dummy.cpp)
target_include_directories(catch2 SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
