# make the examples directory a module
#
# all the scripts in this directory are meant to be run as scripts, and just to
# demonstrate how to write certain programs. By adding __init__ and making this
# dir a module, cmake can run them as a part of the test step, without
# modification of the PYTHONPATH variable
