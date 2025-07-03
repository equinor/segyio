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

def c_endianness(endian):
    endians = {
        'little': 1,
        'lsb': 1,
        'big': 0,
        'msb': 0,
    }

    if endian not in endians:
        problem = 'unknown endianness {}, expected one of: '
        opts = ' '.join(endians.keys())
        raise ValueError(problem.format(endian) + opts)

    return endians[endian]


class FileDatasourceDescriptor():
    def __init__(self, filename, mode):
        self.filename = filename
        self.mode = mode

    def __repr__(self):
        return "'{}', '{}'".format(self.filename, self.mode)

    def __str__(self):
        return "{}".format(self.filename)

    def readonly(self):
        return self.mode == 'rb' or self.mode == 'r'

    def make_segyfile_descriptor(self, endian):
        from . import _segyio
        fd = _segyio.segyfd(
            filename=str(self.filename),
            mode=self.mode,
            endianness=c_endianness(endian)
        )
        return fd


class StreamDatasourceDescriptor():
    def __init__(self, stream, minimize_requests_number):
        if stream is None:
            raise ValueError("stream object is required")
        self.stream = stream
        self.minimize_requests_number = minimize_requests_number

    def __repr__(self):
        return "'{}'".format(self.stream)

    def __str__(self):
       return str(self.stream)

    def readonly(self):
        return not self.stream.writable()

    def make_segyfile_descriptor(self, endian):
        from . import _segyio
        fd = _segyio.segyfd(
            stream=self.stream,
            endianness=c_endianness(endian),
            minimize_requests_number=self.minimize_requests_number,
        )
        return fd


class MemoryBufferDatasourceDescriptor():
    def __init__(self, buffer):
        if buffer is None:
            raise ValueError("buffer object is required")
        self.buffer = buffer

    def __repr__(self):
        return "'memory buffer'"

    def __str__(self):
       return "memory buffer"

    def readonly(self):
        return False

    def make_segyfile_descriptor(self, endian):
        from . import _segyio
        fd = _segyio.segyfd(
            memory_buffer=self.buffer,
            endianness=c_endianness(endian))
        return fd
