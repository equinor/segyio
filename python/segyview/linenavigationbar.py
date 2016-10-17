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

class LineNavigationBar(QtGui.QToolBar):
    def __init__(self, xline_indexes, iline_indexes, depth_indexes, line_selection_monitor):
        super(LineNavigationBar, self).__init__("")
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

        # depth
        self.depth_selector = LineSelector(self, "depth", self.depth_indexes, self.line_selection_monitor.depthUpdated)
        self.addWidget(self.depth_selector)
