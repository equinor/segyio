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
"""

from .segysampleformat import SegySampleFormat
from .tracesortingformat import TraceSortingFormat
from .tracefield import TraceField
from .binfield import BinField
from .open import open
from .create import create
from .segy import file, spec
