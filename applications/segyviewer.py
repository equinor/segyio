#!/usr/bin/env python

import sys

from PyQt4 import QtGui, QtCore
from segyview import *


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
        self.swrap = None

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
        self.progress_bar = ProgressWidget(self.file_activity_monitor.set_cancel_operation)
        self.statusBar().addWidget(self.progress_bar, 1)
        self.progress_bar.hide()

        #set up an initial size
        self.resize(1200, 800)

        # connect cursor overrides to file activity that might take long time
        self.file_activity_monitor.started.connect(
            lambda: QtGui.QApplication.setOverrideCursor(QtGui.QCursor(QtCore.Qt.BusyCursor)))
        self.file_activity_monitor.finished.connect(
            lambda: QtGui.QApplication.restoreOverrideCursor())

        # initiate the first file load
        if filename:
            self.load_file_and_setup_slice_widgets(filename)


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

        # calling close on current filehandler
        if self.swrap is not None:
            self.swrap.close()

        # instantiating a new wrapper
        self.swrap = SegyIOWrapper.open_file_and_wrap(file_name=filename,
                                                      file_activity_monitor=self.file_activity_monitor)
        # callback when finished
        def complete_finished_file_load_operation(status):
            if status == 0:  # when completed successfully
                self.initialize_slice_widgets()
            self.progress_bar.hide()
            self.file_loader_thread.quit()

        # a memory read, might take some time. Set up a separate thread and update the progressbar
        if read_to_memory:
            # worker thread and worker obj are referenced from main thread to not get gc'ed by accident
            self.file_loader_thread = QtCore.QThread()
            self.file_loader_worker = FileLoaderWorker(self.swrap)
            self.file_loader_worker.moveToThread(self.file_loader_thread)

            # what to do
            self.file_loader_thread.started.connect(self.file_loader_worker.load_file)

            # update progress
            self.file_loader_worker.progress.connect(self.progress_bar.set_value)

            # set up finished callback
            self.file_loader_worker.finished.connect(complete_finished_file_load_operation)

            # and start
            self.file_loader_thread.start()
        else:
            complete_finished_file_load_operation(0)

    def initialize_slice_widgets(self):
        self.addToolBar(LineNavigationBar(self.swrap.xlines,
                                          self.swrap.ilines,
                                          range(self.swrap.samples),
                                          self.line_monitor))

        x_wdgt, i_wdgt, d_wdgt = viewer.initialize_slice_widgets(self.swrap, self.line_monitor, self.colormap_monitor)

        self.xdock.setWidget(x_wdgt)
        self.idock.setWidget(i_wdgt)
        self.ddock.setWidget(d_wdgt)

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




