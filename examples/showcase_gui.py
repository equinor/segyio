#!/usr/bin/env python
import sys

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QMainWindow, QWidget, QApplication, QLabel, QVBoxLayout

from segyview import LayoutCombo


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

        central_widget = QWidget()
        layout = QVBoxLayout()
        central_widget.setLayout(layout)

        self._layout_label = QLabel()
        layout.addWidget(self._layout_label)

        self.setCentralWidget(central_widget)

    def _layout_changed(self, layout):
        self._layout_label.setText(str(layout))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        sys.exit()
    else:
        q_app = QApplication(sys.argv)

        gui = TestGUI()
        gui.show()
        gui.raise_()
        sys.exit(q_app.exec_())
