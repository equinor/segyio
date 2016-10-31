from PyQt4 import QtGui


class ProgressWidget(QtGui.QWidget):

    def __init__(self, call_on_cancel):
        super(ProgressWidget, self).__init__()
        self.call_on_cancel = call_on_cancel

        layout = QtGui.QHBoxLayout()
        # progress bar
        self.p_bar = QtGui.QProgressBar(self)
        cancel_file_load_btn = QtGui.QPushButton()
        cancel_file_load_btn.setText("cancel")

        cancel_file_load_btn.clicked.connect(self.call_on_cancel)

        layout.addWidget(self.p_bar, 2)
        layout.addWidget(cancel_file_load_btn)

        self.setLayout(layout)

    def set_value(self, value):
        self.p_bar.setValue(value)

