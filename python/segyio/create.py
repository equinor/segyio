import datetime
import numpy
import segyio
import segyio._segyio as _segyio


def default_text_header(iline, xline, offset):
    lines = {
        1: "DATE %s" % datetime.date.today().isoformat(),
        2: "AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE",
        3: "Written by libsegyio (python)",
        11: "TRACE HEADER POSITION:",
        12: "  INLINE BYTES %03d-%03d    | OFFSET BYTES %03d-%03d" % (iline, iline + 4, int(offset), int(offset) + 4),
        13: "  CROSSLINE BYTES %03d-%03d |" % (xline, xline + 4),
        15: "END EBCDIC HEADER",
    }
    rows = segyio.create_text_header(lines)
    rows = bytearray(rows, 'ascii')  # mutable array of bytes
    rows[-1] = 128  # \x80 -- Unsure if this is really required...
    return bytes(rows)  # immutable array of bytes that is compatible with strings


def create(filename, spec):
    """Create a new segy file.

    Since v1.1

    Create a new segy file with the geometry and properties given by `spec`.
    This enables creating SEGY files from your data. The created file supports
    all segyio modes, but has an emphasis on writing. The spec must be
    complete, otherwise an exception will be raised. A default, empty spec can
    be created with `segyio.spec()`.

    Very little data is written to the file, so just calling create is not
    sufficient to re-read the file with segyio. Rather, every trace header and
    trace must be written to for the file to be considered complete.

    Create should be used together with python's `with` statement. This ensure
    the data is written. Please refer to the examples.

    Args:
        filename (str): Path to file to open.
        spec (:obj: `spec`): Structure of the segy file.

    Examples:
        Create a file::
            >>> spec = segyio.spec()
            >>> spec.ilines  = [1, 2, 3, 4]
            >>> spec.xlines  = [11, 12, 13]
            >>> spec.samples = list(range(50))
            >>> spec.sorting = 2
            >>> spec.format  = 1
            >>> with segyio.create(path, spec) as f:
            ...     ## fill the file with data
            ...

        Copy a file, but shorten all traces by 50 samples::
            >>> with segyio.open(srcpath) as src:
            ...     spec = segyio.spec()
            ...     spec.sorting = src.sorting
            ...     spec.format = src.format
            ...     spec.samples = src.samples[:len(src.samples) - 50]
            ...     spec.ilines = src.ilines
            ...     spec.xline = src.xlines
            ...     with segyio.create(dstpath, spec) as dst:
            ...         dst.text[0] = src.text[0]
            ...         dst.bin = src.bin
            ...         dst.header = src.header
            ...         dst.trace = src.trace
    :rtype: segyio.SegyFile
    """
    f = segyio.SegyFile(filename, "w+")

    f._samples       = numpy.asarray(spec.samples, dtype = numpy.single)
    f._ext_headers   = spec.ext_headers
    f._bsz           = _segyio.trace_bsize(len(f.samples))

    txt_hdr_sz       = _segyio.textheader_size()
    bin_hdr_sz       = _segyio.binheader_size()
    f._tr0           = txt_hdr_sz + bin_hdr_sz + (spec.ext_headers * txt_hdr_sz)
    f._sorting       = spec.sorting
    f._fmt           = int(spec.format)
    f._offsets       = numpy.copy(numpy.asarray(spec.offsets, dtype = numpy.intc))
    f._tracecount    = len(spec.ilines) * len(spec.xlines) * len(spec.offsets)

    f._il            = int(spec.iline)
    f._ilines        = numpy.copy(numpy.asarray(spec.ilines, dtype=numpy.intc))

    f._xl            = int(spec.xline)
    f._xlines        = numpy.copy(numpy.asarray(spec.xlines, dtype=numpy.intc))

    line_metrics = _segyio.init_line_metrics(f.sorting, f.tracecount, len(f.ilines), len(f.xlines), len(f.offsets))

    f._iline_length = line_metrics['iline_length']
    f._iline_stride = line_metrics['iline_stride']

    f._xline_length = line_metrics['xline_length']
    f._xline_stride = line_metrics['xline_stride']

    f.text[0] = default_text_header(f._il, f._xl, segyio.TraceField.offset)
    f.bin     = {
        3213: f.tracecount,
        3217: 4000,
        3221: len(f.samples),
        3225: f.format,
        3505: f.ext_headers,
    }

    return f
