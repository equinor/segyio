import numpy as np
import segyio

try: xrange
except NameError: pass
else: range = xrange


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
            l = len(range(start, stop, step))
            buf, _ = self.trace._readtr(start, step, l, buf)
            return buf

        if int(index) != index:
            raise TypeError("Trace index must be integer or slice.")

        buf = self.trace._trace_buffer(None)
        return self.trace._readtr(int(index), 1, 1, buf)[0]

    def __repr__(self):
        return self.trace.__repr__() + ".raw"
