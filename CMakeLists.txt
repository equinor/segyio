cmake_minimum_required(VERSION 2.8.8)
project(segyio)

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MAC_OS TRUE)
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(LINUX TRUE)
    set(CMAKE_C_FLAGS "-fPIC ${CMAKE_C_FLAGS}")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(WINDOWS TRUE)
endif ()

option(BUILD_MEX    "Build Matlab mex files"    OFF)
option(BUILD_PYTHON "Build Python wrappers"     ON)

include(cmake/segyio_testing.cmake)
enable_testing()
include(cmake/check_includes.cmake)

if (MSVC)
    add_definitions("/W3 /wd4996")
else() # assuming gcc-style options
    set(C_WARNINGS "-Wall -Wextra -pedantic -Wformat-nonliteral -Wcast-align -Wpointer-arith -Wmissing-declarations -Wcast-qual -Wshadow -Wwrite-strings -Wchar-subscripts -Wredundant-decls")
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "${C_WARNINGS} ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "${C_WARNINGS} ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
endif (MSVC)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif ()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include_directories(src)

set(SOURCE_FILES src/segyio/segy.c src/spec/segyspec.c)

install(FILES src/segyio/segy.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/segyio)

# The object library type is a reusable component for later builds: shared, static and python
add_library(segyio-object OBJECT ${SOURCE_FILES})

add_library(segyio-static STATIC $<TARGET_OBJECTS:segyio-object>)
target_link_libraries(segyio-static ${SEGYIO_LIBRARIES})
set_target_properties(segyio-static PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)
set_target_properties(segyio-static PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(segyio-shared SHARED $<TARGET_OBJECTS:segyio-object>)
target_link_libraries(segyio-shared ${SEGYIO_LIBRARIES} ${SEGYIO_LIBRARIES})
set_target_properties(segyio-shared PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)

install(TARGETS segyio-static segyio-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

add_executable(segyinfo src/applications/segyinfo.c)
target_link_libraries(segyinfo segyio-static)
add_dependencies(segyinfo segyio-static)

add_executable(segyinspect src/applications/segyinspect.c)
target_link_libraries(segyinspect segyio-static)
add_dependencies(segyinspect segyio-static)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

if (BUILD_MEX)
    add_subdirectory(mex)
    option(BUILD_MEX_TESTS "Build matlab mex tests" ON)
else (BUILD_MEX)
    unset(MATLAB_MCC CACHE)
    unset(MATLAB_MEX CACHE)
    unset(MATLAB_MEXEXT CACHE)
    unset(MATLAB_ROOT CACHE)
    unset(BUILD_MEX_TESTS CACHE)
endif ()

if (BUILD_PYTHON)
    include(cmake/python.cmake)
    add_subdirectory(python)
endif ()

add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(applications)
