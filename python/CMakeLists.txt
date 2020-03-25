if (SKBUILD)
    # invoked as a part of scikit-build, so this is just a proxy for the python
    # extension cmake. this works around the fundamental limitation in cmake
    # that it looks only for directories with a CMakeLists.txt in it, not for a
    # named file
    include(setup-CMakeLists.txt)
    return ()
endif ()

cmake_minimum_required(VERSION 3.5)
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
    message(WARNING "Could not find python - skipping python bindings. "
                    "Select specific python distribution with "
                    "-DPYTHON_EXECUTABLE=bin/python")
    return()
endif()

if (PYTHON_INSTALL_LAYOUT)
    set(setup-install-layout --install-layout ${PYTHON_INSTALL_LAYOUT})
endif()

set(setup.py ${CMAKE_CURRENT_SOURCE_DIR}/setup.py)
if (CMAKE_BUILD_TYPE)
    # use the cmake_build_type of the source project, unless it has been
    # specifically overriden
    set(SEGYIO_PYTHON_BUILD_TYPE
        --build-type=${CMAKE_BUILD_TYPE}
        CACHE STRING "override CMAKE_BUILD_TYPE in python extension"
    )
endif ()

add_custom_target(
    segyio-python ALL
    COMMENT "Building python library with setup.py"
    SOURCES ${setup.py}
    DEPENDS ${setup.py}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}

    # if DESTDIR is used, either for build and install, and make (or similar)
    # is the generator, the scikit-build's internal install command will
    # respect it and install the build-dir shared object there. The cmake
    # driven install already respects DESTDIR by translating into --root, so
    # simply remove it from then environment when doing python commands
    #
    # This is probably something that should be addressed upstream
    COMMAND ${CMAKE_COMMAND} -E env --unset=DESTDIR
            ${PYTHON_EXECUTABLE} ${setup.py}
        # build the extension inplace (really, once its built, copy it to the
        # source tree) so that post-build, the directory can be used to run
        # tests against
        build_ext --inplace
        build # setup.py build args
            --cmake-executable ${CMAKE_COMMAND}
            --generator ${CMAKE_GENERATOR}
            ${SEGYIO_PYTHON_BUILD_TYPE}
        -- # cmake to the extension
            -Dsegyio_DIR=${SEGYIO_LIB_BINARY_DIR}
            # "install" to the python/dlisio dir with rpath, so there's no need
            # to fiddle with environment in ctest to load the core library from
            # the build tree
            -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON
            -DCMAKE_INSTALL_RPATH=$<TARGET_FILE_DIR:segyio>
            -DCMAKE_INSTALL_NAME_DIR=$<TARGET_FILE_DIR:segyio>
)

add_dependencies(segyio-python segyio)

install(CODE "
    if (DEFINED ENV{DESTDIR})
        get_filename_component(abs-destdir \"\$ENV{DESTDIR}\" ABSOLUTE)
        set(root_destdir --root \${abs-destdir})
    endif()

    if (CMAKE_INSTALL_PREFIX)
        set(prefix --prefix \"${CMAKE_INSTALL_PREFIX}\")
    endif ()

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env --unset=DESTDIR
                ${PYTHON_EXECUTABLE} ${setup.py}
            install
                \${root_destdir}
                --single-version-externally-managed
                --record record.txt
                --cmake-executable \"${CMAKE_COMMAND}\"
                --generator \"${CMAKE_GENERATOR}\"
                \${prefix}
                ${SEGYIO_PYTHON_BUILD_TYPE}
            --
                -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )"
)

option(BUILD_PYDOC "Build python documentation" OFF)

if(BUILD_DOC)
    set(BUILD_PYDOC ON)
endif()

if(BUILD_PYDOC)
    add_custom_target(pydoc
            # run sphinx as the same python version that was just built
            # otherwise, the it will search for the wrong segyio extension and
            # fail
        COMMAND ${PYTHON_EXECUTABLE} -m sphinx
            # use the -d argument to avoid putting cache dir in docs/, because
            # that directory will be install'd
            -d ${CMAKE_CURRENT_BINARY_DIR}/.doctrees
            ${SPHINX_ARGS}
            ${CMAKE_CURRENT_SOURCE_DIR}/docs
            ${CMAKE_CURRENT_BINARY_DIR}/docs
        DEPENDS docs/conf.py
                docs/index.rst
                docs/segyio.rst
        COMMENT "Building python documentation with sphinx"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
    add_dependencies(doc pydoc)
    add_dependencies(pydoc segyio-python)

    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/segyio
    )
endif()

if(NOT BUILD_TESTING)
    return()
endif()

add_test(
    NAME python.unit
    COMMAND ${PYTHON_EXECUTABLE} -m pytest test/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(../test-data/small.sgy write.sgy COPYONLY)

add_test(
    NAME python.example.about
    COMMAND ${PYTHON_EXECUTABLE} -m examples.about
                ../test-data/small.sgy INLINE_3D CROSSLINE_3D
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.write
    COMMAND ${PYTHON_EXECUTABLE} -m examples.write
                ${CMAKE_CURRENT_BINARY_DIR}/write.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.makefile
    COMMAND ${PYTHON_EXECUTABLE} -m examples.make-file
                ../test-data/large-file.sgy 20 1 20 1 20
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.makepsfile
    COMMAND ${PYTHON_EXECUTABLE} -m examples.make-ps-file
                ../test-data/small-prestack.sgy 10 1 5 1 4 1 3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.subcube
    COMMAND ${PYTHON_EXECUTABLE} -m examples.copy-sub-cube
                ../test-data/small.sgy ${CMAKE_CURRENT_BINARY_DIR}/copy.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.rotate
    COMMAND ${PYTHON_EXECUTABLE} -m examples.make-rotated-copies
                ../test-data/small.sgy ex-rotate.sgy ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.scan_min_max
    COMMAND ${PYTHON_EXECUTABLE} -m examples.scan_min_max
                ../test-data/small.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.multi-text
    COMMAND ${PYTHON_EXECUTABLE} -m examples.make-multiple-text
                _skbuild/multi-text.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
