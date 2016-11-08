import sys
from PyQt4 import QtGui, QtCore


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


class MinMaxControls(QtGui.QWidget):
    def __init__(self, segyio_wrapper, sliceview_monitor=None):
        super(MinMaxControls, self).__init__()

        self.swrap = segyio_wrapper
        self.colormap_monitor = sliceview_monitor

        # default to 0,1 when not set
        minv, maxv = self.swrap.min_max or (0,1)

        stepsize = (maxv - minv) / float(100)
        number_of_decimals = self._get_number_of_decimals_for_spinbox(stepsize)
        self.min_input = self._setup_spinbox(stepsize, number_of_decimals, minv)
        self.max_input = self._setup_spinbox(stepsize, number_of_decimals, maxv)

        self.min_input.setToolTip("step in 1% intervals")
        self.max_input.setToolTip("step in 1% intervals")

        # signals
        self.min_input.valueChanged.connect(self.min_input_changed)
        self.max_input.valueChanged.connect(self.max_input_changed)

        # reset button to reset spinboxes to original value from segyio_wrapper
        self.reset_btn = QtGui.QToolButton()
        self.reset_btn.clicked.connect(self.reset_min_max)
        self.reset_btn.setIcon(QtGui.QIcon.fromTheme("edit-undo"))

        self.layout = QtGui.QHBoxLayout(self)
        self.layout.addWidget(QtGui.QLabel("min"))
        self.layout.addWidget(self.min_input)
        self.layout.addWidget(QtGui.QLabel("max"))
        self.layout.addWidget(self.max_input)
        self.layout.addWidget(self.reset_btn)

    @staticmethod
    def _get_number_of_decimals_for_spinbox(step_s):
        # identify the number of decimals for spinbox
        no_of_dec, upper_bound = 1, 1.0

        while step_s < upper_bound:
            upper_bound /= 10
            no_of_dec += 1

        return no_of_dec

    @staticmethod
    def _setup_spinbox(step_s, decimals, initial_value):
        sbox = QtGui.QDoubleSpinBox()
        sbox.setSingleStep(step_s)
        sbox.setDecimals(decimals)
        sbox.setRange(-sys.float_info.max, sys.float_info.max)
        sbox.setValue(initial_value)
        return sbox


    def reset_min_max(self):
        min_v, max_v = self.swrap.min_max or (0, 1)
        self.min_input.setValue(min_v)
        self.max_input.setValue(max_v)

    def min_input_changed(self, val):
        self.max_input.setMinimum(val)
        self.colormap_monitor.min_max_updated((self.min_input.value(),
                                               self.max_input.value()))

    def max_input_changed(self, val):
        self.min_input.setMaximum(val)
        self.colormap_monitor.min_max_updated((self.min_input.value(),
                                               self.max_input.value()))


class SegyViewerToolBar(QtGui.QToolBar):
    def __init__(self, segyio_wrapper, line_selection_monitor, sliceview_monitor):
        super(SegyViewerToolBar, self).__init__("")
        self.swrap = segyio_wrapper
        self.xline_indexes = self.swrap.xlines
        self.iline_indexes = self.swrap.ilines
        self.depth_indexes = range(self.swrap.samples)

        self.line_selection_monitor = line_selection_monitor
        self.colormap_monitor = sliceview_monitor

        # xline
        self.xline_selector = LineSelector(self, "x-line", self.xline_indexes, self.line_selection_monitor.xline_updated)
        self.line_selection_monitor.xline_changed.connect(self.xline_selector.set_index)
        self.addWidget(self.xline_selector)

        # iline
        self.iline_selector = LineSelector(self, "i-line", self.iline_indexes, self.line_selection_monitor.iline_updated)
        self.line_selection_monitor.iline_changed.connect(self.iline_selector.set_index)
        self.addWidget(self.iline_selector)

        # depth
        self.depth_selector = LineSelector(self, "depth", self.depth_indexes, self.line_selection_monitor.depth_updated)
        self.addWidget(self.depth_selector)

        # seperator
        self.addSeparator()

        # controls for adjusting min and max threshold for the slice plots, and colorbar
        self.minmax_ctrls = MinMaxControls(self.swrap, sliceview_monitor=self.colormap_monitor)
        self.addWidget(self.minmax_ctrls)
