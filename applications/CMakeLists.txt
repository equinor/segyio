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

add_executable(segyio-cath segyio-cath.c)
target_link_libraries(segyio-cath segyio)
target_compile_definitions(segyio-cath PRIVATE
    -Dsegyio_MAJOR=${segyio_MAJOR}
    -Dsegyio_MINOR=${segyio_MINOR}
)

add_executable(segyio-catr segyio-catr.c)
target_link_libraries(segyio-catr segyio)
target_compile_definitions(segyio-catr PRIVATE
    -Dsegyio_MAJOR=${segyio_MAJOR}
    -Dsegyio_MINOR=${segyio_MINOR}
)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

install(TARGETS segyio-cath DESTINATION ${CMAKE_INSTALL_BINDIR})

add_python_test(applications.apps   test/apps.py)
