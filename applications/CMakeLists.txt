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
set(test ${CMAKE_CURRENT_SOURCE_DIR}/test)
add_test(NAME catr.arg.t        COMMAND segyio-catr -t 5 ${small})
add_test(NAME catr.arg.r1       COMMAND segyio-catr -r 5 ${small})
add_test(NAME catr.arg.r2       COMMAND segyio-catr -r 1 2 ${small})
add_test(NAME catr.arg.r3       COMMAND segyio-catr -r 1 2 3 ${small} -r 5 6)
add_test(NAME catr.arg.multiple COMMAND segyio-catr "-r 1" 5 2 ${small} -r 1 6)
add_test(NAME catr.arg.rt       COMMAND segyio-catr -r 1 2 ${small} -t 5)

add_test(NAME catb.arg.help     COMMAND segyio-catb --help)
add_test(NAME catb.fail.nosegy  COMMAND segyio-catb ${test}/catb.output)
add_test(NAME catb.fail.nofile  COMMAND segyio-catb not-exist)
add_test(NAME catb.fail.noarg   COMMAND segyio-catb)

add_test(NAME cath.arg.help     COMMAND segyio-cath --help)
add_test(NAME cath.fail.nosegy  COMMAND segyio-cath ${test}/cath.output)
add_test(NAME cath.fail.nofile  COMMAND segyio-cath --strict not-exist)
add_test(NAME cath.fail.noarg   COMMAND segyio-cath)

set_tests_properties(catb.fail.nosegy
                     catb.fail.nofile
                     catb.fail.noarg
                     cath.fail.nosegy
                     cath.fail.nofile
                     cath.fail.noarg
    PROPERTIES WILL_FAIL ON)

add_custom_target(test-app-output ALL DEPENDS catb.out cath.out catr.out)
add_custom_command(
    OUTPUT catb.out cath.out catr.out
    COMMENT "running applications for output comparison"
    DEPENDS segyio-catb segyio-cath segyio-catr
            test/catb.output
            test/cath.output
            test/catr.output
    COMMAND segyio-catb ${small} > catb.out
    COMMAND segyio-cath ${small} > cath.out
    COMMAND segyio-catr -r 4 9 -t 12 ${small} > catr.out
)

add_test(NAME catb.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catb.output catb.out
)
add_test(NAME cath.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/cath.output cath.out
)
add_test(NAME catr.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catr.output catr.out
)
