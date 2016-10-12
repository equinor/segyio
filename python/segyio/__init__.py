"""
simple segy input/output

Welcome to segyio. For help, examples and reference, type `help(function)` in
your favourite python interpreter, or `pydoc function` in the unix console.

The segy library attempts to be easy to use efficently for prototyping and
interaction with possibly large segy files. File reading and writing is
streaming, with large file support out of the box and without hassle. For a
quick start on reading files, type `help(segyio.open)`.

An open segy file is interacted with in modes, found in the segy module. For a
reference with examples, please type `help(segyio.segy)`. For documentation on
individual modes, please refer to the individual modes with
`help(segyio.segy.file.[mode])`, or look it up in the aggregated segyio.segy.
The available modes are:
    * text, for textual headers including extended headers
    * bin, for the binary header
    * header, for the trace headers
    * trace, for trace data
    * iline, for inline biased operations
    * xline, for crossline biased operations

The primary data type is the numpy array. All examples use `np` for the numpy
namespace. That means that any function that returns a trace, a set of samples
or even full lines, returns a numpy array. This enables quick and easy
mathematical operations on the data you care about.

Segyio is designed to blend into regular python code, so python concepts that
map to segy operations are written to behave similarly. That means that
sequences of data support list lookup, slicing (`f.trace[0:10:2]`), `for x in`
etc. Please refer to the individual mode's documentation for a more exhaustive
list with examples.

For all slicing operations that segyio provides the underlying buffer is reused,
so if you want to keep the data between iterations it is necessary to manually
copy the data. Please refer to the examples. (e.g. numpy.copy())
"""


class Enum(object):
    def __init__(self, enum_value):
        super(Enum, self).__init__()
        self._value = int(enum_value)

    def __int__(self):
        return int(self._value)

    def __str__(self):
        for k, v in self.__class__.__dict__.items():
            if isinstance(v, int) and self._value == v:
                return k
        return "Unknown Enum"

    def __hash__(self):
        return hash(self._value)

    def __eq__(self, other):
        try:
            o = int(other)
        except ValueError:
            return super(Enum, self).__eq__(other)
        else:
            return self._value == o

    @classmethod
    def enums(cls):
        result = []
        for k, v in cls.__dict__.items():
            if isinstance(v, int):
                result.append(cls(v))

        return sorted(result, key=int)


from .segysampleformat import SegySampleFormat
from .tracesortingformat import TraceSortingFormat
from .tracefield import TraceField
from .binfield import BinField
from .open import open
from .create import create
from .segy import SegyFile, spec
