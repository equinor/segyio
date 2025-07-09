cmake_minimum_required(VERSION 3.18)
project(segyio-python LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)

find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
# If we execute CMake via a scikit-build-core, i.e., `python -m build`, `pip
# install` etc., we expect that the segyio library is available on the system.
if (SKBUILD)
    find_package(segyio REQUIRED)
endif()

python_add_library(_segyio MODULE WITH_SOABI segyio/segyio.cpp)
target_link_libraries(_segyio PRIVATE segyio::segyio)

if (MSVC)
    target_compile_options(_segyio
        BEFORE
        PRIVATE
            /EHsc
    )
endif ()

if (SKBUILD)
    set(SEGYIO_PYTHON_INSTALL_DIR segyio)
else()
    # CMake defines Python_SITELIB that points to the system path for Python
    # libraries, but we cannot use it here. In case of a non-empty installation
    # prefix, CMake would merge the installation path into something like
    # ${CMAKE_INSTALL_PREFIX}/${Python_SITELIB} which would lead to the wrong
    # full path. For example, it could look like this:
    #
    # /usr/local/usr/lib/python3.12/site-packages
    #   PREFIX  |          SITELIB
    #
    # This is not a path one would expect the Python library to be in.
    set(SEGYIO_PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/segyio)
endif()

install(TARGETS _segyio LIBRARY DESTINATION ${SEGYIO_PYTHON_INSTALL_DIR})

# Emulate in-place build behavior of invoking `python setup.py build_ext -i`.
# This places the binding library into the `python/segyio/` directory. Doing so
# allows the usage of `ctest` to test the core library, but also the Python
# package.
if (MSVC)
    # On Windows, setting the target properties does not work. Whatever is
    # built on Windows does not fullfil CMake's definition of "library",
    # "archive" or other outputs that for which we could set the output
    # directory as target property. Therefore, we copy the library into the
    # correct location.
    add_custom_command(TARGET _segyio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:_segyio> ${CMAKE_CURRENT_SOURCE_DIR}/segyio
    )
else()
    set_target_properties(_segyio PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/segyio
    )
endif()

# If CMake is run via a scikit-build-core, we have set up everything needed and
# we can return from this file. The installation and collection of relevant
# files is handled by scikit-build-core. Other options, like building the
# documentation, are not available via scikit-build-core.
if (SKBUILD)
    return()
endif()

# Explicitly install Python files to avoid copying files that are not required,
# e.g., segyio.cpp or folder like __pycache__ that exist if the tests were run
# via ctest.
install(
    DIRECTORY segyio/
    DESTINATION ${SEGYIO_PYTHON_INSTALL_DIR}
    FILES_MATCHING PATTERN "*.py"
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
        COMMAND ${Python_EXECUTABLE} -m sphinx
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
    COMMAND ${Python_EXECUTABLE} -m pytest test/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

configure_file(../test-data/small.sgy write.sgy COPYONLY)

add_test(
    NAME python.example.about
    COMMAND ${Python_EXECUTABLE} -m examples.about
                ../test-data/small.sgy INLINE_3D CROSSLINE_3D
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.write
    COMMAND ${Python_EXECUTABLE} -m examples.write
                ${CMAKE_CURRENT_BINARY_DIR}/write.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.makefile
    COMMAND ${Python_EXECUTABLE} -m examples.make-file
                ../test-data/large-file.sgy 20 1 20 1 20
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.makepsfile
    COMMAND ${Python_EXECUTABLE} -m examples.make-ps-file
                ../test-data/small-prestack.sgy 10 1 5 1 4 1 3
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.subcube
    COMMAND ${Python_EXECUTABLE} -m examples.copy-sub-cube
                ../test-data/small.sgy ${CMAKE_CURRENT_BINARY_DIR}/copy.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.rotate
    COMMAND ${Python_EXECUTABLE} -m examples.make-rotated-copies
                ../test-data/small.sgy ex-rotate.sgy ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.scan_min_max
    COMMAND ${Python_EXECUTABLE} -m examples.scan_min_max
                ../test-data/small.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
add_test(
    NAME python.example.multi-text
    COMMAND ${Python_EXECUTABLE} -m examples.make-multiple-text
                ${CMAKE_CURRENT_BINARY_DIR}/multi-text.sgy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
