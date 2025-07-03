import segyio
import numpy as np
import pytest

testfile = "tiny.sgy"


def open_with_stream(
    make_stream,
    filename,
    mode="rb",
    minimize_requests_number=True
):
    stream = make_stream(filename, mode)
    return segyio.open_with(
        stream,
        minimize_requests_number=minimize_requests_number
    )


def create_with_stream(
    make_stream,
    filename,
    spec
):
    stream = make_stream(filename, "w+b")
    return segyio.create_with(
        stream,
        spec
    )


def test_tools_metadata(make_stream):
    with open_with_stream(make_stream, testfile) as f:
        spec = segyio.tools.metadata(f)

    assert np.array_equal(spec.ilines, [1, 2, 3])
    assert np.array_equal(spec.xlines, [20, 21])


def test_read_cube(make_stream):
    with open_with_stream(make_stream, testfile) as f:
        cube = segyio.tools.cube(f)

    expected_cube = np.array([
        [
            [1.20, 1.20001, 1.20002, 1.20003],
            [1.21, 1.21001, 1.21002, 1.21003],
        ],
        [
            [2.20, 2.20001, 2.20002, 2.20003],
            [2.21, 2.21001, 2.21002, 2.21003],
        ],
        [
            [3.20, 3.20001, 3.20002, 3.20003],
            [3.21, 3.21001, 3.21002, 3.21003],
        ]
    ], dtype=np.float32)

    assert np.allclose(cube, expected_cube, atol=1e-4)


def verify_trace_reads_on_known_pattern_file(
    f, full_cube, trace_stride, sample_stride
):
    """
    Aims to call all main internal paths that read traces and ensure correct results.
    Works only on test files created through "make_file" example.
    """
    step = 0.00001
    chosen_samples_generator = f.trace[::trace_stride, ::sample_stride]

    for iline_index, iline in enumerate(f.ilines):
        iline_data = f.iline[iline]
        for xline_index, xline in enumerate(f.xlines):
            trace = iline_data[xline_index]
            tracenr = iline_index * len(f.xlines) + xline_index
            if tracenr % trace_stride == 0:
                chosen_samples = next(chosen_samples_generator)

            for sample_index in range(len(f.samples)):
                expected = iline + xline / 100.0 + sample_index * step

                assert np.allclose(trace[sample_index], expected)
                assert f.depth_slice[sample_index][iline_index][xline_index] == trace[sample_index]

                if tracenr % trace_stride == 0 and sample_index % sample_stride == 0:
                    chosen_sample_index = int(sample_index / sample_stride)
                    assert chosen_samples[chosen_sample_index] == trace[sample_index]

            assert np.array_equal(trace, f.trace[tracenr])

        assert np.array_equal(full_cube[iline_index], iline_data)


@pytest.mark.parametrize("minimize_requests_number", [True, False])
def test_read(make_stream, minimize_requests_number):
    with open_with_stream(make_stream, testfile) as f:
        cube = segyio.tools.cube(f)

    with open_with_stream(
        make_stream,
        testfile,
        minimize_requests_number=minimize_requests_number
    ) as f:
        assert f.tracecount == 6
        assert f.bin[segyio.BinField.Samples] == 4
        assert f.header[0][segyio.TraceField.INLINE_3D] == 1
        assert f.header[0][segyio.TraceField.CROSSLINE_3D] == 20

        assert f.attributes(segyio.TraceField.INLINE_3D)[0] == 1
        assert f.attributes(segyio.TraceField.CROSSLINE_3D)[0, 1][0] == 20

        trace_stride = 3
        sample_stride = 2
        verify_trace_reads_on_known_pattern_file(
            f, cube, trace_stride, sample_stride)


def test_create(make_stream):
    fresh = "fresh.sgy"
    with open_with_stream(make_stream, testfile) as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines
        with create_with_stream(make_stream, fresh, spec) as dst:
            dst.bin = src.bin
            dst.header = src.header
            dst.trace = src.trace

            dst.flush()

            assert dst.bin == src.bin
            assert np.array_equal(dst.header, src.header)
            assert np.array_equal(dst.trace, src.trace)

        with open_with_stream(make_stream, fresh) as f:
            assert f.bin == src.bin
            assert np.array_equal(f.header, src.header)
            assert np.array_equal(f.trace, src.trace)


def run_test_update(open_datasource):
    """
    Aims to call all main internal paths that update data and ensure correct results.
    Works only on test files created through "make_file" example.
    """
    with open_datasource(mode='r+b') as f:
        f.bin[segyio.BinField.ReelNumber] = 666
        f.header[0][segyio.TraceField.HourOfDay] = 11

        lines = {1: 'first line', 10: 'last line'}
        text_header = segyio.tools.create_text_header(lines)
        f.text[0] = text_header

        f.trace[0] = f.trace[0] * 2
        f.iline[1] = f.iline[1] * 3
        f.depth_slice[0::2] = [slice * 5 for slice in f.depth_slice[0::2]]

        f.flush()

    with open_datasource() as g:
        assert g.bin[segyio.BinField.ReelNumber] == 666
        assert g.header[0][segyio.TraceField.HourOfDay] == 11
        assert g.text[0] == bytes(bytearray(text_header, 'ascii'))

        step = 0.00001

        for iline_index, iline in enumerate(g.ilines):
            iline_data = g.iline[iline]
            for xline_index, xline in enumerate(g.xlines):
                trace = iline_data[xline_index]
                tracenr = iline_index * len(g.xlines) + xline_index

                for sample_index in range(len(g.samples)):
                    expected = iline + xline / 100.0 + sample_index * step
                    if tracenr == 0:
                        expected *= 2
                    if iline == 1:
                        expected *= 3
                    if sample_index % 2 == 0:
                        expected *= 5
                    err = f"Mismatch at iline {iline_index}, xline {xline_index}, sample {sample_index}"
                    assert np.allclose(
                        trace[sample_index], expected, atol=1e-4
                    ), err


@pytest.fixture
def stream_source(make_stream):
    def open(mode="rb"):
        return open_with_stream(make_stream, testfile, mode=mode)
    return open


def test_update(stream_source):
    run_test_update(stream_source)


def test_none_stream():
    with pytest.raises(ValueError):
        segyio.open_with(None)


def test_reopen_closed(make_stream):
    with open_with_stream(make_stream, testfile) as f:
        pass

    with open_with_stream(make_stream, testfile) as f:
        pass


def test_not_closing_does_not_cause_segfault(make_stream):
    open_with_stream(make_stream, testfile)


@pytest.mark.xfail(
    reason=(
        "Test assures users must get an exception, not a segfault, so xfail is intentional. "
        "Didn't always appear on first execution, so test is repeated a few times. "
        "Test was sponsored by one missing Py_INCREF"
    ),
    strict=True
)
@pytest.mark.parametrize('execution_number', range(3))
def test_resource_deallocation(make_stream, execution_number):
    f = open_with_stream(make_stream, testfile)
    f.xline[1]
    f.close()
