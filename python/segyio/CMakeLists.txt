set(PYTHON_SOURCES
    __init__.py
    segy.py
    tracefield.py
    binfield.py
    open.py
    create.py
    segysampleformat.py
    tracesortingformat.py)

add_python_package(segyio segyio "${PYTHON_SOURCES}")
