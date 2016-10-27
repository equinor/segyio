from .segyplot import SegyPlot
from .segyiowrapper import SegyIOWrapper, SlicesWrapper

try:
    from .linenavigationbar import LineNavigationBar
    from .progresswidget import ProgressWidget
    from .slicewidget import SliceWidget
    from .segyiowrapper import SegyIOWrapper, SlicesWrapper
    from .viewer import *
except ImportError as e:
    import sys
    import traceback
    exc_type, exc_value, exc_traceback = sys.exc_info()
    traceback.print_exception(exc_type, exc_value, exc_traceback, limit=2, file=sys.stderr)

