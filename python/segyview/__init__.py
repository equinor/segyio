import os.path as path

from .segyplot import SegyPlot
from .segyiowrapper import SegyIOWrapper, SlicesWrapper

img_prefix = path.abspath(path.join(path.dirname(path.abspath(__file__)), "resources", "img"))

if not path.exists(img_prefix):
    img_prefix = path.abspath(path.join(path.dirname(path.abspath(__file__)), "..", "..", "resources", "img"))


def resource_icon_path(name):
    return path.join(img_prefix, name)


def resource_icon(name):
    """Load an image as an icon"""
    # print("Icon used: %s" % name)
    from PyQt4.QtGui import QIcon
    return QIcon(resource_icon_path(name))


try:
    from .layoutcombo import LayoutCombo
    from .progresswidget import ProgressWidget
    from .slicewidget import SliceWidget, ColorBarWidget
    from .segyiowrapper import SegyIOWrapper, SlicesWrapper
    from .controlwidgets import *
    from .viewer import *
except ImportError as e:
    import sys
    import traceback
    exc_type, exc_value, exc_traceback = sys.exc_info()
    traceback.print_exception(exc_type, exc_value, exc_traceback, limit=2, file=sys.stderr)

__version__    = '1.0.4'
__copyright__  = 'Copyright 2016, Statoil ASA'
__license__    = 'GNU Lesser General Public License version 3'
__status__     = 'Production'
