if (NOT BUILD_PYTHON)
    return()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/python.cmake)

if (NOT DEFINED PYTHON_EXECUTABLE)
    message("Python interpreter not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()


python_module(numpy REQUIRED 1.6)
if (NOT DEFINED PY_numpy)
    message("numpy module not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()

if (NOT PYTHONLIBS_FOUND)
    message("Python libraries not found.")
    unset(BUILD_PYTHON)
    return()
endif()

include_directories(${PYTHON_INCLUDE_DIRS})
add_library(_segyio SHARED segyio/_segyio.c ${SOURCE_FILES})

if (MSVC)
    set(LIB_SUFFIX "pyd")
    set(LINK_FLAGS "/DLL ${PYTHON_LIBRARY}")
elseif (MAC_OS)
    set(LIB_SUFFIX "so")
    set(LINK_FLAGS "-undefined dynamic_lookup")
else()
    set(LIB_SUFFIX "so")
    set(LINK_FLAGS "-Xlinker -export-dynamic")
endif()

set_target_properties(_segyio PROPERTIES PREFIX "" SUFFIX ".${LIB_SUFFIX}")
set_target_properties(_segyio PROPERTIES LINK_FLAGS ${LINK_FLAGS})
target_link_libraries(_segyio ${SEGYIO_LIBRARIES})

install(TARGETS _segyio DESTINATION ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}/segyio)

add_custom_command(TARGET _segyio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:_segyio>
        ${CMAKE_BINARY_DIR}/python/segyio/_segyio.${LIB_SUFFIX})

add_subdirectory(segyio)
