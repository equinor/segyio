cmake_minimum_required(VERSION 3.18)
project(catch2 CXX)

add_library(catch2 INTERFACE)
target_include_directories(catch2 SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
