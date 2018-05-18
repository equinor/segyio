import collections
import itertools
try: from future_builtins import zip
except ImportError: pass

import segyio
from .line import Line
from .field import Field

class Header(collections.Sequence):
    def __init__(self, segy):
        self.segy = segy
        self.length = segy.tracecount

    @staticmethod
    def _header_buffer(buf=None):
        if buf is None:
            buf = bytearray(segyio._segyio.thsize())
        return buf

    def __getitem__(self, traceno):
        if isinstance(traceno, slice):
            def gen():
                # double-buffer the header. when iterating over a range, we
                # want to make sure the visible change happens as late as
                # possible, and that in the case of exception the last valid
                # header was untouched. this allows for some fancy control
                # flow, and more importantly helps debugging because you can
                # fully inspect and interact with the last good value.
                x = Field.trace(None, self.segy)
                buf = bytearray(x.buf)
                for i in range(*traceno.indices(len(self))):
                    # skip re-invoking __getitem__, just update the buffer
                    # directly with fetch, and save some initialisation work
                    buf = x.fetch(buf, i)
                    x.buf[:] = buf
                    x.traceno = i
                    yield x

            return gen()

        if traceno < 0:
            traceno += len(self)

        if traceno >= len(self) or traceno < 0:
            msg = 'Header out of range: 0 <= {} < {}'
            raise IndexError(msg.format(traceno, self.segy.tracecount))

        return Field.trace(traceno = traceno, segy = self.segy)

    def __setitem__(self, traceno, val):
        # library-provided loops can re-use a buffer for the lookup, even in
        # __setitem__, so we might need to unpack the tuple to reuse the buffer
        if isinstance(traceno, tuple):
            buf = traceno[1]
            traceno = traceno[0]

        self.__getitem__(traceno).update(val)

    def __repr__(self):
        return "Header(traces = {})".format(self.segy.samples)

    def __len__(self):
        return self.length

    def readfn(self, t0, length, stride, *_):
        def gen():
            start = t0
            step = stride * len(self.segy.offsets)
            stop = t0 + (length * step)
            for i in range(start, stop, step):
                yield self[i]

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
