project(segyio-apps)

if( NOT HAVE_GETOPT_H OR NOT HAVE_GETOPT_LONG )
    message(WARNING "Could not find getopt. Not building applications.")
    return ()
endif ()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
endif()

add_executable(segyinfo segyinfo.c)
target_link_libraries(segyinfo segyio)

add_executable(segyinspect segyinspect.c)
target_link_libraries(segyinspect segyio)

add_executable(segycath segycath.c)
target_link_libraries(segycath segyio)
target_compile_definitions(segycath PRIVATE
    -Dsegyio_MAJOR=${segyio_MAJOR}
    -Dsegyio_MINOR=${segyio_MINOR}
)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(TARGETS segycath DESTINATION ${CMAKE_INSTALL_BINDIR})
