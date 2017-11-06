project(segyio-apps)

if( NOT HAVE_GETOPT_H OR NOT HAVE_GETOPT_LONG )
    message(WARNING "Could not find getopt. Not building applications.")
    return ()
endif ()

add_library(apputils STATIC apputils.c)
target_link_libraries(apputils segyio)
target_compile_options(apputils BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)
target_compile_definitions(apputils PUBLIC
    -Dsegyio_MAJOR=${segyio_MAJOR}
    -Dsegyio_MINOR=${segyio_MINOR}
)

add_executable(segyinfo segyinfo.c)
target_link_libraries(segyinfo segyio)
target_compile_options(segyinfo BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

add_executable(segyinspect segyinspect.c)
target_link_libraries(segyinspect segyio)
target_compile_options(segyinspect BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

add_executable(segyio-cath segyio-cath.c)
target_link_libraries(segyio-cath segyio apputils)
target_compile_options(segyio-cath BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

add_executable(segyio-catb segyio-catb.c)
target_link_libraries(segyio-catb segyio apputils)
target_compile_options(segyio-catb BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

add_executable(segyio-catr segyio-catr.c)
target_link_libraries(segyio-catr segyio apputils)
target_compile_options(segyio-catr BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

add_executable(segyio-crop segyio-crop.c)
target_link_libraries(segyio-crop segyio apputils)
target_compile_options(segyio-crop BEFORE
    PRIVATE $<$<CONFIG:Debug>:${warnings-c}> ${c99}
)

install(TARGETS segyio-cath
                segyio-catb
                segyio-catr
                segyio-crop
        DESTINATION ${CMAKE_INSTALL_BINDIR})

if (NOT BUILD_TESTING)
    return ()
endif ()

set(small ${testdata}/small.sgy)
add_test(NAME catr.arg.t        COMMAND segyio-catr -t 5 ${small})
add_test(NAME catr.arg.r1       COMMAND segyio-catr -r 5 ${small})
add_test(NAME catr.arg.r2       COMMAND segyio-catr -r 1 2 ${small})
add_test(NAME catr.arg.r3       COMMAND segyio-catr -r 1 2 3 ${small} -r 5 6)
add_test(NAME catr.arg.multiple COMMAND segyio-catr "-r 1" 5 2 ${small} -r 1 6)
add_test(NAME catr.arg.rt       COMMAND segyio-catr -r 1 2 ${small} -t 5)

add_test(NAME catb.arg.help     COMMAND segyio-catb --help)
add_test(NAME catb.fail.nosegy  COMMAND segyio-catb ${CMAKE_CURRENT_LIST_FILE})
add_test(NAME catb.fail.nofile  COMMAND segyio-catb not-exist)
add_test(NAME catb.fail.noarg   COMMAND segyio-catb)

add_test(NAME cath.arg.help     COMMAND segyio-catb --help)
add_test(NAME cath.fail.nosegy  COMMAND segyio-catb ${CMAKE_CURRENT_LIST_FILE})
add_test(NAME cath.fail.nofile  COMMAND segyio-catb not-exist)
add_test(NAME cath.fail.noarg   COMMAND segyio-catb)

set_tests_properties(catb.fail.nosegy
                     catb.fail.nofile
                     catb.fail.noarg
                     cath.fail.nosegy
                     cath.fail.nofile
                     cath.fail.noarg
    PROPERTIES WILL_FAIL ON)

find_package(PythonInterp)

if (NOT PYTHON_EXECUTABLE)
    message(WARNING "Could not find python, skipping application output tests")
endif()

set(test ${CMAKE_CURRENT_SOURCE_DIR}/test)
add_test(NAME catr.output COMMAND ${PYTHON_EXECUTABLE} ${test}/catr.py)
add_test(NAME catb.output COMMAND ${PYTHON_EXECUTABLE} ${test}/catb.py)
add_test(NAME cath.output COMMAND ${PYTHON_EXECUTABLE} ${test}/cath.py)
