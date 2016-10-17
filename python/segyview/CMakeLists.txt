set(PYTHON_SOURCES
    __init__.py
    linenavigationbar.py
    segyplot.py
    slicewidget.py
    )

add_python_package(segyview segyview "${PYTHON_SOURCES}")
