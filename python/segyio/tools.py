import segyio
from . import TraceSortingFormat
from . import SegySampleFormat

import numpy as np
import textwrap


def dt(f, fallback_dt=4000.0):
    """Delta-time

    Infer a ``dt``, the sample rate, from the file. If none is found, use the
    fallback.

    Parameters
    ----------

    f : segyio.SegyFile
    fallback_dt : float
        delta-time to fall back to, in microseconds

    Returns
    -------
    dt : float

    Notes
    -----

    .. versionadded:: 1.1

    """
    return f.xfd.getdt(fallback_dt)


def sample_indexes(segyfile, t0=0.0, dt_override=None):
    """
    Creates a list of values representing the samples in a trace at depth or time.
    The list starts at *t0* and is incremented with am*dt* for the number of samples.
    If a *dt_override* is not provided it will try to find a *dt* in the file.


    Parameters
    ----------
    segyfile :  segyio.SegyFile
    t0 : float
        initial sample, or delay-recording-time
    dt_override : float or None

    Returns
    -------
    samples : array_like of float

    Notes
    -----

    .. versionadded:: 1.1

    """
    if dt_override is None:
        dt_override = dt(segyfile)

    return [t0 + t * dt_override for t in range(len(segyfile.samples))]


def create_text_header(lines):
    """Format textual header

    Create a "correct" SEG-Y textual header.  Every line will be prefixed with
    C## and there are 40 lines. The input must be a dictionary with the line
    number[1-40] as a key. The value for each key should be up to 76 character
    long string.

    Parameters
    ----------

    lines : dict
        `lines` dictionary with fields:

        - ``no`` : line number (`int`)
        - ``line`` : line (`str`)

    Returns
    -------

    text : str

    """

    rows = []
    for line_no in range(1, 41):
        line = ""
        if line_no in lines:
            line = lines[line_no]
        row = "C{0:>2} {1:76}".format(line_no, line)
        rows.append(row)

    rows = ''.join(rows)
    return rows

def wrap(s, width=80):
    """
    Formats the text input with newlines given the user specified width for
    each line. `wrap` will attempt to decode the input as ascii, ignoring any
    errors, which occurs with many headers in ebcdic. To consider encoding
    errors, decode the textual header before passing it to wrap.

    Parameters
    ----------
    s : text or bytearray or str
    width : int

    Returns
    -------
    text : str

    Notes
    -----
    .. versionadded:: 1.1

    """
    try:
        s = s.decode(errors = 'ignore')
    except AttributeError:
        # Already str-like enough, e.g. wrap(f.text[0].decode()), so just try
        # to wrap-join
        pass

    return '\n'.join(textwrap.wrap(s, width=width))


def native(data,
           format = segyio.SegySampleFormat.IBM_FLOAT_4_BYTE,
           copy = True):
    """Convert numpy array to native float

    Converts a numpy array from raw segy trace data to native floats. Works for numpy ndarrays.

    Parameters
    ----------

    data : numpy.ndarray
    format : int or segyio.SegySampleFormat
    copy : bool
        If True, convert on a copy, and leave the input array unmodified

    Returns
    -------

    data : numpy.ndarray

    Notes
    -----

    .. versionadded:: 1.1

    Examples
    --------

    Convert mmap'd trace to native float:

    >>> d = np.memmap('file.sgy', offset = 3600, dtype = np.uintc)
    >>> samples = 1500
    >>> trace = segyio.tools.native(d[240:240+samples])

    """

    data = data.view( dtype = np.single )
    if copy:
        data = np.copy( data )

    format = int(segyio.SegySampleFormat(format))
    return segyio._segyio.native(data, format)

def collect(itr):
    """Collect traces or lines into one ndarray

    Eagerly copy a series of traces, lines or depths into one numpy ndarray. If
    collecting traces or fast-direction over a post-stacked file, reshaping the
    resulting array is equivalent to calling ``segyio.tools.cube``.

    Parameters
    ----------

    itr : iterable of numpy.ndarray

    Returns
    -------

    data : numpy.ndarray

    Notes
    -----

    .. versionadded:: 1.1

    Examples
    --------

    collect-cube identity:

    >>> with segyio.open('post-stack.sgy') as f:
    >>>     x = segyio.tools.collect(f.trace[:])
    >>>     x = x.reshape((len(f.ilines), len(f.xlines), f.samples))
    >>>     numpy.all(x == segyio.tools.cube(f))

    """
    return np.stack([np.copy(x) for x in itr])

def cube(f):
    """Read a full cube from a file

    Takes an open segy file (created with segyio.open) or a file name.

    If the file is a prestack file, the cube returned has the dimensions
    ``(fast, slow, offset, sample)``. If it is post-stack (only the one
    offset), the dimensions are normalised to ``(fast, slow, sample)``

    Parameters
    ----------

    f : str or segyio.SegyFile

    Returns
    -------

    cube : numpy.ndarray

    Notes
    -----

    .. versionadded:: 1.1

    """

    if not isinstance(f, segyio.SegyFile):
        with segyio.open(f) as fl:
            return cube(fl)

    ilsort = f.sorting == segyio.TraceSortingFormat.INLINE_SORTING
    fast = f.ilines if ilsort else f.xlines
    slow = f.xlines if ilsort else f.ilines
    fast, slow, offs = len(fast), len(slow), len(f.offsets)
    smps = len(f.samples)
    dims = (fast, slow, smps) if offs == 1 else (fast, slow, offs, smps)
    return f.trace.raw[:].reshape(dims)

def rotation(f, line = 'fast'):
    """ Find rotation of the survey

    Find the clock-wise rotation and origin of `line` as ``(rot, cdpx, cdpy)``

    The clock-wise rotation is defined as the angle in radians between line
    given by the first and last trace of the first line and the axis that gives
    increasing CDP-Y, in the direction that gives increasing CDP-X.

    By default, the first line is the 'fast' direction, which is inlines if the
    file is inline sorted, and crossline if it's crossline sorted.


    Parameters
    ----------

    f : SegyFile
    line : { 'fast', 'slow', 'iline', 'xline' }

    Returns
    -------

    rotation : float
    cdpx : int
    cdpy : int


    Notes
    -----

    .. versionadded:: 1.2

    """

    if f.unstructured:
        raise ValueError("Rotation requires a structured file")

    lines = { 'fast': f.fast,
              'slow': f.slow,
              'iline': f.iline,
              'xline': f.xline,
            }

    if line not in lines:
        error = "Unknown line {}".format(line)
        solution = "Must be any of: {}".format(' '.join(lines.keys()))
        raise ValueError('{} {}'.format(error, solution))

    l = lines[line]
    origin = f.header[0][segyio.su.cdpx, segyio.su.cdpy]
    cdpx, cdpy = origin[segyio.su.cdpx], origin[segyio.su.cdpy]

    rot = f.xfd.rotation( len(l),
                          l.stride,
                          len(f.offsets),
                          np.fromiter(l.keys(), dtype = np.intc) )
    return rot, cdpx, cdpy

def metadata(f):
    """Get survey structural properties and metadata

    Create a description object that, when passed to ``segyio.create()``, would
    create a new file with the same structure, dimensions, and metadata as
    ``f``.

    Takes an open segy file (created with segyio.open) or a file name.

    Parameters
    ----------

    f : str or segyio.SegyFile

    Returns
    -------
    spec : segyio.spec

    Notes
    -----

    .. versionadded:: 1.4

    """

    if not isinstance(f, segyio.SegyFile):
        with segyio.open(f) as fl:
            return metadata(fl)

    spec = segyio.spec()

    spec.iline = f._il
    spec.xline = f._xl
    spec.samples = f.samples
    spec.format = f.format

    spec.ilines = f.ilines
    spec.xlines = f.xlines
    spec.offsets = f.offsets
    spec.sorting = f.sorting

    spec.tracecount = f.tracecount

    spec.ext_headers = f.ext_headers
    spec.endian = f.endian

    return spec

def resample(f, rate = None, delay = None, micro = False,
                                           trace = True,
                                           binary = True):
    """Resample a file

    Resample all data traces, and update the file handle to reflect the new
    sample rate. No actual samples (data traces) are modified, only the header
    fields and interpretation.

    By default, the rate and the delay are in millseconds - if you need higher
    resolution, passing micro=True interprets rate as microseconds (as it is
    represented in the file). Delay is always milliseconds.

    By default, both the global binary header and the trace headers are updated
    to reflect this. If preserving either the trace header interval field or
    the binary header interval field is important, pass trace=False and
    binary=False respectively, to not have that field updated. This only apply
    to sample rates - the recording delay is only found in trace headers and
    will be written unconditionally, if delay is not None.

    .. warning::
        This function requires an open file handle and is **DESTRUCTIVE**. It
        will modify the file, and if an exception is raised then partial writes
        might have happened and the file might be corrupted.

    This function assumes all traces have uniform delays and frequencies.

    Parameters
    ----------

    f : SegyFile
    rate : int
    delay : int
    micro : bool
        if True, interpret rate as microseconds
    trace : bool
        Update the trace header if True
    binary : bool
        Update the binary header if True

    Notes
    -----

    .. versionadded:: 1.4

    """

    if rate is not None:
        if not micro: rate *= 1000

        if binary: f.bin[segyio.su.hdt] = rate
        if trace: f.header = { segyio.su.dt: rate}

    if delay is not None:
        f.header = { segyio.su.delrt: delay }

    t0 = delay if delay is not None else f.samples[0]
    rate = rate / 1000 if rate is not None else f.samples[1] - f.samples[0]

    f._samples = (np.arange(len(f.samples)) * rate) + t0

    return f


def from_array(filename, data, iline=189,
                               xline=193,
                               format=SegySampleFormat.IBM_FLOAT_4_BYTE,
                               dt=4000,
                               delrt=0):
    """ Create a new SEGY file from an n-dimentional array. Create a structured
    SEGY file with defaulted headers from a 2-, 3- or 4-dimensional array.
    ilines, xlines, offsets and samples are inferred from the size of the
    array. Please refer to the documentation for functions from_array2D,
    from_array3D and from_array4D to see how the arrays are interpreted.

    Structure-defining fields in the binary header and in the traceheaders are
    set accordingly. Such fields include, but are not limited to iline, xline
    and offset. The file also contains a defaulted textual header.

    Parameters
    ----------
    filename : string-like
        Path to new file
    data : 2-,3- or 4-dimensional array-like
    iline : int or segyio.TraceField
        Inline number field in the trace headers. Defaults to 189 as per the
        SEG-Y rev1 specification
    xline : int or segyio.TraceField
        Crossline number field in the trace headers. Defaults to 193 as per the
        SEG-Y rev1 specification
    format : int or segyio.SegySampleFormat
        Sample format field in the trace header. Defaults to IBM float 4 byte
    dt : int-like
        sample interval
    delrt : int-like

    Notes
    -----
    .. versionadded:: 1.8

    Examples
    --------
    Create a file from a 3D array, open it and read an iline:

    >>> segyio.tools.from_array(path, array3d)
    >>> segyio.open(path, mode) as f:
    ...     iline = f.iline[0]
    ...
    """

    dt = int(dt)
    delrt = int(delrt)

    data = np.asarray(data)
    dimensions = len(data.shape)

    if dimensions not in range(2, 5):
        problem = "Expected 2, 3, or 4 dimensions, {} was given".format(dimensions)
        raise ValueError(problem)

    spec = segyio.spec()
    spec.iline   = iline
    spec.xline   = xline
    spec.format  = format
    spec.sorting = TraceSortingFormat.INLINE_SORTING

    if dimensions == 2:
        spec.ilines  = [1]
        spec.xlines  = list(range(1, np.size(data,0) + 1))
        spec.samples = list(range(np.size(data,1)))
        spec.tracecount = np.size(data, 1)

    if dimensions == 3:
        spec.ilines  = list(range(1, np.size(data, 0) + 1))
        spec.xlines  = list(range(1, np.size(data, 1) + 1))
        spec.samples = list(range(np.size(data, 2)))

    if dimensions == 4:
        spec.ilines  = list(range(1, np.size(data, 0) + 1))
        spec.xlines  = list(range(1, np.size(data, 1) + 1))
        spec.offsets = list(range(1, np.size(data, 2)+ 1))
        spec.samples = list(range(np.size(data,3)))

    samplecount = len(spec.samples)

    with segyio.create(filename, spec) as f:
        tr = 0
        for ilno, il in enumerate(spec.ilines):
            for xlno, xl in enumerate(spec.xlines):
                for offno, off in enumerate(spec.offsets):
                    f.header[tr] = {
                        segyio.su.tracf  : tr,
                        segyio.su.cdpt   : tr,
                        segyio.su.offset : off,
                        segyio.su.ns     : samplecount,
                        segyio.su.dt     : dt,
                        segyio.su.delrt  : delrt,
                        segyio.su.iline  : il,
                        segyio.su.xline  : xl
                    }
                    if dimensions == 2: f.trace[tr] = data[tr, :]
                    if dimensions == 3: f.trace[tr] = data[ilno, xlno, :]
                    if dimensions == 4: f.trace[tr] = data[ilno, xlno, offno, :]
                    tr += 1

        f.bin.update(
            tsort=TraceSortingFormat.INLINE_SORTING,
            hdt=dt,
            dto=dt
        )


def from_array2D(filename, data, iline=189,
                                 xline=193,
                                 format=SegySampleFormat.IBM_FLOAT_4_BYTE,
                                 dt=4000,
                                 delrt=0):
    """ Create a new SEGY file from a 2D array
    Create an structured SEGY file with defaulted headers from a 2-dimensional
    array. The file is inline-sorted and structured as a slice, i.e. it has one
    iline and the xlinecount equals the tracecount. The tracecount and
    samplecount are inferred from the size of the array. Structure-defining
    fields in the binary header and in the traceheaders are set accordingly.
    Such fields include, but are not limited to iline, xline and offset. The
    file also contains a defaulted textual header.

    The 2 dimensional array is interpreted as::

                       samples
                 --------------------
        trace 0 | s0 | s1 | ... | sn |
                 --------------------
        trace 1 | s0 | s1 | ... | sn |
                 --------------------
                          .
                          .
                 --------------------
        trace n | s0 | s1 | ... | sn |
                 --------------------

    traces =  [0, len(axis(0)]
    samples = [0, len(axis(1)]

    Parameters
    ----------
    filename : string-like
        Path to new file
    data : 2-dimensional array-like
    iline : int or segyio.TraceField
        Inline number field in the trace headers. Defaults to 189 as per the
        SEG-Y rev1 specification
    xline : int or segyio.TraceField
        Crossline number field in the trace headers. Defaults to 193 as per the
        SEG-Y rev1 specification
    format : int or segyio.SegySampleFormat
        Sample format field in the trace header. Defaults to IBM float 4 byte
    dt : int-like
        sample interval
    delrt : int-like

    Notes
    -----
    .. versionadded:: 1.8

    Examples
    --------
    Create a file from a 2D array, open it and read a trace:

    >>> segyio.tools.from_array2D(path, array2d)
    >>> segyio.open(path, mode, strict=False) as f:
    ...     tr = f.trace[0]
    """

    data = np.asarray(data)
    dimensions = len(data.shape)

    if dimensions != 2:
        problem = "Expected 2 dimensions, {} was given".format(dimensions)
        raise ValueError(problem)

    from_array(filename, data, iline=iline, xline=xline, format=format,
                                                         dt=dt,
                                                         delrt=delrt)


def from_array3D(filename, data, iline=189,
                                 xline=193,
                                 format=SegySampleFormat.IBM_FLOAT_4_BYTE,
                                 dt=4000,
                                 delrt=0):
    """ Create a new SEGY file from a 3D array
    Create an structured SEGY file with defaulted headers from a 3-dimensional
    array. The file is inline-sorted. ilines, xlines and samples are inferred
    from the array. Structure-defining fields in the binary header and
    in the traceheaders are set accordingly. Such fields include, but are not
    limited to iline, xline and offset. The file also contains a defaulted
    textual header.

    The 3-dimensional array is interpreted as::

             xl0   xl1   xl2
            -----------------
         / | tr0 | tr1 | tr2 | il0
           -----------------
       | / | tr3 | tr4 | tr5 | il1
            -----------------
       | / | tr6 | tr7 | tr8 | il2
            -----------------
       | /      /     /     / n-samples
         ------------------

    ilines =  [1, len(axis(0) + 1]
    xlines =  [1, len(axis(1) + 1]
    samples = [0, len(axis(2)]

    Parameters
    ----------
    filename : string-like
        Path to new file
    data : 3-dimensional array-like
    iline : int or segyio.TraceField
        Inline number field in the trace headers. Defaults to 189 as per the
        SEG-Y rev1 specification
    xline : int or segyio.TraceField
        Crossline number field in the trace headers. Defaults to 193 as per the
        SEG-Y rev1 specification
    format : int or segyio.SegySampleFormat
        Sample format field in the trace header. Defaults to IBM float 4 byte
    dt : int-like
        sample interval
    delrt : int-like

    Notes
    -----
    .. versionadded:: 1.8

    Examples
    --------
    Create a file from a 3D array, open it and read an iline:

    >>> segyio.tools.from_array3D(path, array3d)
    >>> segyio.open(path, mode) as f:
    ...     iline = f.iline[0]
    ...
    """

    data = np.asarray(data)
    dimensions = len(data.shape)

    if dimensions != 3:
        problem = "Expected 3 dimensions, {} was given".format(dimensions)
        raise ValueError(problem)

    from_array(filename, data, iline=iline, xline=xline, format=format,
                                                         dt=dt,
                                                         delrt=delrt)


def from_array4D(filename, data, iline=189,
                                  xline=193,
                                  format=SegySampleFormat.IBM_FLOAT_4_BYTE,
                                  dt=4000,
                                  delrt=0):
    """ Create a new SEGY file from a 4D array
    Create an structured SEGY file with defaulted headers from a 4-dimensional
    array. The file is inline-sorted. ilines, xlines, offsets and samples are
    inferred from the array. Structure-defining fields in the binary header and
    in the traceheaders are set accordingly. Such fields include, but are not
    limited to iline, xline and offset. The file also contains a defaulted
    textual header.

    The 4D array is interpreted:

    ilines =  [1, len(axis(0) + 1]
    xlines =  [1, len(axis(1) + 1]
    offsets = [1, len(axis(2) + 1]
    samples = [0, len(axis(3)]

    Parameters
    ----------
    filename : string-like
        Path to new file
    data : 4-dimensional array-like
    iline : int or segyio.TraceField
        Inline number field in the trace headers. Defaults to 189 as per the
        SEG-Y rev1 specification
    xline : int or segyio.TraceField
        Crossline number field in the trace headers. Defaults to 193 as per the
        SEG-Y rev1 specification
    format : int or segyio.SegySampleFormat
        Sample format field in the trace header. Defaults to IBM float 4 byte
    dt : int-like
        sample interval
    delrt : int-like

    Notes
    -----
    .. versionadded:: 1.8

    Examples
    --------
    Create a file from a 3D array, open it and read an iline:

    >>> segyio.tools.create_from_array4D(path, array4d)
    >>> segyio.open(path, mode) as f:
    ...     iline = f.iline[0]
    ...
    """

    data = np.asarray(data)
    dimensions = len(data.shape)

    if dimensions != 4:
        problem = "Expected 4 dimensions, {} was given".format(dimensions)
        raise ValueError(problem)

    from_array(filename, data, iline=iline, xline=xline, format=format,
                                                         dt=dt,
                                                         delrt=delrt)
