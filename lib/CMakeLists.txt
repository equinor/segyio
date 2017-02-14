project(libsegyio)

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
endif()

set(SOURCE_FILES src/segy.c)

set(STATIC_NAME segyio)
if(WINDOWS)
    # MSVC outputs the same name for static and shared libraries (with the same
    # extension), so we need to differentiate between the two somehow.
    set(STATIC_NAME "${STATIC_NAME}-static")
    set(DLL_EXPORT_FILES src/segy.def)
endif()

#
# static build
#
add_library(segyio-static STATIC ${SOURCE_FILES})
target_link_libraries(segyio-static ${SEGYIO_LIBRARIES})
set_target_properties(segyio-static PROPERTIES
                      OUTPUT_NAME ${STATIC_NAME}
                      CLEAN_DIRECT_OUTPUT 1)
target_include_directories(
    segyio-static PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

#
# dynamic build
#
add_library(segyio-shared SHARED ${SOURCE_FILES} ${DLL_EXPORT_FILES})
target_link_libraries(segyio-shared ${SEGYIO_LIBRARIES})
set_target_properties(segyio-shared PROPERTIES
                      OUTPUT_NAME segyio
                      CLEAN_DIRECT_OUTPUT 1)
target_include_directories(
    segyio-shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

if (BUILD_SHARED_LIBS)
    add_library(segyio ALIAS segyio-shared)
else ()
    add_library(segyio ALIAS segyio-static)
    set_property(TARGET segyio-static PROPERTY POSITION_INDEPENDENT_CODE ON)
endif ()

#
# install & export
#
install(TARGETS segyio-static segyio-shared
        EXPORT segyio
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT segyio DESTINATION share/segyio/cmake)
export(TARGETS segyio-static segyio-shared FILE segyio-config.cmake)

#
# tests
#
configure_file(${testdata}/small.sgy test-data/small.sgy             COPYONLY)
configure_file(${testdata}/small.sgy test-data/small-traceheader.sgy COPYONLY)
configure_file(${testdata}/text.sgy  test-data/text.sgy              COPYONLY)

if(NOT MSVC)
    set(math "m")
endif()

foreach (src segy utils)
    add_executable(test${src} test/${src}.c)
    target_link_libraries(test${src} segyio-shared ${math})
    target_include_directories(test${src} PRIVATE src)
    add_test(c.${src} test${src})
    add_memcheck_test(test${src} test${src})
endforeach()
