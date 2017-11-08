project(segyio-python)

if (REQUIRE_PYTHON)
    set(BUILD_PYTHON ON)
endif()

if (NOT BUILD_PYTHON)
    return()
endif()

find_package(PythonInterp REQUIRED)

if (NOT PYTHON_EXECUTABLE AND REQUIRE_PYTHON)
    message(SEND_ERROR "Could not find python executable")
    return()
endif()

if (NOT PYTHON_EXECUTABLE)
    message(WARNING "Could not find python - skipping python bindings")
    return()
endif()

set(python ${PYTHON_EXECUTABLE})

if (NOT WIN32)
    # setuptools on microsoft compilers doesn't support the --library-dir flag
    # and crashes, so only pass it on non-microsoft platforms
    set(setup-py-libdir build_ext
                            --rpath        $<TARGET_FILE_DIR:segyio-shared>
                            --library-dirs $<TARGET_FILE_DIR:segyio-shared>)
endif ()

set(setup-py ${CMAKE_SOURCE_DIR}/setup.py)
add_custom_target(
    segyio-python ALL
    COMMENT "Building python library with setup.py"
    SOURCES ${setup-py}
    DEPENDS ${setup-py}
    VERBATIM

    # copy the examples to have them runnable from a relative directory, so
    # that a locally-installed segyio can be imported
    COMMAND ${CMAKE_COMMAND} -E
        copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/examples .

    # do the same to tests. running through setup.py test is *very* slow, so
    # invoke unittest manually from the build directory. more importantly,
    # setup.py test will pollute the source directory with egg info and
    # extensions, which is unacceptable in a cmake world
    COMMAND ${CMAKE_COMMAND} -E
        copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/test test

    # setuptools on windows breaks spectacularly when the library isn't
    # available in the same directory, and build_ext --library-dirs is not
    # support on msvc is not supported, so we must copy out the libsegyio core
    # object and put it here
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:segyio-shared>
                                     $<TARGET_FILE_NAME:segyio-shared>

    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_LINKER_FILE:segyio-shared>
                                     $<TARGET_LINKER_FILE_NAME:segyio-shared>

    # install the lib in the build-dir so that the examples can load that from
    # current working dir
    COMMAND ${python} ${setup-py} ${setup-py-libdir} install_lib -d .
)

add_dependencies(segyio-python segyio-shared)

# write egg_info to the build dir in order not to pollute the source directory
# and install as if it was through some other distro by using single-version,
# so that install won't die on possibly missing pythonpath
install(CODE "execute_process(COMMAND
    ${python} ${setup-py}
        install_egg_info --install-dir .
        egg_info --egg-base .
        install --prefix=${CMAKE_INSTALL_PREFIX}
                --single-version-externally-managed
                --record installed-files
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)")

if(NOT BUILD_TESTING)
    return()
endif()

file(GLOB sgys "${testdata}/*.sgy")
foreach (sgy ${sgys})
    get_filename_component(fl ${sgy} NAME)
    configure_file(${sgy} test-data/${fl} COPYONLY)
endforeach ()

set(py-test ${python} -m unittest -v)
add_test(NAME python.segy      COMMAND ${py-test} test.segy)
add_test(NAME python.h.segy    COMMAND ${py-test} test.segyio_c)
add_test(NAME python.enum.segy COMMAND ${py-test} test.segyioenum)
add_test(NAME python.tools     COMMAND ${py-test} test.tools)
add_test(NAME python.context   COMMAND ${py-test} test.context)

configure_file(${testdata}/small.sgy
               test-data/write.sgy
               COPYONLY)

add_test(NAME python.example.about        COMMAND ${python} about.py test-data/small.sgy INLINE_3D CROSSLINE_3D)
add_test(NAME python.example.write        COMMAND ${python} write.py test-data/write.sgy)
add_test(NAME python.example.makefile     COMMAND ${python} make-file.py test-data/large-file.sgy 20 1 20 1 20)
add_test(NAME python.example.makepsfile   COMMAND ${python} make-ps-file.py test-data/small-prestack.sgy 10 1 5 1 4 1 3)
add_test(NAME python.example.subcube      COMMAND ${python} copy-sub-cube.py test-data/small.sgy test-data/copy.sgy)
add_test(NAME python.example.rotate       COMMAND ${python} make-rotated-copies.py test-data/small.sgy)
add_test(NAME python.example.scan_min_max COMMAND ${python} scan_min_max.py test-data/small.sgy)
