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
option(BUILD_JULIA  "Build Julia wrappers"      ON)

include(cmake/check_includes.cmake)
include(cmake/default_warnings.cmake)
include(cmake/segyio_testing.cmake)
enable_testing()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
endif()

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE)
endif ()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

include_directories(src)
set(SOURCE_FILES src/segyio/segy.c src/spec/segyspec.c)

# The object library type is a reusable component for later builds: shared, static and python
add_library(segyio-object OBJECT ${SOURCE_FILES})

add_library(segyio-static STATIC $<TARGET_OBJECTS:segyio-object>)
target_link_libraries(segyio-static ${SEGYIO_LIBRARIES})
set_target_properties(segyio-static PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)
set_target_properties(segyio-static PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(segyio-shared SHARED $<TARGET_OBJECTS:segyio-object>)
target_link_libraries(segyio-shared ${SEGYIO_LIBRARIES} ${SEGYIO_LIBRARIES})
set_target_properties(segyio-shared PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)

# language bindings
add_subdirectory(mex)
add_subdirectory(python)
add_subdirectory(julia)

add_subdirectory(applications)
add_subdirectory(examples)
add_subdirectory(tests)

# install the library
install(FILES src/segyio/segy.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/segyio)
install(TARGETS segyio-static segyio-shared DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
