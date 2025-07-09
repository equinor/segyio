import segyio
import numpy as np
import pytest

from . import testdata

# memory_buffer tests are part of stream test set, so they share some code with
# it
from .stream import (
    run_test_update,
    verify_trace_reads_on_known_pattern_file
)

small = str(testdata / "small.sgy")


def test_tools_metadata():
    with open(small, "rb") as f:
        data = bytearray(f.read())
    with segyio.open_from_memory(data) as f:
        spec = segyio.tools.metadata(f)

    assert np.array_equal(spec.ilines, [1, 2, 3, 4, 5])
    assert np.array_equal(spec.xlines, [20, 21, 22, 23, 24])


def test_read():
    with open(small, "rb") as f:
        data = bytearray(f.read())

    with segyio.open_from_memory(data) as f:
        cube = segyio.tools.cube(f)

        assert f.tracecount == 25
        assert f.bin[segyio.BinField.Samples] == 50
        assert f.header[0][segyio.TraceField.INLINE_3D] == 1
        assert f.header[0][segyio.TraceField.CROSSLINE_3D] == 20

        assert f.attributes(segyio.TraceField.INLINE_3D)[0] == 1
        assert f.attributes(segyio.TraceField.CROSSLINE_3D)[0, 1][0] == 20

        trace_stride = 5
        sample_stride = 10
        verify_trace_reads_on_known_pattern_file(
            f, cube, trace_stride, sample_stride
        )


@pytest.fixture
def memory_source():
    def setup_memory_source(data):
        def open_source(mode="r"):
            return segyio.open_from_memory(data)
        return open_source
    return setup_memory_source


def test_memory_update(memory_source):
    with open(small, "rb") as f:
        data = bytearray(f.read())
    run_test_update(memory_source(data))


def test_reopen_closed():
    with open(small, "rb") as f:
        data = bytearray(f.read())

    with segyio.open_from_memory(data) as f:
        pass

    with segyio.open_from_memory(data) as f:
        pass


def test_not_closing_does_not_cause_segfault():
    with open(small, "rb") as f:
        data = bytearray(f.read())
    segyio.open_from_memory(data)
