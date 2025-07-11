project(segyio-apps)

if (NOT BUILD_BIN)
    return ()
endif ()

if( NOT HAVE_GETOPT_H OR NOT HAVE_GETOPT_LONG )
    message(WARNING "Could not find getopt. Not building applications.")
    return ()
endif ()

set(CMAKE_CXX_STANDARD 11)

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

add_executable(flip-endianness flip-endianness.cpp)
target_link_libraries(flip-endianness segyio)
target_compile_options(flip-endianness BEFORE
    PRIVATE
        $<$<CONFIG:Debug>:${warnings-c}>
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
set(long  ${testdata}/long.sgy)
set(increment  ${testdata}/increment.sgy)
set(decrement  ${testdata}/decrement.sgy)
set(test ${CMAKE_CURRENT_SOURCE_DIR}/test)
add_test(NAME catr.arg.t1       COMMAND segyio-catr -t 0 ${small})
add_test(NAME catr.arg.t2       COMMAND segyio-catr -t 5 ${small})
add_test(NAME catr.arg.r1       COMMAND segyio-catr -r 5 ${small})
add_test(NAME catr.arg.r2       COMMAND segyio-catr -r 1 2 ${small})
add_test(NAME catr.arg.r3       COMMAND segyio-catr -r 1 2 3 ${small} -r 5 6)
add_test(NAME catr.arg.multiple COMMAND segyio-catr "-r 1" 5 2 ${small} -r 1 6)
add_test(NAME catr.arg.rt       COMMAND segyio-catr -r 1 2 ${small} -t 5)
add_test(NAME catr.arg.fmt      COMMAND segyio-catr -f ieee ${small})

add_test(NAME catb.arg.help     COMMAND segyio-catb --help)
add_test(NAME catb.fail.nosegy  COMMAND segyio-catb ${test}/catb.output)
add_test(NAME catb.fail.nofile  COMMAND segyio-catb not-exist)
add_test(NAME catb.fail.noarg   COMMAND segyio-catb)

add_test(NAME cath.arg.help     COMMAND segyio-cath --help)
add_test(NAME cath.fail.nosegy  COMMAND segyio-cath ${test}/cath.output)
add_test(NAME cath.fail.nofile  COMMAND segyio-cath --strict not-exist)
add_test(NAME cath.fail.noarg   COMMAND segyio-cath)

set_tests_properties(catr.arg.t1
                     catb.fail.nosegy
                     catb.fail.nofile
                     catb.fail.noarg
                     cath.fail.nosegy
                     cath.fail.nofile
                     cath.fail.noarg
    PROPERTIES WILL_FAIL ON)

add_custom_target(test-app-output
    ALL
    DEPENDS catb.out
            cath.out
            catr.out
            crop-ns.out
            crop-ns.sgy
)
add_custom_command(
    OUTPUT catb.out cath.out catr.out
    COMMENT "running applications for output comparison"
    DEPENDS segyio-catb segyio-cath segyio-catr
            test/catb.output
            test/cath.output
            test/catr.output
    COMMAND segyio-catb ${small} > catb.out
    COMMAND segyio-catb ${increment} > catb-inc.out
    COMMAND segyio-catb ${decrement} > catb-dec.out
    COMMAND segyio-catb ${small} -n -d > catbnd.out
    COMMAND segyio-cath ${small} > cath.out
    COMMAND segyio-catb ${long} > catb-long.out
    COMMAND segyio-catr -r 4 9 -t 12 ${small} > catr.out
    COMMAND segyio-catr -t 1 ${small} -n -d > catrnd.out
    COMMAND segyio-catr -t 2 ${increment} > catr-inc.out
    COMMAND segyio-catr -t 2 ${decrement} > catr-dec.out
)
add_custom_command(
    OUTPUT crop-ns.out crop-ns.sgy
    COMMENT "running applications for crop testing"
    DEPENDS segyio-crop segyio-catr
            test/crop-ns.output
    COMMAND segyio-crop -S 25 ${small} crop-ns.sgy
    COMMAND segyio-catr crop-ns.sgy > crop-ns.out
)

add_test(NAME catb.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catb.output catb.out
)
add_test(NAME catb.output.nd
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catbnd.output
                                                   catbnd.out
)
add_test(NAME catb-long.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catb-long.output
                                                   catb-long.out
)
add_test(NAME catb-inc.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catb-inc.output
                                                   catb-inc.out
)
add_test(NAME catb-dec.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catb-dec.output
                                                   catb-dec.out
)
add_test(NAME cath.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/cath.output cath.out
)
add_test(NAME catr.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catr.output catr.out
)
add_test(NAME catr-inc.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catr-inc.output
                                                   catr-inc.out
)
add_test(NAME catr-dec.output
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catr-dec.output
                                                   catr-dec.out
)
add_test(NAME catr.output.nd
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/catrnd.output
                                                   catrnd.out
)
add_test(NAME crop.samplecount
         COMMAND ${CMAKE_COMMAND} -E compare_files ${test}/crop-ns.output
                                                   crop-ns.out
)
