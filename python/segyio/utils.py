import warnings
import numpy as np

def castarray(x, dtype):
        try:
            x.dtype
        except AttributeError:
            msg = 'Implicit conversion from {} to {} (performance)'
            warnings.warn(msg.format(type(x), np.ndarray), RuntimeWarning)

            try:
                x = np.require(x, dtype = dtype, requirements = 'CAW')
            except TypeError:
                x = np.fromiter(x, dtype = dtype)

        if not x.flags['C_CONTIGUOUS']:
            msg = 'Implicit conversion to contiguous array'
            warnings.warn(msg, RuntimeWarning)

        if x.dtype != dtype:
            # TODO: message depending on narrowing/float-conversion
            msg = 'Implicit conversion from {} to {} (narrowing)'
            warnings.warn(msg.format(x.dtype, dtype), RuntimeWarning)

        # Ensure the data is C-order contiguous, writable, and aligned, with
        # the appropriate dtype. it won't copy unless it has to, so it's
        # reasonably fast.
        return np.require(x, dtype = dtype, requirements = 'CAW')
