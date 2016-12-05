set(PYTHON_SOURCES
        __init__.py
        _header.py
        _line.py
        _field.py
        _trace.py
        _raw_trace.py
        segy.py
        tracefield.py
        binfield.py
        open.py
        tools.py
        create.py
        segysampleformat.py
        tracesortingformat.py)

add_python_package(segyio segyio "${PYTHON_SOURCES}")
