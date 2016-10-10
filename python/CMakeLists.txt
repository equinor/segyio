find_package(PythonInterp)
find_package(PythonLibs REQUIRED)

if (PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
    add_library(_segyio SHARED segyio/_segyio.c $<TARGET_OBJECTS:segyio-object>)

    if (MAC_OS)
        set_target_properties(_segyio PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    else ()
        set_target_properties(_segyio PROPERTIES LINK_FLAGS "-Xlinker -export-dynamic")
    endif ()

    set_target_properties(_segyio PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS _segyio DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python2.7/site-packages/segyio)

    add_custom_command(TARGET _segyio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:_segyio> ${CMAKE_BINARY_DIR}/python/segyio/_segyio.so)
endif ()

add_subdirectory(segyio)