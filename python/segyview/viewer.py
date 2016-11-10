from segyview import SegyIOWrapper, SliceWidget
from PyQt4 import QtGui, QtCore


class LineSelectionMonitor(QtCore.QObject):
    iline_changed = QtCore.pyqtSignal(int)
    xline_changed = QtCore.pyqtSignal(int)
    depth_changed = QtCore.pyqtSignal(int)

    def __init__(self, parent):
        QtCore.QObject.__init__(self, parent)

    def iline_updated(self, new_index):
        self.iline_changed.emit(new_index)

    def xline_updated(self, new_index):
        self.xline_changed.emit(new_index)

    def depth_updated(self, new_index):
        self.depth_changed.emit(new_index)


class SliceViewMonitor(QtCore.QObject):
    cmap_changed = QtCore.pyqtSignal(str)
    min_max_changed = QtCore.pyqtSignal(object)

    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

    def colormap_updated(self, value):
        self.cmap_changed.emit(str(value))

    def min_max_updated(self, values):
        self.min_max_changed.emit(tuple(map(float, values)))


class FileActivityMonitor(QtCore.QObject):
    started = QtCore.pyqtSignal()
    finished = QtCore.pyqtSignal()
    cancelled_operation = False

    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)
        self.cancelled_operation = False

    def reset(self):
        self.cancelled_operation = False

    def set_file_read_started(self):
        self.started.emit()

    def set_file_read_finished(self):
        self.finished.emit()

    def set_cancel_operation(self):
        self.finished.emit()
        self.cancelled_operation = True


class FileLoaderWorker(QtCore.QObject):
    finished = QtCore.pyqtSignal(int)
    progress = QtCore.pyqtSignal(int)

    def __init__(self, segyio_wrapper):
        QtCore.QObject.__init__(self)
        self.segyio_wrapper = segyio_wrapper

    def load_file(self):

        if self.segyio_wrapper.read_all_traces_to_memory(progress_callback=self.progress.emit):
             self.finished.emit(0)
        else:
            self.finished.emit(1)

        return None


class View(object):
    """ A container for a standalone pre defined three-slices view, wrapped in a single QWidget. Plus monitor instances
    for qt-signaling GUI events either from or to the slice viewers.

    The widget, and monitors are provided through the class properties.
    """

    def __init__(self, segy, read_file_to_memory=False):

        self.segy = segy

        self._file_activity_monitor = FileActivityMonitor()

        self._swrap = SegyIOWrapper.wrap(segy, self.file_activity_monitor)

        if read_file_to_memory:
            self._swrap.read_all_traces_to_memory()

        self._main_widget = QtGui.QWidget()
        self._line_selection_monitor = LineSelectionMonitor(self._main_widget)
        self._sliceview_monitor = SliceViewMonitor(self._main_widget)

        x_slice_widget, i_slice_widget, depth_slice_widget = initialize_slice_widgets(self._swrap,
                                                                                      self._line_selection_monitor,
                                                                                      self._sliceview_monitor)
        # layout for the single parent widget
        top_row = QtGui.QHBoxLayout()
        top_row.addWidget(x_slice_widget, 0)
        top_row.addWidget(depth_slice_widget, 0)

        bottom_row = QtGui.QHBoxLayout()
        bottom_row.addWidget(i_slice_widget)

        layout = QtGui.QVBoxLayout()
        layout.addLayout(top_row)
        layout.addLayout(bottom_row)


        self._main_widget.setLayout(layout)

    @property
    def main_widget(self):
        return self._main_widget

    @property
    def line_selection_monitor(self):
        return self._line_selection_monitor

    @property
    def colormap_monitor(self):
        return self._sliceview_monitor

    @property
    def file_activity_monitor(self):
        return self._file_activity_monitor


def initialize_slice_widgets(segyio_wrapper, line_selection_monitor, sliceview_monitor):
    """
    Given a segio_wrapper, and signal monitors, sliceviewer widgets for all three slices in a segy-cube are created.
    :param segyio_wrapper:
    :param line_selection_monitor:
    :param sliceview_monitor:
    :return: three QWidgets for the three segy slices, in x, i and depth slice order.
    """

    # initialize slice widgets
    x_slice_widget = SliceWidget(segyio_wrapper.xline, segyio_wrapper.xlines,
                                 x_axis_indexes=('i-lines', segyio_wrapper.ilines),
                                 y_axis_indexes=('depth', range(segyio_wrapper.samples)),
                                 show_v_indicator=True,
                                 v_min_max=segyio_wrapper.min_max)

    i_slice_widget = SliceWidget(segyio_wrapper.iline, segyio_wrapper.ilines,
                                 x_axis_indexes=('x-lines', segyio_wrapper.xlines),
                                 y_axis_indexes=('depth', range(segyio_wrapper.samples)),
                                 show_v_indicator=True,
                                 v_min_max=segyio_wrapper.min_max)

    depth_slice_widget = SliceWidget(segyio_wrapper.depth_slice, range(segyio_wrapper.samples),
                                     x_axis_indexes=('i-lines', segyio_wrapper.ilines),
                                     y_axis_indexes=('x-lines', segyio_wrapper.xlines),
                                     show_v_indicator=True,
                                     show_h_indicator=True,
                                     v_min_max=segyio_wrapper.min_max)

    # attach line-index change signals
    x_slice_widget.index_changed.connect(line_selection_monitor.iline_updated)
    i_slice_widget.index_changed.connect(line_selection_monitor.xline_updated)

    line_selection_monitor.iline_changed.connect(x_slice_widget.set_vertical_line_indicator)
    line_selection_monitor.iline_changed.connect(depth_slice_widget.set_vertical_line_indicator)
    line_selection_monitor.iline_changed.connect(i_slice_widget.update_image)

    line_selection_monitor.xline_changed.connect(i_slice_widget.set_vertical_line_indicator)
    line_selection_monitor.xline_changed.connect(depth_slice_widget.set_horizontal_line_indicator)
    line_selection_monitor.xline_changed.connect(x_slice_widget.update_image)

    line_selection_monitor.depth_changed.connect(depth_slice_widget.update_image)

    # colormap signals
    sliceview_monitor.cmap_changed.connect(x_slice_widget.set_cmap)
    sliceview_monitor.cmap_changed.connect(i_slice_widget.set_cmap)
    sliceview_monitor.cmap_changed.connect(depth_slice_widget.set_cmap)

    # setting min max thresholds
    sliceview_monitor.min_max_changed.connect(x_slice_widget.set_min_max)
    sliceview_monitor.min_max_changed.connect(i_slice_widget.set_min_max)
    sliceview_monitor.min_max_changed.connect(depth_slice_widget.set_min_max)

    return x_slice_widget, i_slice_widget, depth_slice_widget

