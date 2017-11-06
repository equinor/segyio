cmake_minimum_required(VERSION 2.8.11)
project(segyio)

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CTest)
include(GNUInstallDirs)


set(segyio_MAJOR 1 CACHE STRING "")
set(segyio_MINOR 3 CACHE STRING "")
set(segyio_PATCH 3 CACHE STRING "")
set(segyio_VERSION ${segyio_MAJOR}.${segyio_MINOR}.${segyio_PATCH})

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

option(BUILD_SHARED_LIBS "Build language bindings shared"           OFF)
option(BUILD_PYTHON      "Build Python library"                     ON)
option(REQUIRE_PYTHON    "Fail cmake if python cannot be built"     OFF)
option(BUILD_MEX         "Build Matlab mex files"                   OFF)

check_include_file(netinet/in.h     HAVE_NETINET_IN_H)
check_include_file(arpa/inet.h      HAVE_ARPA_INET_H)
check_include_file(winsock2.h       HAVE_WINSOCK2_H)
check_include_file(getopt.h         HAVE_GETOPT_H)
check_include_file(sys/mman.h       HAVE_SYS_MMAN_H)
check_include_file(sys/stat.h       HAVE_SYS_STAT_H)
check_function_exists(getopt_long   HAVE_GETOPT_LONG)

if (HAVE_NETINET_IN_H)
    list(APPEND htons -DHAVE_NETINET_IN_H)
elseif (HAVE_ARPA_INET_H)
    list(APPEND htons -DHAVE_ARPA_INET_H)
elseif (HAVE_WINSOCK2_H)
    set(ws2 ws2_32)
    list(APPEND htons -DHAVE_WINSOCK2_H)
else()
    message(FATAL_ERROR "Could not find htons")
endif()

if (HAVE_SYS_MMAN_H)
    list(APPEND mmap -DHAVE_MMAP)
endif()

if (HAVE_SYS_STAT_H)
    list(APPEND fstat -DHAVE_SYS_STAT_H)

    check_function_exists(_fstati64 HAVE_FSTATI64)
    if (HAVE_FSTATI64)
        list(APPEND fstat -DHAVE_FSTATI64)
    endif ()

    check_function_exists(_ftelli64 HAVE_FTELLI64)
    if (HAVE_FTELLI64)
        list(APPEND fstat -DHAVE_FTELLI64)
    endif ()
else()
    message(FATAL_ERROR "Could not find sys/stat.h (fstat/ftelli)")
endif()

check_function_exists(ftello HAVE_FTELLO)
if (HAVE_FTELLO)
    list(APPEND ftello -DHAVE_FTELLO)
endif ()

if(NOT MSVC)
    set(m m)
endif()


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(default_warnings)
include(segyio_testing)

set(testdata ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

add_subdirectory(lib)
# language bindings
add_subdirectory(mex)
add_subdirectory(python)

add_subdirectory(applications)
add_subdirectory(man)
