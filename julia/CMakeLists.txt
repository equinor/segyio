include(${CMAKE_SOURCE_DIR}/cmake/FindJuliaInterp.cmake)

if(NOT JULIAINTERP_FOUND)
    message(STATUS "Disabling Julia bindings")
    return()
endif()

set(JULIA_SOURCES Segyio.jl)

function(add_julia_package PACKAGE_NAME PACKAGE_PATH JULIA_FILES)
    add_custom_target(package_${PACKAGE_NAME} ALL)

    foreach(file ${JULIA_FILES})
        add_custom_command(
            TARGET package_${PACKAGE_NAME}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/julia/${PACKAGE_PATH}
            COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_BINARY_DIR}/julia/${PACKAGE_PATH}
            )
    endforeach()

    set_target_properties(
        package_${PACKAGE_NAME} PROPERTIES
        PACKAGE_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/${JULIA_INSTALL_PREFIX}/${PACKAGE_PATH}
        )
    install(FILES ${JULIA_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${JULIA_INSTALL_PREFIX}/${PACKAGE_PATH})
endfunction()

function(add_julia_test TESTNAME JULIA_TEST_FILE)
    configure_file(${JULIA_TEST_FILE} ${JULIA_TEST_FILE} COPYONLY)
        
    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            COMMAND ${JULIA_EXECUTABLE} ${JULIA_TEST_FILE}
            )

    to_path_list(juliapath "${CMAKE_BINARY_DIR}/julia" "$ENV{JULIA_LOAD_PATH}")
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT "JULIA_LOAD_PATH=${juliapath}")
endfunction()

add_julia_package(Segyio Segyio "${JULIA_SOURCES}")
