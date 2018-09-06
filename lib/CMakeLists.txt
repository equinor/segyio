project(libsegyio C)

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
        ${htons}
        ${mmap}
        ${fstat}
        ${ftello}
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

if (NOT BUILD_TESTING)
    return ()
endif ()

configure_file(${testdata}/small.sgy test-data/small.sgy             COPYONLY)
configure_file(${testdata}/1x1.sgy   test-data/1x1.sgy               COPYONLY)
configure_file(${testdata}/1xN.sgy   test-data/1xN.sgy               COPYONLY)
configure_file(${testdata}/f3.sgy    test-data/f3.sgy                COPYONLY)
configure_file(${testdata}/text.sgy  test-data/text.sgy              COPYONLY)

add_executable(c.segy test/testsuite.cpp
                      test/segy.cpp
                      test/mmap.cpp
)
target_include_directories(c.segy PRIVATE src)
target_link_libraries(c.segy catch2 segyio)
target_compile_options(c.segy BEFORE PRIVATE
    ${c++11}
    ${mmap}
    $<$<CONFIG:Debug>:${warnings-c}>
)
add_test(NAME c.segy COMMAND c.segy ~[mmap] [c.segy])
add_test(NAME c.segy.mmap COMMAND c.segy [mmap])
