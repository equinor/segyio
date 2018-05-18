import collections
try: from future_builtins import zip
except ImportError: pass

import numpy as np
import warnings

from segyio._raw_trace import RawTrace

class Trace(collections.Sequence):
    index_errmsg = "Trace {0} not in range [-{1},{1}]"

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

    def __setitem__(self, index, val):
        if isinstance(index, slice):
            for i, x in zip(range(*index.indices(len(self))), val):
                self.write_trace(i, x, self._file)
            return

        if not 0 <= abs(index) < len(self):
            raise IndexError(self.index_errmsg.format(index, len(self)-1))

        self.write_trace(index, val, self._file)

    def __len__(self):
        return self.length

    def __repr__(self):
        return "Trace(traces = {}, samples = {})".format(len(self), self.shape)

    def _trace_buffer(self, buf=None):
        shape = self._file.samples.shape
        dtype = self._file.dtype

        if buf is None:
            buf = np.empty(shape=shape, dtype=dtype)
        elif not isinstance(buf, np.ndarray):
            raise TypeError("Buffer must be None or numpy.ndarray")
        elif buf.dtype != dtype:
            buf = np.empty(shape=shape, dtype=dtype)

        return buf

    @classmethod
    def write_trace(cls, traceno, buf, segy):
        """
        :type traceno: int
        :type buf: ?
        :type segy: segyio.SegyFile
        """

        dtype = segy.dtype

        try:
            buf.dtype
        except AttributeError:
            msg = 'Implicit conversion from {} to {} (performance)'
            warnings.warn(msg.format(type(buf), np.ndarray), RuntimeWarning)
            buf = np.asarray(buf)

        if buf.dtype != dtype:
            # TODO: message depending on narrowing/float-conversion
            msg = 'Implicit conversion from {} to {} (narrowing)'
            warnings.warn(msg.format(buf.dtype, dtype), RuntimeWarning)

        # asarray only copies if it has to (differing dtypes). non-numpy arrays
        # have already been converted
        xs = np.asarray(buf, order = 'C', dtype = dtype)
        segy.xfd.puttr(traceno, xs)

    @property
    def raw(self):
        """ :rtype: segyio.RawTrace """
        return RawTrace(self)
