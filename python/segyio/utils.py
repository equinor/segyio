import warnings
import numpy as np
import xml.etree.ElementTree as ET

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

def c_encoding(encoding):
    encodings = {
        None: -1,
        'ebcdic': 0,
        'ascii': 1,
    }

    if encoding not in encodings:
        problem = 'unknown encoding {}, expected one of: '
        opts = ' '.join(encodings.keys())
        raise ValueError(problem.format(encoding) + opts)

    return encodings[encoding]

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

    def make_segyfile_descriptor(self, endian, encoding=None):
        from . import _segyio
        fd = _segyio.segyfd(
            filename=str(self.filename),
            mode=self.mode,
            endianness=c_endianness(endian),
            encoding=c_encoding(encoding),
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

    def make_segyfile_descriptor(self, endian, encoding):
        from . import _segyio
        fd = _segyio.segyfd(
            stream=self.stream,
            endianness=c_endianness(endian),
            encoding=c_encoding(encoding),
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

    def make_segyfile_descriptor(self, endian, encoding):
        from . import _segyio
        fd = _segyio.segyfd(
            memory_buffer=self.buffer,
            endianness=c_endianness(endian),
            encoding=c_encoding(encoding),
        )
        return fd


class TraceHeaderLayoutEntry:
    def __init__(self, name, byte, type, requires_nonzero_value):
        self.name = name
        self.byte = int(byte)
        self.type = type
        self.requires_nonzero_value = bool(int(requires_nonzero_value))

    def __repr__(self):
        msg = "TraceHeaderLayoutEntry(name='{}', byte={}, type='{}', requires_nonzero_value={})"
        return msg.format(self.name, self.byte, self.type, self.requires_nonzero_value)


def parse_trace_headers_layout(xml):
    """
    Parse an XML string into a dict of header name: TraceHeaderLayoutEntry.
    """
    try:
        xml = xml.strip()
        root = ET.fromstring(xml)
    except ET.ParseError as e:
        raise ValueError("Invalid XML: {}".format(e))

    if root.tag != 'segy-layout':
        msg = "Root must be 'segy-layout', found '{}'"
        raise ValueError(msg.format(root.tag))

    headers = [root]
    headers.extend([child for child in root if child.tag == 'extension'])

    layout = {}

    for header in headers:
        if header.tag == 'segy-layout':
            name = 'SEG00000'
        else:
            name = header.get('name')
            if not name:
                raise ValueError("Extension header is missing name attribute")
        entries = [
            TraceHeaderLayoutEntry(
                child.get('name'),
                child.get('byte'),
                child.get('type'),
                child.get('if-non-zero', '0')
            ) for child in header if child.tag == 'entry'
        ]
        layout[name] = entries

    return layout
