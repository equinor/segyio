import segyio
import numpy as np
import textwrap


def dt(segyfile, fallback_dt=4000.0):
    """Since v1.1
    Find a *dt* value in the SegyFile. If none is found use the provided *fallback_dt* value.

    :type segyfile: segyio.SegyFile
    :type fallback_dt: float
    :rtype: float
    """
    return segyio._segyio.get_dt(segyfile.xfd, fallback_dt)


def sample_indexes(segyfile, t0=0.0, dt_override=None):
    """Since v1.1

    Creates a list of values representing the samples in a trace at depth or time.
    The list starts at *t0* and is incremented with am*dt* for the number of samples.
    If a *dt_override* is not provided it will try to find a *dt* in the file.

    :type segyfile: segyio.SegyFile
    :type t0: float
    :type dt_override: float or None
    :rtype: list[float]
    """
    if dt_override is None:
        dt_override = dt(segyfile)

    return [t0 + t * dt_override for t in range(len(segyfile.samples))]


def create_text_header(lines):
    """
    Will create a "correct" SEG-Y textual header.
    Every line will be prefixed with C## and there are 40 lines.
    The input must be a dictionary with the line number[1-40] as a key.
    The value for each key should be up to 76 character long string.

    :type lines: dict[int, str]
    :rtype: str
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
    """Since v1.1
    Formats the text input with newlines given the user specified width for each line

    :type s: str
    :type width: int
    :rtype: str
    """
    return '\n'.join(textwrap.wrap(str(s), width=width))


def native(data,
           format = segyio.SegySampleFormat.IBM_FLOAT_4_BYTE,
           copy = True):
    """ Convert numpy array to native float

    Since v1.1

    :type data: numpy.ndarray
    :type format: int|segyio.SegySampleFormat
    :type copy: bool
    :rtype: numpy.ndarray

    Converts a numpy array from raw segy trace data to native floats. Works for numpy ndarrays.

    Examples:
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
    """ Collect traces or lines into one ndarray

    Since v1.1

    Eagerly copy a series of traces, lines or depths into one numpy ndarray. If
    collecting traces or fast-direction over a post-stacked file, reshaping the
    resulting array is equivalent to calling `tools.cube`.

    Examples:

    collect-cube identity::
        >>> f = segyio.open('post-stack.sgy')
        >>> x = segyio.tools.collect(f.traces[:])
        >>> x = x.reshape((len(f.ilines), len(f.xlines), f.samples))
        >>> numpy.all(x == segyio.tools.cube(f))

    :type itr: iterable[numpy.ndarray]
    :rtype: numpy.ndarray
    """
    return np.stack([np.copy(x) for x in itr])

def cube(f):
    """ Read a full cube from a file

    Since v1.1

    Takes an open segy file (created with segyio.open) or a file name.

    If the file is a prestack file, the cube returned has the dimensions
    (fast,slow,offset,sample). If it is post-stack (i.e. only the one offset),
    the dimensions are normalised to (fast,slow,sample)

    :type f: SegyFile|str
    :rtype numpy.ndarray
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
