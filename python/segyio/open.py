import numpy

import segyio


def open(filename, mode="r", iline=189, xline=193):
    """Open a segy file.

    Opens a segy file and tries to figure out its sorting, inline numbers,
    crossline numbers, and offsets, and enables reading and writing to this
    file in a simple manner.

    For reading, the access mode "r" is preferred. All write operations will
    raise an exception. For writing, the mode "r+" is preferred (as "rw" would
    truncate the file). The modes used are standard C file modes; please refer
    to that documentation for a complete reference.

    Open should be used together with python's `with` statement. Please refer
    to the examples. When the `with` statement is used the file will
    automatically be closed when the routine completes or an exception is
    raised.

    Args:
        filename (str): Path to file to open.
        mode (str, optional): File access mode, defaults to "r".
        iline (TraceField): Inline number field in the trace headers. Defaults
                            to 189 as per the SEGY specification.
        xline (TraceField): Crossline number field in the trace headers.
                            Defaults to 193 as per the SEGY specification.

    Examples:
        Open a file in read-only mode::
            >>> with segyio.open(path, "r") as f:
            ...     print(f.ilines)
            ...

        Open a file in read-write mode::
            >>> with segyio.open(path, "r+") as f:
            ...     f.trace = np.arange(100)
            ...

        Open two files at once::
            >>> with segyio.open(path) as src, segyio.open(path, "r+") as dst:
            ...     dst.trace = src.trace # copy all traces from src to dst
            ...
    :rtype: segyio.SegyFile
    """
    f = segyio.SegyFile(filename, mode, iline, xline)

    try:
        header = f.bin.buf
        metrics = segyio._segyio.init_metrics(f.xfd, header, iline, xline)

        f._samples = metrics['sample_count']
        f._tr0 = metrics['trace0']
        f._fmt = metrics['format']
        f._bsz = metrics['trace_bsize']
        f._ext_headers = (f._tr0 - 3600) / 3200  # should probably be from C

        f._tracecount = metrics['trace_count']

        f._sorting = metrics['sorting']
        f._offsets = metrics['offset_count']

        iline_count, xline_count = metrics['iline_count'], metrics['xline_count']

        line_metrics = segyio._segyio.init_line_metrics(f.sorting, f.tracecount, iline_count, xline_count, f.offsets)

        f._ilines = numpy.zeros(iline_count, dtype=numpy.uintc)
        f._xlines = numpy.zeros(xline_count, dtype=numpy.uintc)
        segyio._segyio.init_line_indices(f.xfd, metrics, f.ilines, f.xlines)

        f._iline_length = line_metrics['iline_length']
        f._iline_stride = line_metrics['iline_stride']

        f._xline_length = line_metrics['xline_length']
        f._xline_stride = line_metrics['xline_stride']

    except:
        f.close()
        raise

    return f
