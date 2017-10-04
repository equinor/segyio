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

        # gather[:,:,:], gather[int,:,:], gather[:,int,:]
        # gather[:,:,int] etc
        def gen():
            # precompute the xline number -> xline offset
            xlinds = { xlno: i for i, xlno in enumerate(self.xline.lines) }
            ofinds = { ofno: i for i, ofno in enumerate(self.offsets) }

            # doing range over gathers is VERY expensive, because every lookup
            # with a naive implementations would call _getindex to map lineno
            # -> trace index. However, ranges over gathers are done on a
            # by-line basis so lines can be buffered, and traces can be read
            # from the iline. This is the least efficient when there are very
            # few traces read per inline, but huge savings with larger subcubes
            last_il = self.iline.lines[-1] + 1
            last_xl = self.xline.lines[-1] + 1

            il_slice = il if isslice(il) else slice(il, il+1)
            xl_slice = xl if isslice(xl) else slice(xl, xl+1)

            il_range = range(*il_slice.indices(last_il))
            xl_range = range(*xl_slice.indices(last_xl))

            if not isslice(off):
                for iline in self.iline[il_slice, off]:
                    for xlno in xl_range:
                        yield iline[xlinds[xlno]]

                return

            if len(xs) == 0:
                for _, _ in itertools.product(ilrange, xlrange): yield empty
                return

            for ilno in il_range:
                iline = tools.collect(self.iline[ilno, off])
                for x in xl_range:
                    yield iline[:, xlinds[x]]

        return gen()
