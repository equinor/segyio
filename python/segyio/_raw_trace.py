import numpy as np

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
            start, stop, step = index.indices(len(self.trace))
            mstart, mstop = min(start, stop), max(start, stop)
            length = max(0, (mstop - mstart + (step - (1 if step > 0 else -1))))
            buf = np.zeros(shape = (length, self.trace.shape), dtype = self.trace.dtype)
            l = len(range(start, stop, step))
            return self.trace.filehandle.gettr(buf, start, step, l)

        return self.trace[index]

    def __repr__(self):
        return self.trace.__repr__() + ".raw"
