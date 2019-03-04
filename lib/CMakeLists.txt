project(libsegyio C CXX)

if(MSVC)
    set(DLL_EXPORT_FILES src/segy.def)
endif()

add_library(segyio src/segy.c ${DLL_EXPORT_FILES})
target_link_libraries(segyio ${m} ${ws2})
target_compile_options(segyio BEFORE
    PRIVATE
        ${c99}
        $<$<CONFIG:Debug>:${warnings-c}>
)
target_compile_definitions(segyio
    PRIVATE
        ${mmap}
        ${fstat}
        ${ftello}
        $<${HOST_BIG_ENDIAN}:HOST_BIG_ENDIAN>
)
set_target_properties(segyio
                      PROPERTIES
                      SOVERSION   ${segyio_MAJOR}
                      VERSION     ${segyio_MAJOR}
)
target_include_directories(segyio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        src
        include
)

#
# install & export
#
install(TARGETS segyio
        EXPORT segyio
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT segyio
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/segyio/cmake
        FILE segyio-config.cmake
)
export(TARGETS segyio FILE segyio-config.cmake)

if(EXPERIMENTAL)
    install(DIRECTORY experimental/ DESTINATION include)
endif()

if (NOT BUILD_TESTING)
    return ()
endif ()

configure_file(${testdata}/small.sgy                          test-data/small.sgy                            COPYONLY)
configure_file(${testdata}/small-ps-dec-il-xl-off.sgy         test-data/small-ps-dec-il-xl-off.sgy           COPYONLY)
configure_file(${testdata}/small-ps-dec-il-inc-xl-off.sgy     test-data/small-ps-dec-il-inc-xl-off.sgy       COPYONLY)
configure_file(${testdata}/small-ps-dec-xl-inc-il-off.sgy     test-data/small-ps-dec-xl-inc-il-off.sgy       COPYONLY)
configure_file(${testdata}/small-ps-dec-off-inc-il-xl.sgy     test-data/small-ps-dec-off-inc-il-xl.sgy       COPYONLY)
configure_file(${testdata}/small-ps-dec-il-xl-inc-off.sgy     test-data/small-ps-dec-il-xl-inc-off.sgy       COPYONLY)
configure_file(${testdata}/small-ps-dec-il-off-inc-xl.sgy     test-data/small-ps-dec-il-off-inc-xl.sgy       COPYONLY)
configure_file(${testdata}/small-ps-dec-xl-off-inc-il.sgy     test-data/small-ps-dec-xl-off-inc-il.sgy       COPYONLY)
configure_file(${testdata}/small.sgy                          test-data/small-w.sgy                          COPYONLY)
configure_file(${testdata}/small-lsb.sgy                      test-data/small-lsb.sgy                        COPYONLY)
configure_file(${testdata}/1x1.sgy                            test-data/1x1.sgy                              COPYONLY)
configure_file(${testdata}/1xN.sgy                            test-data/1xN.sgy                              COPYONLY)
configure_file(${testdata}/f3.sgy                             test-data/f3.sgy                               COPYONLY)
configure_file(${testdata}/f3-lsb.sgy                         test-data/f3-lsb.sgy                           COPYONLY)
configure_file(${testdata}/text.sgy                           test-data/text.sgy                             COPYONLY)
configure_file(${testdata}/interval-neg-bin-neg-trace.sgy     test-data/interval-neg-bin-neg-trace.sgy       COPYONLY)
configure_file(${testdata}/interval-neg-bin-pos-trace.sgy     test-data/interval-neg-bin-pos-trace.sgy       COPYONLY)
configure_file(${testdata}/interval-pos-bin-neg-trace.sgy     test-data/interval-pos-bin-neg-trace.sgy       COPYONLY)

configure_file(${testdata}/multiformats/Format1lsb.sgy  test-data/Format1lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format1msb.sgy  test-data/Format1msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format2lsb.sgy  test-data/Format2lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format2msb.sgy  test-data/Format2msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format3lsb.sgy  test-data/Format3lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format3msb.sgy  test-data/Format3msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format4lsb.sgy  test-data/Format4lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format4msb.sgy  test-data/Format4msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format5lsb.sgy  test-data/Format5lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format5msb.sgy  test-data/Format5msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format6lsb.sgy  test-data/Format6lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format6msb.sgy  test-data/Format6msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format8lsb.sgy  test-data/Format8lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format8msb.sgy  test-data/Format8msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format9lsb.sgy  test-data/Format9lsb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format9msb.sgy  test-data/Format9msb.sgy  COPYONLY)
configure_file(${testdata}/multiformats/Format10lsb.sgy test-data/Format10lsb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format10msb.sgy test-data/Format10msb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format11lsb.sgy test-data/Format11lsb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format11msb.sgy test-data/Format11msb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format12lsb.sgy test-data/Format12lsb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format12msb.sgy test-data/Format12msb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format16lsb.sgy test-data/Format16lsb.sgy COPYONLY)
configure_file(${testdata}/multiformats/Format16msb.sgy test-data/Format16msb.sgy COPYONLY)

add_executable(c.segy test/testsuite.cpp
                      test/segy.cpp
                      test/segyio-cpp.cpp
)
target_include_directories(c.segy PRIVATE src experimental)
target_link_libraries(c.segy catch2 segyio)
target_compile_options(c.segy BEFORE
    PRIVATE
        ${c++11}
        ${mmap}
        $<$<CONFIG:Debug>:${warnings-c}>
)
target_compile_definitions(c.segy
    PRIVATE
        ${mmap}
)
add_test(NAME c.segy          COMMAND c.segy)
add_test(NAME c.segy.mmap     COMMAND c.segy --test-mmap)
add_test(NAME c.segy.lsb      COMMAND c.segy --test-lsb)
add_test(NAME c.segy.mmap.lsb COMMAND c.segy --test-mmap --test-lsb)
add_test(NAME cpp.segy        COMMAND c.segy [c++])
