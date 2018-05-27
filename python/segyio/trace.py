import collections
import itertools
try: from future_builtins import zip
except ImportError: pass

import numpy as np

from ._raw_trace import RawTrace
from .line import Line as LineBase
from .field import Field
from .utils import castarray

class Sequence(collections.Sequence):

    # unify the common optimisations and boilerplate of Trace, RawTrace, and
    # Header, which all obey the same index-oriented interface, and all share
    # length and wrap-around properties.
    #
    # It provides a useful negative-wrap index method which deals
    # approperiately with IndexError and python2-3 differences.

    def __init__(self, length):
        self.length = length

    def __len__(self):
        """x.__len__() <==> len(x)"""
        return self.length

    def __iter__(self):
        """x.__iter__() <==> iter(x)"""
        # __iter__ has a reasonable default implementation from
        # collections.Sequence. It's essentially this loop:
        # for i in range(len(self)): yield self[i]
        # However, in segyio that means the double-buffering, buffer reuse does
        # not happen, which is *much* slower (the allocation of otherwised
        # reused numpy objects takes about half the execution time), so
        # explicitly implement it as [:]
        return self[:]

    def wrapindex(self, i):
        if i < 0:
            i += len(self)

        if not 0 <= i < len(self):
            # in python2, int-slice comparison does not raise a type error,
            # (but returns False), so force a type-error if this still isn't an
            # int-like.
            _ = i + 0
            raise IndexError('trace index out of range')

        return i

class Trace(Sequence):
    def __init__(self, file, samples):
        super(Trace, self).__init__(file.tracecount)
        self.filehandle = file.xfd
        self.dtype = file.dtype
        self.shape = samples

    def __getitem__(self, i):
        """trace[i]

        *i*th trace of the file, starting at 0. ``trace[i]`` returns a numpy
        array, and changes to this array will *not* be reflected on disk.

        When i is a slice, a generator of numpy arrays is returned.

        Parameters
        ----------

        i : int or slice

        Returns
        -------

        trace : numpy.ndarray of dtype or generator of numpy.ndarray of dtype

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

        .. note::

            This operator reads lazily from the file, meaning the file is read
            on ``next()``, and only one trace is fixed in memory. This means
            segyio can run through arbitrarily large files without consuming
            much memory, but it is potentially slow if the goal is to read the
            entire file into memory. If that is the case, consider using
            `trace.raw`, which reads eagerly.

        """

        try:
            i = self.wrapindex(i)
            buf = np.zeros(self.shape, dtype = self.dtype)
            return self.filehandle.gettr(buf, i, 1, 1)

        except TypeError:
            def gen():
                # double-buffer the trace. when iterating over a range, we want
                # to make sure the visible change happens as late as possible,
                # and that in the case of exception the last valid trace was
                # untouched. this allows for some fancy control flow, and more
                # importantly helps debugging because you can fully inspect and
                # interact with the last good value.
                x = np.zeros(self.shape, dtype=self.dtype)
                y = np.zeros(self.shape, dtype=self.dtype)

                for j in range(*i.indices(len(self))):
                    self.filehandle.gettr(x, j, 1, 1)
                    x, y = y, x
                    yield y

            return gen()

    def __setitem__(self, i, val):
        """trace[i] = val

        Write the *i*th trace of the file, starting at 0. It accepts any
        array_like, but `val` must be at least as big as the underlying data
        trace.

        If `val` is longer than the underlying trace, `val` is essentially truncated.

        Parameters
        ----------

        i : int or slice
        val : array_like

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

        """
        if isinstance(i, slice):
            for j, x in zip(range(*i.indices(len(self))), val):
                self[j] = x

            return

        xs = castarray(val, self.dtype)

        # TODO:  check if len(xs) > shape, and optionally warn on truncating
        # writes
        self.filehandle.puttr(self.wrapindex(i), xs)

    def __repr__(self):
        return "Trace(traces = {}, samples = {})".format(len(self), self.shape)

    @property
    def raw(self):
        """ :rtype: segyio.RawTrace """
        return RawTrace(self)

class Header(Sequence):
    def __init__(self, segy):
        self.segy = segy
        super(Header, self).__init__(segy.tracecount)

    def __getitem__(self, i):
        """header[i]

        *i*th header of the file, starting at 0

        Parameters
        ----------

        i : int or slice

        Returns
        -------

        field : Field
            dict_like header

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists

        """
        try:
            i = self.wrapindex(i)
            return Field.trace(traceno = i, segy = self.segy)

        except TypeError:
            def gen():
                # double-buffer the header. when iterating over a range, we
                # want to make sure the visible change happens as late as
                # possible, and that in the case of exception the last valid
                # header was untouched. this allows for some fancy control
                # flow, and more importantly helps debugging because you can
                # fully inspect and interact with the last good value.
                x = Field.trace(None, self.segy)
                buf = bytearray(x.buf)
                for j in range(*i.indices(len(self))):
                    # skip re-invoking __getitem__, just update the buffer
                    # directly with fetch, and save some initialisation work
                    buf = x.fetch(buf, j)
                    x.buf[:] = buf
                    x.traceno = j
                    yield x

            return gen()

    def __setitem__(self, i, val):
        """header[i] = val

        Write *i*th header of the file, starting at 0

        Parameters
        ----------

        i : int or slice
        val : Field or array_like of dict_like

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists

        """

        x = self[i]

        try:
            x.update(val)
        except AttributeError:
            if isinstance(val, Field) or isinstance(val, dict):
                val = itertools.repeat(val)

            for h, v in zip(x, val):
                h.update(v)

    @property
    def iline(self):
        return Line(self, self.segy.iline, 'inline')

    @iline.setter
    def iline(self, value):
        """Write iterables to lines

        Examples:
            Supports writing to *all* crosslines via assignment, regardless of
            data source and format. Will respect the sample size and structure
            of the file being assigned to, so if the argument traces are longer
            than that of the file being written to the surplus data will be
            ignored. Uses same rules for writing as `f.iline[i] = x`.
        """
        for i, src in zip(self.segy.ilines, value):
            self.iline[i] = src

    @property
    def xline(self):
        return Line(self, self.segy.xline, 'crossline')

    @xline.setter
    def xline(self, value):
        """Write iterables to lines

        Examples:
            Supports writing to *all* crosslines via assignment, regardless of
            data source and format. Will respect the sample size and structure
            of the file being assigned to, so if the argument traces are longer
            than that of the file being written to the surplus data will be
            ignored. Uses same rules for writing as `f.xline[i] = x`.
        """

        for i, src in zip(self.segy.xlines, value):
            self.xline[i] = src

class Line(LineBase):
    # a lot of implementation details are shared between reading data traces
    # line-by-line and trace headers line-by-line, so (ab)use inheritance for
    # __len__, keys() etc., however, the __getitem__ is  way different and is re-implemented

    def __init__(self, header, base, direction):
        super(Line, self).__init__(header.segy,
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
            return self.header[start::step]

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

        i : int or slice
        offset : int or slice
        val : field or dict_like or iterable of field or iterable of dict_like

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

        try: start = self.heads[index] + self.offsets[offset]
        except TypeError: pass

        else:
            try:
                if hasattr(val, 'keys'):
                    val = itertools.repeat(val)
            except TypeError:
                # already an iterable
                pass

            step = self.stride * len(self.offsets)
            self.header[start::step] = val
            return

        irange, orange = self.ranges(index, offset)
        val = iter(val)
        for line in irange:
            for off in orange:
                try:
                    self[line, off] = next(val)
                except StopIteration:
                    return
