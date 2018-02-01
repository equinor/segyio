try: from future_builtins import zip
except ImportError: pass

import numpy as np

from segyio._raw_trace import RawTrace

class Trace:
    index_errmsg = "Trace {0} not in range [-{1},{1}]"
    def __init__(self, file):
        self._file = file
        """:type: segyio.file"""

    def __getitem__(self, index, buf=None):
        if isinstance(index, tuple):
            return self.__getitem__(index[0], index[1])

        buf = self._trace_buffer(buf)

        if isinstance(index, slice):
            # always read the trace into a second buffer. This is to provide
            # exception safety: if an exception is raised and at least one
            # array has already been yielded to the caller, failing to read the
            # next trace won't make the already-returned array garbage
            def gen():
                buf1 = buf
                buf2 = self._trace_buffer(None)
                for i in range(*index.indices(len(self))):
                    buf1, buf2 = self._readtr(i, 1, 1, buf1, buf2)
                    yield buf1

            return gen()

        if not 0 <= abs(index) < len(self):
            raise IndexError(self.index_errmsg.format(index, len(self)-1))

        # map negative a negative to the corresponding positive value
        start = (index + len(self)) % len(self)
        return self._readtr(start, 1, 1, buf)[0]

    def __setitem__(self, index, val):
        if isinstance(index, slice):
            for i, x in zip(range(*index.indices(len(self))), val):
                self.write_trace(i, x, self._file)
            return

        if not 0 <= abs(index) < len(self):
            raise IndexError(self.index_errmsg.format(index, len(self)-1))

        self.write_trace(index, val, self._file)

    def __len__(self):
        return self._file.tracecount

    def __iter__(self):
        return self[:]

    def __repr__(self):
        return "Trace(traces = {}, samples = {})".format(len(self), self._file.samples)

    def _trace_buffer(self, buf=None):
        shape = self._file.samples.shape

        if buf is None:
            buf = np.empty(shape=shape, dtype=np.single)
        elif not isinstance(buf, np.ndarray):
            raise TypeError("Buffer must be None or numpy.ndarray")
        elif buf.dtype != np.single:
            buf = np.empty(shape=shape, dtype=np.single)

        return buf

    def _readtr(self, start, step, length, buf, buf1 = None):
        if buf1 is None:
            buf1 = buf

        buf1 = self._file.xfd.gettr(buf1, start, step, length)

        return buf1, buf

    @classmethod
    def write_trace(cls, traceno, buf, segy):
        """
        :type traceno: int
        :type buf: ?
        :type segy: segyio.SegyFile
        """
        if isinstance(buf, np.ndarray) and buf.dtype != np.single:
            raise TypeError("Numpy array must be of type single")

        segy.xfd.puttr(traceno, buf)

    @property
    def raw(self):
        """ :rtype: segyio.RawTrace """
        return RawTrace(self)
