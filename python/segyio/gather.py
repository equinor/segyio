import itertools
import numpy as np

import segyio.tools as tools

class Gather(object):
    """
    A gather is in this context the intersection of lines in a cube, i.e. all
    the offsets at some inline/crossline intersection. The primary data type is
    numpy.ndarray, with dimensions depending on the range of offsets specified.

    Implements a dict_like lookup with the line and offset numbers (labels),
    not 0-based indices.

    Notes
    -----
    .. versionadded:: 1.1
    """

    def __init__(self, trace, iline, xline, offsets):
        # cache constructed modes for performance
        self.trace     = trace
        self.iline     = iline
        self.xline     = xline
        self.offsets   = offsets

    def __getitem__(self, index):
        """gather[i, x, o], gather[:,:,:]

        Get the gather or range of gathers, defined as offsets intersection
        between an in- and a crossline. Also works on post-stack files (with
        only 1 offset), although it is less useful in those cases.

        If offsets are omitted, the default is all offsets.

        A group of offsets is always returned as an offset-by-samples
        numpy.ndarray. If either inline, crossline, or both, are slices, a
        generator of such ndarrays are returned.

        If the slice of offsets misses all offsets, a special, empty ndarray is
        returned.

        Parameters
        ----------
        i : int or slice
            inline
        x : int or slice
            crossline
        o : int or slice
            offsets (default is :)

        Returns
        -------
        gather : numpy.ndarray or generator of numpy.ndarray

        Notes
        -----
        .. versionadded:: 1.1

        Examples
        --------
        Read one offset at an intersection:

        >>> gather[200, 241, 25] # returns same shape as trace

        Read all offsets at an intersection:

        >>> gather[200, 241, :] # returns offsets x samples ndarray
        >>> # If no offset is specified, this is implicitly (:)
        >>> gather[200, 241, :] == gather[200, 241]

        All offsets for a set of ilines, intersecting one crossline:

        >>> gather[200:300, 241, :] == gather[200:300, 241]

        Some offsets for a set of ilines, interescting one crossline:

        >>> gather[200:300, 241, 10:25:5]

        Some offsets for a set of ilines and xlines. This effectively yields a
        subcube:

        >>> f.gather[200:300, 241:248, 1:10]
        """
        if len(index) < 3:
            index = (index[0], index[1], None)

        il, xl, off = index

        if off is None and len(self.offsets) == 1:
            off = self.offsets[0]

        # if offset isn't specified, default to all, [:]
        off = off or slice(None)

        def isslice(x): return isinstance(x, slice)

        # gather[int,int,int]
        if not any(map(isslice, [il, xl, off])):
            o = self.iline.offsets[off]
            i = o + self.iline.heads[il] + self.xline.heads[xl]
            return self.trace[i]

        offs = off if isslice(off) else slice(off, off+1, 1)

        xs = list(filter(self.offsets.__contains__,
                    range(*offs.indices(self.offsets[-1]+1))))

        empty = np.empty(0, dtype = self.trace.dtype)
        # gather[int,int,:]
        if not any(map(isslice, [il, xl])):
            if len(xs) == 0: return empty
            i = self.iline.heads[il] + self.xline.heads[xl]
            return tools.collect(self.trace[i + self.iline.offsets[x]] for x in xs)

        # gather[:,:,:], gather[int,:,:], gather[:,int,:]
        # gather[:,:,int] etc
        def gen():
            # precomputed xline number -> xline offset into the iline
            xlinds = { xlno: i for i, xlno in enumerate(self.xline.keys()) }

            # ranges over gathers are done on a by-line basis so lines can be
            # buffered, and traces can be read from the iline. This is the
            # least efficient when there are very few traces read per inline,
            # but huge savings with larger subcubes
            last_il = self.iline.keys()[-1] + 1
            last_xl = self.xline.keys()[-1] + 1

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
                for _, _ in itertools.product(il_range, xl_range): yield empty
                return

            for ilno in il_range:
                iline = tools.collect(self.iline[ilno, off])
                for x in xl_range:
                    yield iline[:, xlinds[x]]

        return gen()
