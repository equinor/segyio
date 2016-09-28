from cwrap import BaseCEnum


class TraceSortingFormat(BaseCEnum):
    TYPE_NAME = "TraceSortingFormat"

    UNKNOWN_SORTING = None
    CROSSLINE_SORTING = None
    INLINE_SORTING = None

TraceSortingFormat.addEnum("UNKNOWN_SORTING", -1)
TraceSortingFormat.addEnum("CROSSLINE_SORTING", 0)
TraceSortingFormat.addEnum("INLINE_SORTING", 1)
