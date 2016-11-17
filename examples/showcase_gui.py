#!/usr/bin/env python
import sys

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QMainWindow, QWidget, QApplication, QLabel, QVBoxLayout

from segyview import LayoutCombo, ColormapCombo, LayoutCanvas


class TestGUI(QMainWindow):
    def __init__(self):
        QMainWindow.__init__(self)

        self.setAttribute(Qt.WA_DeleteOnClose)
        self.setWindowTitle("GUI Test")

        toolbar = self.addToolBar("Stuff")
        """:type: QToolBar"""

        layout_combo = LayoutCombo()
        toolbar.addWidget(layout_combo)
        layout_combo.layout_changed.connect(self._layout_changed)

        self._colormap_combo = ColormapCombo()
        toolbar.addWidget(self._colormap_combo)
        self._colormap_combo.currentIndexChanged[int].connect(self._colormap_changed)

        central_widget = QWidget()
        layout = QVBoxLayout()
        central_widget.setLayout(layout)

        self._layout_canvas = LayoutCanvas(width=5, height=5)
        self._layout_canvas.set_plot_layout(layout_combo.get_current_layout())
        layout.addWidget(self._layout_canvas)

        self._colormap_label = QLabel()
        layout.addWidget(self._colormap_label)

        self.setCentralWidget(central_widget)

    def _layout_changed(self, layout):
        self._layout_canvas.set_plot_layout(layout)

    def _colormap_changed(self, index):
        colormap = str(self._colormap_combo.itemText(index))
        self._colormap_label.setText("Colormap selected: %s" % colormap)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        sys.exit()
    else:
        q_app = QApplication(sys.argv)

        gui = TestGUI()
        gui.show()
        gui.raise_()
        sys.exit(q_app.exec_())
