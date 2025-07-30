import pytest

import os
import io

import shutil


def get_filename_and_filepath(path):
    if os.path.basename(path) != path:
        filename = os.path.basename(path)
        filepath = path
        return filename, filepath

    if path in ("tiny.sgy", "small.sgy"):
        filename = path
        filepath = os.path.abspath("../test-data/"+path)
        return filename, filepath

    # assuming file lies in current working directory
    return path, os.getcwd() + "/" + path


@pytest.fixture()
def make_nothing_fixture():
    def _make_nothing(*args, **kwargs):
        return None
    yield _make_nothing


@pytest.fixture()
def make_python_file_stream_fixture(tmp_path_factory):
    """
    Creates stream from a standard python file.
    """
    filepaths = {}

    def _make_stream(path, mode):
        filename, filepath = get_filename_and_filepath(path)

        if filename in filepaths:
            return open(filepaths[filename], mode)

        path_r = str(filepath)
        path_w = tmp_path_factory.getbasetemp() / filename
        if mode == "rb":
            path = path_r
        elif mode == "w+b":
            path = path_w
        elif mode == "r+b":
            shutil.copy(path_r, path_w)
            path = path_w
        else:
            raise ValueError(f"Unsupported mode {mode}")

        filepaths.update({filename: path})
        return open(path, mode)
    yield _make_stream


@pytest.fixture()
def make_bytes_io_stream_fixture():
    """
    Creates stream from a BytesIO object. To support update functionality,
    buffer contents are saved on close.
    """
    class MemoryIO(io.BytesIO):
        def close(self):
            self.contents = self.getvalue()
            super().close()

    buffers = {}

    def _make_stream(path, mode):
        filename, filepath = get_filename_and_filepath(path)
        if filename in buffers:
            buffer = buffers[filename]
            if buffer.closed:
                buffer = MemoryIO(buffer.contents)
                buffers.update({filename: buffer})
            return buffer

        buffer = MemoryIO(open(filepath, mode).read())
        buffers.update({filename: buffer})
        return buffer
    yield _make_stream


@pytest.fixture()
def make_memory_buffer_fixture():
    """Creates memory buffer from local file."""
    loaded = {}

    def _make_memory_buffer(path, mode):
        filename, filepath = get_filename_and_filepath(path)
        if filename in loaded:
            return loaded[filename]

        with open(filepath, "rb") as f:
            data = bytearray(f.read())

        loaded.update({filename: data})
        return data
    yield _make_memory_buffer


@pytest.fixture()
def make_datasource(make_stream, make_memory_buffer):
    """
    Returns a function that makes all kinds of datasources from provided
    filename/mode. Depending on what datasource-creating functions were provided
    on setup, some datasources will be None. Function is designed that way to
    support reusing tests for various datasources (currently for benchmark
    tests).
    """
    def _make_datasource(filename, mode):
        stream = make_stream(filename, mode)
        memory_buffer = make_memory_buffer(filename, mode)
        return filename, memory_buffer, stream
    yield _make_datasource


def pytest_configure(config):
    # Helps users run segyio tests against their own external datasources. If
    # "--custom_stream" is specified, "make_stream" fixture (which knows how to
    # create new stream) should be provided by user. If "--custom-stream" is not
    # defined, configuration will be set up with datasources available for
    # testing by supplying "datasource" option with predefined choices.
    if not config.getoption("custom_stream"):
        # idea is based on
        # https://github.com/pytest-dev/pytest/issues/2424#issuecomment-333387206 and
        # https://github.com/pytest-dev/pytest/issues/2793

        make_stream = make_nothing_fixture
        make_memory_buffer = make_nothing_fixture
        option = config.getoption("datasource")

        if option == "pyfile":
            make_stream = make_python_file_stream_fixture
        elif option == "pymemory":
            make_stream = make_bytes_io_stream_fixture
        elif option == "nativememory":
            make_memory_buffer = make_memory_buffer_fixture
        else:
            pass
        globals()["make_stream"] = make_stream
        globals()["make_memory_buffer"] = make_memory_buffer


def pytest_addoption(parser):
    parser.addoption(
        "--custom_stream",
        action="store_true",
        help="Should be specified to allow running segyio tests against external stream"
    )
    parser.addoption(
        "--datasource",
        action="store",
        default="default",
        help="Known datasource in testing. Options: pyfile, pymemory, nativememory."
    )
    parser.addoption(
        "--readf",
        action="append",
        default=[],
        help=("Files used for benchmark read tests. "
              "Files size should be compatible with retrieved lines. "
              "Option may be used multiple times.")
    )

    parser.addoption(
        "--writef",
        action="append",
        default=[],
        help=("Files used for benchmark write tests (files will get modified)."
              "File size should be compatible with modified lines."
              "Option may be be used multiple times.")
    )

def pytest_generate_tests(metafunc):
    # default files are expected to be present at the run location if options are not overridden
    if "read_file" in metafunc.fixturenames:
        read_files = metafunc.config.getoption("readf")
        if not read_files:
            read_files = ['file.sgy', 'file-small.sgy']
        metafunc.parametrize("read_file", read_files)

    if "write_file" in metafunc.fixturenames:
        write_files = metafunc.config.getoption("writef")
        if not write_files:
            write_files = ['file-small.sgy']
        metafunc.parametrize("write_file", write_files)
