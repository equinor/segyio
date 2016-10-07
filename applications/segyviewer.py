#!/usr/bin/env python

import segyio

from pylab import *
from PyQt4 import QtGui, QtCore
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
import matplotlib.patches as patches


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


class PlaneCanvas(FigureCanvas):
    """
    Generic plot canvas for all plane views
    """
    indexChanged = QtCore.pyqtSignal(int)

    def __init__(self, planes, indexes, dataset_title=None, cmap='seismic', x_axis_indexes=None, y_axis_indexes=None,
                 display_horizontal_indicator=False,
                 display_vertical_indicator=False, parent=None, width=800, height=100, dpi=20, v_min_max=None):

        self.fig = Figure(figsize=(width, height), dpi=dpi, facecolor='white')
        FigureCanvas.__init__(self, self.fig)

        self.planes = planes
        self.indexes = indexes
        self.cmap = cmap

        self.plane_height = len(self.planes[self.indexes[0]][0])
        self.plane_width = len(self.planes[self.indexes[0]][:])

        self.x_axis_name, self.x_axis_indexes = x_axis_indexes or (None, None)
        self.y_axis_name, self.y_axis_indexes = y_axis_indexes or (None, None)

        self.current_index = 0

        self.setParent(parent)

        self.axes = self.fig.add_subplot(111)
        self.axes.tick_params(axis='both', labelsize=30)

        if self.x_axis_indexes:
            def x_axis_label_formatter(val, position):
                if val >= 0 and val < len(self.x_axis_indexes):
                    return self.x_axis_indexes[int(val)]
                return ''

            self.axes.set_xlabel(self.x_axis_name, fontsize=40)
            self.axes.get_xaxis().set_major_formatter(FuncFormatter(x_axis_label_formatter))
            self.axes.get_xaxis().set_major_locator(MaxNLocator(20))  # max 20 ticks are shown

        if self.y_axis_indexes:
            def y_axis_label_formatter(val, position):
                if val >= 0 and val < len(self.y_axis_indexes):
                    return self.y_axis_indexes[int(val)]
                return ''

            self.axes.set_ylabel(self.y_axis_name, fontsize=40)
            self.axes.get_yaxis().set_major_formatter(FuncFormatter(y_axis_label_formatter))
            self.axes.get_yaxis().set_major_locator(MaxNLocator(20))  # max 20 ticks are shown

        self.im = self.axes.imshow(planes[indexes[0]].T,
                                   interpolation="nearest",
                                   aspect="auto",
                                   cmap=self.cmap,
                                   vmin=v_min_max[0],
                                   vmax=v_min_max[1])

        # connecting matplotlib mouse signals
        self.mpl_connect('motion_notify_event', self.mouse_moved)
        self.mpl_connect('button_press_event', self.mouse_clicked)
        self.mpl_connect('axes_leave_event', self.mouse_left)

        if display_vertical_indicator:
            self.verdical_indicator_rect = self.axes.add_patch(
                patches.Rectangle(
                    (-0.5, -0.5),
                    1,
                    self.plane_height,
                    fill=False,
                    alpha=1,
                    color='black',
                    linestyle='--',
                    linewidth=2
                )
            )

        if display_horizontal_indicator:
            self.horizontal_indicator_rect = self.axes.add_patch(
                patches.Rectangle(
                    (-0.5, -0.5),
                    self.plane_width,
                    1,
                    fill=False,
                    alpha=1,
                    color='black',
                    linestyle='--',
                    linewidth=2
                )
            )

        self.disabled_overlay = self.axes.add_patch(
            patches.Rectangle(
                (-0.5, -0.5),  # (x,y)
                len(self.planes[self.indexes[0]][0]),
                len(self.planes[self.indexes[0]][0]),
                alpha=0.5,
                color='gray',
                visible=False
            )
        )

    def mouse_left(self, evt):
        # for now do nothing
        # self.set_vertical_line_indicator(self.current_index)
        pass

    def mouse_clicked(self, evt):
        if evt.inaxes:
            self.current_index = int(evt.xdata)
            self.indexChanged.emit(self.x_axis_indexes[int(evt.xdata)])

    def mouse_moved(self, evt):
        # for now do nothing
        # if evt.inaxes:
        #   self.set_vertical_line_indicator(int(evt.xdata))
        pass

    def set_colormap(self, cmap):
        self.cmap = cmap
        self.im.set_cmap(cmap)
        self.draw()

    def update_image(self, index):
        self.im.set_data(self.planes[index].T)
        self.draw()

    def set_vertical_line_indicator(self, line_index):
        self.verdical_indicator_rect.set_x(self.x_axis_indexes.index(line_index) - 0.5)
        self.draw()

    def set_horizontal_line_indicator(self, line_index):
        self.horizontal_indicator_rect.set_y(self.y_axis_indexes.index(line_index) - 0.5)
        self.draw()

    def enable_overlay(self):
        self.disabled_overlay.set_visible(True)
        self.draw()

    def disable_overlay(self):
        self.disabled_overlay.set_visible(False)
        self.draw()


class PlotWidget(QtGui.QWidget):
    """
    Main widget holding the figure and slider
    """

    def __init__(self, planes, indexes, dataset_title=None, default_cmap='seismic',
                 x_axis_indexes=None, y_axis_indexes=None,
                 show_h_indicator=False, show_v_indicator=False, v_min_max=None):
        super(PlotWidget, self).__init__()

        self.planes = planes
        self.indexes = indexes

        self.default_cmap = default_cmap
        self.show_h_indicator = show_h_indicator
        self.show_v_indicator = show_v_indicator
        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QtCore.Qt.white)
        self.setPalette(p)

        self.plotCanvas = PlaneCanvas(self.planes, self.indexes, dataset_title, self.default_cmap,
                                      x_axis_indexes=x_axis_indexes, y_axis_indexes=y_axis_indexes,
                                      display_horizontal_indicator=self.show_h_indicator,
                                      display_vertical_indicator=self.show_v_indicator, v_min_max=v_min_max)

        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.plotCanvas)

    def set_cmap(self, action):
        self.plotCanvas.set_colormap(str(action))

    def set_vertical_line_indicator(self, line):
        self.plotCanvas.set_vertical_line_indicator(line)

    def set_horizontal_line_indicator(self, line):
        self.plotCanvas.set_horizontal_line_indicator(line)


class TopMenu(QtGui.QMenuBar):
    def __init__(self, parent, colormap_monitor):
        super(QtGui.QMenuBar, self).__init__(parent)

        self.fileMenu = self.addMenu('&File')
        exitAction = QtGui.QAction('&Exit', self)
        self.fileMenu.addAction(exitAction)

        self.viewMenu = self.addMenu('&View')
        self.colormapMenu = self.viewMenu.addMenu("&Colormap")

        self.colormap_monitor = colormap_monitor

        self.colormap_monitor.cmap_changed.connect(self.set_selected_cmap)

        def colormapChanger(color_map_name):
            def performColorMapChange():
                self.colormap_monitor.colormapUpdated(color_map_name)

            return performColorMapChange

        for item in ['seismic', 'spectral', 'RdGy', 'hot', 'jet', 'gray']:
            action = self.colormapMenu.addAction(item)
            action.setCheckable(True)
            action.triggered.connect(colormapChanger(item))

    def set_selected_cmap(self, cmap_name):
        for item in self.colormapMenu.actions():
            item.setChecked(str(item.text()) == cmap_name)


class LineSelector(QtGui.QWidget):
    indexChanged = QtCore.pyqtSignal(int)

    def __init__(self, parent, label, indexes, monitor_func):
        super(QtGui.QWidget, self).__init__(parent)
        self.label = label
        self.indexes = indexes
        self.monitor_func = monitor_func

        self.layout = QtGui.QHBoxLayout()
        self.slabel = QtGui.QLabel(self.label)
        self.sbox = QtGui.QSpinBox(self)
        self.sbox.setRange(self.indexes[0], self.indexes[-1])
        self.sbox.valueChanged.connect(self.monitor_func)
        self.layout.addWidget(self.slabel)
        self.layout.addWidget(self.sbox)
        self.setLayout(self.layout)

    def index_changed(self, val):
        self.indexChanged.emit(val)

    def set_index(self, val):
        self.sbox.blockSignals(True)
        self.sbox.setValue(val)
        self.sbox.blockSignals(False)


class ToolBar(QtGui.QToolBar):
    def __init__(self, xline_indexes, iline_indexes, depth_indexes, line_selection_monitor):
        super(ToolBar, self).__init__("")
        self.xline_indexes = xline_indexes
        self.iline_indexes = iline_indexes
        self.depth_indexes = depth_indexes
        self.line_selection_monitor = line_selection_monitor

        # xline
        self.xline_selector = LineSelector(self, "x-line", self.xline_indexes, self.line_selection_monitor.xlineUpdated)
        self.line_selection_monitor.xlineChanged.connect(self.xline_selector.set_index)
        self.addWidget(self.xline_selector)

        # iline
        self.iline_selector = LineSelector(self, "i-line", self.iline_indexes, self.line_selection_monitor.ilineUpdated)
        self.line_selection_monitor.ilineChanged.connect(self.iline_selector.set_index)
        self.addWidget(self.iline_selector)

        # iline
        self.depth_selector = LineSelector(self, "depth", self.depth_indexes, self.line_selection_monitor.depthUpdated)
        self.addWidget(self.depth_selector)


class AppWindow(QtGui.QMainWindow):
    def __init__(self, s):
        QtGui.QMainWindow.__init__(self)
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        self.setWindowTitle("Segy Viewer")
        self.main_widget = QtGui.QWidget(self)

        # signal monitors
        colormap_monitor = ColorMapMonitor(self)
        line_monitor = LineSelectionMonitor(self)

        self.setMenuBar(TopMenu(self, colormap_monitor))
        self.addToolBar(ToolBar(s.xlines, s.ilines, range(s.samples), line_monitor))
        self.statusBar()

        depth_planes, min_max = read_traces_to_memory(s)

        # initialize
        x_plane_canvas = PlotWidget(s.xline, s.xlines,
                                    x_axis_indexes=('i-lines', s.ilines),
                                    y_axis_indexes=('depth', range(s.samples)),
                                    show_v_indicator=True,
                                    v_min_max=min_max)
        i_plane_canvas = PlotWidget(s.iline, s.ilines,
                                    x_axis_indexes=('x-lines', s.xlines),
                                    y_axis_indexes=('depth', range(s.samples)),
                                    show_v_indicator=True,
                                    v_min_max=min_max)
        depth_plane_canvas = PlotWidget(depth_planes, range(s.samples),
                                        x_axis_indexes=('i-lines', s.ilines),
                                        y_axis_indexes=('x-lines', s.xlines),
                                        show_v_indicator=True,
                                        show_h_indicator=True,
                                        v_min_max=min_max)

        # attach signals
        x_plane_canvas.plotCanvas.indexChanged.connect(line_monitor.ilineUpdated)
        i_plane_canvas.plotCanvas.indexChanged.connect(line_monitor.xlineUpdated)

        line_monitor.ilineChanged.connect(x_plane_canvas.set_vertical_line_indicator)
        line_monitor.ilineChanged.connect(depth_plane_canvas.set_vertical_line_indicator)
        line_monitor.ilineChanged.connect(i_plane_canvas.plotCanvas.update_image)

        line_monitor.xlineChanged.connect(i_plane_canvas.set_vertical_line_indicator)
        line_monitor.xlineChanged.connect(depth_plane_canvas.set_horizontal_line_indicator)
        line_monitor.xlineChanged.connect(x_plane_canvas.plotCanvas.update_image)

        line_monitor.depthChanged.connect(depth_plane_canvas.plotCanvas.update_image)

        # colormap signals
        colormap_monitor.cmap_changed.connect(x_plane_canvas.set_cmap)
        colormap_monitor.cmap_changed.connect(i_plane_canvas.set_cmap)
        colormap_monitor.cmap_changed.connect(depth_plane_canvas.set_cmap)

        # layout
        xdock = QtGui.QDockWidget("x-line")
        xdock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        xdock.setWidget(x_plane_canvas)

        idock = QtGui.QDockWidget("i-line")
        idock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        idock.setWidget(i_plane_canvas)

        ddock = QtGui.QDockWidget("depth plane")
        ddock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        ddock.setWidget(depth_plane_canvas)

        self.setDockOptions(QtGui.QMainWindow.AllowNestedDocks)
        self.addDockWidget(QtCore.Qt.TopDockWidgetArea, xdock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, idock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, ddock)

        self.main_widget.setFocus()
        self.setCentralWidget(self.main_widget)
        self.main_widget.hide()


def read_traces_to_memory(segy):
    ''' read all samples into memory and identify min and max'''
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
        sys.exit("Usage: segyviewer.py [file]")

    filename = sys.argv[1]

    with segyio.open(filename, "r") as s:
        qApp = QtGui.QApplication(sys.argv)
        aw = AppWindow(s)
        aw.show()
        sys.exit(qApp.exec_())


if __name__ == '__main__':
    main()
