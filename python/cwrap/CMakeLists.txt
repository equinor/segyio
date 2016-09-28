set(PYTHON_SOURCES
    __init__.py
    basecclass.py
    basecenum.py
    basecvalue.py
    metacwrap.py
    prototype.py
)

add_python_package(cwrap cwrap "${PYTHON_SOURCES}")