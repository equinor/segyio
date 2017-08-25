cmake_minimum_required(VERSION 2.8.11)
project(segyio)

include(GNUInstallDirs)

set(segyio_MAJOR 1 CACHE STRING "")
set(segyio_MINOR 3 CACHE STRING "")
set(segyio_PATCH 2 CACHE STRING "")
set(segyio_VERSION ${segyio_MAJOR}.${segyio_MINOR}.${segyio_PATCH})

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

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pycmake)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(check_includes)
include(default_warnings)
include(segyio_testing)
enable_testing()

set(testdata ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

add_subdirectory(lib)
# language bindings
add_subdirectory(mex)
add_subdirectory(python)

add_subdirectory(applications)
add_subdirectory(man)
