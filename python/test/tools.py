import numpy as np
import pytest
from pytest import approx

from test import tmpfiles

import segyio
from segyio import BinField
from segyio import TraceField


@tmpfiles("test-data/small.sgy")
def test_dt_fallback(tmpdir):
    with segyio.open(tmpdir / 'small.sgy', "r+") as f:
        # Both zero
        f.bin[BinField.Interval] = 0
        f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = 0
        f.flush()
        fallback_dt = 4
        assert segyio.dt(f, fallback_dt) == approx(fallback_dt)

        # dt in bin header different from first trace
        f.bin[BinField.Interval] = 6000
        f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = 1000
        f.flush()
        fallback_dt = 4
        assert segyio.dt(f, fallback_dt) == approx(fallback_dt)


@tmpfiles("test-data/small.sgy")
def test_dt_no_fallback(tmpdir):
    dt_us = 6000
    with segyio.open(tmpdir / 'small.sgy', "r+") as f:
        f.bin[BinField.Interval] = dt_us
        f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = dt_us
        f.flush()
        assert segyio.dt(f) == approx(dt_us)


def test_sample_indexes():
    with segyio.open("test-data/small.sgy") as f:
        indexes = segyio.sample_indexes(f)
        step = 4000.0

        assert indexes == [t * step for t in range(len(f.samples))]

        indexes = segyio.sample_indexes(f, t0=1.5)
        assert indexes == [1.5 + t * step for t in range(len(f.samples))]

        indexes = segyio.sample_indexes(f, t0=1.5, dt_override=3.21)
        assert indexes == [1.5 + t * 3.21 for t in range(len(f.samples))]


def test_empty_text_header_creation():
    text_header = segyio.create_text_header({})

    for line_no in range(0, 40):
        line = text_header[line_no * 80: (line_no + 1) * 80]
        assert line == "C{0:>2} {1:76}".format(line_no + 1, "")


def test_wrap():
    with segyio.open("test-data/small.sgy") as f:
        segyio.tools.wrap(f.text[0])
        segyio.tools.wrap(f.text[0], 90)


def test_values_text_header_creation():
    lines = {i + 1: chr(64 + i) * 76 for i in range(40)}
    text_header = segyio.create_text_header(lines)

    for line_no in range(0, 40):
        line = text_header[line_no * 80: (line_no + 1) * 80]
        assert line == "C{0:>2} {1:76}".format(line_no + 1, chr(64 + line_no) * 76)


def test_native():
    with open("test-data/small.sgy", 'rb') as f, segyio.open("test-data/small.sgy") as sgy:
        f.read(3600 + 240)
        filetr = f.read(4 * len(sgy.samples))
        segytr = sgy.trace[0]

        filetr = np.frombuffer(filetr, dtype=np.single)
        assert not np.array_equal(segytr, filetr)
        assert np.array_equal(segytr, segyio.tools.native(filetr))


def test_cube_filename():
    with segyio.open("test-data/small.sgy") as f:
        c1 = segyio.tools.cube(f)
        c2 = segyio.tools.cube("test-data/small.sgy")
        assert np.all(c1 == c2)


def test_cube_identity():
    with segyio.open("test-data/small.sgy") as f:
        x = segyio.tools.collect(f.trace[:])
        x = x.reshape((len(f.ilines), len(f.xlines), len(f.samples)))
        assert np.all(x == segyio.tools.cube(f))


def test_cube_identity_prestack():
    with segyio.open("test-data/small-ps.sgy") as f:
        dims = (len(f.ilines), len(f.xlines), len(f.offsets), len(f.samples))
        x = segyio.tools.collect(f.trace[:]).reshape(dims)
        assert np.all(x == segyio.tools.cube(f))


def test_unstructured_rotation():
    with pytest.raises(ValueError):
        with segyio.open("test-data/small.sgy", ignore_geometry=True) as f:
            segyio.tools.rotation(f)


def test_rotation():
    names = ['normal', 'acute', 'right', 'obtuse',
             'straight', 'reflex', 'left', 'inv-acute']
    angles = [0.000, 0.785, 1.571, 2.356,
              3.142, 3.927, 4.712, 5.498]
    rights = [1.571, 2.356, 3.142, 3.927,
              4.712, 5.498, 0.000, 0.785]

    def rotation(x, **kwargs):
        return segyio.tools.rotation(x, **kwargs)[0]

    for name, angle, right in zip(names, angles, rights):
        src = "test-data/small.sgy".replace('/', '/' + name + '-')

        with segyio.open(src) as f:
            assert angle == approx(rotation(f), abs=1e-3)
            assert angle == approx(rotation(f, line='fast'), abs=1e-3)
            assert angle == approx(rotation(f, line='iline'), abs=1e-3)
            assert right == approx(rotation(f, line='slow'), abs=1e-3)
            assert right == approx(rotation(f, line='xline'), abs=1e-3)


def test_metadata():
    spec = segyio.spec()
    spec.ilines = [1, 2, 3, 4, 5]
    spec.xlines = [20, 21, 22, 23, 24]
    spec.samples = list(range(0, 200, 4))
    spec.sorting = 2
    spec.format = 1

    smallspec = segyio.tools.metadata("test-data/small.sgy")

    assert np.array_equal(spec.ilines, smallspec.ilines)
    assert np.array_equal(spec.xlines, smallspec.xlines)
    assert np.array_equal(spec.offsets, smallspec.offsets)
    assert np.array_equal(spec.samples, smallspec.samples)
    assert spec.sorting == smallspec.sorting
    assert spec.format == int(smallspec.format)


@tmpfiles("test-data/small.sgy")
def test_resample_none(tmpdir):
    old = list(range(0, 200, 4))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f)  # essentially a no-op
        assert np.array_equal(old, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(old, f.samples)


@tmpfiles("test-data/small.sgy")
def test_resample_all(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(12, 112, 2))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, rate=2, delay=12)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)


@tmpfiles("test-data/small.sgy")
def test_resample_rate(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(12, 212, 4))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, delay=12)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)


@tmpfiles("test-data/small.sgy")
def test_resample_delay(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(0, 100, 2))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, rate=2000, micro=True)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)

