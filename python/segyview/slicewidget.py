from PyQt4 import QtGui, QtCore

from segyplot import SegyPlot, ColorBarPlot
from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure


class SliceWidget(QtGui.QWidget):
    """
    Main widget holding the slice matplotlib Figure wrapped in FigureCanvasQTAgg.
    """
    index_changed = QtCore.pyqtSignal(int)

    def __init__(self, slices, indexes, dataset_title=None, default_cmap='seismic',
                 x_axis_indexes=None, y_axis_indexes=None,
                 show_h_indicator=False, show_v_indicator=False, v_min_max=None):
        super(SliceWidget, self).__init__()

        self.slices = slices
        self.indexes = indexes

        self.x_axis_name, self.x_axis_indexes = x_axis_indexes or (None, None)
        self.y_axis_name, self.y_axis_indexes = y_axis_indexes or (None, None)

        self.default_cmap = default_cmap

        self.show_h_indicator = show_h_indicator
        self.show_v_indicator = show_v_indicator

        self.palette().setColor(self.backgroundRole(), QtCore.Qt.white)

        self.current_index = 0

        # setting up the figure and canvas
        self.figure = Figure(figsize=(16, 4), dpi=100, facecolor='white')

        self.axes = self.figure.add_subplot(111)

        self.segy_plot = SegyPlot(self.slices,
                                  self.indexes,
                                  self.axes,
                                  self.default_cmap,
                                  x_axis_indexes=x_axis_indexes,
                                  y_axis_indexes=y_axis_indexes,
                                  display_horizontal_indicator=self.show_h_indicator,
                                  display_vertical_indicator=self.show_v_indicator,
                                  v_min_max=v_min_max)

        self.figure_canvas = FigureCanvas(self.figure)
        self.figure_canvas.setParent(self)

        # connect to mouse click events
        self.figure_canvas.mpl_connect('button_press_event', self._mouse_clicked)

        # widget layout
        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.figure_canvas)

    def _mouse_clicked(self, evt):
        if evt.inaxes is not None:
            self.current_index = int(evt.xdata)
            self._signal_index_change(self.current_index)

    def _signal_index_change(self, x):
        if self.x_axis_indexes is not None:
            self.index_changed.emit(self.x_axis_indexes[x])

    def update_image(self, index):
        self.segy_plot.update_image(index)
        self.figure_canvas.draw()

    def set_cmap(self, cmap):
        self.segy_plot.set_colormap(str(cmap))
        self.figure_canvas.draw()

    def set_min_max(self, min_max):
        self.segy_plot.set_min_max(min_max)
        self.figure_canvas.draw()

    def set_vertical_line_indicator(self, line):
        self.segy_plot.set_vertical_line_indicator(line)
        self.figure_canvas.draw()

    def set_horizontal_line_indicator(self, line):
        self.segy_plot.set_horizontal_line_indicator(line)
        self.figure_canvas.draw()



class ColorBarWidget(QtGui.QWidget):
    """
    Widget displaying a colorbar, with the selected min-max range and colormap
    """
    def __init__(self, segyio_wrapper, colormap_monitor=None):
        super(ColorBarWidget, self).__init__()

        self.swrap = segyio_wrapper

        # setting up the colorbar figure
        self.colormap_monitor = colormap_monitor
        self.figure = Figure(figsize=(1, 3), dpi=50, facecolor='white')
        self.axes = self.figure.add_subplot(111)
        self.cbar_plt = ColorBarPlot(self.axes, cmap='seismic', v_min_max=self.swrap.min_max)
        self.figure_canvas = FigureCanvas(self.figure)
        self.figure_canvas.setParent(self)

        # signals
        self.colormap_monitor.cmap_changed.connect(self.set_cmap)

        self.colormap_monitor.min_max_changed.connect(self.set_min_max)

        self.layout = QtGui.QVBoxLayout(self)
        self.layout.addWidget(self.figure_canvas)


    def set_cmap(self,value):
        self.cbar_plt.set_cmap(str(value))
        self.figure_canvas.draw()

    def set_min_max(self, min_max):
        self.cbar_plt.set_min_max(min_max)
        self.figure_canvas.draw()

