from segyio import Enum


class TraceSortingFormat(Enum):
    UNKNOWN_SORTING = 0
    CROSSLINE_SORTING = 1
    INLINE_SORTING = 2
