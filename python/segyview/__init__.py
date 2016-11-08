from .segyplot import SegyPlot
from .segyiowrapper import SegyIOWrapper, SlicesWrapper

try:
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

__version__    = '1.0.3'
__copyright__  = 'Copyright 2016, Statoil ASA'
__license__    = 'GNU Lesser General Public License version 3'
__status__     = 'Production'
