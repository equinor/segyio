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
