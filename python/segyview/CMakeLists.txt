set(PYTHON_SOURCES
    __init__.py
    linenavigationbar.py
    segyiowrapper.py
    segyplot.py
    slicewidget.py
    )

add_python_package(segyview segyview "${PYTHON_SOURCES}")
