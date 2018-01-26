project(libsegyio C)

if(MSVC)
    set(DLL_EXPORT_FILES src/segy.def)
endif()

add_library(segyio src/segy.c ${DLL_EXPORT_FILES})
target_link_libraries(segyio ${m} ${ws2})
target_compile_options(segyio BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)
target_compile_definitions(segyio PRIVATE
    ${htons} ${mmap} ${fstat} ${ftello})
set_target_properties(segyio PROPERTIES
                      SOVERSION   ${segyio_MAJOR}
                      VERSION     ${segyio_MAJOR}
                      POSITION_INDEPENDENT_CODE ON
                      CLEAN_DIRECT_OUTPUT 1)
target_include_directories(
    segyio PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE src include)

install(TARGETS segyio
        EXPORT segyio
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION include)
install(EXPORT segyio DESTINATION share/segyio/cmake FILE segyio-config.cmake)
export(TARGETS segyio FILE segyio-config.cmake)

if (NOT BUILD_TESTING)
    return ()
endif ()

configure_file(${testdata}/small.sgy test-data/small.sgy             COPYONLY)
configure_file(${testdata}/text.sgy  test-data/text.sgy              COPYONLY)

add_executable(c.segy test/testsuite.cpp
                      test/segy.cpp
                      test/mmap.cpp)
target_include_directories(c.segy PRIVATE src)
target_link_libraries(c.segy catch2 segyio)
target_compile_options(c.segy BEFORE PRIVATE
                       ${c++11} $<$<CONFIG:Debug>:${warnings-c}> ${mmap})
add_test(NAME c.segy COMMAND c.segy ~[mmap] [c.segy])
add_test(NAME c.segy.mmap COMMAND c.segy [mmap])
