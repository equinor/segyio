project(segyio-apps)

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
endif()

add_executable(segyinfo segyinfo.c)
target_link_libraries(segyinfo segyio)

add_executable(segyinspect segyinspect.c)
target_link_libraries(segyinspect segyio)

install(TARGETS segyinfo DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(TARGETS segyinspect DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
