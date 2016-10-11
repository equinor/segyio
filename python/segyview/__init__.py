from .segyplot import SegyPlot
import util

try:
    from .linenavigationbar import LineNavigationBar
    from .slicewidget import SliceWidget
except ImportError as e:
    import sys
    import traceback
    exc_type, exc_value, exc_traceback = sys.exc_info()
    traceback.print_exception(exc_type, exc_value, exc_traceback, limit=2, file=sys.stderr)
