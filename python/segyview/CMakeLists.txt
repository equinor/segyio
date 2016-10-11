set(PYTHON_SOURCES
    __init__.py
    linenavigationbar.py
    slicefigure.py
    slicewidget.py
    )

add_python_package(segyview segyview "${PYTHON_SOURCES}")
