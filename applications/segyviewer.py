#!/usr/bin/env python

import sys

from PyQt4 import QtGui, QtCore
from segyview import *


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


class ColorMapMonitor(QtCore.QObject):
    cmap_changed = QtCore.pyqtSignal(str)

    def __init__(self, parent=None):
        QtCore.QObject.__init__(self, parent)

    def colormap_updated(self, value):
        self.cmap_changed.emit(str(value))


class FileActivityMonitor(QtCore.QObject):
    started = QtCore.pyqtSignal()
    finished = QtCore.pyqtSignal()
    cancelled_operation = False

    def __init__(self, parent=None):
        QtCore.QObject.__init__(self)
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

    def set_value(self, int):
        self.p_bar.setValue(int)


class FileLoaderWorker(QtCore.QObject):
    finished = QtCore.pyqtSignal(int)
    progress = QtCore.pyqtSignal(int)

    def __init__(self, segy_file_wrapper, filename, read_to_memory=False):
        QtCore.QObject.__init__(self)
        self.segy_file_wrapper = segy_file_wrapper
        self.filename = filename
        self.read_to_memory = read_to_memory

    def load_file(self):
        successful_file_open = self.segy_file_wrapper.open_file(self.filename,
                                         read_to_memory=self.read_to_memory,
                                         progress_callback=self.progress.emit)
        if successful_file_open:
            self.finished.emit(0)
        else:
            self.finished.emit(1)

        return None


class SegyViewer(QtGui.QMainWindow):
    def __init__(self, filename):
        QtGui.QMainWindow.__init__(self)

        # qthreads needs to be in scope of main thread to not be "accidentally" garbage collected
        self.file_loader_thread = None
        self.file_loader_worker = None
        self.setAttribute(QtCore.Qt.WA_DeleteOnClose)

        # signal monitors
        self.colormap_monitor = ColorMapMonitor(self)
        self.line_monitor = LineSelectionMonitor(self)
        self.file_activity_monitor = FileActivityMonitor(self)

        # the wrapper around segyio
        self.swrap = SegyIOWrapper(self.file_activity_monitor)

        # menus
        available_colormaps = ['seismic', 'spectral', 'RdGy', 'hot', 'jet', 'gray']
        self.configure_main_menu(self.menuBar(), self.colormap_monitor, available_colormaps)

        # layout
        self.setWindowTitle("SegyViewer")
        self.xdock = QtGui.QDockWidget("x-line")
        self.xdock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)

        self.idock = QtGui.QDockWidget("i-line")
        self.idock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)

        self.ddock = QtGui.QDockWidget("depth slice")
        self.ddock.setFeatures(QtGui.QDockWidget.DockWidgetFloatable | QtGui.QDockWidget.DockWidgetMovable)
        self.setup_dock_widgets()

        # progress bar
        self.progress_bar = ProgressWidget(self.swrap.file_activity_monitor.set_cancel_operation)
        self.statusBar().addWidget(self.progress_bar, 1)
        self.progress_bar.hide()

        #set up an initial size
        self.resize(1200, 800)

        # connect cursor overrides to file activity that might take long time
        self.swrap.file_activity_monitor.started.connect(
            lambda: QtGui.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.BusyCursor)))
        self.swrap.file_activity_monitor.finished.connect(
            lambda: QtGui.QApplication.restoreOverrideCursor())

        # initiate the first file load
        if filename:
            self.load_file_and_setup_slice_widgets(filename)

        # fix for making dock widgets fill entire window
        # self.main_widget = QtGui.QWidget(self)
        # self.main_widget.setFocus()
        # self.setCentralWidget(self.main_widget)
        # self.main_widget.hide()

    def setup_dock_widgets(self):
        self.setDockOptions(QtGui.QMainWindow.AllowNestedDocks)
        self.addDockWidget(QtCore.Qt.TopDockWidgetArea, self.xdock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.idock)
        self.addDockWidget(QtCore.Qt.BottomDockWidgetArea, self.ddock)

    def configure_main_menu(self, menu, colormap_monitor, available_colormaps):
        menu.fileMenu = menu.addMenu('&File')

        exitAction = QtGui.QAction('&Exit', menu)
        menu.fileMenu.addAction(exitAction)

        openAction = QtGui.QAction('&Open', menu)
        openAction.setShortcut("Ctrl+O")
        menu.fileMenu.addAction(openAction)
        openAction.triggered.connect(self.open_file_dialogue)

        menu.viewMenu = menu.addMenu('&View')
        menu.colormapMenu = menu.viewMenu.addMenu("&Colormap")

        def set_selected_cmap(cmap_name):
            for item in menu.colormapMenu.actions():
                item.setChecked(str(item.text()) == cmap_name)

        colormap_monitor.cmap_changed.connect(set_selected_cmap)

        def colormap_changer(color_map_name):
            def perform_colormap_change():
                colormap_monitor.colormap_updated(color_map_name)

            return perform_colormap_change

        for item in available_colormaps:
            action = menu.colormapMenu.addAction(item)
            action.setCheckable(True)
            action.triggered.connect(colormap_changer(item))

    def remove_all_slice_widgets(self):
        # removing old widgets
        for tbar in self.findChildren(QtGui.QToolBar):
            self.removeToolBar(tbar)

        # removing old slice widgets from dock widgets
        for dock_widget in self.findChildren(QtGui.QDockWidget):
            for widgt in dock_widget.findChildren(SliceWidget):
                widgt.deleteLater()

    def setup_widgets_and_controls(self):
        self.addToolBar(LineNavigationBar(self.swrap.xlines,
                                          self.swrap.ilines,
                                          range(self.swrap.samples),
                                          self.line_monitor))

        # initialize slice widgets
        x_slice_widget = SliceWidget(self.swrap.xline, self.swrap.xlines,
                                     x_axis_indexes=('i-lines', self.swrap.ilines),
                                     y_axis_indexes=('depth', range(self.swrap.samples)),
                                     show_v_indicator=True,
                                     v_min_max=self.swrap.min_max)

        i_slice_widget = SliceWidget(self.swrap.iline, self.swrap.ilines,
                                     x_axis_indexes=('x-lines', self.swrap.xlines),
                                     y_axis_indexes=('depth', range(self.swrap.samples)),
                                     show_v_indicator=True,
                                     v_min_max=self.swrap.min_max)

        depth_slice_widget = SliceWidget(self.swrap.depth_slice, range(self.swrap.samples),
                                         x_axis_indexes=('i-lines', self.swrap.ilines),
                                         y_axis_indexes=('x-lines', self.swrap.xlines),
                                         show_v_indicator=True,
                                         show_h_indicator=True,
                                         v_min_max=self.swrap.min_max)

        # attach line-index change signals
        x_slice_widget.index_changed.connect(self.line_monitor.iline_updated)
        i_slice_widget.index_changed.connect(self.line_monitor.xline_updated)

        self.line_monitor.iline_changed.connect(x_slice_widget.set_vertical_line_indicator)
        self.line_monitor.iline_changed.connect(depth_slice_widget.set_vertical_line_indicator)
        self.line_monitor.iline_changed.connect(i_slice_widget.update_image)

        self.line_monitor.xline_changed.connect(i_slice_widget.set_vertical_line_indicator)
        self.line_monitor.xline_changed.connect(depth_slice_widget.set_horizontal_line_indicator)
        self.line_monitor.xline_changed.connect(x_slice_widget.update_image)

        self.line_monitor.depth_changed.connect(depth_slice_widget.update_image)

        # colormap signals
        self.colormap_monitor.cmap_changed.connect(x_slice_widget.set_cmap)
        self.colormap_monitor.cmap_changed.connect(i_slice_widget.set_cmap)
        self.colormap_monitor.cmap_changed.connect(depth_slice_widget.set_cmap)

        self.xdock.setWidget(x_slice_widget)
        self.idock.setWidget(i_slice_widget)
        self.ddock.setWidget(depth_slice_widget)

    def open_file_dialogue(self):

        f_dialog = QtGui.QFileDialog(self)
        f_dialog.setViewMode(QtGui.QFileDialog.Detail)
        f_dialog.setNameFilter("Segy File  (*.seg *.segy *.sgy)")
        load_to_memory_label = QtGui.QLabel("Read entire Segy file to memory")
        load_to_memory_cbox = QtGui.QCheckBox()
        load_to_memory_cbox.setChecked(True)
        layout = f_dialog.layout()
        layout.addWidget(load_to_memory_label)
        layout.addWidget(load_to_memory_cbox)

        def file_selected(filename):

            if filename:
                self.load_file_and_setup_slice_widgets(filename, read_to_memory=load_to_memory_cbox.isChecked())

        f_dialog.fileSelected.connect(file_selected)

        f_dialog.show()

        #filename = f_dialog.getOpenFileName(None, "Open Segy File", filter="Segy file (*.seg *.segy *.sgy)")

    def load_file_and_setup_slice_widgets(self, filename, read_to_memory=True):
        #remove old widgets
        self.remove_all_slice_widgets()

        # progress bar
        self.progress_bar.show()

        self.file_activity_monitor.reset()
        self.swrap = SegyIOWrapper(self.file_activity_monitor)

        # worker thread and worker obj are referenced from main thread to not get gc'ed by accident
        self.file_loader_thread = QtCore.QThread()
        self.file_loader_worker = FileLoaderWorker(self.swrap, filename, read_to_memory=read_to_memory)
        self.file_loader_worker.moveToThread(self.file_loader_thread)

        # what to do
        self.file_loader_thread.started.connect(self.file_loader_worker.load_file)

        # update progress
        self.file_loader_worker.progress.connect(self.progress_bar.set_value)

        # when finished
        self.file_loader_worker.finished.connect(self.complete_finished_file_load_operation)

        self.file_loader_thread.start()

    def complete_finished_file_load_operation(self, status):

        if status == 0:  # when completed successfully
            self.setup_widgets_and_controls()

        self.progress_bar.hide()
        self.file_loader_thread.quit()


def main():

    filename = None
    if len(sys.argv) == 2:
        filename = sys.argv[1]

    q_app = QtGui.QApplication(sys.argv)
    segy_viewer = SegyViewer(filename)
    segy_viewer.show()
    sys.exit(q_app.exec_())

if __name__ == '__main__':
    main()




