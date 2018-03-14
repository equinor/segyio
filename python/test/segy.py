from __future__ import absolute_import

try:
    from future_builtins import zip, map
except ImportError:
    pass

from types import GeneratorType

import itertools
import filecmp
import shutil
import numpy as np
import pytest
from pytest import approx

from test import tmpfiles

import segyio
from segyio import TraceField, BinField
from segyio._field import Field
from segyio._line import Line
from segyio._header import Header
from segyio._trace import Trace


def test_inline_4():
    with segyio.open("test-data/small.sgy") as f:
        sample_count = len(f.samples)
        assert 50 == sample_count

        data = f.iline[4]

        assert 4.2, approx(data[0, 0], abs=1e-6)
        # middle sample
        assert 4.20024, approx(data[0, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.20049, approx(data[0, -1], abs=1e-6)

        # middle xline
        middle_line = 2
        # first sample
        assert 4.22, approx(data[middle_line, 0], abs=1e-5)
        # middle sample
        assert 4.22024, approx(data[middle_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.22049, approx(data[middle_line, -1], abs=1e-6)

        # last xline
        last_line = (len(f.xlines) - 1)
        # first sample
        assert 4.24, approx(data[last_line, 0], abs=1e-5)
        # middle sample
        assert 4.24024, approx(data[last_line, sample_count // 2 - 1], abs=1e-6)
        # last sample
        assert 4.24049, approx(data[last_line, sample_count - 1], abs=1e-6)


def test_xline_22():
    with segyio.open("test-data/small.sgy") as f:
        data = f.xline[22]

        size = len(f.samples)

        # first iline
        # first sample
        assert 1.22, approx(data[0, 0], abs=1e-5)
        # middle sample
        assert 1.22024, approx(data[0, size // 2 - 1], abs=1e-6)
        # last sample
        assert 1.22049, approx(data[0, size - 1], abs=1e-6)

        # middle iline
        middle_line = 2
        # first sample
        assert 3.22, approx(data[middle_line, 0], abs=1e-5)
        # middle sample
        assert 3.22024, approx(data[middle_line, size // 2 - 1], abs=1e-6)
        # last sample
        assert 3.22049, approx(data[middle_line, size - 1], abs=1e-6)

        # last iline
        last_line = len(f.ilines) - 1
        # first sample
        assert 5.22, approx(data[last_line, 0], abs=1e-5)
        # middle sample
        assert 5.22024, approx(data[last_line, size // 2 - 1], abs=1e-6)
        # last sample
        assert 5.22049, approx(data[last_line, size - 1], abs=1e-6)


def test_iline_slicing():
    with segyio.open("test-data/small.sgy") as f:
        assert len(f.ilines) == sum(1 for _ in f.iline)
        assert len(f.ilines) == sum(1 for _ in f.iline[1:6])
        assert len(f.ilines) == sum(1 for _ in f.iline[5:0:-1])
        assert len(f.ilines) // 2 == sum(1 for _ in f.iline[0::2])
        assert len(f.ilines) == sum(1 for _ in f.iline[1:])
        assert 3 == sum(1 for _ in f.iline[::2])
        assert 0 == sum(1 for _ in f.iline[12:24])
        assert 3 == sum(1 for _ in f.iline[:4])
        assert 2 == sum(1 for _ in f.iline[2:6:2])


def test_xline_slicing():
    with segyio.open("test-data/small.sgy") as f:
        assert len(f.xlines) == sum(1 for _ in f.xline)
        assert len(f.xlines) == sum(1 for _ in f.xline[20:25])
        assert len(f.xlines) == sum(1 for _ in f.xline[25:19:-1])
        assert 3 == sum(1 for _ in f.xline[0::2])
        assert 3 == sum(1 for _ in f.xline[::2])
        assert len(f.xlines) == sum(1 for _ in f.xline[20:])
        assert 0 == sum(1 for _ in f.xline[12:18])
        assert 5 == sum(1 for _ in f.xline[:25])
        assert 2 == sum(1 for _ in f.xline[:25:3])


def test_open_transposed_lines():
    with segyio.open("test-data/small.sgy") as f:
        il = f.ilines
        xl = f.xlines

    with segyio.open("test-data/small.sgy", "r", segyio.TraceField.CROSSLINE_3D, segyio.TraceField.INLINE_3D) as f:
        assert list(il) == list(f.xlines)
        assert list(xl) == list(f.ilines)


def test_file_info():
    with segyio.open("test-data/small.sgy") as f:
        assert 2 == f.sorting
        assert 1 == f.offsets
        assert 1 == int(f.format)

        xlines = list(range(20, 25))
        ilines = list(range(1, 6))
        assert xlines == list(f.xlines)
        assert ilines == list(f.ilines)
        assert 25 == f.tracecount
        assert len(f.trace) == f.tracecount
        assert 50 == len(f.samples)


def test_open_nostrict():
    with segyio.open("test-data/small.sgy", strict=False):
        pass


def test_open_ignore_geometry():
    with segyio.open("test-data/small.sgy", ignore_geometry=True) as f:
        with pytest.raises(ValueError):
            _ = f.iline[0]


def test_traces_slicing():
    with segyio.open("test-data/small.sgy") as f:
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
        buf = None
        for i, trace in enumerate(f.trace[0:6:2, buf]):
            assert np.array_equal(trace, traces[i])


def test_traces_offset():
    with segyio.open("test-data/small-ps.sgy") as f:
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
    with segyio.open("test-data/small-ps.sgy") as f:
        il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
        assert f.header[0][il] == f.header[1][il]
        assert f.header[1][il] == f.header[2][il]

        assert f.header[0][xl] == f.header[1][xl]
        assert not f.header[1][xl] == f.header[2][xl]


def test_header_dict_methods():
    with segyio.open("test-data/small.sgy") as f:
        assert 89 == len(list(f.header[0].keys()))
        assert 89 == len(list(f.header[1].values()))
        assert 89 == len(list(f.header[2].items()))
        assert 89 == len(list(f.header[3]))
        assert 0 not in f.header[0]
        assert 1 in f.header[0]
        assert segyio.su.cdpx in f.header[0]
        iter(f.header[0])

        assert 30 == len(f.bin.keys())
        assert 30 == len(list(f.bin.values()))
        assert 30 == len(list(f.bin.items()))
        assert 30 == len(f.bin)
        iter(f.bin)


@tmpfiles("test-data/small-ps.sgy")
def test_headers_line_offset(tmpdir):
    il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
    with segyio.open(tmpdir / "small-ps.sgy", "r+") as f:
        f.header.iline[1, 2] = {il: 11}
        f.header.iline[1, 2] = {xl: 13}

    with segyio.open(tmpdir / "small-ps.sgy", strict=False) as f:
        assert f.header[0][il] == 1
        assert f.header[1][il] == 11
        assert f.header[2][il] == 1

        assert f.header[0][xl] == 1
        assert f.header[1][xl] == 13
        assert f.header[2][xl] == 2


def test_attributes():
    with segyio.open("test-data/small.sgy") as f:
        il = TraceField.INLINE_3D
        xl = TraceField.CROSSLINE_3D

        assert 1 == f.attributes(il)[0]
        assert 20 == f.attributes(xl)[0]

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
    with segyio.open("test-data/small-ps.sgy") as f:
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
    with segyio.open("test-data/small-ps.sgy") as f:
        for i, ln in enumerate(f.iline[:, 1], 1):
            assert i + 100.01 == approx(ln[0][0], abs=1e-4)
            assert i + 100.02 == approx(ln[1][0], abs=1e-4)
            assert i + 100.03 == approx(ln[2][0], abs=1e-4)

            assert i + 100.01001 == approx(ln[0][1], abs=1e-4)
            assert i + 100.01002 == approx(ln[0][2], abs=1e-4)
            assert i + 100.02001 == approx(ln[1][1], abs=1e-4)


def test_iline_slice_fixed_line():
    with segyio.open("test-data/small-ps.sgy") as f:
        for i, ln in enumerate(f.iline[1, :], 1):
            off = i * 100
            assert off + 1.01 == approx(ln[0][0], abs=1e-4)
            assert off + 1.02 == approx(ln[1][0], abs=1e-4)
            assert off + 1.03 == approx(ln[2][0], abs=1e-4)

            assert off + 1.01001 == approx(ln[0][1], abs=1e-4)
            assert off + 1.01002 == approx(ln[0][2], abs=1e-4)
            assert off + 1.02001 == approx(ln[1][1], abs=1e-4)


def test_iline_slice_all_offsets():
    with segyio.open("test-data/small-ps.sgy") as f:
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
    with segyio.open("test-data/small-ps.sgy") as f:
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

        for g, line in zip(f.gather[1:3, 3, 1], f.iline[1:3]):
            assert 10 == len(g)
            assert (10,) == g.shape
            assert np.array_equal(line[2], g)

        for g, line in zip(f.gather[1:3, 3, :], f.iline[1:3]):
            assert 2 == len(g)
            assert (2, 10) == g.shape
            assert np.array_equal(line[2], g[0])

        for g, line in zip(f.gather[1, 1:3, 1], f.xline[1:3]):
            assert 10 == len(g)
            assert (10,) == g.shape
            assert np.array_equal(line[0], g)

        for g, line in zip(f.gather[1, 1:3, :], f.xline[1:3]):
            assert 2 == len(g)
            assert (2, 10) == g.shape
            assert np.array_equal(line[0], g[0])


def test_line_generators():
    with segyio.open("test-data/small.sgy") as f:
        for _ in f.iline:
            pass

        for _ in f.xline:
            pass


def test_fast_slow_dimensions():
    with segyio.open("test-data/small.sgy", 'r') as f:
        for iline, fline in zip(f.iline, f.fast):
            assert np.array_equal(iline, fline)

        for xline, sline in zip(f.xline, f.slow):
            assert np.array_equal(xline, sline)


def test_traces_raw():
    with segyio.open("test-data/small.sgy") as f:
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


def test_read_header():
    with segyio.open("test-data/small.sgy") as f:
        assert 1 == f.header[0][189]
        assert 1 == f.header[1][TraceField.INLINE_3D]
        assert 1 == f.header[1][segyio.su.iline]
        assert 5 == f.header[-1][segyio.su.iline]
        assert 5 == f.header[24][segyio.su.iline]
        assert dict(f.header[-1]) == dict(f.header[24])

        with pytest.raises(IndexError):
            _ = f.header[30]

        with pytest.raises(IndexError):
            _ = f.header[-30]

        with pytest.raises(IndexError):
            _ = f.header[0][188]  # between byte offsets

        with pytest.raises(IndexError):
            _ = f.header[0][-1]

        with pytest.raises(IndexError):
            _ = f.header[0][700]


@tmpfiles("test-data/small.sgy")
def test_write_header(tmpdir):
    with segyio.open(tmpdir / "small.sgy", "r+") as f:
        # assign to a field in a header, write immediately
        f.header[0][189] = 42
        f.flush()

        assert 42 == f.header[0][189]
        assert 1 == f.header[1][189]

        # accessing non-existing offsets raises exceptions
        with pytest.raises(IndexError):
            f.header[0][188] = 1  # between byte offsets

        with pytest.raises(IndexError):
            f.header[0][-1] = 1

        with pytest.raises(IndexError):
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

        # for-each support
        for _ in f.header:
            pass

        # copy a header
        f.header[2] = f.header[1]
        f.flush()

        # don't use this interface in production code, it's only for testing
        # i.e. don't access buf of treat it as a list
        # assertEqual(list(f.header[2].buf), list(f.header[1].buf))


@tmpfiles("test-data/small.sgy")
def test_write_binary(tmpdir):
    with segyio.open(tmpdir / "small.sgy", "r+") as f:
        f.bin[3213] = 5
        f.flush()

        assert 5 == f.bin[3213]

        # accessing non-existing offsets raises exceptions
        with pytest.raises(IndexError):
            _ = f.bin[0]

        with pytest.raises(IndexError):
            _ = f.bin[50000]

        with pytest.raises(IndexError):
            _ = f.bin[3214]

        d = {BinField.Traces: 43,
             BinField.SweepFrequencyStart: 11}

        # assign multiple fields at once by using a dict
        f.bin = d

        f.flush()
        assert 43 == f.bin[segyio.su.ntrpr]
        assert 11 == f.bin[segyio.su.hsfs]

        d = {BinField.Traces: 45,
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
        assert d == f.bin[BinField.Traces, BinField.SweepFrequencyStart]

        # copy a header
        f.bin = f.bin


def test_fopen_error():
    # non-existent file
    with pytest.raises(IOError):
        segyio.open("no_dir/no_file")

    # non-existant mode
    with pytest.raises(ValueError):
        segyio.open("test-data/small.sgy", "foo")

    with pytest.raises(ValueError):
        segyio.open("test-data/small.sgy", "r+b+toolong")


def test_wrong_lineno():
    with pytest.raises(KeyError):
        with segyio.open("test-data/small.sgy") as f:
            _ = f.iline[3000]

    with pytest.raises(KeyError):
        with segyio.open("test-data/small.sgy") as f:
            _ = f.xline[2]


def test_open_wrong_inline():
    with pytest.raises(IndexError):
        with segyio.open("test-data/small.sgy", "r", 2):
            pass

    with segyio.open("test-data/small.sgy", "r", 2, strict=False):
        pass


def test_open_wrong_crossline():
    with pytest.raises(IndexError):
        with segyio.open("test-data/small.sgy", "r", 189, 2):
            pass

    with segyio.open("test-data/small.sgy", "r", 189, 2, strict=False):
        pass


def test_wonky_dimensions():
    with segyio.open("test-data/Mx1.sgy"):
        pass
    with segyio.open("test-data/1xN.sgy"):
        pass
    with segyio.open("test-data/1x1.sgy"):
        pass


def test_open_fails_unstructured():
    with segyio.open("test-data/small.sgy", "r", 37, strict=False) as f:
        with pytest.raises(ValueError):
            _ = f.iline[10]

        with pytest.raises(ValueError):
            _ = f.iline[:, :]

        with pytest.raises(ValueError):
            _ = f.xline[:, :]

        with pytest.raises(ValueError):
            _ = f.depth_slice[2]

        # operations that don't rely on geometry still works
        assert f.header[2][189] == 1
        assert (list(f.attributes(189)[:]) ==
                [(i // 5) + 1 for i in range(len(f.trace))])


@tmpfiles("test-data/small.sgy")
def test_assign_all_traces(tmpdir):
    shutil.copy(tmpdir / 'small.sgy', tmpdir / 'copy_small.sgy')

    with segyio.open(tmpdir / 'small.sgy') as f:
        traces = f.trace.raw[:] * 2.0

    with segyio.open(tmpdir / 'copy_small.sgy', 'r+') as f:
        f.trace[:] = traces[:]

    with segyio.open(tmpdir / 'copy_small.sgy') as f:
        assert np.array_equal(f.trace.raw[:], traces)


def test_traceaccess_from_array():
    a = np.arange(10, dtype=np.int)
    b = np.arange(10, dtype=np.int32)
    c = np.arange(10, dtype=np.int64)
    d = np.arange(10, dtype=np.intc)
    with segyio.open("test-data/small.sgy") as f:
        _ = f.trace[a[0]]
        _ = f.trace[b[1]]
        _ = f.trace[c[2]]
        _ = f.trace[d[3]]


@tmpfiles("test-data/small.sgy")
def test_create_sgy(tmpdir):
    with segyio.open(tmpdir / "small.sgy") as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines

        with segyio.create(tmpdir / "small_created.sgy", spec) as dst:
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

    assert filecmp.cmp(tmpdir / "small.sgy", tmpdir / "small_created.sgy")


@tmpfiles("test-data/small.sgy")
def test_create_sgy_truncate(tmpdir):
    with segyio.open(tmpdir / "small.sgy") as src:
        spec = segyio.tools.metadata(src)

        # repeat the text header 3 times
        text = src.text[0]
        text = text + text + text

        with segyio.create(tmpdir / "text-truncated.sgy", spec) as dst:
            dst.bin = src.bin
            dst.text[0] = text

            dst.header = src.header
            for i, srctr in enumerate(src.trace):
                dst.trace[i] = srctr

            dst.trace = src.trace

    assert filecmp.cmp(tmpdir / "small.sgy", tmpdir / "text-truncated.sgy")


@tmpfiles("test-data/small.sgy")
def test_create_sgy_shorter_traces(tmpdir):
    with segyio.open(tmpdir / "small.sgy") as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples[:20]  # reduces samples per trace
        spec.ilines = src.ilines
        spec.xlines = src.xlines

        with segyio.create(tmpdir / "small_created_shorter.sgy", spec) as dst:
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

        with segyio.open(tmpdir / "small_created_shorter.sgy") as dst:
            assert 20 == len(dst.samples)
            assert [x + 100 for x in src.ilines] == list(dst.ilines)


def test_create_from_naught(tmpdir):
    spec = segyio.spec()
    spec.format = 5
    spec.sorting = 2
    spec.samples = range(150)
    spec.ilines = range(1, 11)
    spec.xlines = range(1, 6)

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

    with segyio.open(tmpdir / "mk.sgy") as f:
        assert 1, approx(f.trace[0][0], abs=1e-4)
        assert 1.001, approx(f.trace[0][1], abs=1e-4)
        assert 1.149, approx(f.trace[0][-1], abs=1e-4)
        assert 50.100, approx(f.trace[-1][100], abs=1e-4)
        assert f.header[0][TraceField.offset] == f.header[1][TraceField.offset]
        assert 1 == f.header[1][TraceField.offset]


def test_create_from_naught_prestack(tmpdir):
    spec = segyio.spec()
    spec.format = 5
    spec.sorting = 2
    spec.samples = range(7)
    spec.ilines = range(1, 4)
    spec.xlines = range(1, 3)
    spec.offsets = range(1, 6)

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

    with segyio.open(tmpdir / "mk-ps.sgy") as f:
        assert 101.010, approx(f.trace[0][0], abs=1e-4)
        assert 101.011, approx(f.trace[0][1], abs=1e-4)
        assert 101.016, approx(f.trace[0][-1], abs=1e-4)
        assert 503.025, approx(f.trace[-1][5], abs=1e-4)
        assert f.header[0][TraceField.offset] != f.header[1][TraceField.offset]
        assert 1 == f.header[0][TraceField.offset]
        assert 2 == f.header[1][TraceField.offset]

        for x, y in zip(f.iline[:, :], cube):
            assert list(x.flatten()) == list(y.flatten())


def test_create_write_lines(tmpdir):
    mklines(tmpdir / "mklines.sgy")

    with segyio.open(tmpdir / "mklines.sgy") as f:
        assert 1, approx(f.iline[1][0][0], abs=1e-4)
        assert 2.004, approx(f.iline[2][0][4], abs=1e-4)
        assert 2.014, approx(f.iline[2][1][4], abs=1e-4)
        assert 8.043, approx(f.iline[8][4][3], abs=1e-4)


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


def test_create_bad_specs():
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
            with segyio.create("foo", c):
                pass

    c.tracecount = 10
    with segyio.create("foo", c):
        pass

    del c.tracecount

    c.ilines = [1, 2, 3]
    with pytest.raises(AttributeError):
        with segyio.create("foo", c):
            pass

    c.xlines = [4, 6, 8]
    with pytest.raises(AttributeError):
        with segyio.create("foo", c):
            pass

    c.offsets = [1]
    with pytest.raises(AttributeError):
        with segyio.create("foo", c):
            pass

    c.sorting = 2
    with segyio.create("foo", c):
        pass


def test_segyio_types():
    with segyio.open("test-data/small.sgy") as f:
        assert isinstance(f.sorting, int)
        assert isinstance(f.ext_headers, int)
        assert isinstance(f.tracecount, int)
        assert isinstance(f.samples, np.ndarray)

        assert isinstance(f.depth_slice, Line)
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
        assert isinstance(f.header.iline, Line)
        assert isinstance(f.header.iline[1], GeneratorType)
        assert isinstance(next(f.header.iline[1]), Field)
        assert isinstance(f.header.xline, Line)
        assert isinstance(f.header.xline[21], GeneratorType)
        assert isinstance(next(f.header.xline[21]), Field)

        assert isinstance(f.trace, Trace)
        assert isinstance(f.trace[0], np.ndarray)

        assert isinstance(f.bin, Field)
        assert isinstance(f.text, object)  # inner TextHeader instance


def test_depth_slice_reading():
    with segyio.open("test-data/small.sgy") as f:
        assert len(f.depth_slice) == len(f.samples)

        for depth_sample in range(len(f.samples)):
            depth_slice = f.depth_slice[depth_sample]
            assert isinstance(depth_slice, np.ndarray)
            assert depth_slice.shape == (5, 5)

            for x, y in itertools.product(f.ilines, f.xlines):
                i, j = x - f.ilines[0], y - f.xlines[0]
                assert depth_slice[i][j] == approx(f.iline[x][j][depth_sample], abs=1e-6)

        for index, depth_slice in enumerate(f.depth_slice):
            assert isinstance(depth_slice, np.ndarray)
            assert depth_slice.shape == (5, 5)

            for x, y in itertools.product(f.ilines, f.xlines):
                i, j = x - f.ilines[0], y - f.xlines[0]
                assert depth_slice[i][j] == approx(f.iline[x][j][index], abs=1e-6)

    with pytest.raises(KeyError):
        _ = f.depth_slice[len(f.samples)]


@tmpfiles("test-data/small.sgy")
def test_depth_slice_writing(tmpdir):

    buf = np.empty(shape=(5, 5), dtype=np.single)

    def value(x, y):
        return x + (1.0 // 5) * y

    for x, y in itertools.product(range(5), range(5)):
        buf[x][y] = value(x, y)

    with segyio.open(tmpdir / "small.sgy", "r+") as f:
        f.depth_slice[7] = buf * 3.14  # assign to depth 7
        assert np.allclose(f.depth_slice[7], buf * 3.14)

        f.depth_slice = [buf * i for i in range(len(f.depth_slice))]  # assign to all depths
        for index, depth_slice in enumerate(f.depth_slice):
            assert np.allclose(depth_slice, buf * index)

def test_no_16bit_overflow_tracecount(tmpdir):
    spec = segyio.spec()
    spec.format = 1
    spec.sorting = 2
    spec.samples = np.arange(501)
    spec.ilines = np.arange(345)
    spec.xlines = np.arange(250)

    # build a file with more than 65k traces, which would cause a 16bit int to
    # overflow.
    # see https://github.com/Statoil/segyio/issues/235
    ones = np.ones(len(spec.samples), dtype = np.single)
    with segyio.create(tmpdir / 'foo.sgy', spec) as f:
        assert f.tracecount > 0
        assert f.tracecount > 2**16 - 1
        for i in range(f.tracecount):
            f.trace[i] = ones
            f.header[i] = {
                    segyio.TraceField.INLINE_3D: i,
                    segyio.TraceField.CROSSLINE_3D: i,
                    segyio.TraceField.offset: 1,
            }
