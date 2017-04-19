import itertools

import segyio
from segyio._line import Line
from segyio._field import Field

try:
    from itertools import izip as zip
except ImportError:  # will be 3.x series
    pass


class Header(object):
    def __init__(self, segy):
        self.segy = segy

    @staticmethod
    def _header_buffer(buf=None):
        if buf is None:
            buf = segyio._segyio.empty_traceheader()
        return buf

    def __getitem__(self, traceno, buf=None):
        if isinstance(traceno, tuple):
            return self.__getitem__(traceno[0], traceno[1])

        buf = self._header_buffer(buf)

        if isinstance(traceno, slice):
            def gen():
                buf1, buf2 = self._header_buffer(), self._header_buffer()
                for i in range(*traceno.indices(self.segy.tracecount)):
                    x = self.__getitem__(i, buf1)
                    buf2, buf1 = buf1, buf2
                    yield x

            return gen()

        return Field.trace(buf, traceno=traceno, segy=self.segy)

    def __setitem__(self, traceno, val):
        buf = None

        # library-provided loops can re-use a buffer for the lookup, even in
        # __setitem__, so we might need to unpack the tuple to reuse the buffer
        if isinstance(traceno, tuple):
            buf = traceno[1]
            traceno = traceno[0]

        self.__getitem__(traceno, buf).update(val)

    def __iter__(self):
        return self[:]

    def __repr__(self):
        return "Header(traces = {})".format(self.segy.samples)

    def readfn(self, t0, length, stride, *_):
        def gen():
            buf1, buf2 = self._header_buffer(), self._header_buffer()
            start = t0
            step = stride * len(self.segy.offsets)
            stop = t0 + (length * step)
            for i in range(start, stop, step):
                x = Field.trace(buf1, traceno=i, segy=self.segy)
                buf2, buf1 = buf1, buf2
                yield x

        return gen()

    def writefn(self, t0, length, stride, val):
        start = t0
        stride *= len(self.segy.offsets)
        stop = t0 + (length * stride)

        if isinstance(val, Field) or isinstance(val, dict):
            val = itertools.repeat(val)

        for i, x in zip(range(start, stop, stride), val):
            self[i] = x

    @property
    def iline(self):
        """:rtype: Line"""
        segy = self.segy
        length = segy._iline_length
        stride = segy._iline_stride
        lines = segy.ilines
        other_lines = segy.xlines
        buffn = self._header_buffer

        return Line(segy, length, stride, lines, other_lines, buffn, self.readfn, self.writefn, "Inline")

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
        """:rtype: Line"""
        segy = self.segy
        length = segy._xline_length
        stride = segy._xline_stride
        lines = segy.xlines
        other_lines = segy.ilines
        buffn = self._header_buffer

        return Line(segy, length, stride, lines, other_lines, buffn, self.readfn, self.writefn, "Crossline")

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
