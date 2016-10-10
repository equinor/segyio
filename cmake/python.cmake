configure_file(cmake/test_runner.py tests/test_runner.py COPYONLY)

if(WINDOWS)
    set(SEP "\\;")
else() # e.g. Linux
    set(SEP ":")
endif()

function(add_memcheck_test NAME BINARY)
    # Valgrind on MacOS is experimental
    if(LINUX AND (${CMAKE_BUILD_TYPE} MATCHES "DEBUG"))
        set(memcheck_command "valgrind --trace-children=yes --leak-check=full --error-exitcode=31415")
        separate_arguments(memcheck_command)
        add_test(memcheck_${NAME} ${memcheck_command} ./${BINARY})
    endif()
endfunction(add_memcheck_test)

function(add_python_package PACKAGE_NAME PACKAGE_PATH PYTHON_FILES)
    add_custom_target(package_${PACKAGE_NAME} ALL)

    foreach (file ${PYTHON_FILES})
        add_custom_command(TARGET package_${PACKAGE_NAME}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/python/${PACKAGE_PATH}
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${file} ${CMAKE_BINARY_DIR}/python/${PACKAGE_PATH}
                )
    endforeach ()
    install(FILES ${PYTHON_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python2.7/site-packages/${PACKAGE_PATH})
endfunction()

function(add_python_test TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)
        
    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            COMMAND python test_runner.py ${PYTHON_TEST_FILE}
            )
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT
                        "PYTHONPATH=${CMAKE_BINARY_DIR}/python${SEP}$ENV{PYTHONPATH}"
                        )
endfunction()

function(add_python_example TESTNAME PYTHON_TEST_FILE)
    configure_file(${PYTHON_TEST_FILE} ${PYTHON_TEST_FILE} COPYONLY)

    add_test(NAME ${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/examples
            COMMAND python ${PYTHON_TEST_FILE} ${ARGN}
            )
    set_tests_properties(${TESTNAME} PROPERTIES ENVIRONMENT
                        "PYTHONPATH=${CMAKE_BINARY_DIR}/python${SEP}$ENV{PYTHONPATH}"
                        )
endfunction()

function(add_segyio_test TESTNAME TEST_SOURCES)
    add_executable(test_${TESTNAME} unittest.h "${TEST_SOURCES}")
    target_link_libraries(test_${TESTNAME} segyio-static m)
    add_dependencies(test_${TESTNAME} segyio-static)
    add_test(NAME ${TESTNAME} COMMAND ${EXECUTABLE_OUTPUT_PATH}/test_${TESTNAME})
    add_memcheck_test(${TESTNAME} ${EXECUTABLE_OUTPUT_PATH}/test_${TESTNAME})
endfunction()
