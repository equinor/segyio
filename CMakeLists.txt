cmake_minimum_required(VERSION 2.8.11)
project(segyio)

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

option(BUILD_SHARED_LIBS "Build language bindings shared" OFF)
option(BUILD_PYTHON "Build Python wrappers"     ON)
option(BUILD_MEX    "Build Matlab mex files"    OFF)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MAC_OS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(check_includes)
include(default_warnings)
include(segyio_testing)
enable_testing()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install"
        CACHE PATH "Default install path" FORCE)
endif ()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release"
        CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(testdata ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

add_subdirectory(lib)
# language bindings
add_subdirectory(mex)
add_subdirectory(python)

add_subdirectory(applications)
