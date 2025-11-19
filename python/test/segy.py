# -*- coding: utf-8 -*-

from __future__ import absolute_import

try:
    from future_builtins import zip, map
except ImportError:
    pass

from types import GeneratorType

import itertools
import filecmp
import shutil
import os
import numpy as np
import numpy.testing as npt
import pytest
from pytest import approx

from . import tmpfiles, small, smallps
from . import testdata

import segyio
from segyio import TraceField, BinField, TraceSortingFormat
from segyio.field import Field
from segyio.line import Line, HeaderLine
from segyio.trace import Trace, Header

small_sus = [
    (segyio.su.open, { 'filename': testdata / 'small.su',
                       'iline': 5,
                       'xline': 21 }),
    (segyio.su.open, { 'filename': testdata / 'small-lsb.su',
                       'iline': 5,
                       'xline': 21,
                       'endian': 'lsb' }),
]

def load_contents(filepath):
    with open(str(filepath), "rb") as f:
        data = bytearray(f.read())
    return data


small_segys = [
    (
        segyio.open,
        {'filename': testdata / 'small.sgy'}
    ),
    (
        segyio.open,
        {'filename': testdata / 'small-lsb.sgy', 'endian': 'little'}
    ),
    (
        segyio.open_from_memory,
        {'memory_buffer': load_contents(testdata / 'small.sgy')}
    ),
]

smallfiles = small_segys + small_sus

@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_inline_4(openfn, kwargs):
    with openfn(**kwargs) as f:
        sample_count = len(f.samples)
        assert 50 == sample_count

        data = f.iline[4]

        assert 4.2 == approx(data[0, 0], abs=1e-6)
        # middle sample
        assert 4.20024 == approx(data[0, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.20049 == approx(data[0, -1], abs=1e-6)

        # middle xline
        middle_line = 2
        # first sample
        assert 4.22 == approx(data[middle_line, 0], abs=1e-5)
        # middle sample
        assert 4.22024 == approx(data[middle_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.22049 == approx(data[middle_line, -1], abs=1e-6)

        # last xline
        last_line = (len(f.xlines) - 1)
        # first sample
        assert 4.24 == approx(data[last_line, 0], abs=1e-5)
        # middle sample
        assert 4.24024 == approx(data[last_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.24049 == approx(data[last_line, sample_count - 1], abs=1e-6)

def test_inline_4_seismic_unix():
    with segyio.su.open(testdata / 'small.su',
            iline = 5,
            xline = 21,
            endian = 'big',
        ) as f:

        sample_count = len(f.samples)
        assert 50 == sample_count

        data = f.iline[4]

        assert 4.2 == approx(data[0, 0], abs=1e-6)
        # middle sample
        assert 4.20024 == approx(data[0, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.20049 == approx(data[0, -1], abs=1e-6)

        # middle xline
        middle_line = 2
        # first sample
        assert 4.22 == approx(data[middle_line, 0], abs=1e-5)
        # middle sample
        assert 4.22024 == approx(data[middle_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.22049 == approx(data[middle_line, -1], abs=1e-6)

        # last xline
        last_line = (len(f.xlines) - 1)
        # first sample
        assert 4.24 == approx(data[last_line, 0], abs=1e-5)
        # middle sample
        assert 4.24024 == approx(data[last_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.24049 == approx(data[last_line, sample_count - 1], abs=1e-6)

@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_xline_22(openfn, kwargs):
    with openfn(**kwargs) as f:
        data = f.xline[22]

        size = len(f.samples)

        # first iline
        # first sample
        assert 1.22 == approx(data[0, 0], abs=1e-5)
        # middle sample
        assert 1.22024 == approx(data[0, size // 2 - 1], abs=1e-6)
        # last sample
        assert 1.22049 == approx(data[0, size - 1], abs=1e-6)

        # middle iline
        middle_line = 2
        # first sample
        assert 3.22 == approx(data[middle_line, 0], abs=1e-5)
        # middle sample
        assert 3.22024 == approx(data[middle_line, size // 2 - 1], abs=1e-6)
        # last sample
        assert 3.22049 == approx(data[middle_line, size - 1], abs=1e-6)

        # last iline
        last_line = len(f.ilines) - 1
        # first sample
        assert 5.22 == approx(data[last_line, 0], abs=1e-5)
        # middle sample
        assert 5.22024 == approx(data[last_line, size // 2 - 1], abs=1e-6)
        # last sample
        assert 5.22049 == approx(data[last_line, size - 1], abs=1e-6)


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_iline_slicing(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert len(f.ilines) == sum(1 for _ in f.iline)
        assert len(f.ilines) == sum(1 for _ in f.iline[1:6])
        assert len(f.ilines) == sum(1 for _ in f.iline[5:0:-1])
        assert len(f.ilines) // 2 == sum(1 for _ in f.iline[0::2])
        assert len(f.ilines) == sum(1 for _ in f.iline[1:])
        assert 3 == sum(1 for _ in f.iline[::2])
        assert 0 == sum(1 for _ in f.iline[12:24])
        assert 3 == sum(1 for _ in f.iline[:4])
        assert 2 == sum(1 for _ in f.iline[2:6:2])


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_xline_slicing(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert len(f.xlines) == sum(1 for _ in f.xline)
        assert len(f.xlines) == sum(1 for _ in f.xline[20:25])
        assert len(f.xlines) == sum(1 for _ in f.xline[25:19:-1])
        assert 3 == sum(1 for _ in f.xline[0::2])
        assert 3 == sum(1 for _ in f.xline[::2])
        assert len(f.xlines) == sum(1 for _ in f.xline[20:])
        assert 0 == sum(1 for _ in f.xline[12:18])
        assert 5 == sum(1 for _ in f.xline[:25])
        assert 2 == sum(1 for _ in f.xline[:25:3])


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_open_transposed_lines(openfn, kwargs):
    with openfn(**kwargs) as f:
        il = f.ilines
        xl = f.xlines

    with segyio.open(testdata / 'small.sgy', "r", segyio.TraceField.CROSSLINE_3D, segyio.TraceField.INLINE_3D) as f:
        assert list(il) == list(f.xlines)
        assert list(xl) == list(f.ilines)


# only run this test for the SEG-Ys, because the SU files are in IEEE float,
# not IBM float
@pytest.mark.parametrize(('openfn', 'kwargs'), small_segys)
def test_file_info(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert 2 == f.sorting
        assert 1 == f.offsets
        assert 1 == int(f.format)
        assert np.single == f.dtype
        assert 'ebcdic' == f.encoding

        xlines = list(range(20, 25))
        ilines = list(range(1, 6))
        assert xlines == list(f.xlines)
        assert ilines == list(f.ilines)
        assert 25 == f.tracecount
        assert len(f.trace) == f.tracecount
        assert 50 == len(f.samples)


def test_open_nostrict():
    with segyio.open(testdata / 'small.sgy', strict=False):
        pass


def test_open_ignore_geometry():
    with segyio.open(testdata / 'small.sgy', ignore_geometry=True) as f:
        with pytest.raises(ValueError):
            _ = f.iline[0]

def test_open_broken_file():
    metadata = {
        "endianness": 0,
        "encoding": 0,
        "format": 1,
        "elemsize": 4,
        "ext_textheader_count": 2,
        "trace0": 10000,
        "samplecount": 4,
        "trace_bsize": 16,
        "traceheader_count": 1,
        "tracecount": -1,
    }
    msg = (
        "unable to gather basic metadata from the file, error segyio.trace.size.mismatch. " +
        "Intermediate state:\n" +
        "\n".join(f"  {k}={v}" for k, v in metadata.items())
    )
    with pytest.raises(RuntimeError, match=msg):
        with segyio.open(testdata / 'broken.sgy') as f:
            pass


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_traces_slicing(openfn, kwargs):
    with openfn(**kwargs) as f:
        traces = list(map(np.copy, f.trace[0:6:2]))
        assert len(traces) == 3
        assert traces[0][49] == f.trace[0][49]
        assert traces[1][49] == f.trace[2][49]
        assert traces[2][49] == f.trace[4][49]

        rev_traces = list(map(np.copy, f.trace[4::-2]))
        assert rev_traces[0][49] == f.trace[4][49]
        assert rev_traces[1][49] == f.trace[2][49]
        assert rev_traces[2][49] == f.trace[0][49]

        # make sure buffers can be reused
        for i, trace in enumerate(f.trace[0:6:2]):
            assert np.array_equal(trace, traces[i])


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_traces_subslicing(openfn, kwargs):
    with openfn(**kwargs) as f:
        # test all sign permutations using full slice
        assert np.array_equal(f.trace[0, 0:6:2], f.trace[0][0:6:2])
        assert np.array_equal(f.trace[0, 0:-2:2], f.trace[0][0:-2:2])
        assert np.array_equal(f.trace[0, 10:2:-3], f.trace[0][10:2:-3])
        assert np.array_equal(f.trace[0, -10:50:1], f.trace[0][-10:50:1])
        assert np.array_equal(f.trace[0, -4:-2:1], f.trace[0][-4:-2:1])
        assert np.array_equal(f.trace[0, -4:0:-2], f.trace[0][-4:0:-2])
        assert np.array_equal(f.trace[0, 50:-50:-3], f.trace[0][50:-50:-3])
        # test all sign permutations using start:stop
        assert np.array_equal(f.trace[0, 0:6], f.trace[0][0:6])
        assert np.array_equal(f.trace[0, 0:-3], f.trace[0][0:-3])
        assert np.array_equal(f.trace[0, -4:-2], f.trace[0][-4:-2])
        assert np.array_equal(f.trace[0, -4:50], f.trace[0][-4:50])
        assert np.array_equal(f.trace[0, -4:-2], f.trace[0][-4:-2])
        # test all sign permutations using start::step
        assert np.array_equal(f.trace[0, 0::2], f.trace[0][0::2])
        assert np.array_equal(f.trace[0, 10::-1], f.trace[0][10::-1])
        assert np.array_equal(f.trace[0, -5::3], f.trace[0][-5::3])
        assert np.array_equal(f.trace[0, -5::-1], f.trace[0][-5::-1])
        # test all sign permutations using :stop:step
        assert np.array_equal(f.trace[0, :6:2], f.trace[0][:6:2])
        assert np.array_equal(f.trace[0, :6:-1], f.trace[0][:6:-1])
        assert np.array_equal(f.trace[0, :-6:2], f.trace[0][:-6:2])
        assert np.array_equal(f.trace[0, :-6:-2], f.trace[0][:-6:-2])
        # test all sign permutations using start:, :stop, and ::step
        assert np.array_equal(f.trace[0, 1:], f.trace[0][1:])
        assert np.array_equal(f.trace[0, -3:], f.trace[0][-3:])
        assert np.array_equal(f.trace[0, :3], f.trace[0][:3])
        assert np.array_equal(f.trace[0, :-1], f.trace[0][:-1])
        assert np.array_equal(f.trace[0, ::-1], f.trace[0][::-1])
        assert np.array_equal(f.trace[0, ::2], f.trace[0][::2])
        # test getting single element

        # should be single a single float, not an array in the case of int(i),
        # int(j)
        with pytest.raises(TypeError):
            len(f.trace[0, 1])

        # even length-of-one slices should give arrays
        assert len(f.trace[0, 1:2]) == 1
        npt.assert_array_equal(f.trace[0, 1:2], f.trace[0][1:2])

        assert f.trace[0, 1] == f.trace[0][1]
        assert f.trace[0, -3] == f.trace[0][-3]

        # Combining trace and sub-trace slicing
        traces = list(map(np.copy, f.trace[0:6:2, 0:6]))
        assert len(traces) == 3
        assert traces[0].shape[0] == 6


def test_traces_offset():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        assert 2 == len(f.offsets)
        assert [1, 2] == list(f.offsets)

        # traces are laid out |l1o1 l1o2 l2o1 l2o2...|
        # where l = iline number and o = offset number
        # traces are not re-indexed according to offsets
        # see make-ps-file.py for value formula
        assert 101.01 == approx(f.trace[0][0], abs=1e-4)
        assert 201.01 == approx(f.trace[1][0], abs=1e-4)
        assert 101.02 == approx(f.trace[2][0], abs=1e-4)
        assert 201.02 == approx(f.trace[3][0], abs=1e-4)
        assert 102.01 == approx(f.trace[6][0], abs=1e-4)


def test_headers_offset():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
        assert f.header[0][il] == f.header[1][il]
        assert f.header[1][il] == f.header[2][il]

        assert f.header[0][xl] == f.header[1][xl]
        assert not f.header[1][xl] == f.header[2][xl]

@pytest.mark.parametrize(('openfn', 'kwargs'), small_sus)
def test_disabled_methods_seismic_unix(openfn, kwargs):
    with openfn(**kwargs) as f:
        with pytest.raises(NotImplementedError):
            _ = f.text[0]

        with pytest.raises(NotImplementedError):
            _ = f.bin

        with pytest.raises(NotImplementedError):
            f.bin = {}

@pytest.mark.parametrize(('openfn', 'kwargs'), small_segys)
def test_header_dict_methods(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert 90 == len(list(f.header[0].keys()))
        assert 90 == len(list(f.header[1].values()))
        assert 90 == len(list(f.header[2].items()))
        assert 90 == len(list(f.header[3]))
        assert 0 not in f.header[0]
        assert 1 in f.header[0]
        assert segyio.su.cdpx in f.header[0]
        iter(f.header[0])

        assert 45 == len(f.bin.keys())
        assert 45 == len(list(f.bin.values()))
        assert 45 == len(list(f.bin.items()))
        assert 45 == len(f.bin)
        iter(f.bin)


def test_header_dropped_writes(small):
    with segyio.open(small, mode='r+') as f:
        f.header[10] = { 1: 5, 5: 10 }

    with segyio.open(small, mode='r+') as f:
        x, y = f.header[10], f.header[10]

        assert x[1, 5] == { 1: 5, 5: 10 }
        assert y[1, 5] == { 1: 5, 5: 10 }

        # write to x[1] is invisible to y
        x[1] = 6
        assert x[1] == 6
        assert y[1] == 5

        y.reload()
        assert x[1] == 6
        assert y[1] == 6

        x[1] = 5
        assert x[1] == 5
        assert y[1] == 6

        # the write to x[1] is lost
        y[5] = 1
        assert x[1] == 5
        assert x.reload()
        assert x[1] == 6
        assert y[1, 5] == { 1: 6, 5: 1 }


def test_headers_line_offset(smallps):
    il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
    with segyio.open(smallps, "r+") as f:
        f.header.iline[1, 2] = {il: 11}
        f.header.iline[1, 2] = {xl: 13}

    with segyio.open(smallps, strict=False) as f:
        assert f.header[0][il] == 1
        assert f.header[1][il] == 11
        assert f.header[2][il] == 1

        assert f.header[0][xl] == 1
        assert f.header[1][xl] == 13
        assert f.header[2][xl] == 2


def test_write_headers_line_slice_offset_int(smallps):
    il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
    with segyio.open(smallps, "r+") as f:
        f.header.iline[1:3, 2] = {xl: 13}

    with segyio.open(smallps, strict=False) as f:
        assert f.header[0][xl] == 1
        assert f.header[1][xl] == 13
        assert f.header[2][xl] == 2
        assert f.header[3][xl] == 13

def test_write_headers_line_int_offset_slice(smallps):
    il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
    with segyio.open(smallps, "r+") as f:
        f.header.iline[1, :] = {xl: 13}

    with segyio.open(smallps, strict=False) as f:
        assert f.header[0][xl] == 13
        assert f.header[1][xl] == 13
        assert f.header[2][xl] == 13

@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_attributes(openfn, kwargs):
    with openfn(**kwargs) as f:
        il = kwargs.get('iline', TraceField.INLINE_3D)
        xl = kwargs.get('xline', TraceField.CROSSLINE_3D)

        assert 1 == f.attributes(il)[0]
        assert 20 == f.attributes(xl)[0]

        assert f.tracecount == len(f.attributes(il))
        assert iter(f.attributes(il))

        ils = [(i // 5) + 1 for i in range(25)]
        attrils = list(map(int, f.attributes(il)[:]))
        assert ils == attrils

        xls = [(i % 5) + 20 for i in range(25)]
        attrxls = list(map(int, f.attributes(xl)[:]))
        assert xls == attrxls

        ils = [(i // 5) + 1 for i in range(25)][::-1]
        attrils = list(map(int, f.attributes(il)[::-1]))
        assert ils == attrils

        xls = [(i % 5) + 20 for i in range(25)][::-1]
        attrxls = list(map(int, f.attributes(xl)[::-1]))
        assert xls == attrxls

        assert f.header[0][il] == f.attributes(il)[0]
        f.mmap()
        assert f.header[0][il] == f.attributes(il)[0]

        ils = [(i // 5) + 1 for i in range(25)][1:21:3]
        attrils = list(map(int, f.attributes(il)[1:21:3]))
        assert ils == attrils

        xls = [(i % 5) + 20 for i in range(25)][2:17:5]
        attrxls = list(map(int, f.attributes(xl)[2:17:5]))
        assert xls == attrxls

        ils = [1, 2, 3, 4, 5]
        attrils = list(map(int, f.attributes(il)[[0, 5, 11, 17, 23]]))
        assert ils == attrils

        ils = [1, 2, 3, 4, 5]
        indices = np.asarray([0, 5, 11, 17, 23])
        attrils = list(map(int, f.attributes(il)[indices]))
        assert ils == attrils


def test_iline_offset():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        line1 = f.iline[1, 1]
        assert 101.01 == approx(line1[0][0], abs=1e-4)
        assert 101.02 == approx(line1[1][0], abs=1e-4)
        assert 101.03 == approx(line1[2][0], abs=1e-4)

        assert 101.01001 == approx(line1[0][1], abs=1e-4)
        assert 101.01002 == approx(line1[0][2], abs=1e-4)
        assert 101.02001 == approx(line1[1][1], abs=1e-4)

        line2 = f.iline[1, 2]
        assert 201.01 == approx(line2[0][0], abs=1e-4)
        assert 201.02 == approx(line2[1][0], abs=1e-4)
        assert 201.03 == approx(line2[2][0], abs=1e-4)

        assert 201.01001 == approx(line2[0][1], abs=1e-4)
        assert 201.01002 == approx(line2[0][2], abs=1e-4)
        assert 201.02001 == approx(line2[1][1], abs=1e-4)

        with pytest.raises(KeyError):
            _ = f.iline[1, 0]

        with pytest.raises(KeyError):
            _ = f.iline[1, 3]

        with pytest.raises(KeyError):
            _ = f.iline[100, 1]

        with pytest.raises(TypeError):
            _ = f.iline[1, {}]


def test_iline_slice_fixed_offset():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        for i, ln in enumerate(f.iline[:, 1], 1):
            assert i + 100.01 == approx(ln[0][0], abs=1e-4)
            assert i + 100.02 == approx(ln[1][0], abs=1e-4)
            assert i + 100.03 == approx(ln[2][0], abs=1e-4)

            assert i + 100.01001 == approx(ln[0][1], abs=1e-4)
            assert i + 100.01002 == approx(ln[0][2], abs=1e-4)
            assert i + 100.02001 == approx(ln[1][1], abs=1e-4)


def test_iline_slice_fixed_line():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        for i, ln in enumerate(f.iline[1, :], 1):
            off = i * 100
            assert off + 1.01 == approx(ln[0][0], abs=1e-4)
            assert off + 1.02 == approx(ln[1][0], abs=1e-4)
            assert off + 1.03 == approx(ln[2][0], abs=1e-4)

            assert off + 1.01001 == approx(ln[0][1], abs=1e-4)
            assert off + 1.01002 == approx(ln[0][2], abs=1e-4)
            assert off + 1.02001 == approx(ln[1][1], abs=1e-4)


def test_iline_slice_all_offsets():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        offs, ils = len(f.offsets), len(f.ilines)
        assert offs * ils == sum(1 for _ in f.iline[:, :])
        assert offs * ils == sum(1 for _ in f.iline[:, ::-1])
        assert offs * ils == sum(1 for _ in f.iline[::-1, :])
        assert offs * ils == sum(1 for _ in f.iline[::-1, ::-1])
        assert 0 == sum(1 for _ in f.iline[:, 10:12])
        assert 0 == sum(1 for _ in f.iline[10:12, :])

        assert (offs // 2) * ils == sum(1 for _ in f.iline[::2, :])
        assert offs * (ils // 2) == sum(1 for _ in f.iline[:, ::2])

        assert (offs // 2) * ils == sum(1 for _ in f.iline[::-2, :])
        assert offs * (ils // 2) == sum(1 for _ in f.iline[:, ::-2])

        assert (offs // 2) * (ils // 2) == sum(1 for _ in f.iline[::2, ::2])
        assert (offs // 2) * (ils // 2) == sum(1 for _ in f.iline[::2, ::-2])
        assert (offs // 2) * (ils // 2) == sum(1 for _ in f.iline[::-2, ::2])


def test_gather_mode():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        empty = np.empty(0, dtype=np.single)
        # should raise
        with pytest.raises(KeyError):
            assert np.array_equal(empty, f.gather[2, 3, 3])

        with pytest.raises(KeyError):
            assert np.array_equal(empty, f.gather[2, 5, 1])

        with pytest.raises(KeyError):
            assert np.array_equal(empty, f.gather[5, 2, 1])

        assert np.array_equal(f.trace[10], f.gather[2, 3, 1])
        assert np.array_equal(f.trace[11], f.gather[2, 3, 2])

        traces = segyio.tools.collect(f.trace[10:12])
        gather = f.gather[2, 3, :]
        assert np.array_equal(traces, gather)
        assert np.array_equal(traces, f.gather[2, 3])
        assert np.array_equal(empty, f.gather[2, 3, 1:0])
        assert np.array_equal(empty, f.gather[2, 3, 3:4])
        # selects the first offset, not the second;
        # see https://github.com/equinor/segyio/issues/499
        assert np.array_equal(traces[:1], f.gather[2, 3, ::2])

        for g, line in zip(f.gather[1:3, 3, 1], f.iline[1:3]):
            assert 10 == len(g)
            assert (10,) == g.shape
            assert np.array_equal(line[2], g)

        for g, line in zip(f.gather[1:3, 3, :], f.iline[1:3]):
            assert 2 == len(g)
            assert (2, 10) == g.shape
            assert np.array_equal(line[2], g[0])

        for g, line in zip(f.gather[:, 2, 1], f.iline[:]):
            assert 10 == len(g)
            assert (10,) == g.shape
            assert np.array_equal(line[1], g)

        for g, line in zip(f.gather[1, :, 1], f.xline[:]):
            assert 10 == len(g)
            assert (10,) == g.shape
            assert np.array_equal(line[0], g)

        # overshoot the inlines, should still work
        for g, line in zip(f.gather[3:10, 3, 1], f.iline[3:]):
            assert 10 == len(g)
            assert (10,) == g.shape
            print(g)
            print(line[0])
            assert np.array_equal(line[2], g)

        for g, line in zip(f.gather[1, 1:3, :], f.xline[1:3]):
            assert 2 == len(g)
            assert (2, 10) == g.shape
            assert np.array_equal(line[0], g[0])

        for g, line in zip(f.gather[1, 1:3, 3:4], f.xline[1:3]):
            assert np.array_equal(empty, g)


def test_line_generators():
    with segyio.open(testdata / 'small.sgy') as f:
        for _ in f.iline:
            pass

        for _ in f.xline:
            pass


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_fast_slow_dimensions(openfn, kwargs):
    with openfn(**kwargs) as f:
        for iline, fline in zip(f.iline, f.fast):
            assert np.array_equal(iline, fline)

        for xline, sline in zip(f.xline, f.slow):
            assert np.array_equal(xline, sline)


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_traces_raw(openfn, kwargs):
    with openfn(**kwargs) as f:
        gen_traces = np.array(list(map(np.copy, f.trace)), dtype=np.single)

        raw_traces = f.trace.raw[:]
        assert np.array_equal(gen_traces, raw_traces)

        assert len(gen_traces) == f.tracecount
        assert len(raw_traces) == f.tracecount

        assert gen_traces[0][49] == raw_traces[0][49]
        assert gen_traces[1][49] == f.trace.raw[1][49]
        assert gen_traces[2][49] == raw_traces[2][49]

        assert np.array_equal(f.trace[10], f.trace.raw[10])

        for raw, gen in zip(f.trace.raw[::2], f.trace[::2]):
            assert np.array_equal(raw, gen)

        for raw, gen in zip(f.trace.raw[::-1], f.trace[::-1]):
            assert np.array_equal(raw, gen)

def test_read_header_embedded_null():
    with segyio.open(testdata / 'text-embed-null.sgy', ignore_geometry = True) as f:
        text = f.text[0]
        assert len(text) == 3200

def test_read_text_sequence():
    with segyio.open(testdata / 'multi-text.sgy', ignore_geometry = True) as f:
        for text in f.text[:]:
            assert text

        assert iter(f.text)

        # file is ebcdic encoded, but values are returned as ascii
        assert f.encoding == "ebcdic"
        assert len(f.text[0]) == 3200
        assert f.text[0][0] == 67  # "C" character in ASCII

        decoded = f.text[0].decode('latin-1')
        lines = list(map(''.join, zip(*[iter(decoded)] * 80)))
        for line in lines:
            assert line.startswith('C')

    with segyio.open(testdata / 'delay-scalar.sgy', ignore_geometry=True) as f:
        # file is ascii encoded and values are returned as ascii
        assert f.encoding == "ascii"
        assert len(f.text[0]) == 3200
        assert f.text[0][0] == 67

        decoded = f.text[0].decode('ascii', errors='replace')
        lines = list(map(''.join, zip(*[iter(decoded)] * 80)))
        for line in lines:
            assert line.startswith('C')

    with segyio.open(testdata / 'stanzas-unknown-count.sgy') as f:
        assert f.tracecount == 6


@tmpfiles(testdata / 'multi-text.sgy')
def test_put_text_sequence(tmpdir):
    lines = { 1: 'first line', 10: 'last line' }
    ref = segyio.tools.create_text_header(lines)
    fname = str(tmpdir / 'multi-text.sgy')

    with segyio.open(fname, mode = 'r+', ignore_geometry = True) as f, \
            segyio.open(testdata / 'small.sgy') as g:
        f.text[0] = ref
        f.text[1] = f.text
        f.text[-1] = ref

        f.text[2:4] = [g.text, f.text]
        assert f.text[2] == g.text[0]
        f.text[2:3] = [ref]

    # ref doesn't have to be bytes for reading, but has to in order to compare
    # with the returned object from text
    ref = bytearray(ref, 'ascii')
    ref = bytes(ref)
    with segyio.open(fname, ignore_geometry = True) as f:
        for text in f.text:
            assert text == ref


def test_update_text_encoding(small):
    lines = {1: 'first line', 10: 'last line'}
    ref = bytes(segyio.tools.create_text_header(lines), 'ascii')

    encodings = ['ascii', 'ebcdic']

    for write_encoding in encodings:
        with segyio.open(small, mode='r+', encoding=write_encoding) as f:
            f.text[0] = ref

        with segyio.open(small) as f:
            assert f.text[0] == ref

        for read_encoding in encodings:
            with segyio.open(small, encoding=read_encoding) as f:
                if read_encoding == write_encoding:
                    assert f.text[0] == ref
                else:
                    assert f.text[0] != ref


def test_create_ascii_text(tmpdir):
    fresh = str(tmpdir / 'fresh.sgy')

    lines = {1: 'first line', 10: 'last line'}
    ref = bytes(segyio.tools.create_text_header(lines), 'ascii')

    spec = segyio.tools.metadata(testdata / 'small-ps.sgy')
    spec.encoding = "ascii"

    with segyio.create(fresh, spec) as f:
        for i in range(spec.tracecount):
            f.trace[i] = np.zeros(len(spec.samples), dtype=np.single)
        f.text[0] = ref

    with segyio.open(fresh, ignore_geometry=True) as f:
        assert f.text[0] == ref


def test_stanzas():
    filename = str(testdata / 'small.sgy')
    with segyio.open(filename, "r") as f:
        assert len(f.stanza) == 0

    filename = str(testdata / 'multi-text.sgy')
    with segyio.open(filename, "r") as f:
        assert f.stanza.names() == ['']
        assert len(f.stanza) == 1

    filename = str(testdata / 'stanzas-known-count.sgy')
    with segyio.open(filename, "r") as f:
        assert f.stanza.names() == [
            'SEGYIO:TEST ASCII  DATA WITH CONTENTTYPE AND BYTES: application/vnd.openxmlformats-officedocument.wordprocessingml.document.glossary+xml:666',
            'SEGYIO:Test EBCDIC data',
            'SEGYIO: test ASCII data'
        ]
        assert len(f.stanza) == 3

        for stanza in f.stanza:  # __iter__ check
            pass

    filename = str(testdata / 'mapping-shifted.sgy')
    with segyio.open(filename, "r") as f:
        layout_stanza_index = -1
        for i, name in enumerate(f.stanza.names()):
            if name.upper().startswith("SEG:LAYOUT"):
                layout_stanza_index = i
        assert layout_stanza_index == 0

    filename = str(testdata / 'stanzas-unknown-count.sgy')
    with segyio.open(filename, "r") as f:
        stanza = f.stanza[0]

        name_len = len(f.stanza.names()[0]) + 4
        block2_offset = 3200 - name_len

        assert len(stanza) == 3200 * 2 - name_len

        expected_start1 = bytes("first part", "ascii")
        expected_start2 = bytes("second part", "ascii")

        assert stanza[0:].startswith(expected_start1)
        assert stanza[block2_offset:].startswith(expected_start2)

        for i in range(len(expected_start1), block2_offset):
            assert stanza[i] == ord(' ')
        for i in range(block2_offset+len(expected_start2), len(stanza)):
            assert stanza[i] == ord(' ')


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_header_getitem_intlikes(openfn, kwargs):
    with openfn(**kwargs) as f:
        h = f.header[0]
        assert 1 == h[37]
        assert 1 == h[segyio.su.offset]
        assert 1 == h[TraceField.offset]

@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_read_header(openfn, kwargs):
    with openfn(**kwargs) as f:
        il = kwargs.get('iline', TraceField.INLINE_3D)
        assert 1 == f.header[0][il]
        assert 5 == f.header[-1][il]
        assert dict(f.header[-1]) == dict(f.header[24])

        with pytest.raises(IndexError):
            _ = f.header[30]

        with pytest.raises(IndexError):
            _ = f.header[-30]

        with pytest.raises(KeyError):
            _ = f.header[0][188]  # between byte offsets

        with pytest.raises(KeyError):
            _ = f.header[0][-1]

        with pytest.raises(KeyError):
            _ = f.header[0][700]

def test_read_header_seismic_unix():
    il = 5
    with segyio.su.open(testdata / 'small.su',
                        ignore_geometry = True,
                        endian = 'big') as f:
        assert 1 == f.header[0][il]
        assert 1 == f.header[1][il]
        assert 5 == f.header[-1][il]
        assert 5 == f.header[24][il]
        assert dict(f.header[-1]) == dict(f.header[24])

def test_write_header(small):
    with segyio.open(small, "r+") as f:
        # assign to a field in a header, write immediately
        f.header[0][189] = 42
        f.flush()

        assert 42 == f.header[0][189]
        assert 1 == f.header[1][189]

        # accessing non-existing offsets raises exceptions
        with pytest.raises(KeyError):
            f.header[0][188] = 1  # between byte offsets

        with pytest.raises(KeyError):
            f.header[0][-1] = 1

        with pytest.raises(KeyError):
            f.header[0][700] = 1

        d = {TraceField.INLINE_3D: 43,
             TraceField.CROSSLINE_3D: 11,
             TraceField.offset: 15}

        # assign multiple fields at once by using a dict
        f.header[1] = d

        f.flush()
        assert 43 == f.header[1][TraceField.INLINE_3D]
        assert 11 == f.header[1][segyio.su.xline]
        assert 15 == f.header[1][segyio.su.offset]

        # looking up multiple values at once returns a { TraceField: value } dict
        assert d == f.header[1][TraceField.INLINE_3D, TraceField.CROSSLINE_3D, TraceField.offset]

        # slice-support over headers (similar to trace)
        for _ in f.header[0:10]:
            pass

        assert 6 == len(list(f.header[10::-2]))
        assert 5 == len(list(f.header[10:5:-1]))
        assert 0 == len(list(f.header[10:5]))

        d = {TraceField.INLINE_3D: 45,
             TraceField.CROSSLINE_3D: 10,
             TraceField.offset: 16}

        # assign multiple values using alternative syntax
        f.header[5].update(d)
        f.flush()
        assert 45 == f.header[5][TraceField.INLINE_3D]
        assert 10 == f.header[5][segyio.su.xline]
        assert 16 == f.header[5][segyio.su.offset]

        # accept anything with a key-value structure
        f.header[5].update([(segyio.su.ns, 12), (segyio.su.dt, 4)])
        f.header[5].update(((segyio.su.muts, 3), (segyio.su.mute, 7)))
        f.header[5].update([(segyio.su.muts, 3)], sx=7)
        f.header[5].update(sy=8)

        with pytest.raises(TypeError):
            f.header[0].update(10)

        with pytest.raises(TypeError):
            f.header[0].update(None)

        with pytest.raises(ValueError):
            f.header[0].update('foo')

        f.flush()
        assert 12 == f.header[5][segyio.su.ns]
        assert 4 == f.header[5][segyio.su.dt]
        assert 3 == f.header[5][segyio.su.muts]
        assert 7 == f.header[5][segyio.su.mute]
        assert 7 == f.header[5][segyio.su.sx]
        assert 8 == f.header[5][segyio.su.sy]

        # for-each support
        for _ in f.header:
            pass

        # copy a header
        f.header[2] = f.header[1]
        f.flush()

        d = {TraceField.INLINE_3D: 12,
             TraceField.CROSSLINE_3D: 13,
             TraceField.offset: 14}

        # assign multiple values with a slice
        f.header[:5] = d
        f.flush()

        for i in range(5):
            assert 12 == f.header[i][TraceField.INLINE_3D]
            assert 13 == f.header[i][segyio.su.xline]
            assert 14 == f.header[i][segyio.su.offset]

        # don't use this interface in production code, it's only for testing
        # i.e. don't access buf of treat it as a list
        # assertEqual(list(f.header[2].buf), list(f.header[1].buf))


def test_field_via_traceheader(small):
    with segyio.open(small, "r+") as f:
        assert isinstance(f.traceheader[0][0], Field)

        assert 1 == f.traceheader[0][0][189]
        assert 5 == f.traceheader[-1][0][189]

        with pytest.raises(KeyError):
            f.traceheader[0][0][700]

        with pytest.raises(KeyError):
            f.traceheader[0][0][700] = 1

        assert f.traceheader[0][0][193] == f.traceheader[0].SEG00000.xline

        dict = {
            f.tracefield.SEG00000.iline.offset(): 42,
            f.tracefield.SEG00000.xline.offset(): 88,
        }
        f.traceheader[0].SEG00000.update(dict)
        f.traceheader[1].SEG00000.xline = 12
        f.flush()

        cached_traceheader = f.traceheader[0].SEG00000
        assert cached_traceheader[189] == 42
        assert cached_traceheader[193] == 88

        assert f.traceheader[1][0][189] == 1
        assert f.traceheader[1][0][193] == 12

        with pytest.raises(ValueError, match=r"Value out of range for *"):
            f.traceheader[0][0][189] = 36.6

        with pytest.raises(TypeError):
            # makes no sense, we do not iterate over field this way
            f.traceheader[0][0][:]


def test_traceheaders(small):
    with segyio.open(small, "r+") as f:
        assert isinstance(f.traceheader[0], segyio.trace.RowFieldAccessor)
        assert len(f.traceheader[0]) == f.traceheader_count
        assert len(list(f.traceheader[0][:])) == f.traceheader_count

        for traceheader in f.traceheader[0]:  # same as f.traceheader[0][:]
            assert isinstance(traceheader, Field)
            # there is only standard traceheader, so iline is always valid
            assert traceheader.iline == 1

        with pytest.raises(IndexError):
            _ = f.traceheader[0][30]

        with pytest.raises(TypeError):
            f.traceheader[1][0] = {189: 43}

        with pytest.raises(TypeError):
            # not supported
            f.traceheader[0][:][189] = 42

        f.traceheader[1][0] = f.traceheader[0][0]
        f.flush()

        assert f.traceheader[1][0] == f.traceheader[0][0]
        assert f.traceheader[1].SEG00000 == f.traceheader[0].SEG00000

        f.traceheader[3][0:1] = f.traceheader[2][0:1]
        assert list(f.traceheader[3][0:1]) == list(f.traceheader[2][0:1])


def test_traces_traceheaders(small):
    with segyio.open(small, "r+") as f:
        assert isinstance(
            f.traceheader, segyio.trace.FileFieldAccessor)
        assert len(f.traceheader) == f.tracecount
        assert len(list(f.traceheader[:])) == f.tracecount

        for traceheader_row in f.traceheader:  # same as f.traceheader[:]
            assert len(traceheader_row) == 1

        with pytest.raises(IndexError):
            _ = f.traceheader[30]

        with pytest.raises(TypeError):
            f.traceheader[1] = {TraceField.INLINE_3D: 43}

        f.traceheader[2] = f.traceheader[1]
        f.flush()

        for i in range(f.traceheader_count):
            assert f.traceheader[1][i] == f.traceheader[2][i]

        f.traceheader[:5] = f.traceheader[-5:]
        f.flush()

        for i in range(f.traceheader_count):
            assert f.traceheader[2][i] == f.traceheader[f.tracecount - 3][i]


def test_traceheader_attributes(small):
    with segyio.open(small, "r") as f:
        attrs = f.attributes(f.tracefield.SEG00000.xline.offset())
        assert len(attrs) == f.tracecount
        assert np.array_equal(list(attrs), [20, 21, 22, 23, 24] * 5)

        assert np.array_equal(f.tracefield.SEG00000.xline[:], attrs)
        assert np.array_equal(f.tracefield.SEG00000.xline[0:5], attrs[0:5])
        assert np.array_equal(f.tracefield.SEG00000.xline[0, 5, 11, 12], attrs[0, 5, 11, 12])

        assert f.tracefield.SEG00000.xline[0] == 20


def test_depricated_fields(small):
    with segyio.open(small, "r") as f:
        assert f.bin[BinField.EnsembleTraces] == 25
        with pytest.warns(DeprecationWarning, match="Traces is deprecated and will be removed in a future version."):
            assert f.bin[BinField.Traces] == 25

        assert f.bin[BinField.AuxEnsembleTraces] == 0
        with pytest.warns(DeprecationWarning, match="AuxTraces is deprecated and will be removed in a future version."):
            assert f.bin[BinField.AuxTraces] == 0

        assert f.bin[BinField.ExtEnsembleTraces] == 0
        with pytest.warns(DeprecationWarning, match="ExtTraces is deprecated and will be removed in a future version."):
            assert f.bin[BinField.ExtTraces] == 0

        assert f.bin[BinField.ExtAuxEnsembleTraces] == 0
        with pytest.warns(DeprecationWarning, match="ExtAuxTraces is deprecated and will be removed in a future version."):
            assert f.bin[BinField.ExtAuxTraces] == 0

def test_write_binary(small):
    with segyio.open(small, "r+") as f:
        f.bin[3213] = 5
        f.flush()

        assert 5 == f.bin[3213]

        # accessing non-existing offsets raises exceptions
        with pytest.raises(KeyError):
            _ = f.bin[0]

        with pytest.raises(KeyError):
            _ = f.bin[50000]

        with pytest.raises(KeyError):
            _ = f.bin[3214]

        d = {BinField.EnsembleTraces: 43,
             BinField.SweepFrequencyStart: 11}

        # assign multiple fields at once by using a dict
        f.bin = d

        f.flush()
        assert 43 == f.bin[segyio.su.ntrpr]
        assert 11 == f.bin[segyio.su.hsfs]

        d = {BinField.EnsembleTraces: 45,
             BinField.SweepFrequencyStart: 10}

        # assign multiple values using alternative syntax
        f.bin.update(d)
        f.flush()
        assert 45 == f.bin[segyio.su.ntrpr]
        assert 10 == f.bin[segyio.su.hsfs]

        # accept anything with a key-value structure
        f.bin.update([(segyio.su.jobid, 12), (segyio.su.lino, 4)])
        f.bin.update(((segyio.su.reno, 3), (segyio.su.hdt, 7)))

        f.flush()
        assert 12 == f.bin[segyio.su.jobid]
        assert 4 == f.bin[segyio.su.lino]
        assert 3 == f.bin[segyio.su.reno]
        assert 7 == f.bin[segyio.su.hdt]

        # looking up multiple values at once returns a { TraceField: value } dict
        assert d == f.bin[BinField.EnsembleTraces, BinField.SweepFrequencyStart]

        # copy a header
        f.bin = f.bin


def test_write_header_update_atomic(small):
    with segyio.open(small, "r+") as f:
        orig = dict(f.header[10])

        d = { 1:  10,
              37: 4,
              73: 15,
              2:  10, # this key raises error
            }

        # use the same instance all the time, to also catch the case where
        # update dirties the backing storage
        header = f.header[10]
        with pytest.raises(KeyError):
            header.update(d)

        assert orig == header

        # flushing the header should just write a clean buffer
        header.flush()
        assert orig == header

        del d[2]
        header.update(d)

        assert header[1] == 10
        assert header[73] == 15
        assert f.header[10][37] == 4

        fresh = orig.copy()
        fresh.update(d)

        assert orig != header
        assert orig != f.header[10]
        assert fresh == f.header[10]
        assert header == f.header[10]


@pytest.mark.parametrize('endian', ['msb', 'lsb'])
def test_field_types_read(endian):
    if endian == 'lsb':
        dname = testdata / 'decrement-lsb.sgy'
        iname = testdata / 'increment-lsb.sgy'
    else:
        dname = testdata / 'decrement.sgy'
        iname = testdata / 'increment.sgy'

    with segyio.open(dname, "r", ignore_geometry=True) as f:
        bdec = f.bin
        tdec = f.header[3]

        assert bdec[BinField.JobID] == 2147483647
        assert bdec[BinField.SamplesOriginal] == 65526
        assert bdec[BinField.VerticalSum] == 32755
        assert bdec[BinField.ExtIntervalOriginal] == 1125899906842593.0
        assert bdec[BinField.SEGYRevisionMinor] == 217
        assert bdec[BinField.NrTracesInStream] == 18446744073709551572

        assert tdec[TraceField.DelayRecordingTime] == 32627
        assert tdec[TraceField.TRACE_SAMPLE_COUNT] == 65382
        assert tdec[TraceField.INLINE_3D] == 2147483355

    with segyio.open(iname, "r", ignore_geometry=True) as f:
        binc = f.bin
        tinc = f.header[3]

        assert binc[BinField.JobID] == -2147483648
        assert binc[BinField.SamplesOriginal] == 8
        assert binc[BinField.VerticalSum] == -32756
        assert binc[BinField.ExtIntervalOriginal] == -1125899906842593.0
        assert binc[BinField.SEGYRevisionMinor] == 37
        assert binc[BinField.NrTracesInStream] == 43

        assert tinc[TraceField.DelayRecordingTime] == -32628
        assert tinc[TraceField.TRACE_SAMPLE_COUNT] == 152
        assert tinc[TraceField.INLINE_3D] == -2147483356


@tmpfiles(testdata / 'small.sgy')
@tmpfiles(testdata / 'small-lsb.sgy')
@pytest.mark.parametrize('endian', ['msb', 'lsb'])
def test_field_types_write(tmpdir, endian):
    if endian == 'lsb':
        updpath = tmpdir / 'small-lsb.sgy'
    else:
        updpath = tmpdir / 'small.sgy'

    with segyio.open(updpath, ignore_geometry=True, endian=endian) as f:
        orig_binh = f.bin
        orig_trh = f.header[0]

    bupd = {
        BinField.JobID: 2147483647,
        BinField.SamplesOriginal: 65535,
        BinField.VerticalSum: -32768,
        BinField.ExtIntervalOriginal: 10.125,
        BinField.SEGYRevisionMinor: 255,
        BinField.NrTracesInStream: 18446744073709551615,
    }

    tupd = {
        TraceField.DelayRecordingTime: 32767,
        TraceField.TRACE_SAMPLE_COUNT: 65535,
        TraceField.INLINE_3D: -2147483648,
    }

    with segyio.open(updpath, mode='r+', ignore_geometry=True, endian=endian) as f:
        f.bin.update(bupd)
        f.header[0].update(tupd)
        f.flush()

    with segyio.open(updpath, ignore_geometry=True, endian=endian) as f:
        binh = f.bin
        trh = f.header[0]

        assert orig_binh != binh
        assert orig_trh != trh

        for k, v in bupd.items():
            assert binh[k] == v

        for k, v in tupd.items():
            assert trh[k] == v


@tmpfiles(testdata / 'small.sgy')
def test_field_types_bad_write(tmpdir):
    updpath = tmpdir / 'small.sgy'

    with segyio.open(updpath, ignore_geometry=True) as f:
        binh = f.bin

    with segyio.open(updpath, mode='r+', ignore_geometry=True) as f:
        bad = {
            BinField.JobID: [18446744073709551616, 2147483648],
            BinField.SamplesOriginal: [65536, -1],
            BinField.VerticalSum: [-32769],
            BinField.ExtIntervalOriginal: ["wrong"],
            BinField.SEGYRevisionMinor: [256, -127],
            BinField.NrTracesInStream: [18446744073709551616],
        }

        for k, vlist in bad.items():
            for v in vlist:
                with pytest.raises(ValueError, match="out of range"):
                    f.bin[k] = v

        f.flush()

    with segyio.open(updpath, ignore_geometry=True) as f:
        assert binh == f.bin


def test_fopen_error():
    # non-existent file
    with pytest.raises(IOError):
        segyio.open("no_dir/no_file")

    # non-existant mode
    with pytest.raises(ValueError):
        segyio.open(testdata / 'small.sgy', "foo")

    with pytest.raises(ValueError):
        segyio.open(testdata / 'small.sgy', "r+b+toolong")


def test_getitem_None():
    with pytest.raises(TypeError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.trace[None]

    with pytest.raises(TypeError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.trace.raw[None]

    with pytest.raises(TypeError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.header[None]

    with pytest.raises(KeyError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.iline[None]

    with pytest.raises(KeyError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.xline[None]

    with pytest.raises(TypeError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.depth_slice[None]

    with pytest.raises(TypeError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.gather[None]

def test_wrong_lineno():
    with pytest.raises(KeyError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.iline[3000]

    with pytest.raises(KeyError):
        with segyio.open(testdata / 'small.sgy') as f:
            _ = f.xline[2]


def test_open_wrong_inline():
    with segyio.open(testdata / 'small.sgy', "r", 2, strict=False):
        pass


def test_open_wrong_crossline():
    with segyio.open(testdata / 'small.sgy', 'r', 189, 2, strict=False):
        pass


def test_wonky_dimensions():
    with segyio.open(testdata / 'Mx1.sgy'):
        pass
    with segyio.open(testdata / '1xN.sgy'):
        pass
    with segyio.open(testdata / '1x1.sgy'):
        pass


def test_open_fails_unstructured():
    with segyio.open(testdata / 'small.sgy', 'r', 37, strict=False) as f:
        with pytest.raises(ValueError):
            _ = f.iline[10]

        with pytest.raises(ValueError):
            _ = f.iline[:, :]

        with pytest.raises(ValueError):
            _ = f.xline[:, :]

        # operations that don't rely on geometry still works
        assert f.header[2][189] == 1
        assert (list(f.attributes(189)[:]) ==
                [(i // 5) + 1 for i in range(len(f.trace))])


def test_write_with_narrowing(small):
    with segyio.open(small, mode = 'r+') as f:

        with pytest.warns(RuntimeWarning):
            ones = np.ones(len(f.samples), dtype=np.float64)
            f.trace[0] = ones
            assert np.array_equal(f.trace[0], ones)

        with pytest.warns(RuntimeWarning):
            twos = [np.single(2.0) for _ in range(len(f.samples))]
            f.trace[1] = twos
            assert np.array_equal(f.trace[1], twos)

        with pytest.warns(RuntimeWarning):
            first = f.ilines[0]
            shape = f.iline[first].shape
            ones = np.ones(shape, dtype=np.float64)
            f.iline[first] = ones
            assert np.array_equal(f.iline[first], ones)

        with pytest.warns(RuntimeWarning):
            last = f.xlines[-1]
            shape = f.xline[last].shape
            threes = np.ones(shape, dtype=np.float64) * 3
            f.xline[last] = threes
            assert np.array_equal(f.xline[last], threes)


def test_write_with_array_likes(small):
    with segyio.open(small, mode = 'r+') as f:

        with pytest.warns(RuntimeWarning):
            ones = np.ones(3 * len(f.samples), dtype='single')
            # ::3 makes the array non-contiguous
            f.trace[0] = ones[::3]
            assert np.array_equal(f.trace[0], ones[::3])

        with pytest.warns(RuntimeWarning):
            ones = np.ones(len(f.samples), dtype='single')
            f.trace[0] = (1 for _ in range(len(f.samples)))
            assert np.array_equal(f.trace[0], ones)


def test_assign_all_traces(small):
    orig = str(small.dirname + '/small.sgy')
    copy = str(small.dirname + '/copy.sgy')
    shutil.copy(orig, copy)

    with segyio.open(orig) as f:
        traces = f.trace.raw[:] * 2.0

    with segyio.open(copy, 'r+') as f:
        f.trace[:] = traces[:]

    with segyio.open(copy) as f:
        assert np.array_equal(f.trace.raw[:], traces)

    with segyio.open(copy, 'r+') as f:
        traces = f.trace.raw[:]
        f.trace[::2] = [trace * 2.0 for trace in traces[::2]]
        f.trace[1::2] = [trace * 3.0 for trace in traces[1::2]]

    with segyio.open(copy) as f:
        for index, trace in enumerate(f.trace):
            # assuming precision error comes from ibm-iee conversion as file is ibm
            if index % 2 == 0:
                npt.assert_array_almost_equal(
                    trace, traces[index] * 2.0, decimal=5)
            else:
                npt.assert_array_almost_equal(
                    trace, traces[index] * 3.0, decimal=5)


def test_traceaccess_from_array():
    a = np.arange(10, dtype=int)
    b = np.arange(10, dtype=np.int32)
    c = np.arange(10, dtype=np.int64)
    d = np.arange(10, dtype=np.intc)
    with segyio.open(testdata / 'small.sgy') as f:
        _ = f.trace[a[0]]
        _ = f.trace[b[1]]
        _ = f.trace[c[2]]
        _ = f.trace[d[3]]


def test_create_sgy(small):
    orig = str(small.dirname + '/small.sgy')
    fresh = str(small.dirname + '/fresh.sgy')
    with segyio.open(orig) as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines

        with segyio.create(fresh, spec) as dst:
            dst.text[0] = src.text[0]
            dst.bin = src.bin

            # copy all headers
            dst.header = src.header

            for i, srctr in enumerate(src.trace):
                dst.trace[i] = srctr

            dst.trace = src.trace

            # this doesn't work yet, some restructuring is necessary
            # if it turns out to be a desired feature it's rather easy to do
            # for dsth, srch in zip(dst.header, src.header):
            #    dsth = srch

            # for dsttr, srctr in zip(dst.trace, src.trace):
            #    dsttr = srctr

    assert filecmp.cmp(orig, fresh)


def test_ref_getitem(small):
    with segyio.open(small, mode = 'r+') as f:
        with f.trace.ref as ref:
            expected = ref[10].copy()
            x = ref[10]
            y = ref[10]
            x[5] = 0
            # getting a new trace within the same with block should not
            # invalidate other refs
            y = ref[11]
            x[6] = 1.6721
            x[5] = 52
            y[0] = 0
            assert ref[10][5] == 52
            assert ref[10][6] == approx(1.6721)

            assert len(ref.refs) == 2
            z = ref[12]
            z[0] = 0
            assert len(ref.refs) == 3

            x, y = None, None

            ref.flush()
            assert len(ref.refs) == 1

    with segyio.open(small) as f:
        expected[5] = 52
        expected[6] = 1.6721
        npt.assert_array_almost_equal(expected, f.trace[10])


def test_ref_inplace_add_foreach(small):
    with segyio.open(small, mode = 'r+') as f:
        expected = f.trace.raw[:] + 1.617

        with f.trace.ref as ref:
            for x in ref[:]:
                x += 1.617

    with segyio.open(small) as f:
        npt.assert_array_almost_equal(expected, f.trace.raw[:])


def test_ref_preserve_change_except_block(small):
    with segyio.open(small, mode = 'r+') as f:
        expected = f.trace.raw[:]
        expected[10][0] = 0

        with f.trace.ref as ref:
            try:
                for i, x in enumerate(ref[:]):
                    if i == 10: raise RuntimeError
            except RuntimeError:
                x[0] = 0

    with segyio.open(small) as f:
        result = f.trace.raw[:]
        npt.assert_array_almost_equal(expected[10], result[10])
        npt.assert_array_almost_equal(expected, result)


def test_ref_post_loop_var(small):
    with segyio.open(small, mode = 'r+') as f:
        expected = f.trace[-1]
        expected[0] = 1.617

        with f.trace.ref as ref:
            for x in ref[:]: pass

            x[0] = 1.617

    with segyio.open(small) as f:
        npt.assert_array_almost_equal(expected, f.trace[-1])


def test_ref_sliced(small):
    with segyio.open(small, mode = 'r+') as f:
        expected = f.trace.raw[:]
        expected[10:15] += expected[:5]

        with f.trace.ref as ref:
            for x, y in zip(ref[10:15], f.trace[:]):
                np.copyto(x, x + y)

    with segyio.open(small) as f:
        npt.assert_array_almost_equal(expected, f.trace.raw[:])

def test_ref_mixed_for_else(small):
    with segyio.open(small, mode = 'r+') as f:
        samples = len(f.samples)
        zeros = np.zeros(samples, dtype = f.dtype)
        ones = np.ones(samples, dtype = f.dtype)
        expected = f.trace.raw[:]
        expected[7] = zeros
        expected[5:10] += 1.617
        expected[9] = ones

        with f.trace.ref as ref:
            ref[7] = zeros
            for x in ref[5:10]:
                # break won't run, so enter else block
                if len(x) == 0: break
                x += 1.617
            else: # else should preserve last x
                np.copyto(x, ones)

    with segyio.open(small) as f:
        npt.assert_array_almost_equal(expected, f.trace.raw[:])


def test_ref_new_file(small):
    # this is the case the trace.ref feature was designed to support, namely
    # creating a file trace-by-trace based on some transformation of another
    # file, or an operation on multiple, where the trace index itself is
    # uninteresting.

    orig = str(small.dirname + '/small.sgy')
    fresh = str(small.dirname + '/fresh.sgy')
    with segyio.open(orig) as src:

        spec = segyio.tools.metadata(src)
        with segyio.create(fresh, spec) as dst:
            dst.text[0] = src.text[0]
            dst.bin = src.bin

            # copy all headers
            dst.header = src.header

            with dst.trace.ref as ref:
                for x, y in zip(ref[:], src.trace):
                    # x default-inits to 0, so += is essentially copyto()
                    # since traces hasn't been written yet, this has to handle
                    # read miss errors
                    x += y

        with segyio.open(fresh) as dst:
            npt.assert_array_almost_equal(src.trace.raw[:], dst.trace.raw[:])


def test_create_sgy_truncate(small):
    orig = str(small.dirname + '/small.sgy')
    trunc = str(small.dirname + '/text-truncated.sgy')
    with segyio.open(orig) as src:
        spec = segyio.tools.metadata(src)

        # repeat the text header 3 times
        text = src.text[0]
        text = text + text + text

        with segyio.create(trunc, spec) as dst:
            dst.bin = src.bin
            dst.text[0] = text

            dst.header = src.header
            for i, srctr in enumerate(src.trace):
                dst.trace[i] = srctr

            dst.trace = src.trace

    assert filecmp.cmp(orig, trunc)


def test_create_sgy_shorter_traces(small):
    orig = str(small.dirname + '/small.sgy')
    fresh = str(small.dirname + '/small_created_shorter.sgy')
    with segyio.open(orig) as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples[:20]  # reduces samples per trace
        spec.ilines = src.ilines
        spec.xlines = src.xlines

        with segyio.create(fresh, spec) as dst:
            for i, srch in enumerate(src.header):
                dst.header[i] = srch
                d = {TraceField.INLINE_3D: srch[TraceField.INLINE_3D] + 100}
                dst.header[i] = d

            for lineno in dst.ilines:
                dst.iline[lineno] = src.iline[lineno]

            # alternative form using left-hand-side slices
            dst.iline[2:4] = src.iline

            for lineno in dst.xlines:
                dst.xline[lineno] = src.xline[lineno]

        with segyio.open(fresh) as dst:
            assert 20 == len(dst.samples)
            assert [x + 100 for x in src.ilines] == list(dst.ilines)


@pytest.mark.parametrize('endian', ['lsb', 'msb'])
def test_create_from_naught(endian, tmpdir):
    spec = segyio.spec()
    spec.format = 5
    spec.sorting = 1
    spec.samples = range(150)
    spec.ilines = range(1, 11)
    spec.xlines = range(1, 6)
    spec.endian = endian

    with segyio.create(tmpdir / "mk.sgy", spec) as dst:
        tr = np.arange(start=1.000, stop=1.151, step=0.001, dtype=np.single)

        for i in range(len(dst.trace)):
            dst.trace[i] = tr
            tr += 1.000

        for il in spec.ilines:
            dst.header.iline[il] = {TraceField.INLINE_3D: il}

        for xl in spec.xlines:
            dst.header.xline[xl] = {TraceField.CROSSLINE_3D: xl}

        # Set header field 'offset' to 1 in all headers
        dst.header = {TraceField.offset: 1}

    with segyio.open(tmpdir / "mk.sgy", endian = endian) as f:
        assert 1 == approx(f.trace[0][0], abs=1e-4)
        assert 1.001 == approx(f.trace[0][1], abs=1e-4)
        assert 1.149 == approx(f.trace[0][-1], abs=1e-4)
        assert 50.100 == approx(f.trace[-1][100], abs=1e-4)
        assert f.header[0][TraceField.offset] == f.header[1][TraceField.offset]
        assert 1 == f.header[1][TraceField.offset]

        assert f.sorting == TraceSortingFormat.CROSSLINE_SORTING


@pytest.mark.parametrize('endian', ['lsb', 'msb'])
def test_create_from_naught_prestack(endian, tmpdir):
    spec = segyio.spec()
    spec.format = 5
    #spec.sorting not set by test design
    spec.samples = range(7)
    spec.ilines = range(1, 4)
    spec.xlines = range(1, 3)
    spec.offsets = range(1, 6)
    spec.endian = endian

    with segyio.create(tmpdir / "mk-ps.sgy", spec) as dst:
        arr = np.arange(start=0.000,
                        stop=0.007,
                        step=0.001,
                        dtype=np.single)

        arr = np.concatenate([[arr + 0.01], [arr + 0.02]], axis=0)
        lines = [arr + i for i in spec.ilines]
        cube = [(off * 100) + line for line in lines for off in spec.offsets]

        dst.iline[:, :] = cube

        for of in spec.offsets:
            for il in spec.ilines:
                dst.header.iline[il, of] = {TraceField.INLINE_3D: il,
                                            TraceField.offset: of
                                            }
            for xl in spec.xlines:
                dst.header.xline[xl, of] = {TraceField.CROSSLINE_3D: xl}

    with segyio.open(tmpdir / "mk-ps.sgy", endian = endian) as f:
        assert 101.010 == approx(f.trace[0][0], abs=1e-4)
        assert 101.011 == approx(f.trace[0][1], abs=1e-4)
        assert 101.016 == approx(f.trace[0][-1], abs=1e-4)
        assert 503.025 == approx(f.trace[-1][5], abs=1e-4)
        assert f.header[0][TraceField.offset] != f.header[1][TraceField.offset]
        assert 1 == f.header[0][TraceField.offset]
        assert 2 == f.header[1][TraceField.offset]

        for x, y in zip(f.iline[:, :], cube):
            assert list(x.flatten()) == list(y.flatten())

        assert f.sorting == TraceSortingFormat.INLINE_SORTING


@pytest.mark.parametrize('endian', ['lsb', 'msb'])
def test_create_unstructured_hasattrs(endian, tmpdir):
    spec = segyio.spec()
    spec.format = 5
    spec.samples = range(150)
    spec.tracecount = 50
    spec.endian = endian

    with segyio.create(tmpdir / "mk.sgy", spec) as dst:
        # accessing the sorting, inline and crossline attributes should work,
        # but not raise errors
        assert not dst.sorting
        assert not dst.ilines
        assert not dst.xlines
        assert dst.unstructured


@pytest.mark.parametrize('endian', ['lsb', 'msb'])
def test_create_from_naught_unstructured(endian, tmpdir):
    spec = segyio.spec()
    spec.format = 5
    spec.samples = range(150)
    spec.tracecount = 50
    spec.endian = endian

    with segyio.create(tmpdir / "unstructured.sgy", spec) as dst:
        tr = np.array(range(150), dtype = np.single)

        for i in range(len(dst.trace)):
            dst.trace[i] = tr
            tr += 1

        # Set header field 'offset' to 1 in all headers
        dst.header = {TraceField.offset: 1}

    with segyio.open(tmpdir / "unstructured.sgy",
                     ignore_geometry=True,
                     endian=endian) as f:
        assert 1 == approx(f.trace[1][0], abs=1e-4)
        assert 2 == approx(f.trace[1][1], abs=1e-4)
        assert 150 == approx(f.trace[1][-1], abs=1e-4)
        assert 149 == approx(f.trace[-1][100], abs=1e-4)
        assert f.header[10][TraceField.offset] == f.header[25][TraceField.offset]
        assert 1 == f.header[1][TraceField.offset]

def test_create_non_4byte_file(tmpdir):
    ref_trace = np.array([2, 1, 4], np.int8)

    spec = segyio.spec()
    spec.format = 8
    spec.tracecount = 1
    spec.samples = list(range(len(ref_trace)))

    with segyio.create(tmpdir / 'non-4-byte.sgy', spec) as f:
        f.text[0] = ''
        f.bin = {}
        f.header[0] = {}
        f.trace[0] = ref_trace

    with segyio.open(tmpdir / 'non-4-byte.sgy', ignore_geometry = True) as f:
        np.testing.assert_almost_equal(f.trace[0], ref_trace)

def test_create_write_lines(tmpdir):
    mklines(tmpdir / "mklines.sgy")

    with segyio.open(tmpdir / "mklines.sgy") as f:
        assert 1 == approx(f.iline[1][0][0], abs=1e-4)
        assert 2.004 == approx(f.iline[2][0][4], abs=1e-4)
        assert 2.014 == approx(f.iline[2][1][4], abs=1e-4)
        assert 8.043 == approx(f.iline[8][4][3], abs=1e-4)


def test_create_sgy_skip_lines(tmpdir):
    mklines(tmpdir / "lines.sgy")

    with segyio.open(tmpdir / "lines.sgy") as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines[::2]
        spec.xlines = src.xlines[::2]

        with segyio.create(tmpdir / "lines-halved.sgy", spec) as dst:
            # use the inline headers as base
            dst.header.iline = src.header.iline[::2]
            # then update crossline numbers from the crossline headers
            for xl in dst.xlines:
                f = next(src.header.xline[xl])[TraceField.CROSSLINE_3D]
                dst.header.xline[xl] = {TraceField.CROSSLINE_3D: f}

            # but we override the last xline to be 6, not 5
            dst.header.xline[5] = {TraceField.CROSSLINE_3D: 6}
            dst.iline = src.iline[::2]

    with segyio.open(tmpdir / "lines-halved.sgy") as f:
        assert list(f.ilines) == list(spec.ilines)
        assert list(f.xlines) == [1, 3, 6]
        assert 1 == approx(f.iline[1][0][0], abs=1e-4)
        assert 3.004 == approx(f.iline[3][0][4], abs=1e-4)
        assert 3.014 == approx(f.iline[3][1][4], abs=1e-4)
        assert 7.023 == approx(f.iline[7][2][3], abs=1e-4)

def test_create_uint16_samples(tmpdir):
    spec = segyio.spec()
    spec.tracecount = 1
    spec.samples = list(range(np.power(2, 16) - 1))
    spec.format = 1

    with segyio.create(tmpdir / '65k.sgy', spec) as f:
        assert len(f.samples) == 65535
        assert f.bin[segyio.su.hns] == len(f.samples)
        f.trace[0] = spec.samples
        f.header[0] = {
            segyio.su.ns: len(f.samples),
        }

    with segyio.open(tmpdir / '65k.sgy', ignore_geometry = True) as f:
        assert len(f.samples) == 65535
        assert f.bin[segyio.su.hns] == len(f.samples)
        assert f.header[0][segyio.su.ns] == len(f.samples)

def test_create_very_long_traces_65k_samples(tmpdir):
    spec = segyio.spec()
    spec.tracecount = 1
    spec.samples = list(range(np.power(2, 17)))
    spec.format = 1

    try:
        with segyio.create(tmpdir / 'long-traces.sgy', spec) as f:
            assert len(f.samples) == 2 ** 17
            assert f.bin[segyio.su.rev] == 2
            assert f.bin[segyio.su.exthns] == 2 ** 17
            assert f.bin[segyio.su.extnso] == 2 ** 17
            f.trace[0] = spec.samples


        with segyio.open(tmpdir / 'long-traces.sgy', ignore_geometry = True) as f:
            assert len(f.samples) == 2 ** 17
            assert f.bin[segyio.su.rev] == 2
            assert f.bin[segyio.su.exthns] == len(f.samples)

    except RuntimeError as e:
        assert e.args[0] == "uncaught exception: code 6"

def mklines(fname):
    spec = segyio.spec()
    spec.format = 5
    spec.sorting = 2
    spec.samples = range(10)
    spec.ilines = range(1, 11)
    spec.xlines = range(1, 6)

    # create a file with 10 inlines, with values on the form l.0tv where
    # l = line no
    # t = trace number (within line)
    # v = trace value
    # i.e. 2.043 is the value at inline 2's fourth trace's third value
    with segyio.create(fname, spec) as dst:
        ln = np.arange(start=0,
                       stop=0.001 * (5 * 10),
                       step=0.001,
                       dtype=np.single).reshape(5, 10)

        for il in spec.ilines:
            ln += 1

            dst.header.iline[il] = {TraceField.INLINE_3D: il}
            dst.iline[il] = ln

        for xl in spec.xlines:
            dst.header.xline[xl] = {TraceField.CROSSLINE_3D: xl}


def test_create_bad_specs(tmpdir):
    class C:
        pass

    c = C()

    mandatory = [('iline', 189),
                 ('xline', 193),
                 ('samples', [10, 11, 12]),
                 ('format', 1),
                 ('t0', 10.2)]

    for attr, val in mandatory:
        setattr(c, attr, val)
        with pytest.raises(AttributeError):
            with segyio.create(tmpdir / 'foo' + attr, c):
                pass

    c.tracecount = 10
    with segyio.create(tmpdir / 'tracecount', c):
        pass

    del c.tracecount

    c.ilines = [1, 2, 3]
    with pytest.raises(AttributeError):
        with segyio.create(tmpdir / 'ilines', c):
            pass

    c.xlines = [4, 6, 8]
    with pytest.raises(AttributeError):
        with segyio.create(tmpdir / 'xlines', c):
            pass

    c.offsets = [1]
    with segyio.create(tmpdir / 'ok.sgy', c):
        pass


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_segyio_types(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert isinstance(f.sorting, int)
        assert isinstance(f.ext_headers, int)
        assert isinstance(f.tracecount, int)
        assert isinstance(f.samples, np.ndarray)

        from segyio.depth import Depth
        assert isinstance(f.depth_slice, Depth)
        assert isinstance(f.depth_slice[1], np.ndarray)
        assert isinstance(f.depth_slice[1:23], GeneratorType)

        assert isinstance(f.ilines, np.ndarray)
        assert isinstance(f.iline, Line)
        assert isinstance(f.iline[1], np.ndarray)
        assert isinstance(f.iline[1:3], GeneratorType)
        assert isinstance(f.iline[1][0], np.ndarray)
        assert isinstance(f.iline[1][0:2], np.ndarray)
        assert isinstance(float(f.iline[1][0][0]), float)
        assert isinstance(f.iline[1][0][0:3], np.ndarray)

        assert isinstance(f.xlines, np.ndarray)
        assert isinstance(f.xline, Line)
        assert isinstance(f.xline[21], np.ndarray)
        assert isinstance(f.xline[21:23], GeneratorType)
        assert isinstance(f.xline[21][0], np.ndarray)
        assert isinstance(f.xline[21][0:2], np.ndarray)
        assert isinstance(float(f.xline[21][0][0]), float)
        assert isinstance(f.xline[21][0][0:3], np.ndarray)

        assert isinstance(f.header, Header)
        assert isinstance(f.header.iline, HeaderLine)
        assert isinstance(f.header.iline[1], GeneratorType)
        assert isinstance(next(f.header.iline[1]), Field)
        assert isinstance(f.header.xline, HeaderLine)
        assert isinstance(f.header.xline[21], GeneratorType)
        assert isinstance(next(f.header.xline[21]), Field)

        assert isinstance(f.trace, Trace)
        assert isinstance(f.trace[0], np.ndarray)

@pytest.mark.parametrize(('openfn', 'kwargs'), small_segys)
def test_segyio_segy_only_types(openfn, kwargs):
    with openfn(**kwargs) as f:
        assert isinstance(f.bin, Field)
        assert isinstance(f.text, object)  # inner TextHeader instance


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_depth_slice_reading(openfn, kwargs):
    from itertools import islice
    with openfn(**kwargs) as f:
        assert len(f.depth_slice) == len(f.samples)

        for depth_sample in range(len(f.samples))[::5]:
            depth_slice = f.depth_slice[depth_sample]
            assert isinstance(depth_slice, np.ndarray)
            assert depth_slice.shape == (5, 5)

            for x, y in itertools.product(f.ilines, f.xlines):
                i, j = x - f.ilines[0], y - f.xlines[0]
                assert depth_slice[i][j] == approx(f.iline[x][j][depth_sample], abs=1e-6)

        itr = iter(enumerate(f.depth_slice))
        for index, depth_slice in itr:
            assert isinstance(depth_slice, np.ndarray)
            assert depth_slice.shape == (5, 5)

            for x, y in itertools.product(f.ilines, f.xlines):
                i, j = x - f.ilines[0], y - f.xlines[0]
                assert depth_slice[i][j] == approx(f.iline[x][j][index], abs=1e-6)

            next(islice(itr, 5, 5), None)

    with pytest.raises(IndexError):
        _ = f.depth_slice[len(f.samples)]


def test_depth_slice_array_shape():
    with segyio.open(testdata / '1xN.sgy') as f:
        shape = (len(f.fast), len(f.slow))
        assert f.depth_slice.shape == shape


@pytest.mark.parametrize(('openfn', 'kwargs'), smallfiles)
def test_depth_slice_unstructured(openfn, kwargs):
    with openfn(ignore_geometry = True, **kwargs) as f:
        traces = f.trace.raw[:]
        shape = len(f.trace)
        assert f.depth_slice.shape == shape
        np.testing.assert_almost_equal(f.depth_slice[0], traces[:,0])


def test_depth_slice_writing(small):
    from itertools import islice

    buf = np.empty(shape=(5, 5), dtype=np.single)

    def value(x, y):
        return x + (1.0 // 5) * y

    for x, y in itertools.product(range(5), range(5)):
        buf[x][y] = value(x, y)

    with segyio.open(small, "r+") as f:
        f.depth_slice[7] = buf * 3.14  # assign to depth 7
        assert np.allclose(f.depth_slice[7], buf * 3.14)

        f.depth_slice = [buf * i for i in range(len(f.depth_slice))]  # assign to all depths
        itr = iter(enumerate(f.depth_slice))
        for index, depth_slice in itr:
            assert np.allclose(depth_slice, buf * index)
            next(islice(itr, 3, 3), None)

        other = [buf * i * 2 for i in range(len(f.depth_slice))]
        f.depth_slice[::2] = other[::2]

        itr = iter(enumerate(f.depth_slice))
        for index, depth_slice in itr:
            if index % 2 == 0:
                assert np.allclose(depth_slice, buf * index * 2)
            else:
                assert np.allclose(depth_slice, buf * index)


@pytest.mark.parametrize('endian', ['little', 'big'])
def test_no_16bit_overflow_tracecount(endian, tmpdir):
    spec = segyio.spec()
    spec.format = 1
    spec.sorting = 2
    spec.samples = np.arange(501)
    spec.ilines = np.arange(345)
    spec.xlines = np.arange(250)
    spec.endian = endian

    # build a file with more than 65k traces, which would cause a 16bit int to
    # overflow.
    # see https://github.com/Statoil/segyio/issues/235
    ones = np.ones(len(spec.samples), dtype = np.single)

    try:
        with segyio.create(tmpdir / 'foo.sgy', spec) as f:
            assert f.tracecount > 0
            assert f.tracecount > 2**16 - 1
            f.trace[-1] = ones
            f.header[-1] = {
                        segyio.TraceField.INLINE_3D: 10,
                        segyio.TraceField.CROSSLINE_3D: 10,
                        segyio.TraceField.offset: 1,
            }
    except RuntimeError as e:
        assert e.args[0] == "uncaught exception: code 6"

def test_open_2byte_int_format():
    with segyio.open(testdata / 'f3.sgy') as f:
        assert int(f.format)  == 3
        assert len(f.samples) == 75
        assert f.tracecount   == 414
        assert list(f.ilines) == list(range(111, 111 + 23))
        assert list(f.xlines) == list(range(875, 875 + 18))
        assert f.dtype        == np.dtype(np.int16)


def test_readtrace_int16():
    with segyio.open(testdata / 'f3.sgy') as f:
        tr = f.trace[10]
        assert list(tr[20:45: 5]) == [0, -1170, 5198, -2213, -888]
        assert list(tr[40:19:-5]) == [-888, -2213, 5198, -1170, 0]
        assert list(tr[53:50:-1]) == [-2609, -2625, 681]

def test_attributes_shortword_little_endian():
    f3msb = testdata / 'f3.sgy'
    f3lsb = testdata / 'f3-lsb.sgy'
    word = segyio.su.dt
    # this test (in particular) is a pretty good candidate for fuzzing
    with segyio.open(f3msb) as msb:
        with segyio.open(f3lsb, endian = 'little') as lsb:
            msba = msb.attributes(word)
            lsba = lsb.attributes(word)
            npt.assert_array_equal(msba[:], lsba[:])

def test_attributes_header_shortword_equal():
    f3lsb = testdata / 'f3-lsb.sgy'
    word = segyio.su.scalco
    with segyio.open(f3lsb, endian = 'little') as f3:
        assert f3.header[6][word] == f3.attributes(word)[6][0]

@tmpfiles(testdata / 'f3.sgy')
def test_writetrace_int16(tmpdir):
    with segyio.open(tmpdir / 'f3.sgy', mode = 'r+') as f:
        tr = np.asarray(range(len(f.samples)), dtype = np.int16)
        f.trace[0] = tr
        f.trace[1] = tr + 1

    with segyio.open(tmpdir / 'f3.sgy') as f:
        # read both with trace and raw-with-slice, since sliced raw allocates
        # and internal buffer that must match the type
        assert np.array_equal(f.trace[0], tr)
        assert np.array_equal(f.trace[1], tr + 1)
        assert np.array_equal(f.trace.raw[:2], [tr, tr+1])


@tmpfiles(testdata / 'f3.sgy')
def test_write_iline_int16(tmpdir):
    with segyio.open(tmpdir / 'f3.sgy', mode = 'r+') as f:
        shape = f.iline[f.ilines[0]].shape
        il = np.arange(np.prod(shape), dtype = np.int16).reshape(shape)
        f.iline[f.ilines[0]] = il

    with segyio.open(tmpdir / 'f3.sgy') as f:
        assert np.array_equal(f.iline[f.ilines[0]], il)


def test_utf8_filename():
    with segyio.open(testdata / '小文件.sgy') as f:
        assert list(f.ilines) == [1, 2, 3, 4, 5]


@tmpfiles(os.path.join(str(testdata), u'小文件.sgy'))
def test_utf8_filename_pypath(tmpdir):
    with segyio.open(tmpdir / '小文件.sgy') as f:
        assert list(f.ilines) == [1, 2, 3, 4, 5]


def test_interpret_invalid_args():
    with segyio.open(testdata / 'small.sgy', ignore_geometry=True) as f:
        with pytest.raises(ValueError):
            il = [1, 2, 3, 4]
            xl = [20, 21, 22, 23, 24]
            f.interpret(il, xl)

        with pytest.raises(ValueError):
            il = [1, 2, 3, 4, 5]
            xl = [20, 21, 22, 23, 24]
            f.interpret(il, xl, sorting=0)

        with pytest.raises(ValueError):
            il = [1, 2, 3, 4, 4]
            xl = [20, 21, 22, 23, 24]
            f.interpret(il, xl, sorting=0)

def test_group_single_key():
    with segyio.open(testdata / 'shot-gather.sgy', ignore_geometry = True) as f:
        group = f.group(segyio.su.fldr)
        assert len(group) == 4

        assert 2 in group
        assert 4 not in group

        expected_keys = [2, 3, 5, 8]

        for key, shot in zip(expected_keys, group.values()):
            assert key == shot.key

            for header in shot.header:
                assert key == header[segyio.su.fldr]

            for trace in shot.trace:
                assert key == trace[0]

def test_group_key_variations():
    with segyio.open(testdata / 'shot-gather.sgy', ignore_geometry = True) as f:
        group = f.group((segyio.su.fldr, segyio.su.grnofr))
        # both dict and iterator-of-pair accepted
        assert { segyio.su.fldr: 2, segyio.su.grnofr: 1 } in group
        assert ((segyio.su.fldr, 2), (segyio.su.grnofr, 1)) in group
        # not order sensitive
        assert ((segyio.su.grnofr, 1), (segyio.su.fldr, 2)) in group
        assert { segyio.su.fldr: 4, segyio.su.grnofr: 1 } not in group


def test_group_multi_key_corret_index():
    with segyio.open(testdata / 'shot-gather.sgy', ignore_geometry = True) as f:
        group = f.group((segyio.su.fldr, segyio.su.grnofr))
        assert len(group) == 8

        expected_keys = [
            ((segyio.su.fldr, 2), (segyio.su.grnofr, 1)),
            ((segyio.su.fldr, 2), (segyio.su.grnofr, 2)),
            ((segyio.su.fldr, 3), (segyio.su.grnofr, 1)),
            ((segyio.su.fldr, 3), (segyio.su.grnofr, 2)),
            ((segyio.su.fldr, 5), (segyio.su.grnofr, 1)),
            ((segyio.su.fldr, 5), (segyio.su.grnofr, 2)),
            ((segyio.su.fldr, 8), (segyio.su.grnofr, 1)),
            ((segyio.su.fldr, 8), (segyio.su.grnofr, 2)),
        ]

        expected_keys = [frozenset(k) for k in expected_keys]

        indices = [
            # fldr = 2, grnofr = 1
            [0, 2, 4, 6, 8],
            # fldr = 2, grnofr = 2
            [1, 3, 5, 7, 9],

            # fldr = 3, grnofr = 1
            [10, 12, 14, 16, 18, 20],
            # fldr = 3, grnofr = 2
            [11, 13, 15, 17, 19, 21],

            # fldr = 5, grnofr = 1
            [22, 24, 26, 28, 30, 32, 34],
            # fldr = 5, grnofr = 2
            [23, 25, 27, 29, 31, 33],

            # fldr = 8, grnofr = 1
            [35, 37, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59],
            # fldr = 8, grnofr = 2
            [36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60],
        ]

        # this test checks that every individual group maps to the known index
        # it's supposed to model. it would be more elegant to represent the
        # indices as a key -> index dict, but that makes verifying that every
        # key/index pair is covered uglier
        assert len(expected_keys) == len(indices)

        for key, index, shot in zip(expected_keys, indices, group.values()):
            assert index == shot.index
            assert key == shot.key

def test_specific_group_sort():
    with segyio.open(testdata / 'shot-gather.sgy', ignore_geometry = True) as f:
        group = f.group((segyio.su.fldr, segyio.su.grnofr))

        left_key = ((segyio.su.fldr, 2), (segyio.su.grnofr, 1))
        left_shot = group[left_key]
        left_shot.sort([segyio.su.tracf, segyio.su.grnofr, segyio.su.fldr])
        # tracf is descending, so sorting by it ascending should reverse order
        assert left_shot.index == [8, 6, 4, 2, 0]

        right_key = ((segyio.su.fldr, 2), (segyio.su.grnofr, 2))
        right_shot = group[right_key]
        # the next shot is untouched
        assert right_shot.index == [1, 3, 5, 7, 9]

def test_all_group_sort():
    with segyio.open(testdata / 'shot-gather.sgy', ignore_geometry = True) as f:
        group = f.group((segyio.su.fldr, segyio.su.grnofr))
        group.sort([segyio.su.tracf, segyio.su.grnofr, segyio.su.fldr])

        left_key = ((segyio.su.fldr, 2), (segyio.su.grnofr, 1))
        left_shot = group[left_key]
        assert left_shot.index == [8, 6, 4, 2, 0]

        right_key = ((segyio.su.fldr, 2), (segyio.su.grnofr, 2))
        right_shot = group[right_key]
        assert right_shot.index == [9, 7, 5, 3, 1]

def test_groups_gather_equivalence():
    with segyio.open(testdata / 'small.sgy') as f:
        groups = f.group((segyio.su.iline, segyio.su.xline))
        key = ((segyio.su.iline, 1), (segyio.su.xline, 21))
        group = groups[key]
        assert len(group.index) == 1

        # stack gathers in a list-of-traces, but there should only be 1 in this
        # group
        from_group = np.stack([tr.copy() for tr in groups[key].trace])
        assert from_group.shape == (1, 50)

        from_group = from_group[0]
        from_gather = f.gather[1, 21]

        # group[(il, xl)] == gather[il, xl]
        npt.assert_array_equal(from_group, from_gather)

def test_delay_scalar_applied():
    with segyio.open(testdata / 'delay-scalar.sgy') as f:
        assert f.samples[0] == 1000


def test_open_with_custom_mapping():
    parse_error = "error parsing trace header mappings"
    with pytest.raises(ValueError, match=parse_error):
        segyio.open(testdata / 'mapping-unparsable.sgy')

    with pytest.raises(ValueError, match=parse_error):
        segyio.open(testdata / 'mapping-invalid-root.sgy')

    with pytest.raises(ValueError, match=parse_error):
        segyio.open(testdata / 'mapping-invalid-attribute.sgy')

    with pytest.raises(ValueError, match=parse_error):
        segyio.open(testdata / 'mapping-invalid-value.sgy')

    with pytest.raises(RuntimeError, match=r"unable to gather basic metadata*"):
        segyio.open(testdata / 'mapping-empty.sgy')

    ilines = list(range(1, 4))
    xlines = list(range(20, 22))
    offsets = list(range(1, 2))

    with segyio.open(testdata / 'mapping-default.sgy') as f:
        assert list(f.ilines) == ilines
        assert list(f.xlines) == xlines
        assert list(f.offsets) == offsets

    with segyio.open(testdata / 'mapping-multiple-stanzas.sgy') as f:
        assert list(f.ilines) == ilines
        assert list(f.xlines) == xlines
        assert list(f.offsets) == offsets

    with segyio.open(testdata / 'mapping-shifted.sgy') as f:
        assert list(f.ilines) == ilines
        assert list(f.xlines) == xlines
        assert list(f.offsets) == offsets

    with segyio.open(testdata / 'mapping-minimal.sgy') as f:
        assert list(f.ilines) == ilines
        assert list(f.xlines) == xlines
        assert list(f.offsets) == offsets

    with pytest.raises(ValueError, match=r"invalid field datatype*"):
        segyio.open(testdata / 'mapping-unsupported-type.sgy')

    with segyio.open(testdata / 'mapping-mixed-order.sgy') as f:
        assert f.tracefield.names() == ["SEG00000", "SEG00001", "PRIVATE1", "PRIVATE2"]

    with segyio.open(testdata / 'mapping-no-extension1.sgy') as f:
        assert f.tracefield.names() == ["SEG00000", "PRIVATE1"]

    with segyio.open(testdata / 'mapping-encoding-ext1.sgy') as f:
        assert f.tracefield.names() == ["SEG00000", "SEG00001"]

    with pytest.raises(KeyError, match=r"traceheader mapping for .* not found"):
        segyio.open(testdata / 'mapping-encoding-private.sgy')


def test_tracefields():
    with segyio.open(testdata / 'trace-header-extensions.sgy') as f:
        assert len(f.tracefield) == 3
        for x in f.tracefield:
            assert isinstance(x, segyio.trace.HeaderLayoutEntries)

        assert len(f.tracefield[1]) == 2
        for x in f.tracefield[1]:
            assert isinstance(x, segyio.trace.FieldLayoutEntry)

        assert f.tracefield.names() == ['SEG00000', 'SEG00001', 'PRIVATE1']
        assert f.tracefield[0].names() == f.tracefield.SEG00000.names()

        assert f.tracefield.SEG00000.names() == [
            'linetrc', 'offset', 'delay', 'dt', 'iline', 'xline', 'tm_scal', 'header_name'
        ]
        assert f.tracefield.SEG00001.names() == [
            'linetrc', 'header_name'
        ]
        assert f.tracefield.PRIVATE1.names() == [
            'precious', 'precious2', 'precious3', 'precious4', 'header_name'
        ]
        with pytest.raises(AttributeError):
            f.tracefield.Missing

        assert f.tracefield.PRIVATE1.precious3.offset() == 5
        with pytest.raises(AttributeError):
            f.tracefield.PRIVATE1.PRECIOUS3

        assert f.tracefield.SEG00000.xline.offset() == 193
        assert f.tracefield[0][5].name() == 'xline'
        assert f.tracefield.SEG00001.linetrc.type() == "linetrc8"
        assert f.tracefield.SEG00000.linetrc.use_only_if_non_zero() is False
        assert f.tracefield.SEG00001.linetrc.use_only_if_non_zero() is True

        str_rep = "linetrc (offset=1, type=linetrc8, use_only_if_non_zero=True)"
        assert str(f.tracefield.SEG00001.linetrc) == str_rep

def test_trace_header_extension1():
    with segyio.open(testdata / 'trace-header-extension1.sgy', "r") as f:
        assert f.tracefield.names() == ['SEG00000', 'SEG00001']
        names = f.tracefield.SEG00001.names()
        assert len(names) == 26
        assert 'header_name' in names

        assert f.tracefield.SEG00000.linetrc[0] == 0x11111111
        assert f.traceheader[0].SEG00001.linetrc == 0x22222222_22222221
        assert f.tracefield.SEG00001.linetrc[0] == 0x22222222_22222221

        assert np.array_equal(
            f.tracefield.SEG00001.linetrc[0, 4],
            [0x22222222_22222221, 0x22222222_22222225]
        )


def test_trace_header_extensions():
    with segyio.open(testdata / 'trace-header-extensions.sgy') as f:
        h0 = f.header[0]
        h1 = f.header[1]
        assert h0[TraceField.CROSSLINE_3D] == 20
        assert h1[TraceField.CROSSLINE_3D] == 21

        assert np.array_equal(f.attributes(TraceField.INLINE_3D), [1, 1])
        assert np.array_equal(f.attributes(TraceField.CROSSLINE_3D), [20, 21])


@pytest.mark.parametrize(
    "endianness, filename",
    [
        ("big", "mapping-all-types.sgy"),
        ("little", "mapping-all-types-lsb.sgy"),
    ]
)
@tmpfiles(testdata / 'mapping-all-types.sgy')
@tmpfiles(testdata / 'mapping-all-types-lsb.sgy')
def test_read_write_all_custom_mapping_types(tmpdir, endianness, filename):
    from dataclasses import dataclass
    import numbers

    @dataclass
    class HeaderTest:
        name: str
        opened: numbers.Real
        changed: numbers.Real

    trace0_proprietary_cases = [
        HeaderTest("int2", 1, -101),
        HeaderTest("int4", 2, -102),
        HeaderTest("int8", 3, -103),
        HeaderTest("uint2", 4, 104),
        HeaderTest("uint4", 5, 105),
        HeaderTest("uint8", 6, 106),
        HeaderTest("ibmfp", 7.0, -107.0),
        HeaderTest("ieee32", 8.0, -108.0),
        HeaderTest("ieee64", 9.0, -109.0),
        HeaderTest("linetrc", 10, 110),
        HeaderTest("reeltrc", 11, 111),
        HeaderTest("linetrc8", 12, 112),
        HeaderTest("reeltrc8", 13, 113),
        # scaled values are not scaled. As in segyio 1.x, raw value is returned
        HeaderTest("coor4", 14, -114),
        HeaderTest("elev4", 15, -115),
        HeaderTest("time2", 16, -116),
        HeaderTest("spnum4", 17, -117),
        # scale 6 type is inconsistent and not supported well yet
        HeaderTest("scale6_mant", 18, -118),
        HeaderTest("scale6_exp", 19, -119),
        HeaderTest("header_name", b"TYPES\x00\x00\x00", b"TYPES   "),
    ]

    trace1_proprietary_cases = [
        HeaderTest("int2", -1001, 10001),
        HeaderTest("int4", -1002, 2000002),
        HeaderTest("int8", -1003, 3000000003),
        HeaderTest("uint2", 1004, 4004),
        HeaderTest("uint4", 1005, 5000005),
        HeaderTest("uint8", 1006, 60000000006),
        HeaderTest("ibmfp", -1007.0, 707.125),
        HeaderTest("ieee32", -1008.0, 808.75),
        HeaderTest("ieee64", -1009.0, 909.5),
        HeaderTest("linetrc", 1010, 10010),
        HeaderTest("reeltrc", 1011, 10011),
        HeaderTest("linetrc8", 1012, 1000012),
        HeaderTest("reeltrc8", 1013, 1000013),
        HeaderTest("coor4", -1014, 100014),
        HeaderTest("elev4", -1015, 100015),
        HeaderTest("time2", -1016, 10016),
        HeaderTest("spnum4", -1017, 100017),
        # scale 6 type is inconsistent and not supported well yet
        HeaderTest("scale6_mant", -1018, 1018),
        HeaderTest("scale6_exp", -1019, 1019),
        HeaderTest("header_name", b"TYPES\x00\x00\x00", b"TYPES   "),
    ]

    headers = {
        0: trace0_proprietary_cases,
        1: trace1_proprietary_cases,
    }

    traceheader_index = 1

    with segyio.open(tmpdir / filename, mode='r+', endian=endianness) as f:
        types = f.tracefield[traceheader_index]
        for trace_index, header_cases in headers.items():
            for case in header_cases:
                offset = types.__getattr__(case.name).offset()

                assert f.traceheader[trace_index][traceheader_index][offset] == case.opened
                f.traceheader[trace_index][traceheader_index][offset] = case.changed
                assert f.traceheader[trace_index][traceheader_index][offset] == case.changed

        with pytest.raises(ValueError, match=r"Value out of range*"):
            offset = types.int2.offset()
            f.traceheader[trace_index][traceheader_index][offset] = 50000

        for i, case in enumerate(trace0_proprietary_cases):
            if (case.name == "scale6_mant") or (case.name == "scale6_exp"):
                # scale6 is not properly supported yet
                continue

            expected = np.array(
                [trace0_proprietary_cases[i].changed, trace1_proprietary_cases[i].changed])

            attrs = types.__getattr__(case.name)[:]
            np.testing.assert_array_equal(attrs, expected)


def test_revision_2_1_file_creation(tmpdir):
    orig = str(testdata / 'trace-header-extensions.sgy')
    fresh = tmpdir / '2.1-copied.sgy'
    with segyio.open(orig) as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines
        spec.ext_headers = src.ext_headers
        spec.traceheader_count = src.traceheader_count
        spec.tracecount = src.tracecount

        # find index of stanza name from src.stanza.names() that starts with SEG:Layout, case insensitive
        layout_index = -1
        for i, stanza_name in enumerate (src.stanza.names()):
            if stanza_name.lower().startswith('seg:layout'):
                layout_index = i
        layout = src.stanza[layout_index]

        with segyio.create(fresh, spec, layout_xml=layout) as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            dst.bin = src.bin
            dst.trace = src.trace
            dst.traceheader = src.traceheader

    assert filecmp.cmp(orig, fresh)
