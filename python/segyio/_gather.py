import itertools
import numpy as np

import segyio.tools as tools
from segyio.tracesortingformat import TraceSortingFormat

class Gather:
    """ Gather mode. Internal.

    Provides the implementation for f.gather, reading n offsets from the
    intersection of two lines in a cube.
    """

    def __init__(self, trace, iline, xline, offsets, sort):
        # cache constructed modes for performance
        self.trace     = trace
        self.iline     = iline
        self.xline     = xline
        self.offsets   = offsets
        self.sort      = sort

    def _getindex(self, il, xl, offset, sorting):
        """ Get the trace index for an (il, xl, offset) tuple

        :rtype: int
        """
        if sorting == TraceSortingFormat.INLINE_SORTING:
            xlind = self.xline._index(xl, None)
            return self.iline._index(il, offset) + xlind
        else:
            ilind = self.iline._index(il, None)
            return self.xline._index(xl, offset) + ilind

    def __getitem__(self, index):
        """ :rtype: iterator[numpy.ndarray]|numpy.ndarray """
        if len(index) < 3:
            index = (index[0], index[1], slice(None))

        il, xl, off = index
        sort = self.sort

        def isslice(x): return isinstance(x, slice)

        # gather[int,int,int]
        if not any(map(isslice, [il, xl, off])):
            return self.trace[self._getindex(il, xl, off, sort)]

        offs = off if isslice(off) else slice(off, off+1, 1)

        xs = list(filter(self.offsets.__contains__,
                    range(*offs.indices(self.offsets[-1]+1))))

        empty = np.empty(0, dtype = np.single)
        # gather[int,int,:]
        if not any(map(isslice, [il, xl])):
            if len(xs) == 0: return empty
            return tools.collect(
                    self.trace[self._getindex(il, xl, x, sort)]
                    for x in xs)

        # gather[:,:,:]
        def gen():
            ils, _ = self.iline._indices(il, None)
            xls, _ = self.xline._indices(xl, None)

            # gather[:, :, int]
            # i.e. every yielded array is 1-dimensional
            if not isslice(off):
                for i, x in itertools.product(ils, xls):
                    yield self.trace[self._getindex(i, x, off, sort)]
                return

            if len(xs) == 0:
                for _, _ in itertools.product(ils, xls): yield empty
                return

            for i, x in itertools.product(ils, xls):
                yield tools.collect(
                        self.trace[self._getindex(i, x, o, sort)]
                        for o in xs)

        return gen()
