import collections
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

class Line(collections.Mapping):

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

        When `i` or `o` is a slice, a generator of numpy arrays is returned.

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

        i : int or slice
        offset : int or slice
        val : array_like

        Raises
        ------

        KeyError
            If `i` or `o` don't exist

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

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

    # can't rely on most collections.Mapping default implementations of
    # dict-like, because iter() does not yield keys for this class, it gives
    # the lines themselves. that violates some assumptions (but segyio's always
    # worked that way), and it's the more natural behaviour for segyio, so it's
    # acceptible. additionally, the default implementations would be very slow
    # and ineffective because they assume __getitem__ is sufficiently cheap,
    # but it isn't here since it involves a disk operation
    def __len__(self):
        return len(self.heads)

    def __iter__(self):
        return self[:]

    def __contains__(self, key):
        return key in self.heads

    def keys(self):
        return sorted(self.heads.keys())

    def values(self):
        return self[:]

    def items(self):
        return zip(self.keys(), self[:])
