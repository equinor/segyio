#!/usr/bin/env python

import sys

import numpy as np
import segyio
from PyQt4 import QtGui, QtCore
from segyview import *


class LineSelectionMonitor(QtCore.QObject):
    ilineChanged = QtCore.pyqtSignal(int)
    xlineChanged = QtCore.pyqtSignal(int)
    depthChanged = QtCore.pyqtSignal(int)

    def __init__(self, parent):
        QtCore.QObject.__init__(self, parent)

    def ilineUpdated(self, new_index):
        self.ilineChanged.emit(new_index)

    def xlineUpdated(self, new_index):
        self.xlineChanged.emit(new_index)

    def depthUpdated(self, new_index):
        self.depthChanged.emit(new_index)


class ColorMapMonitor(QtCore.QObject):
    cmap_changed = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

    def colormapUpdated(self, value):
        self.cmap_changed.emit(str(value))


def configure_main_menu(menu, colormap_monitor):
    menu.fileMenu = menu.addMenu('&File')
    exitAction = QtGui.QAction('&Exit', menu)
    menu.fileMenu.addAction(exitAction)

    menu.viewMenu = menu.addMenu('&View')
    menu.colormapMenu = menu.viewMenu.addMenu("&Colormap")

    def set_selected_cmap(cmap_name):
        for item in menu.colormapMenu.actions():
            item.setChecked(str(item.text()) == cmap_name)

    colormap_monitor.cmap_changed.connect(set_selected_cmap)

    def colormapChanger(color_map_name):
        def performColorMapChange():
            colormap_monitor.colormapUpdated(color_map_name)

        return performColorMapChange

    for item in ['seismic', 'spectral', 'RdGy', 'hot', 'jet', 'gray']:
        action = menu.colormapMenu.addAction(item)
        action.setCheckable(True)
        action.triggered.connect(colormapChanger(item))


class SegyViewer(QtGui.QMainWindow):
    def __init__(self, s):
        QtGui.QMainWindow.__init__(self)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowTitle("SegyViewer")
        main_widget = QtGui.QWidget(self)

        # signal monitors
        colormap_monitor = ColorMapMonitor(self)
        line_monitor = LineSelectionMonitor(self)

        #menus
        configure_main_menu(self.menuBar(),colormap_monitor)

        self.addToolBar(LineNavigationBar(s.xlines, s.ilines, range(s.samples), line_monitor))
        self.statusBar()

        depth_slices, min_max = read_traces_to_memory(s)

        # initialize
        x_slice_widget = SliceWidget(s.xline, s.xlines,
                                     x_axis_indexes=('i-lines', s.ilines),
                                     y_axis_indexes=('depth', range(s.samples)),
                                     show_v_indicator=True,
                                     v_min_max=min_max)
        i_slice_widget = SliceWidget(s.iline, s.ilines,
                                     x_axis_indexes=('x-lines', s.xlines),
                                     y_axis_indexes=('depth', range(s.samples)),
                                     show_v_indicator=True,
                                     v_min_max=min_max)
        depth_slice_widget = SliceWidget(depth_slices, range(s.samples),
                                         x_axis_indexes=('i-lines', s.ilines),
                                         y_axis_indexes=('x-lines', s.xlines),
                                         show_v_indicator=True,
                                         show_h_indicator=True,
                                         v_min_max=min_max)

        # attach signals
        x_slice_widget.indexChanged.connect(line_monitor.ilineUpdated)
        i_slice_widget.indexChanged.connect(line_monitor.xlineUpdated)

        line_monitor.ilineChanged.connect(x_slice_widget.set_vertical_line_indicator)
        line_monitor.ilineChanged.connect(depth_slice_widget.set_vertical_line_indicator)
        line_monitor.ilineChanged.connect(i_slice_widget.update_image)

        line_monitor.xlineChanged.connect(i_slice_widget.set_vertical_line_indicator)
        line_monitor.xlineChanged.connect(depth_slice_widget.set_horizontal_line_indicator)
        line_monitor.xlineChanged.connect(x_slice_widget.update_image)

        line_monitor.depthChanged.connect(depth_slice_widget.update_image)

        # colormap signals
        colormap_monitor.cmap_changed.connect(x_slice_widget.set_cmap)
        colormap_monitor.cmap_changed.connect(i_slice_widget.set_cmap)
        colormap_monitor.cmap_changed.connect(depth_slice_widget.set_cmap)

        # layout
        xdock = QtGui.QDockWidget("x-line")
        xdock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        xdock.setWidget(x_slice_widget)

        idock = QtGui.QDockWidget("i-line")
        idock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        idock.setWidget(i_slice_widget)

        ddock = QtGui.QDockWidget("depth slice")
        ddock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        ddock.setWidget(depth_slice_widget)

        self.setDockOptions(QtGui.QMainWindow.AllowNestedDocks)
        self.addDockWidget(QtCore.Qt.TopDockWidgetArea, xdock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, idock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, ddock)

        main_widget.setFocus()
        self.setCentralWidget(main_widget)
        main_widget.hide()


def read_traces_to_memory(segy):
    ''' read all samples into memory and identify min and max. A temporary utility method to handle the
    challenge of navigating  up and down in depth slices. As each depth slice consist of samples from all traces in
    the file '''
    all_traces = np.empty(shape=((len(segy.ilines) * len(segy.xlines)), segy.samples), dtype=np.float32)

    min_value = sys.float_info.max
    max_value = sys.float_info.min

    for i, t in enumerate(segy.trace):
        all_traces[i] = t

        local_min = np.nanmin(t)
        local_max = np.nanmax(t)

        if np.isfinite(local_min):
            min_value = min(local_min, min_value)

        if np.isfinite(local_max):
            max_value = max(local_max, max_value)

    all_traces2 = all_traces.reshape(len(segy.ilines), len(segy.xlines), segy.samples)

    transposed_traces = all_traces2.transpose(2, 0, 1)

    return transposed_traces, (min_value, max_value)


def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: view.py [file]")

    filename = sys.argv[1]

    with segyio.open(filename, "r") as s:
        qApp = QtGui.QApplication(sys.argv)
        aw = SegyViewer(s)
        aw.show()
        sys.exit(qApp.exec_())


if __name__ == '__main__':
    main()
