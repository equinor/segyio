include(cmake/find_python_module.cmake)
include(cmake/python_module_version.cmake)

find_package(PythonInterp)
find_package(PythonLibs REQUIRED)

configure_file(cmake/test_runner.py tests/test_runner.py COPYONLY)

if (EXISTS "/etc/debian_version")
    set( PYTHON_PACKAGE_PATH "dist-packages")
else()
    set( PYTHON_PACKAGE_PATH "site-packages")
endif()
set(PYTHON_INSTALL_PREFIX "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/${PYTHON_PACKAGE_PATH}" CACHE STRING "Subdirectory to install Python modules in")

function(add_python_package PACKAGE_NAME PACKAGE_PATH PYTHON_FILES)
    add_custom_target(package_${PACKAGE_NAME} ALL)

    foreach (file ${PYTHON_FILES})
        add_custom_command(TARGET package_${PACKAGE_NAME}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/python/${PACKAGE_PATH}
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_BINARY_DIR}/python/${PACKAGE_PATH}
                )
    endforeach ()
    install(FILES ${PYTHON_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}/${PACKAGE_PATH})
endfunction()

function(add_python_test TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)
        
    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            COMMAND python test_runner.py ${PYTHON_TEST_FILE}
            )

    to_path_list(pythonpath "${CMAKE_BINARY_DIR}/python" "$ENV{PYTHONPATH}")
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT "PYTHONPATH=${pythonpath}")
endfunction()

function(add_python_example TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)

    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples
            COMMAND python ${PYTHON_TEST_FILE} ${ARGN}
            )
    to_path_list(pythonpath "${CMAKE_BINARY_DIR}/python" "$ENV{PYTHONPATH}")
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT "PYTHONPATH=${pythonpath}")
endfunction()
