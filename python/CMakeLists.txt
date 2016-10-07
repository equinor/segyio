if (NOT DEFINED PYTHON_EXECUTABLE)
    message("Python interpreter not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()


python_module(numpy 1.6)
if (NOT DEFINED PY_numpy)
    message("numpy module not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()

if (PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
    add_library(_segyio SHARED segyio/_segyio.c $<TARGET_OBJECTS:segyio-object>)

    if (MAC_OS)
        set_target_properties(_segyio PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    else ()
        set_target_properties(_segyio PROPERTIES LINK_FLAGS "-Xlinker -export-dynamic")
    endif ()

    set_target_properties(_segyio PROPERTIES PREFIX "" SUFFIX ".so")
    install(TARGETS _segyio DESTINATION ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}/segyio)

    add_custom_command(TARGET _segyio POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:_segyio> ${CMAKE_BINARY_DIR}/python/segyio/_segyio.so)
endif ()

add_subdirectory(segyio)
add_subdirectory(segyview)
