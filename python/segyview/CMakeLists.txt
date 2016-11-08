set(PYTHON_SOURCES
    __init__.py
    controlwidgets.py
    progresswidget.py
    segyiowrapper.py
    segyplot.py
    slicewidget.py
    viewer.py
    )

add_python_package(segyview segyview "${PYTHON_SOURCES}")
