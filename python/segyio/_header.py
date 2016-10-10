import itertools

import segyio
from segyio._line import Line
from segyio._field import Field


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

        if isinstance(traceno, slice):
            gen_buf = self._header_buffer(buf)

            def gen():
                for i in xrange(*traceno.indices(self.segy.tracecount)):
                    yield self.__getitem__(i, gen_buf)

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

    def readfn(self, t0, length, stride, buf):
        def gen():
            start = t0
            stop = t0 + (length * stride)
            for i in xrange(start, stop, stride):
                yield Field.trace(buf, traceno=i, segy=self.segy)

        return gen()

    def writefn(self, t0, length, stride, val):
        start = t0
        stop = t0 + (length * stride)

        if isinstance(val, Field) or isinstance(val, dict):
            val = itertools.repeat(val)

        for i, x in itertools.izip(xrange(start, stop, stride), val):
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
        for i, src in itertools.izip(self.segy.ilines, value):
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

        for i, src in itertools.izip(self.segy.xlines, value):
            self.xline[i] = src
