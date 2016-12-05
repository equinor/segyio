import segyio
import numpy as np


def dt(segyfile, fallback_dt=4):
    """
    Find a *dt* value in the SegyFile. If none is found use the provided *fallback_dt* value.

    :type segyfile: segyio.SegyFile
    :type fallback_dt: float
    :rtype: float
    """
    return segyio._segyio.get_dt(segyfile.xfd, fallback_dt)


def sample_indexes(segyfile, t0=0.0, dt_override=None):
    """
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

    return [t0 + t * dt_override for t in range(segyfile.samples)]
