add_executable(segyinfo segyinfo.c)
target_link_libraries(segyinfo segyio-static)
add_dependencies(segyinfo segyio-static)

add_executable(segyinspect segyinspect.c)
target_link_libraries(segyinspect segyio-static)
add_dependencies(segyinspect segyio-static)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
