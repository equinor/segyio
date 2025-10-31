import datetime
import numpy
import segyio

from . import TraceSortingFormat
from .utils import (
    FileDatasourceDescriptor,
    StreamDatasourceDescriptor,
    to_c_endianness,
    to_c_encoding
)

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

    if spec.ilines  is None: return False
    if spec.xlines  is None: return False
    if spec.offsets is None: return False

    if not list(spec.ilines):  return False
    if not list(spec.xlines):  return False
    if not list(spec.offsets): return False

    return True

def create(filename, spec):
    """Create a new segy file.

    Create a new segy file with the geometry and properties given by `spec`.
    This enables creating SEGY files from your data. The created file supports
    all segyio modes, but has an emphasis on writing. The spec must be
    complete, otherwise an exception will be raised. A default, empty spec can
    be created with ``segyio.spec()``.

    Very little data is written to the file, so just calling create is not
    sufficient to re-read the file with segyio. Rather, every trace header and
    trace must be written to the file to be considered complete.

    Create should be used together with python's ``with`` statement. This ensure
    the data is written. Please refer to the examples.

    The ``segyio.spec()`` function will default sorting, offsets and everything
    in the mandatory group, except format and samples, and requires the caller
    to fill in *all* the fields in either of the exclusive groups.

    If any field is missing from the first exclusive group, and the tracecount
    is set, the resulting file will be considered unstructured. If the
    tracecount is set, and all fields of the first exclusive group are
    specified, the file is considered structured and the tracecount is inferred
    from the xlines/ilines/offsets. The offsets are defaulted to ``[1]`` by
    ``segyio.spec()``.

    Parameters
    ----------
    filename : str
        Path to file to create
    spec : segyio.spec
        Structure of the segy file

    Returns
    -------
    file : segyio.SegyFile
        An open segyio file handle, similar to that returned by `segyio.open`

    See also
    --------
    segyio.spec : template for the `spec` argument


    Notes
    -----

    .. versionadded:: 1.1

    .. versionchanged:: 1.4
       Support for creating unstructured files

    .. versionchanged:: 1.8
       Support for creating lsb files

    The ``spec`` is any object that has the following attributes

    Mandatory::

        iline   : int or segyio.BinField
        xline   : int or segyio.BinField
        samples : array of int
        format  : { 1, 5 }
            1 = IBM float, 5 = IEEE float

    Exclusive::

        ilines  : array_like of int
        xlines  : array_like of int
        offsets : array_like of int
        sorting : int or segyio.TraceSortingFormat

        OR

        tracecount : int

    Optional::

        ext_headers : int
        endian : str { 'big', 'msb', 'little', 'lsb' }
            defaults to 'big'
        encoding : {'ebcdic', 'ascii'}
            defaults to 'ebcdic'.


    Examples
    --------

    Create a file:

    >>> spec = segyio.spec()
    >>> spec.ilines  = [1, 2, 3, 4]
    >>> spec.xlines  = [11, 12, 13]
    >>> spec.samples = list(range(50))
    >>> spec.sorting = 2
    >>> spec.format  = 1
    >>> with segyio.create(path, spec) as f:
    ...     ## fill the file with data
    ...     pass
    ...

    Copy a file, but shorten all traces by 50 samples:

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
    ...         # this is writing a sparse file, which might be slow on some
    ...         # systems
    ...         dst.header = src.header
    ...         dst.trace = src.trace

     Copy a file, but shift samples time by 50:

    >>> with segyio.open(srcpath) as src:
    ...     delrt = 50
    ...     spec = segyio.spec()
    ...     spec.samples = src.samples + delrt
    ...     spec.ilines = src.ilines
    ...     spec.xline = src.xlines
    ...     with segyio.create(dstpath, spec) as dst:
    ...         dst.text[0] = src.text[0]
    ...         dst.bin = src.bin
    ...         dst.header = src.header
    ...         dst.header = { TraceField.DelayRecordingTime: delrt }
    ...         dst.trace = src.trace

    Copy a file, but shorten all traces by 50 samples (since v1.4):

    >>> with segyio.open(srcpath) as src:
    ...     spec = segyio.tools.metadata(src)
    ...     spec.samples = spec.samples[:len(spec.samples) - 50]
    ...     with segyio.create(dstpath, spec) as dst:
    ...         dst.text[0] = src.text[0]
    ...         dst.bin = src.bin
    ...         dst.header = src.header
    ...         dst.trace = src.trace
    """
    return _create(FileDatasourceDescriptor(filename, "w+"), spec)


def create_with(stream, spec, minimize_requests_number=True):
    """
    Creates a segy file on stream.

    Function behaves the same as `segyio.create`, but outputs data to a
    finite stream instead of a file. Stream's close() will be called when
    SegyFile is closed.

    Note that `segyio.create_with` can be very slow. `segyio.create` is
    generally a preferred option when speed matters.

    Parameters
    ----------

    stream : file-like object
        Data destination. It is up to the user to assure stream is opened in w+b mode.
    spec : segyio.spec
        Structure of the segy file
    minimize_requests_number : bool
        Configuration for some internal algorithms. True to minimize number of
        requests to the stream at the cost of higher memory usage. False to
        minimize memory usage at the cost of more requests to the stream.
    """
    return _create(
        StreamDatasourceDescriptor(
            stream,
            minimize_requests_number
        ),
        spec
    )


def _create(datasource_descriptor, spec):
    if not structured(spec):
        tracecount = spec.tracecount
    else:
        tracecount = len(spec.ilines) * len(spec.xlines) * len(spec.offsets)

    ext_headers = spec.ext_headers if hasattr(spec, 'ext_headers') else 0
    samples = numpy.asarray(spec.samples)

    endian = spec.endian if hasattr(spec, 'endian') else 'big'
    if endian is None:
        endian = 'big'

    encoding = spec.encoding if hasattr(spec, 'encoding') else 'ebcdic'
    if encoding is None:
        encoding = 'ebcdic'

    fd = datasource_descriptor.make_segyfile_descriptor()
    fd.segymake(
        samples = len(samples),
        tracecount = tracecount,
        endianness=to_c_endianness(endian),
        encoding=to_c_encoding(encoding),
        format = int(spec.format),
        ext_headers = int(ext_headers),
    )

    # note: even if iline/xline are overridden, file is already created with
    # standard iline/xline position, so those can mismatch

    f = segyio.SegyFile(fd,
            datasource_descriptor,
            endian = endian,
    )

    f._samples       = samples

    if structured(spec):
        sorting = spec.sorting if hasattr(spec, 'sorting') else None
        if sorting is None:
            sorting = TraceSortingFormat.INLINE_SORTING
        f.interpret(spec.ilines, spec.xlines, spec.offsets, sorting)

    f.text[0] = default_text_header(f._il, f._xl, segyio.TraceField.offset)

    if len(samples) == 1:
        interval = int(samples[0] * 1000)
    else:
        interval = int((samples[1] - samples[0]) * 1000)

    binary_header = {}
    binary_header.update(
        ntrpr    = 1,
        hdt      = interval,
        dto      = interval,
        format   = int(spec.format),
        fold     = 1,
        tsort    = 4,
        exth     = ext_headers,
    )

    if len(samples) > 2**16 - 1:
        # when using the ext-samples field, also set rev2, even though it's a
        # soft lie and files aren't really compliant
        binary_header.update(
            exthns = len(samples),
            extnso = len(samples),
            rev    = 2
        )
    else:
        binary_header.update(
            hns = len(samples),
            nso = len(samples),
        )

    f.bin.update(**binary_header)

    return f
