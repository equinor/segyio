from __future__ import absolute_import

import os
import gc

import numpy
import pytest
from pytest import approx

from . import tmpfiles
from . import testdata

import segyio
import segyio._segyio as _segyio

SEG00000_INDEX = 0


def segyfile(filename, mode):
    return _segyio.segyfd(filename=filename, mode=mode)


def test_binary_header_size():
    assert 400 == _segyio.binsize()


def test_textheader_size():
    assert 3200 == _segyio.textsize()


def test_open_non_existing_file():
    with pytest.raises(IOError):
        _ = segyfile("non-existing", "r")


def test_close_non_existing_file():
    with pytest.raises(TypeError):
        _segyio.segyfd.close(None)


def test_open_and_close_file():
    f = segyfile(str(testdata / 'small.sgy'), "r")
    f.close()


def test_open_flush_and_close_file():
    f = segyfile(str(testdata / 'small.sgy'), "r")
    f.flush()
    f.close()

    with pytest.raises(ValueError):
        f.flush()


def test_read_text_header_mmap():
    test_read_text_header(True)


def test_read_text_header(mmap=False):
    f = segyfile(str(testdata / 'small.sgy'), "r")
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
    f = get_instance_segyfile(tmpdir)
    write_text_header(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_write_text_header(tmpdir):
    f = get_instance_segyfile(tmpdir)
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


def get_instance_segyfile(tmpdir,
                          file_name="small.sgy",
                          mode="r+",
                          samples = None,
                          tracecount = None,
                          ):
    path = str(tmpdir)
    f = os.path.join(path, file_name)

    if samples is not None:
        return segyfile(f, mode).segymake(samples, tracecount)
    else:
        return segyfile(f, mode).segyopen()


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_binary_header(tmpdir):
    f = get_instance_segyfile(tmpdir)
    read_and_write_binary_header(f, False)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_binary_header_mmap(tmpdir):
    f = get_instance_segyfile(tmpdir)
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
    f = segyfile(str(testdata / 'small.sgy'), "r")
    if mmap:
        f.mmap()

    binary_header = f.getbin()

    with pytest.raises(TypeError):
        _ = f.getfield([], SEG00000_INDEX, 0)

    with pytest.raises(KeyError):
        _ = f.getfield(binary_header, SEG00000_INDEX, -1)

    assert f.getfield(binary_header, SEG00000_INDEX, 3225) == 1
    assert f.getfield(binary_header, SEG00000_INDEX, 3221) == 50

    f.close()


def test_custom_iline_xline():
    with pytest.raises(ValueError):
        segyfile(str(testdata / 'small.sgy'), "r").segyopen(iline=0)
    with pytest.raises(ValueError):
        segyfile(str(testdata / 'small.sgy'), "r").segyopen(xline=241)

    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen(iline=9, xline=17)
    layouts = f.traceheader_layouts()

    standard_header_layout = layouts["SEG00000"]
    entry = standard_header_layout.entry_by_byte(9)
    assert entry.name == "iline"

    xline_entries = [e for e in standard_header_layout if e.name == "xline"]
    # entry names do not get overwritten if new iline/xline are defined
    assert len(xline_entries) == 2

    f.close()


def test_line_metrics_mmap():
    test_line_metrics(True)


def test_line_metrics(mmap=False):
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()
    if mmap:
        f.mmap()

    metrics = f.metrics()
    metrics.update(f.cube_metrics())
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
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()
    if mmap:
        f.mmap()

    metrics = f.metrics()
    metrics.update(f.cube_metrics())

    assert metrics['trace0'] == _segyio.textsize() + _segyio.binsize()
    assert metrics['samplecount'] == 50
    assert metrics['format'] == 1
    assert metrics['trace_bsize'] == 200
    assert metrics['sorting'] == 2 # inline sorting = 2, crossline sorting = 1
    assert metrics['tracecount'] == 25
    assert metrics['offset_count'] == 1
    assert metrics['iline_count'] == 5
    assert metrics['xline_count'] == 5
    assert metrics['encoding'] == 0

    f.close()


def test_indices_mmap():
    test_indices(True)


def test_indices(mmap=False):
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()
    if mmap:
        f.mmap()

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

    metrics.update(f.cube_metrics())

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
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()
    if mmap:
        f.mmap()

    metrics = f.metrics()
    metrics.update(f.cube_metrics())

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


@tmpfiles(testdata / 'small.sgy')
def test_get_and_putfield(tmpdir):
    f = get_instance_segyfile(tmpdir)
    hdr = bytearray(_segyio.thsize())

    with pytest.raises(BufferError):
        f.getfield(".", SEG00000_INDEX, 0)

    with pytest.raises(TypeError):
        f.getfield([], SEG00000_INDEX, 0)

    with pytest.raises(TypeError):
        f.putfield({}, SEG00000_INDEX, 0, 1)

    with pytest.raises(KeyError):
        f.getfield(hdr, SEG00000_INDEX, 0)

    with pytest.raises(KeyError):
        f.putfield(hdr, SEG00000_INDEX, 0, 1)

    f.putfield(hdr, SEG00000_INDEX, 1, 127)
    f.putfield(hdr, SEG00000_INDEX, 5, 67)
    f.putfield(hdr, SEG00000_INDEX, 9, 19)

    assert f.getfield(hdr, SEG00000_INDEX, 1) == 127
    assert f.getfield(hdr, SEG00000_INDEX, 5) == 67
    assert f.getfield(hdr, SEG00000_INDEX, 9) == 19


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_standard_traceheader_mmap(tmpdir):
    f = get_instance_segyfile(tmpdir)
    read_and_write_standard_traceheader(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_standard_traceheader(tmpdir):
    f = get_instance_segyfile(tmpdir)
    read_and_write_standard_traceheader(f, False)


def read_and_write_standard_traceheader(f, mmap):
    if mmap:
        f.mmap()

    ilb = 189
    xlb = 193

    def mkempty():
        return bytearray(_segyio.thsize())

    with pytest.raises(TypeError):
        f.getth("+")

    with pytest.raises(TypeError):
        f.getth(0, SEG00000_INDEX, None)

    trace_header = f.getth(0, SEG00000_INDEX, mkempty())

    assert f.getfield(trace_header, SEG00000_INDEX, ilb) == 1
    assert f.getfield(trace_header, SEG00000_INDEX, xlb) == 20

    trace_header = f.getth(1, SEG00000_INDEX, mkempty())

    assert f.getfield(trace_header, SEG00000_INDEX, ilb) == 1
    assert f.getfield(trace_header, SEG00000_INDEX, xlb) == 21

    f.putfield(trace_header, SEG00000_INDEX, ilb, 99)
    f.putfield(trace_header, SEG00000_INDEX, xlb, 42)

    f.putth(0, SEG00000_INDEX, trace_header)

    trace_header = f.getth(0, SEG00000_INDEX, mkempty())

    assert f.getfield(trace_header, SEG00000_INDEX, ilb) == 99
    assert f.getfield(trace_header, SEG00000_INDEX, xlb) == 42

    f.close()


@tmpfiles(testdata / 'trace-header-extensions.sgy')
def test_read_and_write_traceheader(tmpdir):
    field = 1
    start, stop, step = 0, 2, 1
    indices = numpy.asarray([0, 1], dtype=numpy.intc)
    attrs = numpy.empty(len(indices), dtype=numpy.intc)
    trace_index = 0

    def mkempty():
        return bytearray(_segyio.thsize())

    def verify_traceheader(f, traceheader_index, type, expected):
        attrs = numpy.empty(len(indices), dtype=type)

        traceheader = f.getth(trace_index, traceheader_index, mkempty())
        assert f.getfield(traceheader, traceheader_index, field) == expected[0]

        f.field_forall(attrs, traceheader_index, start, stop, step, field)
        numpy.testing.assert_array_equal(attrs, expected)

        f.field_foreach(attrs, traceheader_index, indices, field)
        numpy.testing.assert_array_equal(attrs, expected)

        f.putfield(traceheader, traceheader_index, field, 42)
        f.putth(trace_index, traceheader_index, traceheader)

        traceheader = f.getth(trace_index, traceheader_index, mkempty())
        assert f.getfield(traceheader, traceheader_index, field) == 42

    f = get_instance_segyfile(tmpdir, 'trace-header-extensions.sgy')

    standard_index = 0
    extension1_index = 1
    proprietary_index = 2
    nonexistent_index = 3

    expected = {
        standard_index: [0x11111111, 0x44444444],
        extension1_index: [0x22222222_22222222, 0x55555555_55555555],
        proprietary_index: [0x3333, 0x6666],
    }

    with pytest.raises(KeyError):
        f.getth(0, nonexistent_index, mkempty())

    with pytest.raises(KeyError):
        f.getfield(mkempty(), nonexistent_index, field)

    with pytest.raises(KeyError):
        f.field_forall(attrs, nonexistent_index, start, stop, step, field)

    with pytest.raises(KeyError):
        f.field_foreach(attrs, nonexistent_index, indices, field)

    verify_traceheader(
        f, standard_index, numpy.uint32, expected[standard_index]
    )

    verify_traceheader(
        f, extension1_index, numpy.uint64, expected[extension1_index]
    )

    verify_traceheader(
        f, proprietary_index, numpy.int16, expected[proprietary_index]
    )

    f.close()


def test_read_traceheaders():
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()
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
        f.field_forall(attrs, SEG00000_INDEX, start, stop, 0, field)

    buf_handle = f.field_forall(attrs, SEG00000_INDEX, start, stop, step, field)
    numpy.testing.assert_array_equal(attrs, [5, 4, 3, 2, 1])
    assert buf_handle is attrs


def read_traceheaders_foreach(f, mmap):
    if mmap:
        f.mmap()

    indices = numpy.asarray([7, 4, 1, 18, 20], dtype=numpy.intc)
    attrs = numpy.empty(len(indices), dtype=numpy.intc)
    field = segyio.TraceField.CROSSLINE_3D

    with pytest.raises(ValueError):
        f.field_foreach(numpy.empty(1, dtype=numpy.intc), SEG00000_INDEX, indices, field)

    buf_handle = f.field_foreach(attrs, SEG00000_INDEX, indices, field)
    numpy.testing.assert_array_equal(attrs, [22, 24, 21, 23, 20])
    assert buf_handle is attrs


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_trace_mmap(tmpdir):
    f = get_instance_segyfile(tmpdir,
        "trace-wrt.sgy",
        "w+",
        samples = 25,
        tracecount = 100,
    )
    read_and_write_trace(f, True)


@tmpfiles(testdata / 'small.sgy')
def test_read_and_write_trace(tmpdir):
    f = get_instance_segyfile(tmpdir,
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
    assert sum(sum(buf), numpy.double(0)) == approx(800.061169624, abs=1e-6)

    f.getline(iline_trace0, len(xline_idx), iline_stride, offsets, buf)
    assert sum(sum(buf), numpy.double(0)) == approx(305.061146736, abs=1e-6)

    f.close()


def read_small(mmap=False):
    f = segyfile(str(testdata / 'small.sgy'), "r").segyopen()

    if mmap:
        f.mmap()

    metrics = f.metrics()
    metrics.update(f.cube_metrics())

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


@pytest.mark.parametrize("datasource", ["stream", "memory"])
def test_datasource_little_endian(datasource):
    if datasource == "stream":
        stream = open(testdata / "small-lsb.sgy", "rb")
        fd = _segyio.segyfd(
            stream=stream,
            minimize_requests_number=False
        ).segyopen(endianness=1)
    else:
        with open(testdata / "small-lsb.sgy", "rb") as f:
            data = bytearray(f.read())
        fd = _segyio.segyfd(
            memory_buffer=data,
            minimize_requests_number=False
        ).segyopen(endianness=1)

    binary_header = fd.getbin()
    assert fd.getfield(binary_header, SEG00000_INDEX, 3221) == 50

    fd.close()


def test_memory_buffer_memory_management():
    with open(testdata / "small.sgy", "rb") as f:
        data = bytearray(f.read())
    fd = _segyio.segyfd(memory_buffer=data).segyopen()
    # assure test code owns no "data" reference, so it can be garbage collected
    del data
    gc.collect()

    # trying our best to accidentally overwrite memory actually used by "data"
    _ = [numpy.zeros(2500, dtype=numpy.single) for _ in range(100)]

    # internal code should still have reference to original "data" buffer, so
    # there should be no error
    binary_header = fd.getbin()
    assert fd.getfield(binary_header, SEG00000_INDEX, 3225) == 1

    fd.close()


@pytest.mark.xfail(
    reason=(
        "Test assures users get an exception, not a segfault, "
        "so xfail is intentional."
    ),
    strict=True
)
def test_memory_buffer_memory_management_on_uncaught_error():
    with open(testdata / "small.sgy", "rb") as f:
        data = bytearray(f.read())
    fd = _segyio.segyfd(memory_buffer=data).segyopen()
    # assure test code owns no "data" reference, so it can be garbage collected
    del data
    gc.collect()

    # trying our best to accidentally overwrite memory actually used by "data"
    _ = [numpy.zeros(2500, dtype=numpy.single) for _ in range(100)]

    fd.putbin("Causing 'buffer too small' error")


def test_stanza_names():
    filename = str(testdata / 'small.sgy')
    f = segyfile(filename, "r").segyopen()
    assert f.stanza_names() == []
    f.close()

    filename = str(testdata / 'multi-text.sgy')
    f = segyfile(filename, "r").segyopen()
    assert f.stanza_names() == ['']
    f.close()

    filename = str(testdata / 'stanzas-known-count.sgy')
    f = segyfile(filename, "r")
    f.mmap()
    f.segyopen()
    assert f.stanza_names() == [
        'SEGYIO:TEST ASCII  DATA WITH CONTENTTYPE AND BYTES: application/vnd.openxmlformats-officedocument.wordprocessingml.document.glossary+xml:666',
        'SEGYIO:Test EBCDIC data',
        'SEGYIO: test ASCII data'
    ]
    f.close()

    filename = str(testdata / 'stanzas-unknown-count.sgy')
    f = segyfile(filename, "r").segyopen()
    assert f.stanza_names() == ['segyio: test ()(test1) ', '  seg: endTEXt  ']
    assert f.gettext(1).strip() == bytes("((segyio: test ()(test1) ))first part", "ascii")
    assert f.gettext(2).strip() == bytes("second part", "ascii")
    f.close()
