"""
segy modes

Welcome to segyio.segy. Here you will find references and examples for the
various segy modes and how to interact with segy files. To start interacting
with files, please refer to the segyio.open and segyio.create documentation, by
typing `help(segyio.open)` or `help(segyio.create)`.

The primary way of obtaining a file instance is calling segyio.open. When you
have a file instance you can interact with it as described in this module.

The explanations and examples here are meant as a quick guide and reference.
You can also have a look at the example programs that are distributed with
segyio which you can find in the examples directory or where your distribution
installs example programs.
"""
import itertools
import numpy as np

from segyio._header import Header
from segyio._line import Line
from segyio._trace import Trace
from segyio._field import Field
import segyio._segyio as _segyio

from segyio.tracesortingformat import TraceSortingFormat


class SegyFile(object):

    def __init__(self, filename, mode, iline=189, xline=193, t0=1111.0):
        """
        Constructor, internal.
        """

        self._filename = filename
        self._mode = mode
        self._il = iline
        self._xl = xline

        # property value holders
        self._ilines = None
        self._xlines = None
        self._tracecount = None
        self._sorting = None
        self._offsets = None
        self._ext_headers = None
        self._samples = None

        # private values
        self._iline_length = None
        self._iline_stride = None
        self._xline_length = None
        self._xline_stride = None
        self._fmt = None
        self._tr0 = None
        self._bsz = None

        self.xfd = _segyio.open(filename, mode)

        super(SegyFile, self).__init__()

    def __enter__(self):
        """Internal.
        :rtype: segyio.segy.SegyFile
        """
        return self

    def __exit__(self, type, value, traceback):
        """Internal."""
        self.close()

    def flush(self):
        """Flush a file - write the library buffers to disk.

        This method is mostly useful for testing.

        It is not necessary to call this method unless you want to observe your
        changes while the file is still open. The file will automatically be
        flushed for you if you use the `with` statement when your routine is
        completed.

        Examples:
            Flush::
                >>> with segyio.open(path) as f:
                ...     # write something to f
                ...     f.flush()
        """
        _segyio.flush(self.xfd)

    def close(self):
        """Close the file.#

        This method is mostly useful for testing.

        It is not necessary to call this method if you're using the `with`
        statement, which will close the file for you.
        """
        _segyio.close(self.xfd)

    @property
    def sorting(self):
        """ :rtype: int """
        return self._sorting

    @property
    def tracecount(self):
        """ :rtype: int """
        return self._tracecount

    @property
    def samples(self):
        """ :rtype: int """
        return self._samples

    @property
    def offsets(self):
        """ :rtype: int """
        return self._offsets

    @property
    def ext_headers(self):
        """ :rtype: int """
        return self._ext_headers


    @property
    def header(self):
        """ Interact with segy in header mode.

        This mode gives access to reading and writing functionality of headers,
        both in individual (trace) mode and line mode. Individual headers are
        accessed via generators and are not read from or written to disk until
        the generator is realised and the header in question is used. Supports
        python slicing (yields a generator), as well as direct lookup.


        Examples:
            Reading a field in a trace::
                >>> import segyio
                >>> f = segyio.open("filename")
                >>> f.header[10][TraceField.offset]

            Writing a field in a trace::
                >>> f.header[10][TraceField.offset] = 5

            Copy a header from another header::
                >>> f.header[28] = f.header[29]

            Reading multiple fields in a trace. If raw, numerical offsets are
            used they must align with the defined byte offsets by the SEGY
            specification::
                >>> f.header[10][TraceField.offset, TraceField.INLINE_3D]
                >>> f.header[10][37, 189]

            Write multiple fields in a trace::
                >>> f.header[10] = { 37: 5, TraceField.INLINE_3D: 2484 }

            Iterate over headers and gather line numbers::
                >>> [h[TraceField.INLINE_3D] for h in f.header]
                >>> [h[25, 189] for h in f.header]

            Write field in all headers::
                >>> for h in f.header:
                ...     h[37] = 1
                ...     h = { TraceField.offset: 1, 2484: 10 }
                ...

            Read a field in 10 first headers::
                >>> [h[25] for h in f.header[0:10]]

            Read a field in every other header::
                >>> [h[37] for h in f.header[::2]]

            Write a field in every other header::
                >>> for h in f.header[::2]:
                ...     h = { TraceField.offset : 2 }
                ...

            Cache a header:
                >>> h = f.header[12]
                >>> x = foo()
                >>> h[37] = x

            A convenient way for operating on all headers of a file is to use the
            default full-file range.  It will write headers 0, 1, ..., n, but uses
            the iteration specified by the right-hand side (i.e. can skip headers
            etc).

            If the right-hand-side headers are exhausted before all the destination
            file headers the writing will stop, i.e. not all all headers in the
            destination file will be written to.

            Copy headers from file g to file f:
                >>> f = segyio.open("path to file")
                >>> g = segyio.open("path to another file")
                >>> f.header = g.header

            Set offset field::
                >>> f.header = { TraceField.offset: 5 }

            Copy every 12th header from the file g to f's 0, 1, 2...::
                >>> f.header = g.header[::12]
                >>> f.header[0] == g.header[0]
                True
                >>> f.header[1] == g.header[12]
                True
                >>> f.header[2] == g.header[2]
                False
                >>> f.header[2] == g.header[24]
                True
        :rtype: segyio._header.Header
        """
        return Header(self)

    @header.setter
    def header(self, val):
        if isinstance(val, Field) or isinstance(val, dict):
            val = itertools.repeat(val)

        h, buf = self.header, None
        for i, v in itertools.izip(xrange(self.tracecount), val):
            h[i, buf] = v

    @property
    def trace(self):
        """ Interact with segy in trace mode.

        This mode gives access to reading and writing functionality for traces.
        The primary data type is the numpy ndarray. Traces can be accessed
        individually or with python slices, and writing is done via assignment.

        All examples use `np` for `numpy`.

        Examples:
            Read all traces in file f and store in a list::
                >>> l = [np.copy(tr) for tr in f.trace]

            Do numpy operations on a trace::
                >>> tr = f.trace[10]
                >>> tr = np.transpose(tr)
                >>> tr = tr * 2
                >>> tr = tr - 100
                >>> avg = np.average(tr)

            Do numpy operations on every other trace::
                >>> for tr in f.trace[::2]:
                ...     print( np.average(tr) )
                ...

            Traverse traces in reverse::
                >>> for tr in f.trace[::-1]:
                ...     print( np.average(tr) )
                ...

            Double every trace value and write to disk. Since accessing a trace
            gives a numpy value, to write to the respective trace we need its index::
                >>> for i, tr in enumerate(f.trace):
                ...     tr = tr * 2
                ...     f.trace[i] = tr
                ...

            Reuse an array for memory efficiency when working with indices.
            When using slices or full ranges this is done for you::
                >>> tr = None
                >>> for i in xrange(100):
                ...     tr = f.trace[i, tr]
                ...     tr = tr * 2
                ...     print(np.average(tr))
                ...

            Read a value directly from a file. The second [] is numpy access
            and supports all numpy operations, including negative indexing and
            slicing::
                >>> f.trace[0][0]
                1490.2
                >>> f.trace[0][1]
                1490.8
                >>> f.trace[0][-1]
                1871.3
                >>> f.trace[-1][100]
                1562.0

            Trace mode supports len(), returning the number of traces in a
            file::
                >>> len(f.trace)
                300

            Convenient way for setting traces from 0, 1, ... n, based on the
            iterable set of traces on the right-hand-side.

            If the right-hand-side traces are exhausted before all the destination
            file traces the writing will stop, i.e. not all all traces in the
            destination file will be written.

            Copy traces from file f to file g::
                >>> f.trace = g.trace.

            Copy first half of the traces from g to f::
                >>> f.trace = g.trace[:len(g.trace)/2]

            Fill the file with one trace (filled with zeros)::
                >>> tr = np.zeros(f.samples)
                >>> f.trace = itertools.repeat(tr)

            For advanced users: sometimes you want to load the entire segy file
            to memory and apply your own structural manipulations or operations
            on it. Some segy files are very large and may not fit, in which
            case this feature will break down. This is an optimisation feature;
            using it should generally be driven by measurements.

            Read the first 10 traces::
                >>> f.trace.raw[0:10]

            Read *all* traces to memory::
                >>> f.trace.raw[:]

            Read every other trace to memory::
                >>> f.trace.raw[::2]
        """

        return Trace(self)

    @trace.setter
    def trace(self, val):
        tr = self.trace
        for i, v in itertools.izip(xrange(len(tr)), val):
            tr[i] = v

    def _shape_buffer(self, shape, buf):
        if buf is None:
            return np.empty(shape=shape, dtype=np.single)
        if not isinstance(buf, np.ndarray):
            return buf
        if buf.dtype != np.single:
            return np.empty(shape=shape, dtype=np.single)
        if buf.shape[0] == shape[0]:
            return buf
        if buf.shape != shape and buf.size == np.prod(shape):
            return buf.reshape(shape)
        return buf

    def _line_buffer(self, length, buf=None):
        shape = (length, self.samples)
        return self._shape_buffer(shape, buf)

    def _fread_line(self, trace0, length, stride, buf):
        return _segyio.read_line(self.xfd, trace0, length, stride, buf, self._tr0, self._bsz, self._fmt, self.samples)

    @property
    def ilines(self):
        """ :rtype: numpy.ndarray"""
        return self._ilines

    @property
    def iline(self):
        """ Interact with segy in inline mode.

        This mode gives access to reading and writing functionality for inlines.
        The primary data type is the numpy ndarray. Inlines can be accessed
        individually or with python slices, and writing is done via assignment.
        Note that accessing inlines uses the line numbers, not their position,
        so if a files has inlines [2400..2500], accessing line [0..100] will be
        an error. Note that each line is returned as a numpy array, meaning
        accessing the intersections of the inline and crossline is 0-indexed.

        Examples:
            Read an inline::
                >>> il = f.iline[2400]

            Copy every inline into a list::
                >>> l = [np.copy(x) for x in f.iline]

            The number of inlines in a file::
                >>> len(f.iline)

            Numpy operations on every other inline::
                >>> for line in f.iline[::2]:
                ...     line = line * 2
                ...     avg = np.average(line)
                ...     print(avg)
                ...

            Read inlines up to 2430::
                >>> for line in f.iline[:2430]:
                ...     print(np.average(line))
                ...

            Copy a line from file g to f::
                >>> f.iline[2400] = g.iline[2834]

            Copy lines from the first line in g to f, starting at 2400,
            ending at 2410 in f::
                >>> f.iline[2400:2411] = g.iline

            Convenient way for setting inlines, from left-to-right as the inline
            numbers are specified in the file.ilines property, from an iterable
            set on the right-hand-side.

            If the right-hand-side inlines are exhausted before all the destination
            file inlines the writing will stop, i.e. not all all inlines in the
            destination file will be written.

            Copy inlines from file f to file g::
                >>> f.iline = g.iline.

            Copy first half of the inlines from g to f::
                >>> f.iline = g.iline[:g.ilines[len(g.ilines)/2]]

            Copy every other inline from a different file::
                >>> f.iline = g.iline[::2]
        """
        il_len, il_stride = self._iline_length, self._iline_stride
        lines = self.ilines
        other_lines = self.xlines
        buffn = lambda x=None: self._line_buffer(il_len, x)
        readfn = self._fread_line

        def writefn(t0, length, step, val):
            val = buffn(val)
            for i, v in itertools.izip(xrange(t0, t0 + step * length, step), val):
                Trace.write_trace(i, v, self)

        return Line(self, il_len, il_stride, lines, other_lines, buffn, readfn, writefn, "Inline")

    @iline.setter
    def iline(self, value):
        self.iline[:] = value

    @property
    def xlines(self):
        """ :rtype: numpy.ndarray"""
        return self._xlines

    @property
    def xline(self):
        """ Interact with segy in crossline mode.

        This mode gives access to reading and writing functionality for crosslines.
        The primary data type is the numpy ndarray. crosslines can be accessed
        individually or with python slices, and writing is done via assignment.
        Note that accessing crosslines uses the line numbers, not their position,
        so if a files has crosslines [1400..1450], accessing line [0..100] will be
        an error. Note that each line is returned as a numpy array, meaning
        accessing the intersections of the inline and crossline is 0-indexed.

        Examples:
            Read an crossline::
                >>> il = f.xline[1400]

            Copy every crossline into a list::
                >>> l = [np.copy(x) for x in f.xline]

            The number of crosslines in a file::
                >>> len(f.xline)

            Numpy operations on every third crossline::
                >>> for line in f.xline[::3]:
                ...     line = line * 6
                ...     avg = np.average(line)
                ...     print(avg)
                ...

            Read crosslines up to 1430::
                >>> for line in f.xline[:1430]:
                ...     print(np.average(line))
                ...

            Copy a line from file g to f::
                >>> f.xline[1400] = g.xline[1603]

            Copy lines from the first line in g to f, starting at 1400,
            ending at 1415 in f::
                >>> f.xline[1400:1416] = g.xline


            Convenient way for setting crosslines, from left-to-right as the crossline
            numbers are specified in the file.xlines property, from an iterable
            set on the right-hand-side.

            If the right-hand-side crosslines are exhausted before all the destination
            file crosslines the writing will stop, i.e. not all all crosslines in the
            destination file will be written.

            Copy crosslines from file f to file g::
                >>> f.xline = g.xline.

            Copy first half of the crosslines from g to f::
                >>> f.xline = g.xline[:g.xlines[len(g.xlines)/2]]

            Copy every other crossline from a different file::
                >>> f.xline = g.xline[::2]
        """
        xl_len, xl_stride = self._xline_length, self._xline_stride
        lines = self.xlines
        other_lines = self.ilines
        buffn = lambda x=None: self._line_buffer(xl_len, x)
        readfn = self._fread_line

        def writefn(t0, length, step, val):
            val = buffn(val)
            for i, v in itertools.izip(xrange(t0, t0 + step * length, step), val):
                Trace.write_trace(i, v, self)

        return Line(self, xl_len, xl_stride, lines, other_lines, buffn, readfn, writefn, "Crossline")

    @xline.setter
    def xline(self, value):
        self.xline[:] = value

    def _depth_buffer(self, buf=None):
        il_len = self._iline_length
        xl_len = self._xline_length

        if self.sorting == TraceSortingFormat.CROSSLINE_SORTING:
            shape = (xl_len, il_len)
        elif self.sorting == TraceSortingFormat.INLINE_SORTING:
            shape = (il_len, xl_len)
        else:
            raise RuntimeError("Unexpected sorting type")

        return self._shape_buffer(shape, buf)

    @property
    def depth_slice(self):
        """ Interact with segy in depth slice mode.

        This mode gives access to reading and writing functionality for depth slices.
        The primary data type is the numpy ndarray. Depth slices can be accessed
        individually or with python slices, and writing is done via assignment.
        Note that each slice is returned as a numpy array, meaning
        accessing the values of the slice is 0-indexed.

        Examples:
            Read a depth slice:
                >>> il = f.depth_slice[199]

            Copy every depth slice into a list::
                >>> l = [np.copy(x) for x in f.depth_slice]

            The number of depth slices in a file::
                >>> len(f.depth_slice)

            Numpy operations on every third depth slice::
                >>> for depth_slice in f.depth_slice[::3]:
                ...     depth_slice = depth_slice * 6
                ...     avg = np.average(depth_slice)
                ...     print(avg)
                ...

            Read depth_slices up to 250::
                >>> for depth_slice in f.depth_slice[:250]:
                ...     print(np.average(depth_slice))
                ...

            Copy a line from file g to f::
                >>> f.depth_slice[4] = g.depth_slice[19]

            Copy lines from the first line in g to f, starting at 10,
            ending at 49 in f::
                >>> f.depth_slice[10:50] = g.depth_slice


            Convenient way for setting depth slices, from left-to-right as the depth slices
            numbers are specified in the file.depth_slice property, from an iterable
            set on the right-hand-side.

            If the right-hand-side depth slices are exhausted before all the destination
            file depth slices the writing will stop, i.e. not all all depth slices in the
            destination file will be written.

            Copy depth slices from file f to file g::
                >>> f.depth_slice = g.depth_slice.

            Copy first half of the depth slices from g to f::
                >>> f.depth_slice = g.depth_slice[:g.samples/2]]

            Copy every other depth slices from a different file::
                >>> f.depth_slice = g.depth_slice[::2]
        """
        indices = np.asarray(list(range(self.samples)), dtype=np.uintc)
        other_indices = np.asarray([0], dtype=np.uintc)
        buffn = self._depth_buffer

        def readfn(depth, length, stride, buf):
            buf_view = buf.reshape(self._iline_length * self._xline_length)

            for i, trace_buf in enumerate(self.trace):
                buf_view[i] = trace_buf[depth]

            return buf

        def writefn(depth, length, stride, val):
            val = buffn(val)

            buf_view = val.reshape(self._iline_length * self._xline_length)

            for i, trace_buf in enumerate(self.trace):
                trace_buf[depth] = buf_view[i]
                self.trace[i] = trace_buf

        return Line(self, self.samples, 1, indices, other_indices, buffn, readfn, writefn, "Depth")

    @depth_slice.setter
    def depth_slice(self, value):
        self.depth_slice[:] = value

    @property
    def text(self):
        """ Interact with segy in text mode.

        This mode gives access to reading and writing functionality for textual
        headers.

        The primary data type is the python string. Reading textual headers is
        done via [], and writing is done via assignment. No additional
        structure is built around the textual header, so everything is treated
        as one long string without line breaks.

        Examples:
            Print the textual header::
                >>> print(f.text[0])

            Print the first extended textual header::
                >>> print(f.text[1])

            Write a new textual header::
                >>> f.text[0] = make_new_header()

            Print a textual header line-by-line::
                >>> # using zip, from the zip documentation
                >>> text = f.text[0]
                >>> lines = map(''.join, zip( *[iter(text)] * 80))
                >>> for line in lines:
                ...     print(line)
                ...
        """

        class TextHeader:

            def __getitem__(inner, index):
                if index > self.ext_headers:
                    raise IndexError("Textual header %d not in file" % index)

                return _segyio.read_textheader(self.xfd, index)

            def __setitem__(inner, index, val):
                if index > self.ext_headers:
                    raise IndexError("Textual header %d not in file" % index)

                _segyio.write_textheader(self.xfd, index, val)

        return TextHeader()

    @property
    def bin(self):
        """ Interact with segy in binary mode.

        This mode gives access to reading and writing functionality for the
        binary header. Please note that using numeric binary offsets uses the
        offset numbers from the specification, i.e. the first field of the
        binary header starts at 3201, not 1. If you're using the enumerations
        this is handled for you.

        Examples:
            Copy a header from file g to file f::
                >>> f.bin = g.bin

            Reading a field in a trace::
                >>> traces_per_ensemble = f.bin[3213]

            Writing a field in a trace::
                >>> f.bin[BinField.Traces] = 5

            Reading multiple fields::
                >>> d = f.bin[BinField.Traces, 3233]

            Copy a field from file g to file f::
                >>> f.bin[BinField.Format] = g.bin[BinField.Format]

            Copy full binary from file f to file g::
                >>> f.bin = g.bin

            Copy multiple fields from file f to file g::
                >>> f.bin = g.bin[BinField.Traces, 3233]

            Write field in binary header via dict::
                >>> f.bin = { BinField.Traces: 350 }

            Write multiple fields in a trace::
                >>> f.bin = { 3213: 5, BinField.SweepFrequencyStart: 17 }
        """

        return Field.binary(self)

    @bin.setter
    def bin(self, value):
        self.bin.update(value)

    @property
    def format(self):
        d = {
            1: "4-byte IBM float",
            2: "4-byte signed integer",
            3: "2-byte signed integer",
            4: "4-byte fixed point with gain",
            5: "4-byte IEEE float",
            8: "1-byte signed char"
        }

        class fmt:
            def __int__(inner):
                return self._fmt

            def __str__(inner):
                if not self._fmt in d:
                    return "Unknown format"

                return d[self._fmt]

        return fmt()

    def free(self):
        """Internal."""
        pass


class spec:
    def __init__(self):
        self.iline = 189
        self.ilines = None
        self.xline = 193
        self.xlines = None
        self.offsets = 1
        self.samples = None
        self.tracecount = None
        self.ext_headers = 0
        self.format = None
        self.sorting = None
        self.t0 = 1111.0
