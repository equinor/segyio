import datetime
import numpy
import segyio

def default_text_header(iline, xline, offset):
    return ''.join([
    "C 1 DATE: %s                                                            ",
    "C 2 AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE           ",
    "C 3 Written by libsegyio (python)                                               ",
    "C 4                                                                             ",
    "C 5                                                                             ",
    "C 6                                                                             ",
    "C 7                                                                             ",
    "C 8                                                                             ",
    "C 9                                                                             ",
    "C10                                                                             ",
    "C11 TRACE HEADER POSITION:                                                      ",
    "C12   INLINE BYTES %03d-%03d    | OFFSET BYTES %03d-%03d                            ",
    "C13   CROSSLINE BYTES %03d-%03d |                                                 ",
    "C14                                                                             ",
    "C15 END EBCDIC HEADER                                                           ",
    "C16                                                                             ",
    "C17                                                                             ",
    "C18                                                                             ",
    "C19                                                                             ",
    "C20                                                                             ",
    "C21                                                                             ",
    "C22                                                                             ",
    "C23                                                                             ",
    "C24                                                                             ",
    "C25                                                                             ",
    "C26                                                                             ",
    "C27                                                                             ",
    "C28                                                                             ",
    "C29                                                                             ",
    "C30                                                                             ",
    "C31                                                                             ",
    "C32                                                                             ",
    "C33                                                                             ",
    "C34                                                                             ",
    "C35                                                                             ",
    "C36                                                                             ",
    "C37                                                                             ",
    "C38                                                                             ",
    "C39                                                                             ",
    "C40                                                                            \x80"]) \
    % (datetime.date.today(), iline, iline + 4, int(offset), int(offset) + 4, xline, xline + 4)

def create(filename, spec):
    """Create a new segy file.

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
            >>> spec.samples = 50
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
            ...     spec.samples = src.samples - 50
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

    f._samples       = spec.samples
    f._ext_headers   = spec.ext_headers
    f._bsz          = segyio._segyio.trace_bsize(f.samples)

    txt_hdr_sz = segyio._segyio.textheader_size()
    bin_hdr_sz = segyio._segyio.binheader_size()
    f._tr0          = -1 + txt_hdr_sz + bin_hdr_sz + (spec.ext_headers * (txt_hdr_sz - 1))
    f._sorting       = spec.sorting
    f._fmt          = spec.format
    f._offsets       = spec.offsets
    f._tracecount    = len(spec.ilines) * len(spec.xlines) * spec.offsets

    f._il           = int(spec.iline)
    f._ilines        = numpy.copy(numpy.asarray(spec.ilines, dtype=numpy.uintc))

    f._xl           = int(spec.xline)
    f._xlines        = numpy.copy(numpy.asarray(spec.xlines, dtype=numpy.uintc))

    line_metrics = segyio._segyio.init_line_metrics(f.sorting, f.tracecount, len(f.ilines), len(f.xlines), f.offsets)

    f._iline_length = line_metrics['iline_length']
    f._iline_stride = line_metrics['iline_stride']

    f._xline_length = line_metrics['xline_length']
    f._xline_stride = line_metrics['xline_stride']

    f.text[0] = default_text_header(f._il, f._xl, segyio.TraceField.offset)
    f.bin     = {   3213: f.tracecount,
                    3217: 4000,
                    3221: f.samples,
                    3225: f.format,
                    3505: f.ext_headers,
                }

    return f
