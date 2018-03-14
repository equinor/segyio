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


def structured(spec):
    if not hasattr(spec, 'ilines' ): return False
    if not hasattr(spec, 'xlines' ): return False
    if not hasattr(spec, 'offsets'): return False
    if not hasattr(spec, 'sorting'): return False

    if spec.ilines  is None: return False
    if spec.xlines  is None: return False
    if spec.offsets is None: return False
    if spec.sorting is None: return False

    if not list(spec.ilines):  return False
    if not list(spec.xlines):  return False
    if not list(spec.offsets): return False
    if not int(spec.sorting):  return False

    return True

def create(filename, spec):
    """Create a new segy file.

    Since v1.1

    Unstructured file creation since v1.4

    Create a new segy file with the geometry and properties given by `spec`.
    This enables creating SEGY files from your data. The created file supports
    all segyio modes, but has an emphasis on writing. The spec must be
    complete, otherwise an exception will be raised. A default, empty spec can
    be created with `segyio.spec()`.

    Very little data is written to the file, so just calling create is not
    sufficient to re-read the file with segyio. Rather, every trace header and
    trace must be written to the file to be considered complete.

    Create should be used together with python's `with` statement. This ensure
    the data is written. Please refer to the examples.

    The `spec` is any object that has the following attributes:
        Mandatory:
            iline   (int/segyio.BinField)
            xline   (int/segyio.BinField)
            samples (array-like of int)
            format  (int), 1 = IBM float, 5 = IEEE float

        Exclusive:
            ilines  (array-like of int)
            xlines  (array-like of int)
            offsets (array-like of int)
            sorting (int/segyio.TraceSortingFormat)

            OR

            tracecount (int)

        Optional:
            ext_headers (int)

    The `segyio.spec()` function will default offsets and everything in the
    mandatory group, except format and samples, and requires the caller to fill
    in *all* the fields in either of the exclusive groups.

    If any field is missing from the first exclusive group, and the tracecount
    is set, the resulting file will be considered unstructured. If the
    tracecount is set, and all fields of the first exclusive group are
    specified, the file is considered structured and the tracecount is inferred
    from the xlines/ilines/offsets. The offsets are defaulted to [1] by
    `segyio.spec()`.

    Args:
        filename (str-like): Path to file to open.
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

        Copy a file, but shorten all traces by 50 samples (since v1.4)::
            >>> with segyio.open(srcpath) as src:
            ...     spec = segyio.tools.metadata(src)
            ...     spec.samples = spec.samples[:len(spec.samples) - 50]
            ...     with segyio.create(dstpath, spec) as dst:
            ...         dst.text[0] = src.text[0]
            ...         dst.bin = src.bin
            ...         dst.header = src.header
            ...         dst.trace = src.trace
    :rtype: segyio.SegyFile
    """


    if not structured(spec):
        tracecount = spec.tracecount
    else:
        tracecount = len(spec.ilines) * len(spec.xlines) * len(spec.offsets)

    ext_headers = spec.ext_headers if hasattr(spec, 'ext_headers') else 0
    samples = numpy.asarray(spec.samples, dtype = numpy.single)

    binary = bytearray(_segyio.binsize())
    _segyio.putfield(binary, 3217, 4000)
    _segyio.putfield(binary, 3221, len(samples))
    _segyio.putfield(binary, 3225, int(spec.format))
    _segyio.putfield(binary, 3505, int(ext_headers))

    f = segyio.SegyFile(str(filename), "w+", tracecount = tracecount,
                                             binary = binary)

    f._il            = int(spec.iline)
    f._xl            = int(spec.xline)
    f._samples       = samples

    if structured(spec):
        f._sorting       = spec.sorting
        f._offsets       = numpy.copy(numpy.asarray(spec.offsets, dtype = numpy.intc))

        f._ilines        = numpy.copy(numpy.asarray(spec.ilines, dtype=numpy.intc))
        f._xlines        = numpy.copy(numpy.asarray(spec.xlines, dtype=numpy.intc))

        line_metrics = _segyio.line_metrics(f.sorting,
                                            tracecount,
                                            len(f.ilines),
                                            len(f.xlines),
                                            len(f.offsets))

        f._iline_length = line_metrics['iline_length']
        f._iline_stride = line_metrics['iline_stride']

        f._xline_length = line_metrics['xline_length']
        f._xline_stride = line_metrics['xline_stride']

    f.text[0] = default_text_header(f._il, f._xl, segyio.TraceField.offset)
    f.xfd.putbin(binary)

    return f
