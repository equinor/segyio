set(PYTHON_SOURCES
    __init__.py
    linenavigationbar.py
    segyplot.py
    slicewidget.py
    util.py
    )

add_python_package(segyview segyview "${PYTHON_SOURCES}")
