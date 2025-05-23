cmake_minimum_required(VERSION 3.11)
project(segyio LANGUAGES C CXX)

include(CheckFunctionExists)
include(CheckIncludeFile)
include(CTest)
include(GNUInstallDirs)
include(TestBigEndian)

if (DEFINED ENV{SEGYIO_NO_GIT_VER})
    set(SEGYIO_NO_GIT_VER CACHE BOOL "Ignore version from git" ON)
endif ()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/.git AND NOT SEGYIO_NO_GIT_VER)
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
option(BUILD_BIN         "Build applications"                       ON)
option(BUILD_PYTHON      "Build Python library"                     ON)
option(REQUIRE_PYTHON    "Fail cmake if python cannot be built"     OFF)
option(BUILD_MEX         "Build Matlab mex files"                   OFF)
option(BUILD_DOC         "Build documentation"                      OFF)
option(EXPERIMENTAL      "Enable experimental features"             OFF)

check_include_file(getopt.h         HAVE_GETOPT_H)
check_include_file(sys/mman.h       HAVE_SYS_MMAN_H)
check_include_file(sys/stat.h       HAVE_SYS_STAT_H)
check_function_exists(getopt_long   HAVE_GETOPT_LONG)

if (HAVE_SYS_MMAN_H)
    list(APPEND mmap -DHAVE_MMAP)
endif()

if (HAVE_SYS_STAT_H)
    list(APPEND fstat -DHAVE_SYS_STAT_H)

    check_function_exists(_fstati64 HAVE_FSTATI64)
    if (HAVE_FSTATI64)
        list(APPEND fstat -DHAVE_FSTATI64)
    endif ()
else()
    message(FATAL_ERROR "Could not find sys/stat.h")
endif()

check_function_exists(_ftelli64 HAVE_FTELLI64)
check_function_exists(_fseeki64 HAVE_FSEEKI64)
check_function_exists(ftello HAVE_FTELLO)
check_function_exists(fseeko HAVE_FSEEKO)

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

test_big_endian(HOST_BIG_ENDIAN)

# add documentation master target for other documentation sub targets to hook
# into
add_custom_target(doc ALL)

set(testdata ${CMAKE_CURRENT_SOURCE_DIR}/test-data)

add_subdirectory(external/catch2)
add_subdirectory(lib)
# language bindings
add_subdirectory(mex)
add_subdirectory(python)

add_subdirectory(applications)
add_subdirectory(man)
