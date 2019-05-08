import numpy

import segyio

def infer_geometry(f, metrics, iline, xline, strict):
    try:
        cube_metrics = f.xfd.cube_metrics(iline, xline)
        f._sorting   = cube_metrics['sorting']
        iline_count  = cube_metrics['iline_count']
        xline_count  = cube_metrics['xline_count']
        offset_count = cube_metrics['offset_count']
        metrics.update(cube_metrics)

        ilines  = numpy.zeros(iline_count,  dtype=numpy.intc)
        xlines  = numpy.zeros(xline_count,  dtype=numpy.intc)
        offsets = numpy.zeros(offset_count, dtype=numpy.intc)

        f.xfd.indices(metrics, ilines, xlines, offsets)
        f.interpret(ilines, xlines, offsets, f._sorting)

    except:
        if not strict:
            f._ilines  = None
            f._xlines  = None
            f._offsets = None
        else:
            f.close()
            raise

    return f


def open(filename, mode="r", iline = 189,
                             xline = 193,
                             strict = True,
                             ignore_geometry = False,
                             endian = 'big'):
    """Open a segy file.

    Opens a segy file and tries to figure out its sorting, inline numbers,
    crossline numbers, and offsets, and enables reading and writing to this
    file in a simple manner.

    For reading, the access mode `r` is preferred. All write operations will
    raise an exception. For writing, the mode `r+` is preferred (as `rw` would
    truncate the file). Any mode with `w` will raise an error. The modes used
    are standard C file modes; please refer to that documentation for a
    complete reference.

    Open should be used together with python's ``with`` statement. Please refer
    to the examples. When the ``with`` statement is used the file will
    automatically be closed when the routine completes or an exception is
    raised.

    By default, segyio tries to open in ``strict`` mode. This means the file will
    be assumed to represent a geometry with consistent inline, crosslines and
    offsets. If strict is False, segyio will still try to establish a geometry,
    but it won't abort if it fails. When in non-strict mode is opened,
    geometry-dependent modes such as iline will raise an error.

    If ``ignore_geometry=True``, segyio will *not* try to build iline/xline or
    other geometry related structures, which leads to faster opens. This is
    essentially the same as using ``strict=False`` on a file that has no
    geometry.

    Parameters
    ----------

    filename : str
        Path to file to open

    mode : {'r', 'r+'}
        File access mode, read-only ('r', default) or read-write ('r+')

    iline : int or segyio.TraceField
        Inline number field in the trace headers. Defaults to 189 as per the
        SEG-Y rev1 specification

    xline : int or segyio.TraceField
        Crossline number field in the trace headers. Defaults to 193 as per the
        SEG-Y rev1 specification

    strict : bool, optional
        Abort if a geometry cannot be inferred. Defaults to True.

    ignore_geometry : bool, optional
        Opt out on building geometry information, useful for e.g. shot
        organised files. Defaults to False.

    endian : {'big', 'msb', 'little', 'lsb'}
        File endianness, big/msb (default) or little/lsb

    Returns
    -------

    file : segyio.SegyFile
        An open segyio file handle

    Raises
    ------

    ValueError
        If the mode string contains 'w', as it would truncate the file

    Notes
    -----

    .. versionadded:: 1.1

    .. versionchanged:: 1.8
        endian argument

    When a file is opened non-strict, only raw traces access is allowed, and
    using modes such as ``iline`` raise an error.


    Examples
    --------

    Open a file in read-only mode:

    >>> with segyio.open(path, "r") as f:
    ...     print(f.ilines)
    ...
    [1, 2, 3, 4, 5]

    Open a file in read-write mode:

    >>> with segyio.open(path, "r+") as f:
    ...     f.trace = np.arange(100)

    Open two files at once:

    >>> with segyio.open(src_path) as src, segyio.open(dst_path, "r+") as dst:
    ...     dst.trace = src.trace # copy all traces from src to dst

    Open a file little-endian file:

    >>> with segyio.open(path, endian = 'little') as f:
    ...     f.trace[0]

    """

    if 'w' in mode:
        problem = 'w in mode would truncate the file'
        solution = 'use r+ to open in read-write'
        raise ValueError(', '.join((problem, solution)))

    endians = {
        'little': 256, # (1 << 8)
        'lsb': 256,
        'big': 0,
        'msb': 0,
    }

    if endian not in endians:
        problem = 'unknown endianness {}, expected one of: '
        opts = ' '.join(endians.keys())
        raise ValueError(problem.format(endian) + opts)

    from . import _segyio
    fd = _segyio.segyiofd(str(filename), mode, endians[endian])
    fd.segyopen()
    metrics = fd.metrics()

    f = segyio.SegyFile(fd,
            filename = str(filename),
            mode = mode,
            iline = iline,
            xline = xline,
            endian = endian,
    )

    try:
        dt = segyio.tools.dt(f, fallback_dt = 4000.0) / 1000.0
        t0 = f.header[0][segyio.TraceField.DelayRecordingTime]
        samples = metrics['samplecount']
        f._samples = (numpy.arange(samples) * dt) + t0

    except:
        f.close()
        raise

    if ignore_geometry:
        return f

    return infer_geometry(f, metrics, iline, xline, strict)
