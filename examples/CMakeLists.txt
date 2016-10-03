configure_file(../tests/test-data/small.sgy test-data/small.sgy COPYONLY)
configure_file(segyviewer.py segyviewer.py COPYONLY)

add_python_example(python.examples.about about.py test-data/small.sgy INLINE_3D CROSSLINE_3D)
add_python_example(python.examples.write write.py test-data/small.sgy)
add_python_example(python.examples.makefile make-file.py test-data/large-file.sgy 20 1 20 1 20)
add_python_example(python.examples.subcube copy-sub-cube.py test-data/small.sgy test-data/copy.sgy)
add_python_example(python.examples.scan_min_max scan_min_max.py test-data/small.sgy)
