import numpy as np
import pytest
from pytest import approx

from . import tmpfiles
from . import testdata

import segyio
from segyio import BinField
from segyio import TraceField
from segyio import TraceSortingFormat
from segyio import SegySampleFormat


@tmpfiles(testdata / 'small.sgy')
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


@tmpfiles(testdata / 'small.sgy')
def test_dt_no_fallback(tmpdir):
    dt_us = 6000
    with segyio.open(tmpdir / 'small.sgy', "r+") as f:
        f.bin[BinField.Interval] = dt_us
        f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = dt_us
        f.flush()
        assert segyio.dt(f) == approx(dt_us)


def test_sample_indexes():
    with segyio.open(testdata / 'small.sgy') as f:
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
    with segyio.open(testdata / 'small.sgy') as f:
        segyio.tools.wrap(f.text[0])
        segyio.tools.wrap(f.text[0], 90)

def test_wrap_only_stringifies_content():
    """
    in python2, str(text) gives the content of the text header as a byte array,
    as a string really is a bytearray, whereas in python3 str(bytearray) gives
    the string "bytearray(b'C 1...')".

    https://github.com/equinor/segyio/issues/444
    """
    with segyio.open(testdata / 'small.sgy') as f:
        s = segyio.tools.wrap(f.text[0])
        assert s.startswith('C 1')

    with segyio.open(testdata / 'small.sgy') as f:
        s = segyio.tools.wrap(f.text[0].decode(errors = 'ignore'))
        assert s.startswith('C 1')


def test_values_text_header_creation():
    lines = {i + 1: chr(64 + i) * 76 for i in range(40)}
    text_header = segyio.create_text_header(lines)

    for line_no in range(0, 40):
        line = text_header[line_no * 80: (line_no + 1) * 80]
        assert line == "C{0:>2} {1:76}".format(line_no + 1, chr(64 + line_no) * 76)


def test_native():
    with open(str(testdata / 'small.sgy'), 'rb') as f:
        with segyio.open(testdata / 'small.sgy') as sgy:
            f.read(3600 + 240)
            filetr = f.read(4 * len(sgy.samples))
            segytr = sgy.trace[0]

            filetr = np.frombuffer(filetr, dtype=np.single)
            assert not np.array_equal(segytr, filetr)
            assert np.array_equal(segytr, segyio.tools.native(filetr))


def test_cube_filename():
    with segyio.open(testdata / 'small.sgy') as f:
        c1 = segyio.tools.cube(f)
        c2 = segyio.tools.cube(testdata / 'small.sgy')
        assert np.all(c1 == c2)


def test_cube_identity():
    with segyio.open(testdata / 'small.sgy') as f:
        x = segyio.tools.collect(f.trace[:])
        x = x.reshape((len(f.ilines), len(f.xlines), len(f.samples)))
        assert np.all(x == segyio.tools.cube(f))


def test_cube_identity_prestack():
    with segyio.open(testdata / 'small-ps.sgy') as f:
        dims = (len(f.ilines), len(f.xlines), len(f.offsets), len(f.samples))
        x = segyio.tools.collect(f.trace[:]).reshape(dims)
        assert np.all(x == segyio.tools.cube(f))


def test_unstructured_rotation():
    with pytest.raises(ValueError):
        with segyio.open(testdata / 'small.sgy', ignore_geometry=True) as f:
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
        src = testdata / 'name-small.sgy'.replace('name', name)
        print(src)

        with segyio.open(src) as f:
            assert angle == approx(rotation(f), abs=1e-3)
            assert angle == approx(rotation(f, line='fast'), abs=1e-3)
            assert angle == approx(rotation(f, line='iline'), abs=1e-3)
            assert right == approx(rotation(f, line='slow'), abs=1e-3)
            assert right == approx(rotation(f, line='xline'), abs=1e-3)

def test_rotation_mmap():
    angle = 0.000
    from segyio.tools import rotation
    with segyio.open(testdata / 'normal-small.sgy') as f:
        f.mmap()
        assert 0.000 == approx(rotation(f, line = 'fast')[0], abs = 1e-3)
        assert 1.571 == approx(rotation(f, line = 'slow')[0], abs = 1e-3)

def test_rotation_lsb():
    from segyio.tools import rotation
    with segyio.open(testdata / 'f3.sgy') as msb:
        with segyio.open(testdata / 'f3-lsb.sgy', endian = 'little') as lsb:
            assert rotation(msb, line = 'fast') == rotation(lsb, line = 'fast')
            assert rotation(msb, line = 'slow') == rotation(lsb, line = 'slow')

def test_metadata():
    spec = segyio.spec()
    spec.ilines = [1, 2, 3, 4, 5]
    spec.xlines = [20, 21, 22, 23, 24]
    spec.samples = list(range(0, 200, 4))
    spec.sorting = 2
    spec.format = 1

    smallspec = segyio.tools.metadata(testdata / 'small.sgy')

    assert np.array_equal(spec.ilines, smallspec.ilines)
    assert np.array_equal(spec.xlines, smallspec.xlines)
    assert np.array_equal(spec.offsets, smallspec.offsets)
    assert np.array_equal(spec.samples, smallspec.samples)
    assert spec.sorting == smallspec.sorting
    assert spec.format == int(smallspec.format)


@tmpfiles(testdata / 'small.sgy')
def test_resample_none(tmpdir):
    old = list(range(0, 200, 4))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f)  # essentially a no-op
        assert np.array_equal(old, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(old, f.samples)


@tmpfiles(testdata / 'small.sgy')
def test_resample_all(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(12, 112, 2))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, rate=2, delay=12)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)


@tmpfiles(testdata / 'small.sgy')
def test_resample_rate(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(12, 212, 4))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, delay=12)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)


@tmpfiles(testdata / 'small.sgy')
def test_resample_delay(tmpdir):
    old = list(range(0, 200, 4))
    new = list(range(0, 100, 2))

    with segyio.open(tmpdir / 'small.sgy', 'r+') as f:
        assert np.array_equal(old, f.samples)
        segyio.tools.resample(f, rate=2000, micro=True)
        assert np.array_equal(new, f.samples)

    with segyio.open(tmpdir / 'small.sgy') as f:
        assert np.array_equal(new, f.samples)


def createfromany(path, data, il=189, xl=193, sample_format=1, dt=4000, delrt=0):
    segyio.tools.from_array(path, data, il, xl, sample_format, dt, delrt)


def createfrom2d(path, data, il=189, xl=193, sample_format=1, dt=4000, delrt=0):
    segyio.tools.from_array2D(path, data, il, xl, sample_format, dt, delrt)


def createfrom3d(path, data, il=189, xl=193, sample_format=1, dt=4000, delrt=0):
    segyio.tools.from_array3D(path, data, il, xl, sample_format, dt, delrt)


def createfrom4d(path, data, il=189, xl=193, sample_format=1, dt=4000, delrt=0):
    segyio.tools.from_array4D(path, data, il, xl, sample_format, dt, delrt)


@pytest.mark.parametrize("create", [createfrom2d, createfromany])
def testfrom_array_2D(tmpdir, create):
    fresh = str(tmpdir / 'fresh.sgy')
    data = np.arange(250, dtype=np.float32).reshape((10,25))
    dt, delrt = 4000, 0
    create(fresh, data, dt=dt, delrt=delrt)

    with segyio.open(fresh, 'r') as f:
        assert int(f.format)     == SegySampleFormat.IBM_FLOAT_4_BYTE
        assert int(f.sorting)    == TraceSortingFormat.INLINE_SORTING
        assert len(f.samples)    == 25
        assert int(f.tracecount) == 10

        assert np.array_equal(f.iline[1], data)
        assert list(f.ilines)  == [1]
        assert list(f.xlines)  == list(range(1, 11))
        assert list(f.offsets) == list(range(1, 2))
        assert list(f.samples) == list(
            (np.arange(len(f.samples)) * dt/1000) + delrt)

        ilines  = np.ones(10)
        xlines  = range(1, 11)
        offsets = np.ones(10)
        assert list(f.attributes(TraceField.INLINE_3D))    == list(ilines)
        assert list(f.attributes(TraceField.CROSSLINE_3D)) == list(xlines)
        assert list(f.attributes(TraceField.offset))       == list(offsets)
        assert list(f.attributes(TraceField.TraceNumber))  == list(range(10))
        assert list(f.attributes(TraceField.CDP_TRACE))    == list(range(10))
        assert list(f.attributes(TraceField.TRACE_SAMPLE_INTERVAL)) == list(
            4000 * np.ones(10))
        assert list(f.attributes(TraceField.TRACE_SAMPLE_COUNT)) == list(
            25 * np.ones(10))
        assert list(f.attributes(TraceField.DelayRecordingTime)) == list(
            np.zeros(10)
        )


@pytest.mark.parametrize("create", [createfrom3d, createfromany])
def test_from_array3D(tmpdir, create):
    fresh = str(tmpdir / 'fresh.sgy')

    with segyio.open( testdata / 'small.sgy') as f:
        cube = segyio.cube(f)
        dt, delrt = 4000, 0
        create(fresh, cube, dt=dt, delrt=delrt)
        with segyio.open(fresh) as g:
            assert int(g.format)  == SegySampleFormat.IBM_FLOAT_4_BYTE
            assert int(g.sorting) == TraceSortingFormat.INLINE_SORTING
            assert len(g.samples) == len(f.samples)
            assert g.tracecount   == f.tracecount

            assert np.array_equal(f.trace, g.trace)
            assert list(g.ilines) == list(range(1, 6))
            assert list(g.xlines) == list(range(1, 6))
            assert list(f.offsets) == list(range(1, 2))
            assert list(f.samples) == list(
                (np.arange(len(f.samples)) * dt / 1000) + delrt)

            xlines = np.tile(np.arange(1, 6), 5)
            ilines = np.repeat(np.arange(1, 6), 5)
            offsets = np.ones(25)
            assert list(g.attributes(TraceField.INLINE_3D))    == list(ilines)
            assert list(g.attributes(TraceField.CROSSLINE_3D)) == list(xlines)
            assert list(g.attributes(TraceField.offset))       == list(offsets)
            assert list(g.attributes(TraceField.TraceNumber))  == list(range(25))
            assert list(g.attributes(TraceField.CDP_TRACE))    == list(range(25))
            assert list(g.attributes(TraceField.TRACE_SAMPLE_INTERVAL)) == list(
                4000 * np.ones(25))
            assert list(g.attributes(TraceField.TRACE_SAMPLE_COUNT)) == list(
                50 * np.ones(25))

            assert g.bin[BinField.SortingCode] == 2


@pytest.mark.parametrize("create", [createfrom4d, createfromany])
def test_from_array4D(tmpdir, create):
    fresh = str(tmpdir / 'fresh.sgy')
    data = np.repeat(np.arange(24, dtype=np.float32), 10).reshape((4,3,2,10))
    dt, delrt = 4000, 0
    create(fresh, data, dt=dt, delrt=delrt)

    with segyio.open(fresh) as f:
        assert int(f.format)     == SegySampleFormat.IBM_FLOAT_4_BYTE
        assert int(f.sorting)    == TraceSortingFormat.INLINE_SORTING
        assert len(f.samples)    == 10
        assert int(f.tracecount) == 24

        assert list(f.ilines)  == list(range(1, 5))
        assert list(f.xlines)  == list(range(1, 4))
        assert list(f.offsets) == list(range(1, 3))
        assert list(f.samples) == list(
            (np.arange(len(f.samples)) * dt / 1000) + delrt)

        iline4 = f.iline[4, 1]
        assert list(iline4[0, :]) == list(18*np.ones(10))
        assert list(iline4[2, :]) == list(22*np.ones(10))

        xline2 = f.xline[2, 2]
        assert list(xline2[1, :]) == list(9 * np.ones(10))
        assert list(xline2[3, :]) == list(21 * np.ones(10))

        ilines  = np.repeat(np.arange(1, 5), 6)
        xlines = np.repeat(np.tile(np.arange(1, 4), 4), 2)
        offsets = np.tile(np.arange(1, 3), 12)
        assert list(f.attributes(TraceField.INLINE_3D))    == list(ilines)
        assert list(f.attributes(TraceField.CROSSLINE_3D)) == list(xlines)
        assert list(f.attributes(TraceField.offset))       == list(offsets)


@pytest.mark.parametrize("create", [createfrom2d,
                                    createfrom3d,
                                    createfrom4d,
                                    createfromany])
def test_create_from_array_invalid_args(tmpdir, create):
    fresh = str(tmpdir / 'fresh.sgy')

    data = np.arange(100, dtype=np.float32)
    with pytest.raises(ValueError):
        create(fresh, data)

    data = "rubbish-input"
    with pytest.raises(ValueError):
        create(fresh, data)
