try:
    from collections.abc import Mapping # noqa
except ImportError:
    from collections import Mapping # noqa

import itertools
try: from future_builtins import zip
except ImportError: pass
import numpy as np

from .utils import castarray

# in order to support [:end] syntax, we must make sure
# start has a non-None value. lineno.indices() would set it
# to 0, but we don't know if that's a reasonable value or
# not. If start is None we set it to the first line
def sanitize_slice(s, source):
    if all((s.start, s.stop, s.step)):
        return s

    start, stop, step = s.start, s.stop, s.step
    increasing = step is None or step > 0

    if start is None:
        start = min(source) if increasing else max(source)

    if stop is None:
        stop = max(source) + 1 if increasing else min(source) - 1

    return slice(start, stop, step)

class Line(Mapping):
    """
    The Line implements the dict interface, with a fixed set of int_like keys,
    the line numbers/labels. Data is read lazily from disk, so iteration does
    not consume much memory, and are returned as numpy.ndarrays.

    It provides a convenient interface for reading data in a cartesian grid
    system, provided one exists and is detectable by segyio.

    Lines can be accessed individually or with slices, and writing is done via
    assignment. Note that accessing lines uses the line numbers, not their
    position, so if a files has lines [2400..2500], accessing line [0..100]
    will be an error. Note that since each line is returned as a numpy.ndarray,
    meaning accessing the intersections of the inline and crossline is
    0-indexed - orthogonal labels are not preserved.

    Additionally, the line has a concept of offsets, which is useful when
    dealing with prestack files. Offsets are accessed via sub indexing, meaning
    iline[10, 4] will give you line 10 at offset 4.  Please note that offset,
    like lines, are accessed via their labels, not their indices. If your file
    has the offsets [150, 250, 350, 450] and the lines [2400..2500], you can
    access the third offset with [2403, 350]. Please refer to the examples for
    more details. If no offset is specified, segyio will give you the first.

    Notes
    -----
    .. versionadded:: 1.1

    .. versionchanged:: 1.6
        common dict operations (Mapping)
    """

    def __init__(self, filehandle, labels, length, stride, offsets, name):
        self.filehandle = filehandle.xfd
        self.lines = labels
        self.length = length
        self.stride = stride
        self.shape = (length, len(filehandle.samples))
        self.dtype = filehandle.dtype

        # pre-compute all line beginnings
        from ._segyio import fread_trace0
        self.heads = {
            label: fread_trace0(label,
                                length,
                                stride,
                                len(offsets),
                                labels,
                                name)
            for label in labels
        }

        self.offsets = { x: i for i, x in enumerate(offsets) }
        self.default_offset = offsets[0]

    def ranges(self, index, offset):
        if not isinstance(index, slice):
            index = slice(index, index + 1)

        if not isinstance(offset, slice):
            offset = slice(offset, offset + 1)

        index  = sanitize_slice(index, self.heads.keys())
        offset = sanitize_slice(offset, self.offsets.keys())
        irange = range(*index.indices(max(self.heads.keys()) + 1))
        orange = range(*offset.indices(max(self.offsets.keys()) + 1))
        irange = filter(self.heads.__contains__, irange)
        orange = filter(self.offsets.__contains__, orange)
        # offset-range is used in inner loops, so make it a list for
        # reusability. offsets are usually few, so no real punishment by using
        # non-generators here
        return irange, list(orange)

    def __getitem__(self, index):
        """line[i] or line[i, o]

        The line `i`, or the line `i` at a specific offset `o`. ``line[i]``
        returns a numpy array, and changes to this array will *not* be
        reflected on disk.

        The `i` and `o` are *keys*, and should correspond to the line- and
        offset labels in your file, and in the `ilines`, `xlines`, and
        `offsets` attributes.

        Slices can contain lines and offsets not in the file, and like with
        list slicing, these are handled gracefully and ignored.

        When `i` or `o` is a slice, a generator of numpy arrays is returned. If
        the slice is defaulted (:), segyio knows enough about the structure to
        give you all of the respective labels.

        When both `i` and `o` are slices, only one generator is returned, and
        the lines are yielded offsets-first, roughly equivalent to the double
        for loop::

            >>> for line in lines:
            ...     for off in offsets:
            ...         yield line[line, off]
            ...

        Parameters
        ----------
        i : int or slice
        o : int or slice

        Returns
        -------
        line : numpy.ndarray of dtype or generator of numpy.ndarray of dtype

        Raises
        ------
        KeyError
            If `i` or `o` don't exist

        Notes
        -----
        .. versionadded:: 1.1

        Examples
        --------

        Read an inline:

        >>> x = line[2400]

        Copy every inline into a list:

        >>> l = [numpy.copy(x) for x in iline[:]]

        Numpy operations on every other inline:

        >>> for line in line[::2]:
        ...     line = line * 2
        ...     avg = np.average(line)

        Read lines up to 2430:

        >>> for line in line[:2430]:
        ...     line.mean()

        Copy all lines at all offsets:

        >>> l = [numpy.copy(x) for x in line[:,:]]

        Copy all offsets of a line:

        >>> x = numpy.copy(iline[10,:])

        Copy all lines at a fixed offset:

        >>> x = numpy.copy(iline[:, 120])

        Copy every other line and offset:

        >>> map(numpy.copy, line[::2, ::2])

        Copy all offsets [200, 250, 300, 350, ...] in the range [200, 800) for
        all lines [2420,2460):

        >>> l = [numpy.copy(x) for x in line[2420:2460, 200:800:50]]
        """

        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        # prioritise the code path that's potentially in loops externally
        try:
            head = self.heads[index] + self.offsets[offset]
        except TypeError:
            # index is either unhashable (because it's a slice), or offset is a
            # slice.
            pass
        else:
            return self.filehandle.getline(head,
                                           self.length,
                                           self.stride,
                                           len(self.offsets),
                                           np.empty(self.shape, dtype=self.dtype),
                                          )

        # at this point, either offset or index is a slice (or proper
        # type-error), so we're definitely making a generator. make them both
        # slices to unify all code paths
        irange, orange = self.ranges(index, offset)

        def gen():
            x = np.empty(self.shape, dtype=self.dtype)
            y = np.copy(x)

            # only fetch lines that exist. the slice can generate both offsets
            # and line numbers that don't exist, so filter out misses before
            # they happen
            for line in irange:
                for off in orange:
                    head = self.heads[line] + self.offsets[off]
                    self.filehandle.getline(head,
                                            self.length,
                                            self.stride,
                                            len(self.offsets),
                                            y,
                                           )
                    y, x = x, y
                    yield x

        return gen()

    def __setitem__(self, index, val):
        """line[i] = val or line[i, o] = val

        Follows the same rules for indexing and slicing as ``line[i]``.

        In either case, if the `val` iterable is exhausted before the line(s),
        assignment stops with whatever is written so far. If `val` is longer
        than an individual line, it's essentially truncated.

        Parameters
        ----------
        i       : int or slice
        offset  : int or slice
        val     : array_like

        Raises
        ------
        KeyError
            If `i` or `o` don't exist

        Notes
        -----
        .. versionadded:: 1.1

        Examples
        --------
        Copy a full line:

        >>> line[2400] = other[2834]

        Copy first half of the inlines from g to f:

        >>> line[:] = other[:labels[len(labels) / 2]]

        Copy every other line consecutively:

        >>> line[:] = other[::2]

        Copy every third offset:

        >>> line[:,:] = other[:,::3]

        Copy a line into a set line and offset:

        >>> line[12, 200] = other[21]
        """

        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        try: head = self.heads[index] + self.offsets[offset]
        except TypeError: pass
        else:
            return self.filehandle.putline(head,
                                           self.length,
                                           self.stride,
                                           len(self.offsets),
                                           index,
                                           offset,
                                           castarray(val, dtype = self.dtype),
                                          )

        irange, orange = self.ranges(index, offset)

        val = iter(val)
        for line in irange:
            for off in orange:
                head = self.heads[line] + self.offsets[off]
                try: self.filehandle.putline(head,
                                            self.length,
                                            self.stride,
                                            len(self.offsets),
                                            line,
                                            off,
                                            next(val),
                                           )
                except StopIteration: return

    # can't rely on most Mapping default implementations of
    # dict-like, because iter() does not yield keys for this class, it gives
    # the lines themselves. that violates some assumptions (but segyio's always
    # worked that way), and it's the more natural behaviour for segyio, so it's
    # acceptible. additionally, the default implementations would be very slow
    # and ineffective because they assume __getitem__ is sufficiently cheap,
    # but it isn't here since it involves a disk operation
    def __len__(self):
        """x.__len__() <==> len(x)"""
        return len(self.heads)

    def __iter__(self):
        """x.__iter__() <==> iter(x)"""
        return self[:]

    def __contains__(self, key):
        """x.__contains__(y) <==> y in x"""
        return key in self.heads

    def keys(self):
        """D.keys() -> a set-like object providing a view on D's keys"""
        return sorted(self.heads.keys())

    def values(self):
        """D.values() -> generator of D's values"""
        return self[:]

    def items(self):
        """D.values() -> generator of D's (key,values), as 2-tuples"""
        return zip(self.keys(), self[:])

class HeaderLine(Line):
    """
    The Line implements the dict interface, with a fixed set of int_like keys,
    the line numbers/labels. The values are iterables of Field objects.

    Notes
    -----
    .. versionadded:: 1.1

    .. versionchanged:: 1.6
        common dict operations (Mapping)
    """
    # a lot of implementation details are shared between reading data traces
    # line-by-line and trace headers line-by-line, so (ab)use inheritance for
    # __len__, keys() etc., however, the __getitem__ is  way different and is re-implemented

    def __init__(self, header, base, direction):
        super(HeaderLine, self).__init__(header.segy,
                                          base.lines,
                                          base.length,
                                          base.stride,
                                          sorted(base.offsets.keys()),
                                          'header.' + direction,
                                         )
        self.header = header

    def __getitem__(self, index):
        """line[i] or line[i, o]

        The line `i`, or the line `i` at a specific offset `o`. ``line[i]``
        returns an iterable of `Field` objects, and changes to these *will* be
        reflected on disk.

        The `i` and `o` are *keys*, and should correspond to the line- and
        offset labels in your file, and in the `ilines`, `xlines`, and
        `offsets` attributes.

        Slices can contain lines and offsets not in the file, and like with
        list slicing, these are handled gracefully and ignored.

        When `i` or `o` is a slice, a generator of iterables of headers are
        returned.

        When both `i` and `o` are slices, one generator is returned for the
        product `i` and `o`, and the lines are yielded offsets-first, roughly
        equivalent to the double for loop::

            >>> for line in lines:
            ...     for off in offsets:
            ...         yield line[line, off]
            ...

        Parameters
        ----------

        i : int or slice
        o : int or slice

        Returns
        -------
        line : iterable of Field or generator of iterator of Field

        Raises
        ------
        KeyError
            If `i` or `o` don't exist

        Notes
        -----
        .. versionadded:: 1.1

        """
        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        try:
            start = self.heads[index] + self.offsets[offset]
        except TypeError:
            # index is either unhashable (because it's a slice), or offset is a
            # slice.
            pass

        else:
            step = self.stride * len(self.offsets)
            stop = start + step * self.length
            return self.header[start:stop:step]

        def gen():
            irange, orange = self.ranges(index, offset)
            for line in irange:
                for off in orange:
                    yield self[line, off]

        return gen()

    def __setitem__(self, index, val):
        """line[i] = val or line[i, o] = val

        Follows the same rules for indexing and slicing as ``line[i]``. If `i`
        is an int, and `val` is a dict or Field, that value is replicated and
        assigned to every trace header in the line, otherwise it's treated as
        an iterable, and each trace in the line is assigned the ``next()``
        yielded value.

        If `i` or `o` is a slice, `val` must be an iterable.

        In either case, if the `val` iterable is exhausted before the line(s),
        assignment stops with whatever is written so far.

        Parameters
        ----------
        i       : int or slice
        offset  : int or slice
        val     : dict_like or iterable of dict_like

        Raises
        ------
        KeyError
            If `i` or `o` don't exist

        Notes
        -----
        .. versionadded:: 1.1

        Examples
        --------
        Rename the iline 3 to 4:

        >>> line[3] = { TraceField.INLINE_3D: 4 }
        >>> # please note that rewriting the header won't update the
        >>> # file's interpretation of the file until you reload it, so
        >>> # the new iline 4 will be considered iline 3 until the file
        >>> # is reloaded

        Set offset line 3 offset 3 to 5:

        >>> line[3, 3] = { TraceField.offset: 5 }
        """
        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        try: start = self.heads[index] + self.offsets[offset]
        except TypeError: pass
        else:
            step = self.stride * len(self.offsets)
            stop  = start + step * self.length
            self.header[start:stop:step] = val
            return

        # if this is a dict-like, just repeat it
        if hasattr(val, 'keys'):
            val = itertools.repeat(val)

        irange, orange = self.ranges(index, offset)
        val = iter(val)
        for line in irange:
            for off in orange:
                try:
                    self[line, off] = next(val)
                except StopIteration:
                    return
