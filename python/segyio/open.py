import segyio

def open(filename, mode = "r", iline = 189, xline = 193):
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
    """
    f = segyio.file(filename, mode, iline, xline)

    try:
        header = f.bin.buf

        f.samples = f._get_samples(header)
        f._tr0 = f._trace0(header)
        f._fmt = f._format(header)
        f._bsz = f._trace_bsize(f.samples)
        f.ext_headers   = (f._tr0 - 3600) / 3200 # should probably be from C

        f.tracecount = f._init_traces()
        f.sorting    = f._init_sorting()
        f.offsets    = f._init_offsets()

        iline_count, xline_count = f._init_line_count()

        f.ilines, f._raw_ilines = f._init_ilines(iline_count, xline_count)
        f._iline_length = f._init_iline_length(xline_count)
        f._iline_stride = f._init_iline_stride(iline_count)

        f.xlines, f._raw_xlines = f._init_xlines(iline_count, xline_count)
        f._xline_length = f._init_xline_length(iline_count)
        f._xline_stride = f._init_xline_stride(xline_count)

    except:
        f.close()
        raise

    return f

