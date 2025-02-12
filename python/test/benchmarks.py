# Benchmarking suite, not run by default with pytest.
# To run this file pytest-benchmark must be installed via pip.

import os
import shutil
import segyio
import pytest


# Files are expected to be present at the run location
# Files size should be compatible with retrieved lines
files = [
    'file.sgy',
    'file-ps.sgy'
]


def run(filepath, mmap, func):
    with segyio.open(filepath) as f:
        if mmap:
            f.mmap()
        func(f)


def iline_slice(f):
    f.iline[200]


def xline_slice(f):
    f.xline[300]


def depth_slice(f):
    f.depth_slice[1000]


def cube(filepath):
    segyio.tools.cube(filepath)


def change_data(output_file):
    # multiply data by 2
    with segyio.open(output_file, "r+") as f:
        for i in f.ilines:
            f.iline[i] = 2 * f.iline[i]


slices = [
    iline_slice,
    xline_slice,
    depth_slice,
]


@pytest.mark.benchmark(group="nommap")
@pytest.mark.parametrize("file", files)
@pytest.mark.parametrize("func", slices)
def test_read_speed(benchmark, file, func):
    benchmark(run, file, False, func)


@pytest.mark.benchmark(group="with mmap")
@pytest.mark.parametrize("file", files)
@pytest.mark.parametrize("func", slices)
def test_mmap_read_speed(benchmark, file, func):
    benchmark(run, file, True, func)


@pytest.mark.benchmark(group="cube")
@pytest.mark.parametrize("file", files)
def test_cube_speed(benchmark, file):
    benchmark.pedantic(run, rounds=15, args=[file, False, cube])


@pytest.mark.benchmark(group="write")
@pytest.mark.parametrize("file", files)
def test_write_file(benchmark, file, tmp_path):
    output_file = tmp_path / 'output.sgy'

    def setup():
        if os.path.exists(output_file):
            os.remove(output_file)
        shutil.copyfile(file, output_file)

    benchmark.pedantic(change_data, setup=setup, rounds=5, args=[output_file])

    if os.path.exists(output_file):
        os.remove(output_file)
