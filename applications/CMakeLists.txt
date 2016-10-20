configure_file(segyviewer.py segyviewer COPYONLY)

if (BUILD_PYTHON)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/segyviewer
            PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_EXECUTE WORLD_READ
            DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
endif ()

add_executable(segyinfo segyinfo.c)
target_link_libraries(segyinfo segyio-static)
add_dependencies(segyinfo segyio-static)

add_executable(segyinspect segyinspect.c)
target_link_libraries(segyinspect segyio-static)
add_dependencies(segyinspect segyio-static)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
