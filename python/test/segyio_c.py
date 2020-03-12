from __future__ import absolute_import

import os

import numpy
import pytest
from pytest import approx

from . import tmpfiles
from . import testdata

import segyio
import segyio._segyio as _segyio


def test_binary_header_size():
    assert 400 == _segyio.binsize()


def test_textheader_size():
    assert 3200 == _segyio.textsize()


def test_open_non_existing_file():
    with pytest.raises(IOError):
        _ = _segyio.segyiofd("non-existing", "r", 0)


def test_close_non_existing_file():
    with pytest.raises(TypeError):
        _segyio.segyiofd.close(None)


def test_open_and_close_file():
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0)
    f.close()


def test_open_flush_and_close_file():
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0)
    f.flush()
    f.close()

    with pytest.raises(IOError):
        f.flush()


def test_read_text_header_mmap():
    test_read_text_header(True)


def test_read_text_header(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0)
    if mmap:
        f.mmap()

    lines = {
        1: "DATE: 2016-09-19",
        2: "AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE",
        3: "Written by libsegyio (python)",
        11: "TRACE HEADER POSITION:",
        12: "  INLINE BYTES 189-193    | OFFSET BYTES 037-041",
        13: "  CROSSLINE BYTES 193-197 |",
        15: "END EBCDIC HEADER"
    }

    rows = segyio.create_text_header(lines)
    rows = bytearray(rows, 'ascii')  # mutable array of bytes
    rows[-1] = 128  # \x80
    actual_text_header = bytes(rows)

    assert f.gettext(0) == actual_text_header

    with pytest.raises(Exception):
        _segyio.read_texthdr(None, 0)

    f.close()


@tmpfiles(testdata / 'small.sgy')
def test_write_text_header_mmap(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    write_text_header(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_write_text_header(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    write_text_header(f, False)


def write_text_header(f, mmap):
    if mmap:
        f.mmap()

    f.puttext(0, "")

    textheader = f.gettext(0)
    assert textheader == bytearray(3200)

    f.puttext(0, "yolo" * 800)

    textheader = f.gettext(0)
    textheader = textheader.decode('ascii')  # Because in Python 3.5 bytes are not comparable to strings
    assert textheader == "yolo" * 800

    f.close()


def get_instance_segyiofd(tmpdir,
                          file_name="small.sgy",
                          mode="r+",
                          samples = None,
                          tracecount = None,
                          ):
    path = str(tmpdir)
    f = os.path.join(path, file_name)

    if samples is not None:
        return _segyio.segyiofd(f, mode, 0).segymake(samples, tracecount)
    else:
        return _segyio.segyiofd(f, mode, 0).segyopen()


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_binary_header(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    read_and_write_binary_header(f, False)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_binary_header_mmap(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    read_and_write_binary_header(f, True)


def read_and_write_binary_header(f, mmap):
    if mmap:
        f.mmap()

    binary_header = f.getbin()

    with pytest.raises(ValueError):
        f.putbin("Buffer too small")

    f.putbin(binary_header)
    f.close()


def test_read_binary_header_fields_mmap():
    test_read_binary_header_fields(True)


def test_read_binary_header_fields(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0)
    if mmap:
        f.mmap()

    binary_header = f.getbin()

    with pytest.raises(TypeError):
        _ = _segyio.getfield([], 0)

    with pytest.raises(KeyError):
        _ = _segyio.getfield(binary_header, -1)

    assert _segyio.getfield(binary_header, 3225) == 1
    assert _segyio.getfield(binary_header, 3221) == 50

    f.close()


def test_line_metrics_mmap():
    test_line_metrics(True)


def test_line_metrics(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193
    metrics = f.metrics()
    metrics.update(f.cube_metrics(ilb, xlb))
    f.close()

    sorting = metrics['sorting']
    trace_count = metrics['tracecount']
    inline_count = metrics['iline_count']
    crossline_count = metrics['xline_count']
    offset_count = metrics['offset_count']

    metrics = _segyio.line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

    assert metrics['xline_length'] == 5
    assert metrics['xline_stride'] == 5
    assert metrics['iline_length'] == 5
    assert metrics['iline_stride'] == 1

    # (sorting, trace_count, inline_count, crossline_count, offset_count)
    metrics = _segyio.line_metrics(1, 15, 3, 5, 1)

    assert metrics['xline_length'] == 3
    assert metrics['xline_stride'] == 1
    assert metrics['iline_length'] == 5
    assert metrics['iline_stride'] == 3

    metrics = _segyio.line_metrics(2, 15, 3, 5, 1)

    assert metrics['xline_length'] == 3
    assert metrics['xline_stride'] == 5
    assert metrics['iline_length'] == 5
    assert metrics['iline_stride'] == 1


def test_metrics_mmap():
        test_metrics(True)


def test_metrics(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193

    with pytest.raises(IndexError):
        metrics = f.metrics()
        metrics.update(f.cube_metrics(ilb + 1, xlb))

    metrics = f.metrics()
    metrics.update(f.cube_metrics(ilb, xlb))

    assert metrics['trace0'] == _segyio.textsize() + _segyio.binsize()
    assert metrics['samplecount'] == 50
    assert metrics['format'] == 1
    assert metrics['trace_bsize'] == 200
    assert metrics['sorting'] == 2 # inline sorting = 2, crossline sorting = 1
    assert metrics['tracecount'] == 25
    assert metrics['offset_count'] == 1
    assert metrics['iline_count'] == 5
    assert metrics['xline_count'] == 5

    f.close()


def test_indices_mmap():
    test_indices(True)


def test_indices(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193
    metrics = f.metrics()
    dmy = numpy.zeros(2, dtype=numpy.intc)

    dummy_metrics = {'xline_count':   2,
                     'iline_count':   2,
                     'offset_count':  1}

    with pytest.raises(TypeError):
        f.indices("-", dmy, dmy, dmy)

    with pytest.raises(TypeError):
        f.indices(dummy_metrics, 1, dmy, dmy)

    with pytest.raises(TypeError):
        f.indices(dummy_metrics, dmy, 2, dmy)

    with pytest.raises(TypeError):
        f.indices(dummy_metrics, dmy, dmy, 2)

    one = numpy.zeros(1, dtype=numpy.intc)
    two = numpy.zeros(2, dtype=numpy.intc)
    off = numpy.zeros(1, dtype=numpy.intc)
    with pytest.raises(ValueError):
        f.indices(dummy_metrics, one, two, off)

    with pytest.raises(ValueError):
        f.indices(dummy_metrics, two, one, off)

    metrics.update(f.cube_metrics(ilb, xlb))

    # Happy Path
    iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
    xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
    offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
    f.indices(metrics, iline_indexes, xline_indexes, offsets)

    assert [1, 2, 3, 4, 5] == list(iline_indexes)
    assert [20, 21, 22, 23, 24] == list(xline_indexes)
    assert [1] == list(offsets)

    f.close()


def test_fread_trace0_mmap():
    test_fread_trace0(True)


def test_fread_trace0(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193

    metrics = f.metrics()
    metrics.update(f.cube_metrics(ilb, xlb))

    sorting = metrics['sorting']
    trace_count = metrics['tracecount']
    inline_count = metrics['iline_count']
    crossline_count = metrics['xline_count']
    offset_count = metrics['offset_count']

    line_metrics = _segyio.line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

    iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
    xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
    offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
    f.indices(metrics, iline_indexes, xline_indexes, offsets)

    with pytest.raises(KeyError):
        _segyio.fread_trace0(0, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")

    with pytest.raises(KeyError):
        _segyio.fread_trace0(2, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")

    value = _segyio.fread_trace0(1, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")
    assert value == 0

    value = _segyio.fread_trace0(2, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")
    assert value == 5

    value = _segyio.fread_trace0(21, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")
    assert value == 1

    value = _segyio.fread_trace0(22, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")
    assert value == 2

    f.close()


def test_get_and_putfield():
    hdr = bytearray(_segyio.thsize())

    with pytest.raises(BufferError):
        _segyio.getfield(".", 0)

    with pytest.raises(TypeError):
        _segyio.getfield([], 0)

    with pytest.raises(TypeError):
        _segyio.putfield({}, 0, 1)

    with pytest.raises(KeyError):
        _segyio.getfield(hdr, 0)

    with pytest.raises(KeyError):
        _segyio.putfield(hdr, 0, 1)

    _segyio.putfield(hdr, 1, 127)
    _segyio.putfield(hdr, 5, 67)
    _segyio.putfield(hdr, 9, 19)

    assert _segyio.getfield(hdr, 1) == 127
    assert _segyio.getfield(hdr, 5) == 67
    assert _segyio.getfield(hdr, 9) == 19


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_traceheader_mmap(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    read_and_write_traceheader(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_traceheader(tmpdir):
    f = get_instance_segyiofd(tmpdir)
    read_and_write_traceheader(f, False)


def read_and_write_traceheader(f, mmap):
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193

    def mkempty():
        return bytearray(_segyio.thsize())

    with pytest.raises(TypeError):
        f.getth("+")

    with pytest.raises(TypeError):
        f.getth(0, None)

    trace_header = f.getth(0, mkempty())

    assert _segyio.getfield(trace_header, ilb) == 1
    assert _segyio.getfield(trace_header, xlb) == 20

    trace_header = f.getth(1, mkempty())

    assert _segyio.getfield(trace_header, ilb) == 1
    assert _segyio.getfield(trace_header, xlb) == 21

    _segyio.putfield(trace_header, ilb, 99)
    _segyio.putfield(trace_header, xlb, 42)

    f.putth(0, trace_header)

    trace_header = f.getth(0, mkempty())

    assert _segyio.getfield(trace_header, ilb) == 99
    assert _segyio.getfield(trace_header, xlb) == 42

    f.close()


def test_read_traceheaders():
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()
    read_traceheaders_forall(f, False)
    read_traceheaders_forall(f, True)

    read_traceheaders_foreach(f, False)
    read_traceheaders_foreach(f, True)

    f.close()


def read_traceheaders_forall(f, mmap):
    if mmap:
        f.mmap()
    start, stop, step = 20, -1, -5
    indices = range(start, stop, step)
    attrs = numpy.empty(len(indices), dtype=numpy.intc)
    field = segyio.TraceField.INLINE_3D

    with pytest.raises(ValueError):
        f.field_forall(attrs, start, stop, 0, field)

    buf_handle = f.field_forall(attrs, start, stop, step, field)
    numpy.testing.assert_array_equal(attrs, [5, 4, 3, 2, 1])
    assert buf_handle is attrs


def read_traceheaders_foreach(f, mmap):
    if mmap:
        f.mmap()

    indices = numpy.asarray([7, 4, 1, 18, 20], dtype=numpy.intc)
    attrs = numpy.empty(len(indices), dtype=numpy.intc)
    field = segyio.TraceField.CROSSLINE_3D

    with pytest.raises(ValueError):
        f.field_foreach(numpy.empty(1, dtype=numpy.intc), indices, field)

    buf_handle = f.field_foreach(attrs, indices, field)
    numpy.testing.assert_array_equal(attrs, [22, 24, 21, 23, 20])
    assert buf_handle is attrs


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_trace_mmap(tmpdir):
    f = get_instance_segyiofd(tmpdir,
        "trace-wrt.sgy",
        "w+",
        samples = 25,
        tracecount = 100,
    )
    read_and_write_trace(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_trace(tmpdir):
    f = get_instance_segyiofd(tmpdir,
            "trace-wrt.sgy",
            "w+",
            samples = 25,
            tracecount = 100,
    )
    read_and_write_trace(f, False)


def read_and_write_trace(f, mmap):
    if mmap:
        f.mmap()

    buf = numpy.ones(25, dtype=numpy.single)
    buf[11] = 3.1415
    f.puttr(0, buf)
    buf[:] = 42.0
    f.puttr(1, buf)

    f.flush()

    buf = numpy.zeros(25, dtype=numpy.single)

    f.gettr(buf, 0, 1, 1, 0, 25, 1, 25)

    assert buf[10] == approx(1.0, abs=1e-4)
    assert buf[11] == approx(3.1415, abs=1e-4)

    f.gettr(buf, 1, 1, 1, 0, 25, 1, 25)

    assert sum(buf) == approx(42.0 * 25, abs=1e-4)

    f.close()


def test_read_line():
    f, metrics, iline_idx, xline_idx = read_small(False)
    read_line(f, metrics, iline_idx, xline_idx)


def test_read_line_mmap():
    f, metrics, iline_idx, xline_idx = read_small(True)
    read_line(f, metrics, iline_idx, xline_idx)


def read_line(f, metrics, iline_idx, xline_idx):
    samples = metrics['samplecount']
    xline_stride = metrics['xline_stride']
    iline_stride = metrics['iline_stride']
    offsets = metrics['offset_count']

    xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, offsets, xline_idx, "crossline")
    iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, offsets, iline_idx, "inline")

    buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

    f.getline(xline_trace0, len(iline_idx), xline_stride, offsets, buf)
    assert sum(sum(buf)) == approx(800.061169624, abs=1e-6)

    f.getline(iline_trace0, len(xline_idx), iline_stride, offsets, buf)
    assert sum(sum(buf)) == approx(305.061146736, abs=1e-6)

    f.close()


def read_small(mmap=False):
    f = _segyio.segyiofd(str(testdata / 'small.sgy'), "r", 0).segyopen()

    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193

    metrics = f.metrics()
    metrics.update(f.cube_metrics(ilb, xlb))

    sorting = metrics['sorting']
    trace_count = metrics['tracecount']
    inline_count = metrics['iline_count']
    crossline_count = metrics['xline_count']
    offset_count = metrics['offset_count']

    line_metrics = _segyio.line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

    metrics.update(line_metrics)

    iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
    xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
    offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
    f.indices(metrics, iline_indexes, xline_indexes, offsets)

    return f, metrics, iline_indexes, xline_indexes


def test_fread_trace0_for_depth():
    elements = list(range(25))
    indices = numpy.asarray(elements, dtype=numpy.intc)

    for index in indices:
        d = _segyio.fread_trace0(index, 1, 1, 1, indices, "depth")
        assert d == index

    with pytest.raises(KeyError):
        _segyio.fread_trace0(25, 1, 1, 1, indices, "depth")

