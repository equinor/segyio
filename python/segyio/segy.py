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

import sys, os, errno
import itertools
import numpy as np
import ctypes as ct
from cwrap import BaseCClass, BaseCEnum, Prototype
from segyio import TraceField, BinField

class _Segyio(Prototype):
    SEGYIO_LIB = ct.CDLL("libsegyio.so", use_errno=True)

    def __init__(self, prototype, bind=True):
        super(_Segyio, self).__init__(_Segyio.SEGYIO_LIB, prototype, bind=bind)

def _floatp(obj):
    return obj.ctypes.data_as(ct.POINTER(ct.c_float))

# match the SEGY_ERROR enum from segy.h
class _error(BaseCEnum):
    TYPE_NAME = "SEGY_ERROR"

    OK = None
    FOPEN_ERROR = None
    FSEEK_ERROR = None
    FREAD_ERROR = None
    FWRITE_ERROR = None
    INVALID_FIELD = None
    INVALID_SORTING = None
    MISSING_LINE_INDEX = None
    INVALID_OFFSETS = None
    TRACE_SIZE_MISMATCH = None

_error.addEnum("OK", 0)
_error.addEnum("FOPEN_ERROR", 1)
_error.addEnum("FSEEK_ERROR", 2)
_error.addEnum("FREAD_ERROR", 3)
_error.addEnum("FWRITE_ERROR", 4)
_error.addEnum("INVALID_FIELD", 5)
_error.addEnum("INVALID_SORTING", 6)
_error.addEnum("MISSING_LINE_INDEX", 7)
_error.addEnum("INVALID_OFFSETS", 8)
_error.addEnum("TRACE_SIZE_MISMATCH", 9)

def _header_buffer(buf = None):
    if buf is None:
        return (ct.c_char * 240)()

    if len(buf) < 240:
        raise ValueError("Buffer must be a minimum %d size'd byte array." % 240)

    return buf

def _set_field(buf, field, x):
    errc = file._set_field(buf, int(field), x)
    err = _error(errc)

    if err != _error.OK:
        raise IndexError("Invalid byte offset %d" % field)

def _get_field(buf, field, x):
    errc = file._get_field(buf, int(field), ct.byref(x))
    err = _error(errc)

    if err != _error.OK:
        raise IndexError("Invalid byte offset %d" % field)

    return int(x.value)

def _write_header(buf, segy, traceno):
    errc = segy._write_header(traceno, buf, segy._tr0, segy._bsz)
    err = _error(errc)

    if err != _error.OK:
        errno = ct.get_errno()
        raise OSError(errno, "Error writing header for trace %d: %s" % (traceno, os.strerror(errno)))


class _line:
    """ Line mode for traces and trace headers. Internal.

    The _line class provides an interface for line-oriented operations. The
    line reading operations themselves are not streaming - it's assumed than
    when a line is queried it's somewhat limited in size and will comfortably
    fit in memory, and that the full line is interesting. This also applies to
    line headers; however, all returned values support the iterable protocol so
    they work fine together with the streaming bits of this library.

    _line should not be instantiated directly by users, but rather returned
    from the iline/xline properties of file or from the header mode. Any
    direct construction of this should be conisdered an error.
    """
    def __init__(self, segy, length, stride, lines, raw_lines, other_lines, buffn, readfn, writefn, name):
        self.segy = segy
        self.len = length
        self.stride = stride
        self.lines = lines
        self.raw_lines = raw_lines
        self.other_lines = other_lines
        self.name = name
        self.buffn = buffn
        self.readfn = readfn
        self.writefn = writefn

    def __getitem__(self, lineno, buf = None):
        if isinstance(lineno, tuple):
            return self.__getitem__(lineno[0], lineno[1])

        buf = self.buffn(buf)

        if isinstance(lineno, int):
            t0 = self.segy._fread_trace0(lineno, len(self.other_lines), self.stride, self.raw_lines, self.name)
            return self.readfn(t0, self.len, self.stride, buf)

        elif isinstance(lineno, slice):
            # in order to support [:end] syntax, we must make sure
            # start has a non-None value. lineno.indices() would set it
            # to 0, but we don't know if that's a reasonable value or
            # not. If start is None we set it to the first line
            if lineno.start is None:
                lineno = slice(self.lines[0], lineno.stop, lineno.step)

            def gen():
                s = set(self.lines)
                rng = xrange(*lineno.indices(self.lines[-1] + 1))

                # use __getitem__ lookup to avoid tuple
                # construction and unpacking and fast-forward
                # into the interesting code path
                for i in itertools.ifilter(s.__contains__, rng):
                    yield self.__getitem__(i, buf)

            return gen()

    def __setitem__(self, lineno, val):
        if isinstance(lineno, slice):
            if lineno.start is None:
                lineno = slice(self.lines[0], lineno.stop, lineno.step)

            rng = xrange(*lineno.indices(self.lines[-1] + 1))
            s = set(self.lines)

            for i, x in itertools.izip(filter(s.__contains__, rng), val):
                self.__setitem__(i, x)

            return

        t0 = self.segy._fread_trace0(lineno, len(self.other_lines), self.stride, self.raw_lines, self.name)
        self.writefn(t0, self.len, self.stride, val)

    def __len__(self):
        return len(self.lines)

    def __iter__(self):
        buf = self.buffn()
        for i in self.lines:
            yield self.__getitem__(i, buf)

class _header:
    def __init__(self, segy):
        self.segy = segy

    class proxy:
        def __init__(inner, buf, traceno = None, segy = None, get_field = _get_field, set_field = _set_field, write = _write_header, field_type = TraceField):
            inner.buf = buf
            inner.traceno = traceno
            inner._segy = segy
            inner._get_field = get_field
            inner._set_field = set_field
            inner._field_type = field_type
            inner._write = write

        def __getitem__(inner, field):
            val = ct.c_int()

            # add some structure so we can always iterate over fields
            if isinstance(field, int) or isinstance(field, inner._field_type):
                field = [field]

            d = { inner._field_type(f): inner._get_field(inner.buf, f, val) for f in field }

            # unpack the dictionary. if header[field] is requested, a
            # plain, unstructed output is expected, but header[f1,f2,f3]
            # yields a dict
            if len(d) == 1:
                return d.values()[0]

            return d

        def __setitem__(inner, field, val):
            inner._set_field(inner.buf, field, val)
            inner._write(inner.buf, inner._segy, inner.traceno)

    def __getitem__(self, traceno, buf = None):
        if isinstance(traceno, tuple):
            return self.__getitem__(traceno[0], traceno[1])

        buf = _header_buffer(buf)

        if isinstance(traceno, slice):
            def gen():
                for i in xrange(*traceno.indices(self.segy.tracecount)):
                    yield self.__getitem__(i, buf)

            return gen()

        buf = self.segy._readh(traceno, buf)
        return _header.proxy(buf, traceno = traceno, segy = self.segy)

    def __setitem__(self, traceno, val):
        buf = None

        # library-provided loops can re-use a buffer for the lookup, even in
        # __setitem__, so we might need to unpack the tuple to reuse the buffer
        if isinstance(traceno, tuple):
            buf = traceno[1]
            traceno = traceno[0]

        if isinstance(val, dict):
            try:
                # try to read a buffer. If the file was created by
                # libsegyio this might not have been written to before and
                # getitem might fail, so we try to read it and if it fails
                # we check if we're still within bounds if we are we create
                # an empty header and write to that
                buf = self.__getitem__(traceno, buf).buf

            except:
                if traceno >= self.segy.tracecount: raise
                buf = _header_buffer(buf)

            for f, v in val.items():
                _set_field(buf, f, v)

        else:
            buf = val.buf

        _write_header(buf, self.segy, traceno)

    def __iter__(self):
        return self[:]

    def readfn(self, t0, length, stride, buf):
        def gen():
            start = t0
            stop  = t0 + (length * stride)
            for i in xrange(start, stop, stride):
                self.segy._readh(i, buf)
                yield _header.proxy(buf, traceno = i, segy = self.segy)

        return gen()

    def writefn(self, t0, length, stride, val):
        start = t0
        stop  = t0 + (length * stride)

        if isinstance(val, _header.proxy) or isinstance(val, dict):
            val = itertools.repeat(val)

        for i, x in itertools.izip(xrange(start, stop, stride), val):
            self[i] = x


    @property
    def iline(self):
        segy = self.segy
        length = segy._iline_length
        stride = segy._iline_stride
        lines  = segy.ilines
        raw_lines = segy._raw_ilines
        other_lines  = segy.xlines
        buffn = _header_buffer

        return _line(segy, length, stride, lines, raw_lines, other_lines, buffn, self.readfn, self.writefn, "Inline")

    @property
    def xline(self):
        segy = self.segy
        length = segy._xline_length
        stride = segy._xline_stride
        lines  = segy.xlines
        raw_lines = segy._raw_xlines
        other_lines  = segy.xlines
        buffn = _header_buffer

        return _line(segy, length, stride, lines, raw_lines, other_lines, buffn, self.readfn, self.writefn, "Crossline")

    def __setattr__(self, name, value):
        """Write iterables to lines

        Examples:
            setattr supports writing to *all* inlines and crosslines via
            assignment, regardless of data source and format. Will respect the
            sample size and structure of the file being assigned to, so if the
            argument traces are longer than that of the file being written to
            the surplus data will be ignored. Uses same rules for writing as
            `f.iline[i] = x`.
        """
        if name == "iline":
            for i, src in itertools.izip(self.segy.ilines, value):
                self.iline[i] = src

        if name == "xline":
            for i, src in itertools.izip(self.segy.xlines, value):
                self.xline[i] = src

        else:
            self.__dict__[name] = value
            return

class file(BaseCClass):
    TYPE_NAME = "FILE"

    _open  = _Segyio("void* fopen(char*, char*)", bind = False)
    _flush = _Segyio("int fflush(FILE)")
    _close = _Segyio("int fclose(FILE)")

    _binheader_size     = _Segyio("uint segy_binheader_size()", bind = False)
    _binheader          = _Segyio("int segy_binheader(FILE, char*)")
    _write_binheader    = _Segyio("int segy_write_binheader(FILE, char*)")

    _trace0         = _Segyio("int  segy_trace0(char*)", bind = False)
    _get_samples    = _Segyio("uint segy_samples(char*)", bind = False)
    _format         = _Segyio("int  segy_format(char*)", bind = False)
    _sorting        = _Segyio("int  segy_sorting(FILE, int, int, int*, long, uint)")
    _trace_bsize    = _Segyio("uint segy_trace_bsize(uint)", bind = False)
    _traces         = _Segyio("uint segy_traces(FILE, uint*, long, uint)")
    _offsets        = _Segyio("uint segy_offsets(FILE, int, int, uint, uint*, long, uint)")

    _get_field      = _Segyio("int segy_get_field(char*, int, int*)", bind = False)
    _set_field      = _Segyio("int segy_set_field(char*, int, int)", bind = False)

    _get_bfield   = _Segyio("int segy_get_bfield(char*, int, int*)", bind = False)
    _set_bfield   = _Segyio("int segy_set_bfield(char*, int, int)", bind = False)

    _to_native      = _Segyio("int segy_to_native(int, uint, float*)", bind = False)
    _from_native    = _Segyio("int segy_from_native(int, uint, float*)", bind = False)

    _read_header    = _Segyio("int segy_traceheader(FILE, uint, char*, long, uint)")
    _write_header   = _Segyio("int segy_write_traceheader(FILE, uint, char*, long, uint)")
    _read_trace     = _Segyio("int segy_readtrace(FILE, uint, float*, long, uint)")
    _write_trace    = _Segyio("int segy_writetrace(FILE, uint, float*, long, uint)")

    _count_lines        = _Segyio("int segy_count_lines(FILE, int, uint, uint*, uint*, long, uint)")
    _inline_length      = _Segyio("int segy_inline_length(int, uint, uint, uint, uint*)", bind = False)
    _inline_stride      = _Segyio("int segy_inline_stride(int, uint, uint*)", bind = False)
    _inline_indices     = _Segyio("int segy_inline_indices(FILE, int, int, uint, uint, uint, uint*, long, uint)")
    _crossline_length   = _Segyio("int segy_crossline_length(int, uint, uint, uint, uint*)", bind = False)
    _crossline_stride   = _Segyio("int segy_crossline_stride(int, uint, uint*)", bind = False)
    _crossline_indices  = _Segyio("int segy_crossline_indices(FILE, int, int, uint, uint, uint, uint*, long, uint)")
    _line_trace0        = _Segyio("int segy_line_trace0( uint, uint, uint, uint*, uint, uint*)", bind = False)
    _read_line          = _Segyio("int segy_read_line(FILE, uint, uint, uint, float*, long, uint)")
    _write_line         = _Segyio("int segy_write_line(FILE, uint, uint, uint, float*, long, uint)")

    _textsize = _Segyio("int segy_textheader_size()", bind = False)
    _texthdr  = _Segyio("int segy_textheader(FILE, char*)")
    _write_texthdr  = _Segyio("int segy_write_textheader(FILE, uint, char*)")

    def __init__(self, filename, mode, iline = 189 , xline = 193, t0 = 1111.0 ):
        """
        Constructor, internal.
        """
        self._filename  = filename
        self._mode      = mode
        self._il        = iline
        self._xl        = xline
        fd              = self._open(filename, mode)

        if not fd:
            errno = ct.get_errno()
            strerror = os.strerror(errno)
            raise OSError(errno, "Opening file '%s' failed: %s" % (filename, strerror))

        super(file, self).__init__(fd)

    def _init_traces(self):
        traces = ct.c_uint()
        errc = self._traces(ct.byref(traces), self._tr0, self._bsz)
        err = _error(errc)

        if err == _error.OK:
            return int(traces.value)

        if err == _error.TRACE_SIZE_MISMATCH:
            raise RuntimeError("Number of traces is not consistent with file size. File probably corrupt.")

        errno = ct.get_errno()
        raise OSError("Error while detecting number of traces: %s" % os.strerror(errno))

    def _init_sorting(self):
        sorting = ct.c_int()
        errc = self._sorting( self._il, self._xl, ct.byref(sorting), self._tr0, self._bsz)

        err = _error(errc)

        if err == _error.OK:
            return int(sorting.value)

        if err == _error.INVALID_FIELD:
            raise ValueError("Invalid inline (%d) or crossline (%d) field/byte offset. "\
                             "Too large or between valid byte offsets" % (self._il, self._xl))

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unable to determine sorting. File probably corrupt.")

        errno = ct.get_errno()
        raise OSError(errno, "Error while detecting file sorting: %s" % os.strerror(errno))

        return int(sorting.value)

    def _init_offsets(self):
        offsets = ct.c_uint()
        errc = self._offsets(self._il, self._xl, self.tracecount, ct.byref(offsets), self._tr0, self._bsz)

        err = _error(errc)

        if err == _error.OK:
            return int(offsets.value)

        if err == _error.INVALID_FIELD:
            raise ValueError("Invalid inline (%d) or crossline (%d) field/byte offset. "\
                             "Too large or between valid byte offsets" % (self._il, self._xl))

        if err == _error.INVALID_OFFSETS:
            raise RuntimeError("Found more offsets than traces. File probably corrupt.")

    def _init_line_count(self):
        ilines_sz, xlines_sz = ct.c_uint(), ct.c_uint()

        if self.sorting == 1: #crossline sorted
            fi = self._il
            l1out = ct.byref(xlines_sz)
            l2out = ct.byref(ilines_sz)
        elif self.sorting == 2: #inline sorted
            fi = self._xl
            l1out = ct.byref(ilines_sz)
            l2out = ct.byref(xlines_sz)
        else:
            raise RuntimeError("Unable to determine sorting. File probably corrupt.")

        errc = self._count_lines(fi, self.offsets, l1out, l2out, self._tr0, self._bsz)
        err = _error(errc)

        if err == _error.OK:
            return int(ilines_sz.value), int(xlines_sz.value)

        errno = ct.get_errno()
        raise OSError(errno, "Error while counting lines: %s", os.strerror(errno))


    def _init_ilines(self, iline_count, xline_count):
        ilines = (ct.c_uint * iline_count)()
        errc = self._inline_indices(self._il, self.sorting, iline_count, xline_count, self.offsets, ilines, self._tr0, self._bsz)
        err = _error(errc)

        if err == _error.OK:
            return map(int, ilines), ilines

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

        errno = ct.get_errno()
        raise OSError(errno, "Error while reading inline indices: %s", os.strerror(errno))

    def _init_iline_length(self, xline_count):
        length = ct.c_uint()
        errc = self._inline_length(self.sorting, self.tracecount, xline_count, self.offsets, ct.byref(length))
        err = _error(errc)

        if err == _error.OK:
            return int(length.value)

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

        errno = ct.get_errno()
        raise OSError(errno, "Error while determining inline length: %s", os.strerror(errno))

    def _init_iline_stride(self, iline_count):
        stride = ct.c_uint()
        errc = self._inline_stride(self.sorting, iline_count, ct.byref(stride))
        err = _error(errc)

        if err == _error.OK:
            return int(stride.value)

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

    def _init_xlines(self, iline_count, xline_count):
        xlines = (ct.c_uint * xline_count)()
        errc = self._crossline_indices(self._xl, self.sorting, iline_count, xline_count, self.offsets, xlines, self._tr0, self._bsz)
        err = _error(errc)

        if err == _error.OK:
            return map(int, xlines), xlines

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

        errno = ct.get_errno()
        raise OSError(errno, "Error while reading crossline indices: %s", os.strerror(errno))

    def _init_xline_length(self, iline_count):
        length = ct.c_uint()
        errc = self._crossline_length(self.sorting, self.tracecount, iline_count, self.offsets, ct.byref(length))
        err = _error(errc)

        if err == _error.OK:
            return int(length.value)

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

        errno = ct.get_errno()
        raise OSError(errno, "Error while determining crossline length: %s", os.strerror(errno))


    def _init_xline_stride(self, xline_count):
        stride = ct.c_uint()
        errc = self._crossline_stride(self.sorting, xline_count, ct.byref(stride))
        err = _error(errc)

        if err == _error.OK:
            return int(stride.value)

        if err == _error.INVALID_SORTING:
            raise RuntimeError("Unknown file sorting.")

    def __enter__(self):
        """Internal."""
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
        self._flush()

    def close(self):
        """Close the file.

        This method is mostly useful for testing.

        It is not necessary to call this method if you're using the `with`
        statement, which will close the file for you.
        """
        self._close()

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
                ...     h = { TraceField.offset = 2 }
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

            Copy headers from file f to file g::
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
        """
        return _header(self)

    @header.setter
    def header(self, val):
        if isinstance(val, _header.proxy) or isinstance(val, dict):
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
        """
        class trace:
            def __getitem__(inner, index, buf = None):
                if isinstance(index, tuple):
                    return inner.__getitem__(index[0], index[1])

                buf = self._trace_buffer(buf)

                if isinstance(index, int):
                    if not 0 <= abs(index) < len(inner):
                        raise IndexError("Trace %d not in range (-%d,%d)", (index, len(inner), len(inner)))

                    return self._readtr(index, buf)

                elif isinstance(index, slice):
                    def gen():
                        for i in xrange(*index.indices(len(inner))):
                            yield self._readtr(i, buf)

                    return gen()

                else:
                    raise TypeError( "Key must be int, slice, (int,np.ndarray) or (slice,np.ndarray)" )

            def __setitem__(inner, index, val):
                if not 0 <= abs(index) < len(inner):
                    raise IndexError("Trace %d not in range (-%d,%d)", (index, len(inner), len(inner)))

                if not isinstance( val, np.ndarray ):
                    raise TypeError( "Value must be numpy.ndarray" )

                if val.dtype != np.float32:
                    raise TypeError( "Numpy array must be float32" )

                shape = (self.samples,)

                if val.shape[0] < shape[0]:
                    raise TypeError( "Array wrong shape. Expected minimum %s, was %s" % (shape, val.shape))

                if isinstance(index, int):
                    self._writetr(index, val)

                elif isinstance(index, slice):
                    for i, buf in xrange(*index.indices(len(inner))), val:
                        self._writetr(i, val)

                else:
                    raise KeyError( "Wrong shape of index" )


            def __len__(inner):
                return self.tracecount

            def __iter__(inner):
                return inner[:]

        return trace()

    @trace.setter
    def trace(self, val):
        tr = self.trace
        for i, v in itertools.izip(xrange(len(tr)), val):
            tr[i] = v

    def _line_buffer(self, length, buf = None):
        shape = (length, self.samples)

        if buf is None:
            return np.empty(shape = shape, dtype = np.float32)

        if not isinstance(buf, np.ndarray):
            return buf

        if buf.dtype != np.float32:
            return np.empty(shape = shape, dtype = np.float32)

        if buf.shape[0] == shape[0]:
            return buf

        if buf.shape != shape and buf.size == np.prod(shape):
            return buf.reshape(shape)

        return buf

    def _fread_trace0(self, lineno, length, stride, linenos, line_type):
        line0 = ct.c_uint()
        errc = self._line_trace0(lineno, length, stride, linenos, len(linenos), ct.byref(line0))
        err = _error(errc)

        if err == _error.OK:
            return int(line0.value)

        if errc == _error.MISSING_LINE_INDEX:
            raise KeyError("%s number %d does not exist." % (line_type, lineno))

        errno = ct.get_errno()
        raise OSError( errno, "Unable to read line %d: %s" % (lineno, os.strerror(errno)))

    def _fread_line(self, trace0, length, stride, buf):
        errc = self._read_line(trace0, length, stride, _floatp(buf), self._tr0, self._bsz)
        err = _error(errc)

        if err != _error.OK:
            errno = ct.get_errno()
            raise OSError(errno, "Unable to read line starting at trace %d: %s" % (trace0, os.strerror(errno)))

        errc = self._to_native(self._fmt, buf.size, _floatp(buf))
        err = _error(errc)

        if err == _error.OK:
            return buf

        raise BufferError("Unable to convert line to native float")


    def _fwrite_line(self, trace0, length, stride, buf):
        errc_conv = self._from_native(self._fmt, buf.size, _floatp(buf))
        err_conv = _error(errc_conv)

        if err_conv != _error.OK:
            raise BufferError("Unable to convert line from native float.")

        errc = self._write_line(trace0, length, stride, _floatp(buf), self._tr0, self._bsz)
        errc_conv = self._to_native(self._fmt, buf.size, _floatp(buf))

        err = _error(errc)
        err_conv = _error(errc_conv)

        if err != _error.OK:
            errno = ct.get_errno()
            raise OSError(errno, "Error writing line starting at trace %d: %s" % (trace0, os.strerror(errno)))

        if err_conv != _error.OK:
            raise BufferError("Unable to convert line from native float.")

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
        lines, raw_lines = self.ilines, self._raw_ilines
        other_lines = self.xlines
        buffn = lambda x = None: self._line_buffer(il_len, x)
        readfn = self._fread_line

        def writefn(t0, length, step, val):
            val = buffn(val)
            for i, v in itertools.izip(xrange(t0, t0 + step*length, step), val):
                self._writetr(i, v)

        return _line(self, il_len, il_stride, lines, raw_lines, other_lines, buffn, readfn, writefn, "Inline")

    @iline.setter
    def iline(self, value):
        self.iline[:] = value

    @property
    def xline(self):
        """ Interact with segy in crossline mode.

        This mode gives access to reading and writing functionality for crosslines.
        The primary data type is the numpy ndarray. crosslines can be accessed
        individually or with python slices, and writing is done via assignment.
        Note that accessing crosslines uses the line numbers, not their position,
        so if a files has crosslines [1400..1450], accessing line [0..100] will be
        an error. Note that each line is returned as a numpy array, meaning
        accessing the intersections of the crossline and crossline is 0-indexed.

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
        lines, raw_lines = self.xlines, self._raw_xlines
        other_lines = self.ilines
        buffn = lambda x = None: self._line_buffer(xl_len, x)
        readfn = self._fread_line

        def writefn(t0, length, step, val):
            val = buffn(val)
            for i, v in itertools.izip(xrange(t0, t0 + step*length, step), val):
                self._writetr(i, v)

        return _line(self, xl_len, xl_stride, lines, raw_lines, other_lines, buffn, readfn, writefn, "Crossline")

    @xline.setter
    def xline(self, value):
        self.xline[:] = value

    def _readh(self, index, buf = None):
        errc = self._read_header(index, buf, self._tr0, self._bsz)
        err = _error(errc)

        if err != _error.OK:
            errno = ct.get_errno()
            raise OSError(errno, os.strerror(errno))

        return buf

    def _trace_buffer(self, buf = None):
        samples = self.samples

        if buf is None:
            buf = np.empty( shape = samples, dtype = np.float32 )
        elif not isinstance( buf, np.ndarray ):
            raise TypeError("Buffer must be None or numpy.ndarray" )
        elif buf.dtype != np.float32:
            buf = np.empty( shape = samples, dtype = np.float32 )
        elif buf.shape != samples:
            buf.reshape( samples )

        return buf

    def _readtr(self, traceno, buf = None):
        if traceno < 0:
            traceno += self.tracecount

        buf = self._trace_buffer(buf)
        bufp = _floatp(buf)

        errc = self._read_trace(traceno, bufp, self._tr0, self._bsz)
        err = _error(errc)

        if err != _error.OK:
            errno = ct.get_errno()
            raise OSError(errno, "Could not read trace %d: %s" % (traceno, os.strerror(errno)))

        errc = self._to_native(self._fmt, self.samples, bufp)
        err = _error(errc)

        if err == _error.OK:
            return buf

        raise BufferError("Error converting to native float.")

    def _writetr(self, traceno, buf):
        bufp = _floatp(buf)
        errc = self._from_native(self._fmt, self.samples, bufp)
        err = _error(errc)

        if err != _error.OK:
            raise BufferError("Error converting from native float.")

        errc = self._write_trace(traceno, bufp, self._tr0, self._bsz)
        errc_conv = self._to_native(self._fmt, self.samples, bufp)

        err, err_conv = _error(errc), _error(errc_conv)

        if err != _error.OK and err_conv != SEGY_OK:
            errno = ct.get_errno()
            raise OSError(errno, "Writing trace failed, and array integrity can not be guaranteed: %s" % os.strerror(errno))

        if err != _error.OK:
            errno = ct.get_errno()
            raise OSError(errno, "Error writing trace %d: %s" % (traceno, os.strerror(errno)))

        if err_conv != _error.OK:
            raise BufferError("Could convert to native float. The array integrety can not be guaranteed.")

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
        class text:
            def __init__(inner):
                inner.size = self._textsize()

            def __getitem__(inner, index):
                if index > self.ext_headers:
                    raise IndexError("Textual header %d not in file" % index)

                buf = ct.create_string_buffer(inner.size)
                err = self._texthdr( buf )

                if err == 0: return buf.value

                errno = ct.get_errno()
                raise OSError(errno, "Could not read text header: %s" % os.strerror(errno))

            def __setitem__(inner, index, val):
                if index > self.ext_headers:
                    raise IndexError("Textual header %d not in file" % index)

                buf = ct.create_string_buffer(inner.size)
                for i, x in enumerate(val[:inner.size]):
                    buf[i] = x

                err = self._write_texthdr(index, buf)

                if err == 0: return

                errno = ct.get_errno()
                raise OSError(errno, "Could not write text header: %s" % os.strerror(errno))

        return text()

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
        def get_bfield(buf, field, val):
            errc = self._get_bfield(buf, int(field), ct.byref(val))
            err = _error(errc)

            if err != _error.OK:
                raise IndexError("Invalid byte offset %d" % field)

            return int(val.value)

        def set_bfield(buf, field, val):
            errc = self._set_bfield(buf, int(field), val)
            err = _error(errc)

            if err != _error.OK:
                raise IndexError("Invalid byte offset %d" % field)

        buf = (ct.c_char * self._binheader_size())()
        gt = get_bfield
        st = set_bfield
        wr = self._write_binheader
        ty = BinField

        err = self._binheader(buf)
        if err == 0:
            return _header.proxy(buf, get_field = gt, set_field = st, write = wr, field_type = ty)

        errno = ct.get_errno()
        raise OSError(errno, "Could not read binary header: %s" % os.strerror(errno))

    @bin.setter
    def bin(self, value):
        try:
            buf = self.bin.buf

        except OSError:
            # the file was probably newly created and the binary header hasn't
            # been written yet.  if this is the case we want to try and write
            # it. if the file was broken, permissions were wrong etc writing
            # will fail too
            buf = (ct.c_char * self._binheader_size())()

        if isinstance(value, dict):
            for k, v in value.items():
                self._set_bfield(buf, int(k), v)
        else:
            buf = value.buf

        err = self._write_binheader(buf)
        if err == 0: return

        errno = ct.get_errno()
        raise OSError(errno, "Could not write text header: %s" % os.strerror(errno))

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

                return d[ self._fmt ]

        return fmt()

    def free(self):
        """Internal."""
        pass

class spec:
    def __init__(self):
        self.iline          = 189
        self.ilines         = None
        self.xline          = 193
        self.xlines         = None
        self.offsets        = 1
        self.samples        = None
        self.tracecount     = None
        self.ext_headers    = 0
        self.format         = None
        self.sorting        = None
        self.t0             = 1111.0
