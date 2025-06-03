# Benchmarking suite, not run by default with pytest.
# To run this file pytest-benchmark must be installed via pip.

import os
import segyio
import pytest
import numpy as np


# Files are expected to be present at the run location
# Files size should be compatible with retrieved lines
read_files = [
    'file.sgy',
    'file-small.sgy',
]

write_files = [
    'file-small.sgy',
]


def run(filepath, mmap, func, mode="r"):
    with segyio.open(filepath, mode=mode) as f:
        if mmap:
            f.mmap()
        func(f)


def iline_slice(f):
    f.iline[200]


def iline_strided(f):
    list(f.iline[0:400:4])


def xline_slice(f):
    f.xline[300]


def xline_strided(f):
    list(f.xline[0:400:4])


def depth_slice(f):
    f.depth_slice[400]


def depth_strided(f):
    list(f.depth_slice[300:310:4])


def reverse_traces(f):
    f.trace.raw[::-100]


def sparse_samples(f):
    list(f.trace[::3, ::4])


def binary_header(f):
    f.bin


def trace_header(f):
    list(f.header[100:200:2])


def attributes(f):
    list(f.attributes(segyio.TraceField.INLINE_3D))


def cube(filepath):
    segyio.tools.cube(filepath)


def update_by_iline(f):
    for i in f.ilines:
        f.iline[i] = 2 * f.iline[i]


def update_by_xline(f):
    for i in f.xlines:
        f.xline[i] = 4 * f.xline[i]


def update_by_depth(f):
    f.depth_slice[20:30:5] = np.ones(
        (2, len(f.ilines), len(f.xlines)), dtype=np.float32
    )


def update_by_traces(f):
    new = [trace * 5.0 for trace in f.trace.raw[:]]
    for i in range(len(f.trace)):
        f.trace[i] = new[i]


def create(output_file):
    spec = segyio.spec()

    spec.sorting = 2
    spec.format = 1
    spec.samples = range(1000)
    spec.ilines = range(600)
    spec.xlines = range(600)

    with segyio.create(output_file, spec) as f:
        ref = np.ones((len(f.ilines), len(f.xlines)), dtype=np.float32)
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                f.header[tr] = {
                    segyio.su.offset: 1,
                    segyio.su.iline: il,
                    segyio.su.xline: xl
                }
                f.trace[tr] = ref
                tr += 1


operations = [
    iline_slice,
    iline_strided,
    xline_slice,
    xline_strided,
    depth_slice,
    depth_strided,
    reverse_traces,
    sparse_samples,
    binary_header,
    trace_header,
    attributes
]

write_operations = [
    update_by_iline,
    update_by_xline,
    update_by_depth,
    update_by_traces,
]


@pytest.mark.benchmark(group="nommap")
@pytest.mark.parametrize("file", read_files)
@pytest.mark.parametrize("func", operations)
def test_read_speed(benchmark, file, func):
    benchmark(run, file, False, func)


@pytest.mark.benchmark(group="with mmap")
@pytest.mark.parametrize("file", read_files)
@pytest.mark.parametrize("func", operations)
def test_mmap_read_speed(benchmark, file, func):
    benchmark(run, file, True, func)


@pytest.mark.benchmark(group="cube")
@pytest.mark.parametrize("file", read_files)
def test_cube_speed(benchmark, file):
    benchmark.pedantic(run, rounds=5, args=[file, False, cube])


@pytest.mark.benchmark(group="write")
@pytest.mark.parametrize("file", write_files)
@pytest.mark.parametrize("func", write_operations)
def test_write_file(benchmark, file, func):
    # note that original file will get overwritten
    benchmark.pedantic(run, rounds=7, args=[file, False, func, "r+"])


@pytest.mark.benchmark(group="create")
def test_create_file(benchmark, tmp_path):
    output_file = tmp_path / 'new.sgy'

    def setup():
        if os.path.exists(output_file):
            os.remove(output_file)

    benchmark.pedantic(create, setup=setup, rounds=5, args=[output_file])

    if os.path.exists(output_file):
        os.remove(output_file)
