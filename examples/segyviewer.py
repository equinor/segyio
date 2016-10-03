from PyQt4.QtGui import QAction

from PyQt4.QtCore import *
from PyQt4.QtGui import *

import segyio

from pylab import *

from PyQt4 import QtGui, QtCore

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

import matplotlib.patches as patches


class LineSelectionMonitor(QObject):
    ilineChanged = pyqtSignal(int)
    xlineChanged = pyqtSignal(int)
    depthChanged = pyqtSignal(int)

    def __init__(self, parent=None):
        QObject.__init__(self, parent)

    def ilineUpdated(self, new_index):
        print("iline:{0} updated", new_index)
        self.ilineChanged.emit(new_index)

    def xlineUpdated(self, new_index):
        print("xline:{0} updated", new_index)
        self.xlineChanged.emit(new_index)

    def depthUpdated(self, new_index):
        print("depth:{0} updated", new_index)
        self.depthChanged.emit(new_index)


class ColorMapMonitor(QObject):
    cmap_changed = pyqtSignal(str)

    def __init__(self, parent=None):
        QObject.__init__(self, parent)

    def colormapUpdated(self, value):
        self.cmap_changed.emit(str(value))


class PlotCanvas(FigureCanvas):
    """
    Generic plot canvas for all plane views
    """

    indexChanged = pyqtSignal(int)

    def __init__(self, planes, indexes, dataset_title, cmap, display_horizontal_indicator=False,
                 display_vertical_indicator=False, parent=None, width=800, height=100, dpi=20):

        self.planes = planes
        self.indexes = indexes

        self.dataset_title = dataset_title

        self.fig = Figure(figsize=(width, height), dpi=dpi, facecolor='white')
        FigureCanvas.__init__(self, self.fig)

        self.setParent(parent)

        self.axes = self.fig.add_subplot(111)
        # self.axes.set_title(self.dataset_title, fontsize=50)
        self.axes.set_xticks(indexes)
        self.axes.tick_params(axis='both', labelsize=30)

        # the default colormap
        self.cmap = cmap
        self.im = self.axes.imshow(planes[indexes[0]].T, interpolation="nearest", aspect="auto", cmap=self.cmap)

        # initialize plot and indicator_rect
        # self.plot_image(self.planes[self.indexes[0]])

        self.current_index = 0

        # connecting matplotlib mouse signals
        self.mpl_connect('motion_notify_event', self.mouse_moved)
        self.mpl_connect('button_press_event', self.mouse_clicked)
        self.mpl_connect('axes_leave_event', self.mouse_left)

        if display_vertical_indicator:
            self.verdical_indicator_rect = self.axes.add_patch(
                patches.Rectangle(
                    (-0.5, 0),  # (x,y)
                    1,  # width - bredde er pr dot ikke pixel...
                    len(self.planes[self.indexes[0]][0]),  # height / depth
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
                    (-0.5, 0),  # (x,y)
                    len(self.planes[self.indexes[0]][0]),  # width - bredde er pr dot ikke pixel...
                    1,  # height / depth
                    fill=False,
                    alpha=1,
                    color='black',
                    linestyle='--',
                    linewidth=2
                )
            )

        self.disabled_overlay = self.axes.add_patch(
            patches.Rectangle(
                (-0.5, 0),  # (x,y)
                len(self.planes[self.indexes[0]][0]),  # width - bredde er pr dot ikke pixel...
                len(self.planes[self.indexes[0]][0]),  # height / depth
                alpha=0.5,
                color='gray',
                visible=False
            )
        )

        # initialize plot
        print(self.dataset_title, self.cmap)

        # self.plot_image(planes[indexes[0]])

    def mouse_left(self, evt):
        pass
        # self.set_vertical_line_indicator(self.current_index)

    def mouse_clicked(self, evt):
        if evt.inaxes:
            self.current_index = int(evt.xdata)
            self.indexChanged.emit(self.indexes[int(evt.xdata)])

    def mouse_moved(self, evt):
        # do nothing
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
        self.verdical_indicator_rect.set_x(line_index - 0.5)
        self.verdical_indicator_rect.set_y(0)
        self.draw()

    def set_horizontal_line_indicator(self, line_index):
        self.horizontal_indicator_rect.set_x(-0.5)
        self.horizontal_indicator_rect.set_y(line_index)
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

    # indexChanged = pyqtSignal(int)

    def __init__(self, planes, indexes, dataset_title, default_cmap='seismic',
                 show_h_indicator=False, show_v_indicator=False):
        super(PlotWidget, self).__init__()

        self.planes = planes
        self.indexes = indexes
        self.dataset_title = dataset_title
        self.default_cmap = default_cmap
        self.show_h_indicator = show_h_indicator
        self.show_v_indicator = show_v_indicator
        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), Qt.white)
        self.setPalette(p)

        self.plotCanvas = PlotCanvas(self.planes, self.indexes, self.dataset_title, self.default_cmap,
                                     display_horizontal_indicator=self.show_h_indicator,
                                     display_vertical_indicator=self.show_v_indicator)

        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.plotCanvas)

    def set_cmap(self, action):
        self.plotCanvas.set_colormap(str(action))

    def set_vertical_line_indicator(self, line):
        print("set vertical line ind:", line)
        self.plotCanvas.set_vertical_line_indicator(line)

    def set_horizontal_line_indicator(self, line):
        print("set horizontal line ind:", line)
        self.plotCanvas.set_horizontal_line_indicator(line)


class TopMenu(QMenuBar):
    def __init__(self, parent, colormap_monitor):
        super(QMenuBar, self).__init__(parent)

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
    indexChanged = pyqtSignal(int)

    def __init__(self, parent, label, indexes, monitor_func):
        super(QtGui.QWidget, self).__init__(parent)
        self.label = label
        self.indexes = indexes
        self.monitor_func = monitor_func

        self.layout = QHBoxLayout()
        self.slabel = QLabel(self.label)
        self.sbox = QtGui.QSpinBox(self)
        self.sbox.setRange(self.indexes[0], self.indexes[-1])
        self.sbox.valueChanged.connect(self.monitor_func)
        self.layout.addWidget(self.slabel)
        self.layout.addWidget(self.sbox)
        self.setLayout(self.layout)

    def index_changed(self, val):
        self.indexChanged.emit(self.indexes[val])

    def set_index(self, val):
        self.sbox.setValue(val)


class ToolBar(QToolBar):
    def __init__(self, xline_indexes, iline_indexes, depth_indexes, line_selection_monitor):
        super(ToolBar, self).__init__("")
        self.xline_indexes = xline_indexes
        self.iline_indexes = iline_indexes
        self.depth_indexes = depth_indexes
        self.line_selection_monitor = line_selection_monitor

        # xline
        self.xline_selector = LineSelector(self, "xline", self.xline_indexes, self.line_selection_monitor.xlineUpdated)
        self.line_selection_monitor.xlineChanged.connect(self.xline_selector.set_index)
        self.addWidget(self.xline_selector)

        # iline
        self.iline_selector = LineSelector(self, "iline", self.iline_indexes, self.line_selection_monitor.ilineUpdated)
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

        # depth = s.samples
        # depth_plane = []  # [ : for s.xline[:,depth] in s.xline]
        # all_traces = np.empty(shape=((len(s.ilines) * len(s.xlines)), s.samples), dtype=np.float32)
        #
        # for i, t in enumerate(s.trace):
        #     all_traces[i] = t
        #
        # all_traces2 = all_traces.reshape(len(s.ilines), len(s.xlines), s.samples)
        #
        # all_traces3 = all_traces2.transpose(2, 0, 1)



        # initialize
        x_plane_canvas = PlotWidget(s.xline, s.xlines, "xlines", show_v_indicator=True)
        i_plane_canvas = PlotWidget(s.iline, s.ilines, "ilines", show_v_indicator=True)
        depth_plane_canvas = PlotWidget(s.depth_plane, range(s.samples), "depth",
                                        show_v_indicator=True, show_h_indicator=True)

        # attach signals
        x_plane_canvas.plotCanvas.indexChanged.connect(line_monitor.ilineUpdated)
        i_plane_canvas.plotCanvas.indexChanged.connect(line_monitor.xlineUpdated)

        line_monitor.ilineChanged.connect(x_plane_canvas.set_vertical_line_indicator)
        line_monitor.ilineChanged.connect(depth_plane_canvas.set_horizontal_line_indicator)
        line_monitor.ilineChanged.connect(i_plane_canvas.plotCanvas.update_image)

        line_monitor.xlineChanged.connect(i_plane_canvas.set_vertical_line_indicator)
        line_monitor.xlineChanged.connect(depth_plane_canvas.set_vertical_line_indicator)
        line_monitor.xlineChanged.connect(x_plane_canvas.plotCanvas.update_image)

        line_monitor.depthChanged.connect(depth_plane_canvas.plotCanvas.update_image)

        # colormap signals
        colormap_monitor.cmap_changed.connect(x_plane_canvas.set_cmap)
        colormap_monitor.cmap_changed.connect(i_plane_canvas.set_cmap)
        colormap_monitor.cmap_changed.connect(depth_plane_canvas.set_cmap)

        # layout
        xdock = QDockWidget("x-lines")
        xdock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        xdock.setWidget(x_plane_canvas)

        idock = QDockWidget("i-lines")
        idock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        idock.setWidget(i_plane_canvas)

        ddock = QDockWidget("depth plane")
        ddock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        ddock.setWidget(depth_plane_canvas)

        self.setDockOptions(QMainWindow.AllowNestedDocks)
        self.addDockWidget(Qt.TopDockWidgetArea, xdock)
        self.addDockWidget(Qt.BottomDockWidgetArea, idock)
        self.addDockWidget(Qt.BottomDockWidgetArea, ddock)

        self.main_widget.setFocus()
        self.setCentralWidget(self.main_widget)
        self.main_widget.hide()


def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: segyviewer.py [file]")

    filename = sys.argv[1]

    # read the file given by cmd arg
    with segyio.open(filename, "r") as s:
        qApp = QtGui.QApplication(sys.argv)
        aw = AppWindow(s)
        aw.show()
        sys.exit(qApp.exec_())


if __name__ == '__main__':
    main()
