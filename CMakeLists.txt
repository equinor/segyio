cmake_minimum_required(VERSION 2.8)
project(segyio)

if(POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif()

option(BUILD_MEX    "Build Matlab mex files"    OFF)
option(BUILD_PYTHON "Build Python wrappers"     ON)

include(cmake/python.cmake)
enable_testing()

set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path" FORCE )
endif()

#set(CMAKE_BUILD_TYPE RELEASE)

include_directories(src)

set(SOURCE_FILES src/segyio/segy.c src/spec/segyspec.c)

install(FILES src/segyio/segy.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/segyio)

add_library(segyio-static STATIC ${SOURCE_FILES})
set_target_properties(segyio-static PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)
set_target_properties(segyio-static PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(segyio-shared SHARED ${SOURCE_FILES})
set_target_properties(segyio-shared PROPERTIES OUTPUT_NAME segyio CLEAN_DIRECT_OUTPUT 1)


if(BUILD_PYTHON)
    find_package(PythonInterp)
    find_package(PythonLibs REQUIRED)

    if (PYTHONLIBS_FOUND)
        include_directories(${PYTHON_INCLUDE_DIRS})
        add_library(_segyio SHARED python/segyio/_segyio.c ${SOURCE_FILES})

        if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            set_target_properties(_segyio PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
        else()
            set_target_properties(_segyio PROPERTIES LINK_FLAGS "-Xlinker -export-dynamic")
        endif()

        set_target_properties(_segyio PROPERTIES PREFIX "" SUFFIX ".so")
        install(TARGETS _segyio DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python2.7/site-packages/segyio)

        add_custom_command(TARGET _segyio POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_BINARY_DIR}/_segyio.so ${CMAKE_BINARY_DIR}/python/segyio/_segyio.so)
    endif()
    add_subdirectory(python)
endif()

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
endif()

add_subdirectory(tests)
add_subdirectory(examples)
add_subdirectory(applications)
