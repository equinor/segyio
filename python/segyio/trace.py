import collections
try: from future_builtins import zip
except ImportError: pass

import numpy as np
import warnings

from segyio._raw_trace import RawTrace

class Trace(collections.Sequence):
    def __init__(self, file, samples):
        self._file = file
        self.length = file.tracecount
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
            if i < 0:
                i += len(self)

            if not 0 <= i < len(self):
                # in python2, int-slice comparison does not raise a type error,
                # (but returns False), so force a type-error if this still
                # isn't an int-like.
                i += 0
                msg = 'Trace out of range: 0 <= {} < {}'
                raise IndexError(msg.format(i, len(self)))

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
        dtype = self.dtype

        try:
            val.dtype
        except AttributeError:
            msg = 'Implicit conversion from {} to {} (performance)'
            warnings.warn(msg.format(type(val), np.ndarray), RuntimeWarning)
            val = np.asarray(val)

        if val.dtype != dtype:
            # TODO: message depending on narrowing/float-conversion
            msg = 'Implicit conversion from {} to {} (narrowing)'
            warnings.warn(msg.format(val.dtype, dtype), RuntimeWarning)

        # asarray only copies if it has to (differing dtypes). non-numpy arrays
        # have already been converted
        xs = np.asarray(val, order = 'C', dtype = dtype)

        if isinstance(i, slice):
            for j, x in zip(range(*i.indices(len(self))), val):
                self[j] = x

            return

        if i < 0:
            i += len(self)

        if not 0 <= i < len(self):
            # in python2, int-slice comparison does not raise a type error,
            # (but returns False), so force a type-error if this still
            # isn't an int-like.
            i += 0
            msg = 'Trace out of range: 0 <= {} < {}'
            raise IndexError(msg.format(i, len(self)))

        # TODO:  check if len(xs) > shape, and optionally warn on truncating
        # writes

        self.filehandle.puttr(i, xs)

    def __len__(self):
        return self.length

    def __repr__(self):
        return "Trace(traces = {}, samples = {})".format(len(self), self.shape)

    @property
    def raw(self):
        """ :rtype: segyio.RawTrace """
        return RawTrace(self)
