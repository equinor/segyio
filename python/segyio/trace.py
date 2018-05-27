import collections
import itertools
try: from future_builtins import zip
except ImportError: pass

import numpy as np

from ._raw_trace import RawTrace
from .line import HeaderLine
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
    """
    The Trace implements the array interface, where every array element, the
    data trace, is a numpy.ndarray. As all arrays, it can be random accessed,
    iterated over, and read strided. Data is read lazily from disk, so
    iteration does not consume much memory. If you want eager reading, use
    Trace.raw.

    This mode gives access to reading and writing functionality for traces.
    The primary data type is ``numpy.ndarray``. Traces can be accessed
    individually or with python slices, and writing is done via assignment.

    Notes
    -----
    .. versionadded:: 1.1

    .. versionchanged:: 1.6
        common list operations (collections.Sequence)

    Examples
    --------
    Read all traces in file f and store in a list:

    >>> l = [np.copy(tr) for tr in f.trace]

    Do numpy operations on a trace:

    >>> tr = f.trace[10]
    >>> tr = np.transpose(tr)
    >>> tr = tr * 2
    >>> tr = tr - 100
    >>> avg = np.average(tr)

    Double every trace value and write to disk. Since accessing a trace
    gives a numpy value, to write to the respective trace we need its index:

    >>> for i, tr in enumerate(trace):
    ...     tr = tr * 2
    ...     trace[i] = tr

    """

    def __init__(self, file, samples):
        super(Trace, self).__init__(file.tracecount)
        self.filehandle = file.xfd
        self.dtype = file.dtype
        self.shape = samples

    def __getitem__(self, i):
        """trace[i]

        ith trace of the file, starting at 0. trace[i] returns a numpy array,
        and changes to this array will *not* be reflected on disk.

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

        Examples
        --------
        Read every other trace:

        >>> for tr in f.trace[::2]:
        ...     print(tr)

        Read all traces, last-to-first:

        >>> for tr in f.trace[::-1]:
        ...     tr.mean()

        Read a single value. The second [] is regular numpy array indexing, and
        supports all numpy operations, including negative indexing and slicing:

        >>> trace[0][0]
        1490.2
        >>> trace[0][1]
        1490.8
        >>> trace[0][-1]
        1871.3
        >>> trace[-1][100]
        1562.0
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

        Write the ith trace of the file, starting at 0. It accepts any
        array_like, but val must be at least as big as the underlying data
        trace.

        If val is longer than the underlying trace, it is essentially
        truncated.

        For the best performance, val should be a numpy.ndarray of sufficient
        size and same dtype as the file. segyio will warn on mismatched types,
        and attempt a conversion for you.

        Data is written immediately to disk. If writing multiple traces at
        once, and a write fails partway through, the resulting file is left in
        an unspecified state.

        Parameters
        ----------
        i   : int or slice
        val : array_like

        Notes
        -----
        .. versionadded:: 1.1

        Behaves like [] for lists.

        Examples
        --------
        Write a single trace:

        >>> trace[10] = list(range(1000))

        Write multiple traces:

        >>> trace[10:15] = np.array([cube[i] for i in range(5)])

        Write multiple traces with stride:

        >>> trace[10:20:2] = np.array([cube[i] for i in range(5)])

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
        """
        Headers, accessed by inline

        Returns
        -------
        line : HeaderLine
        """
        return HeaderLine(self, self.segy.iline, 'inline')

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
        """
        Headers, accessed by crossline

        Returns
        -------
        line : HeaderLine
        """
        return HeaderLine(self, self.segy.xline, 'crossline')

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
