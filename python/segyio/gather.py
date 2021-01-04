import collections
try:
    from collections.abc import Mapping
except ImportError:
    from collections import Mapping # noqa

import itertools
import numpy as np

try: from future_builtins import zip
except ImportError: pass

import segyio.tools as tools

from .line import sanitize_slice


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

        if isslice(off):
            offs = sanitize_slice(off, self.offsets)
        else:
            offs = slice(off, off + 1, 1)

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

            # the slice could still be defaulted (:), in which case this starts
            # at zero. the lookups are guarded by try-excepts so it won't fail,
            # but it's unnecessary to chck all keys up until the first xline
            # because that will never be a hit anyway
            if il_slice.start is None:
                start = self.iline.keys()[0]
                il_slice = slice(start, il_slice.stop, il_slice.step)

            if xl_slice.start is None:
                start = self.xline.keys()[0]
                xl_slice = slice(start, xl_slice.stop, xl_slice.step)

            il_range = range(*il_slice.indices(last_il))
            xl_range = range(*xl_slice.indices(last_xl))

            # the try-except-else is essentially a filter on in/xl keys, but
            # delegates the work (and decision) to the iline and xline modes
            if not isslice(off):
                for iline in self.iline[il_slice, off]:
                    for xlno in xl_range:
                        try:
                            xind = xlinds[xlno]
                        except KeyError:
                            pass
                        else:
                            yield iline[xind]

                return

            if len(xs) == 0:
                for _, _ in itertools.product(il_range, xl_range): yield empty
                return

            for ilno in il_range:
                iline = tools.collect(self.iline[ilno, off])
                for x in xl_range:
                    try:
                        xind = xlinds[x]
                    except KeyError:
                        pass
                    else:
                        yield iline[:, xind]

        return gen()

class Group(object):
    """
    The inner representation of the Groups abstraction provided by Group.

    A collection of trace indices that have identical `key`.

    Notes
    -----
    .. versionadded:: 1.9
    """
    def __init__(self, key, parent, index):
        self.parent = parent
        self.index = index
        self.key = key

    @property
    def header(self):
        """
        A generator of the the read-only headers in this group

        Returns
        -------
        headers : iterator of Header

        Notes
        -----
        The generator respects the order of the index - to iterate over headers
        in a different order, the index attribute can be re-organised.

        .. versionadded:: 1.9
        """
        source = self.parent.header
        for i in self.index:
            yield source[i]

    @property
    def trace(self):
        """
        A generator of the the read-only traces in this group

        Returns
        -------
        traces : iterator of Trace

        Notes
        -----
        The generator respects the order of the index - to iterate over headers
        in a different order, the index attribute can be re-organised.

        .. versionadded:: 1.9
        """
        source = self.parent.trace
        for i in self.index:
            yield source[i]

    def sort(self, fields):
        """
        Sort the traces in the group, obeying the `fields` order of
        most-to-least significant word.
        """
        # TODO: examples

        headers = [dict(self.parent.header[i]) for i in self.index]
        index = list(zip(headers, self.index))
        # sorting is stable, so sort the whole set by field, applied in the
        # reverse order:
        for field in reversed(fields):
            index.sort(key = lambda x: x[0][field])

        # strip off all the headers
        index = [i for _, i in index]
        self.index = index

class Groups(Mapping):
    """
    The Groups implements the dict interface, grouping all traces that match a
    given `fingerprint`. The fingerprint is a signature derived from a set of
    trace header words, called a `key`.

    Consider a file with five traces, and some selected header words::

        0: {offset: 1, fldr: 1}
        1: {offset: 1, fldr: 2}
        2: {offset: 1, fldr: 1}
        3: {offset: 2, fldr: 1}
        4: {offset: 1, fldr: 2}

    With key = (offset, fldr), there are 3 groups::

        {offset: 1, fldr: 1 }: [0, 2]
        {offset: 1, fldr: 2 }: [1, 4]
        {offset: 2, fldr: 1 }: [3]

    With a key = offset, there are 2 groups::

        {offset: 1}: [0, 1, 2, 4]
        {offset: 2}: [3]

    The Groups class is intended to easily process files without the rigid
    in/crossline structure of iline/xline/gather, but where there is sufficient
    structure in the headers. This is common for some types of pre-stack data,
    shot gather data etc.

    Notes
    -----
    .. versionadded:: 1.9
    """
    # TODO: only group in range of traces?
    # TODO: cache header dicts?
    def __init__(self, trace, header, key):
        bins = collections.OrderedDict()
        for i, h in enumerate(header[:]):
            k = self.fingerprint(h[key])
            if k in bins:
                bins[k].append(i)
            else:
                bins[k] = [i]

        self.trace = trace
        self.header = header
        self.key = key
        self.bins = bins

    @staticmethod
    def normalize_keys(items):
        """
        Normalize the key representation to integers, so that they're hashable,
        even when a key is built with enumerators.

        This function is intended for internal use, and provides the mapping
        from accepted key representation to a canonical key.

        Parameters
        ----------
        items : iterator of (int_like, array_like)

        Returns
        -------
        items : generator of (int, array_like)

        Warnings
        --------
        This function provides no guarantees for value and type compatibility,
        even between minor versions.

        Notes
        -----
        .. versionadded:: 1.9
        """
        return ((int(k), v) for k, v in items)

    @staticmethod
    def fingerprint(key):
        """
        Compute a hashable fingerprint for a key. This function is intended for
        internal use. Relies on normalize_keys for transforming keys to
        canonical form. The output of this function is used for the group ->
        index mapping.

        Parameters
        ----------
        key : int_like or dict of {int_like: int} or iterable of (int_like,int)

        Returns
        -------
        key
            A normalized canonical representation of key

        Warnings
        --------
        This function provides no guarantees for value and type compatibility,
        even between minor versions.

        Notes
        -----
        .. versionadded:: 1.9
        """
        try:
            return int(key)
        except TypeError:
            pass

        try:
            items = key.items()
        except AttributeError:
            items = iter(key)

        # map k -> tracefield -> int
        return frozenset(Groups.normalize_keys(items))

    def __len__(self):
        """x.__len__() <==> len(x)"""
        return len(self.bins)

    def __contains__(self, key):
        """x.__len__() <==> len(x)"""
        return self.fingerprint(key) in self.bins

    def __getitem__(self, key):
        """g[key]

        Read the group associated with key.

        Key can be any informal mapping between a header word (TraceField, su
        header words, or raw integers) and a value.

        Parameters
        ----------
        key

        Returns
        -------
        group : Group

        Notes
        -----
        .. versionadded:: 1.9

        Examples
        --------

        Group on FieldRecord, and get the group FieldRecord == 5:

        >>> fr = segyio.TraceField.FieldRecord
        >>> records = f.group(fr)
        >>> record5 = records[5]
        """
        key = self.fingerprint(key)
        return Group(key, self, self.bins[key])

    def values(self):
        for key, index in self.bins.items():
            yield Group(key, self, index)

    def items(self):
        for key, index in self.bins.items():
            yield key, Group(key, self, index)

    def __iter__(self):
        return self.bins.keys()

    def sort(self, fields):
        """
        Reorganise the indices in all groups by fields
        """
        bins = collections.OrderedDict()

        for key, index in self.bins.items():
            g = Group(key, self, index)
            g.sort(fields)
            bins[key] = g.index

        self.bins = bins
