import numpy as np
import segyio

class RawTrace(object):
    def __init__(self, trace):
        self.trace = trace

    def __getitem__(self, index):
        """ :rtype: numpy.ndarray """
        buf = None
        if isinstance(index, slice):
            f = self.trace._file
            start, stop, step = index.indices(f.tracecount)
            mstart, mstop = min(start, stop), max(start, stop)
            length = max(0, (mstop - mstart + (step - (1 if step > 0 else -1))))
            buf = np.zeros(shape = (length, len(f.samples)), dtype = np.single)

        return self.trace._readtr(index, buf)

    def __repr__(self):
        return self.trace.__repr__() + ".raw"
