project(segyio-python)

if (NOT BUILD_PYTHON)
    return()
endif()

include(python)

if (NOT DEFINED PYTHON_EXECUTABLE)
    message("Python interpreter not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()


python_module(numpy REQUIRED 1.6)
if (NOT DEFINED PY_numpy)
    message("numpy module not found - Python wrappers not enabled")
    unset(BUILD_PYTHON)
    return()
endif()

if (NOT PYTHONLIBS_FOUND)
    message("Python libraries not found.")
    unset(BUILD_PYTHON)
    return()
endif()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")
endif()

add_library(_segyio MODULE segyio/_segyio.c)
target_include_directories(_segyio PRIVATE ${PYTHON_INCLUDE_DIRS})
target_link_libraries(_segyio segyio ${PYTHON_LIBRARIES})
export(TARGETS _segyio segyio-shared segyio-static FILE segyio-config.cmake)

add_python_package(pysegyio segyio
                    TARGETS _segyio
                    SOURCES segyio/__init__.py
                            segyio/_header.py
                            segyio/_gather.py
                            segyio/_line.py
                            segyio/_field.py
                            segyio/_trace.py
                            segyio/_raw_trace.py
                            segyio/segy.py
                            segyio/tracefield.py
                            segyio/binfield.py
                            segyio/open.py
                            segyio/tools.py
                            segyio/create.py
                            segyio/segysampleformat.py
                            segyio/tracesortingformat.py)

file(GLOB sgys "${CMAKE_SOURCE_DIR}/test-data/*.sgy")
foreach (sgy ${sgys})
    get_filename_component(fl ${sgy} NAME)
    configure_file(${sgy} test-data/${fl} COPYONLY)
endforeach ()

configure_file(test/test_context.py test/test_context.py COPYONLY)
add_python_test(python.segy         test/segy.py)
add_python_test(python.h.segy       test/segyio_c.py)
add_python_test(python.enum.segy    test/segyioenum.py)
add_python_test(python.tools        test/tools.py)
add_python_test(python.context      test/context.py)

configure_file(${CMAKE_SOURCE_DIR}/test-data/small.sgy
               test-data/write.sgy
               COPYONLY)
add_python_example(pysegyio python.example.about         examples/about.py test-data/small.sgy INLINE_3D CROSSLINE_3D)
add_python_example(pysegyio python.example.write         examples/write.py test-data/write.sgy)
add_python_example(pysegyio python.example.makefile      examples/make-file.py test-data/large-file.sgy 20 1 20 1 20)
add_python_example(pysegyio python.example.makepsfile    examples/make-ps-file.py test-data/small-prestack.sgy 10 1 5 1 4 1 3)
add_python_example(pysegyio python.example.subcube       examples/copy-sub-cube.py test-data/small.sgy test-data/copy.sgy)
add_python_example(pysegyio python.example.scan_min_max  examples/scan_min_max.py test-data/small.sgy)
