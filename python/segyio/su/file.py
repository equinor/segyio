from ..open import infer_geometry
from ..segy import SegyFile
from . import words

import numpy

class sufile(SegyFile):
    def __init__(self, *args, **kwargs):
        super(sufile, self).__init__(*args, **kwargs)

    @property
    def text(self):
        raise NotImplementedError

    @property
    def bin(self):
        raise NotImplementedError

    @bin.setter
    def bin(self, _):
        raise NotImplementedError

def open(filename, mode = 'r', iline = 189,
                               xline = 193,
                               strict = True,
                               ignore_geometry = False,
                               endian = 'big' ):
    """Open a seismic unix file.

    Behaves identically to open(), except it expects the seismic unix format,
    not SEG-Y.

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
    file : segyio.su.file
        An open seismic unix file handle

    Raises
    ------
    ValueError
        If the mode string contains 'w', as it would truncate the file

    See also
    --------
    segyio.open : SEG-Y open

    Notes
    -----
    .. versionadded:: 1.8
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
        problem = 'unknown endianness, must be one of: '
        candidates = ' '.join(endians.keys())
        raise ValueError(problem + candidates)

    from .. import _segyio
    fd = _segyio.segyiofd(str(filename), mode, endians[endian])
    fd.suopen()
    metrics = fd.metrics()

    f = sufile(
        fd,
        filename = str(filename),
        mode = mode,
        iline = iline,
        xline = xline,
    )

    h0 = f.header[0]

    dt = h0[words.dt] / 1000.0
    t0 = h0[words.delrt]
    samples = metrics['samplecount']
    f._samples = (numpy.arange(samples) * dt) + t0

    if ignore_geometry:
        return f

    return infer_geometry(f, metrics, iline, xline, strict)
