from PyQt4 import QtGui
from PyQt4.QtCore import pyqtSignal, Qt

from matplotlib.backends.backend_qt4agg import FigureCanvasQTAgg as FigureCanvas

from segyview import LayoutFigure


class Keys(object):
    def __init__(self, key=None, ctrl=False, alt=False, shift=False, meta=False):
        super(Keys, self).__init__()
        self.key = key
        self.alt = alt
        self.ctrl = ctrl
        self.super = meta
        self.shift = shift

    def state(self, ctrl=False, alt=False, shift=False, meta=False):
        return self.ctrl == ctrl and self.alt == alt and self.shift == shift and self.super == meta

    def __bool__(self):
        return not (self.key is None and not self.alt and not self.ctrl and not self.super and not self.shift)

    __nonzero__ = __bool__

    def __str__(self):
        return "%s ctrl: %s shift: %s alt: %s super: %s self: %s" % (self.key, self.ctrl, self.shift, self.alt, self.super, bool(self))


class LayoutCanvas(FigureCanvas):
    layout_changed = pyqtSignal()
    subplot_pressed = pyqtSignal(dict)
    subplot_released = pyqtSignal(dict)
    subplot_motion = pyqtSignal(dict)
    subplot_scrolled = pyqtSignal(dict)

    def __init__(self, width=11.7, height=8.3, dpi=100, parent=None):
        self._figure = LayoutFigure(width, height, dpi)

        FigureCanvas.__init__(self, self._figure)
        self.setParent(parent)

        self.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        self.updateGeometry()
        self.setFocusPolicy(Qt.WheelFocus)
        self.setFocus()

        self.mpl_connect('scroll_event', self._mouse_scrolled)
        self.mpl_connect('button_press_event', self._mouse_pressed)
        self.mpl_connect('button_release_event', self._mouse_released)
        self.mpl_connect('motion_notify_event', self._mouse_motion)
        self.mpl_connect('key_press_event', self._key_press_event)
        self.mpl_connect('key_release_event', self._key_release_event)

        self._start_x = None
        self._start_y = None
        self._start_subplot_index = None
        self._keys = Keys()

    def _create_event(self, event):
        data = {
            "x": event.xdata,
            "y": event.ydata,
            "mx": event.x,
            "my": event.y,
            "dx": None if self._start_x is None else event.xdata - self._start_x,
            "dy": None if self._start_y is None else event.ydata - self._start_y,
            "button": event.button,
            "key": self._keys,
            "step": event.step,
            "subplot_index": self._start_subplot_index or self._figure.index(event.inaxes),
            "gui_event": event.guiEvent
        }
        return data

    def _key_press_event(self, event):
        if len(event.key) > 0:
            self._keys.ctrl = 'ctrl' in event.key or 'control' in event.key
            self._keys.shift = 'shift' in event.key
            self._keys.alt = 'alt' in event.key
            self._keys.meta = 'super' in event.key
            keys = event.key.replace('ctrl', '')
            keys = keys.replace('control', '')
            keys = keys.replace('alt', '')
            keys = keys.replace('super', '')
            keys = keys.replace('+', '')
            self._keys.keys = keys

    def _key_release_event(self, event):
        # the event is unclear on what key is actually released -> wiping every key :)
        self._keys.keys = None
        self._keys.ctrl = False
        self._keys.alt = False
        self._keys.shift = False
        self._keys.super = False

    def _mouse_scrolled(self, event):
        if event.inaxes is not None:
            data = self._create_event(event)
            self.subplot_scrolled.emit(data)

    def _mouse_pressed(self, event):
        if event.inaxes is not None:
            data = self._create_event(event)
            self._start_x = data['x']
            self._start_y = data['y']
            self._start_subplot_index = data['subplot_index']
            self.subplot_pressed.emit(data)

    def _mouse_released(self, event):
        if event.inaxes is not None:
            self._start_x = None
            self._start_y = None
            self._start_subplot_index = None
            data = self._create_event(event)
            self.subplot_released.emit(data)

    def _mouse_motion(self, event):
        if event.inaxes is not None:
            data = self._create_event(event)
            self.subplot_motion.emit(data)

    def set_plot_layout(self, layout_spec):
        self._figure.set_plot_layout(layout_spec)
        self.layout_changed.emit()
        self.draw()

    def layout_figure(self):
        """ :rtype: LayoutFigure """
        return self._figure
