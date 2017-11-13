cmake_minimum_required(VERSION 2.8.12)
project(segyio)

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CTest)
include(GNUInstallDirs)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git)
    find_program(git-bin git)
    execute_process(COMMAND ${git-bin} describe --tags
                    OUTPUT_VARIABLE git-describe
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE describe-failure
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    )
    if (NOT describe-failure)
        message(STATUS "Found version ${git-describe} from git")
    else ()
        message(STATUS "No version from git - falling back to 0.0.0")
        set(git-describe 0.0.0)
    endif ()

    string(REGEX REPLACE "^v" "" ver-describe "${git-describe}")
    unset(git-bin)
elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/python/segyio/version.py)
    # tarball checkout - the version file should've been written when the
    # tarball was built
    file(READ python/segyio/version.py ver-describe)
    string(REGEX REPLACE "^version =" "" ver-describe ${ver-describe})
    string(REGEX REPLACE "'" "" ver-describe ${ver-describe})
    string(STRIP ${ver-describe} ver-describe)
    message(STATUS "Found version ${ver-describe} from segyio/version.py")
else ()
    set(ver-describe 0.0.0)
    message(STATUS "Could not find version, guessing ${ver-describe}")
endif ()

string(REPLACE . ";" version-list ${ver-describe})
list(GET version-list 0 segyio-major)
list(GET version-list 1 segyio-minor)
list(GET version-list 2 segyio-patch)
unset(version-list)

# versions can always be overriden with -Dsegyio_MAJOR=N
set(segyio_MAJOR ${segyio-major} CACHE STRING "Major version")
set(segyio_MINOR ${segyio-minor} CACHE STRING "Minor version")
set(segyio_PATCH ${segyio-patch} CACHE STRING "Patch")
set(segyio_VERSION ${segyio_MAJOR}.${segyio_MINOR}.${segyio_PATCH})
message(STATUS "segyio version ${segyio_VERSION}")

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

if (NOT MSVC)
    # assuming gcc-style options
    set(c99 -std=c99)
    # add warnings in debug mode
    list(APPEND warnings-c -Wall -Wextra -pedantic -Wformat-nonliteral
                           -Wcast-align -Wpointer-arith -Wmissing-declarations
                           -Wcast-qual -Wwrite-strings -Wchar-subscripts
                           -Wredundant-decls
    )
else()
    list(APPEND warnings-c /W3 /wd4996)
endif ()

set(testdata ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

add_subdirectory(lib)
# language bindings
add_subdirectory(mex)
add_subdirectory(python)

add_subdirectory(applications)
add_subdirectory(man)
