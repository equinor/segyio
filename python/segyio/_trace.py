import numpy as np
import segyio
from segyio._raw_trace import RawTrace


class Trace:

    def __init__(self, file):
        self._file = file
        """:type: segyio.file"""

    def __getitem__(self, index, buf=None):
        if isinstance(index, tuple):
            return self.__getitem__(index[0], index[1])

        buf = self._trace_buffer(buf)

        if isinstance(index, int):
            if not 0 <= abs(index) < len(self):
                raise IndexError("Trace %d not in range (-%d,%d)", (index, len(self), len(self)))

            return self._readtr(index, buf)

        elif isinstance(index, slice):
            def gen():
                for i in range(*index.indices(len(self))):
                    yield self._readtr(i, buf)

            return gen()

        else:
            raise TypeError("Key must be int, slice, (int,np.ndarray) or (slice,np.ndarray)")

    def __setitem__(self, index, val):
        if not 0 <= abs(index) < len(self):
            raise IndexError("Trace %d not in range (-%d,%d)", (index, len(self), len(self)))

        if not isinstance(val, np.ndarray):
            raise TypeError("Value must be numpy.ndarray")

        if val.dtype != np.single:
            raise TypeError("Numpy array must be of type single")

        shape = (len(self._file.samples),)

        if val.shape[0] < shape[0]:
            raise TypeError("Array wrong shape. Expected minimum %s, was %s" % (shape, val.shape))

        if isinstance(index, int):
            self._writetr(index, val)

        elif isinstance(index, slice):
            for i, buf in range(*index.indices(len(self))), val:
                self._writetr(i, val)

        else:
            raise KeyError("Wrong shape of index")

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

    def _readtr(self, traceno, buf=None):
        buf = self._trace_buffer(buf)

        tracecount = self._file.tracecount
        trace0 = self._file._tr0
        bsz = self._file._bsz
        fmt = self._file._fmt
        smp = len(self._file.samples)
        return segyio._segyio.read_trace(self._file.xfd, traceno, tracecount, buf, trace0, bsz, fmt, smp)

    def _writetr(self, traceno, buf):
        self.write_trace(traceno, buf, self._file)

    @classmethod
    def write_trace(cls, traceno, buf, segy):
        """
        :type traceno: int
        :type buf: ?
        :type segy: segyio.SegyFile
        """
        segyio._segyio.write_trace(segy.xfd, traceno,
                                   buf,
                                   segy._tr0, segy._bsz,
                                   segy._fmt, len(segy.samples))

    @property
    def raw(self):
        """ :rtype: segyio.RawTrace """
        return RawTrace(self)
