import shutil
from unittest import TestCase

import numpy
import segyio._segyio as _segyio

ACTUAL_TEXT_HEADER = "C 1 DATE: 2016-09-19                                                            " \
                     "C 2 AN INCREASE IN AMPLITUDE EQUALS AN INCREASE IN ACOUSTIC IMPEDANCE           " \
                     "C 3 Written by libsegyio (python)                                               " \
                     "C 4                                                                             " \
                     "C 5                                                                             " \
                     "C 6                                                                             " \
                     "C 7                                                                             " \
                     "C 8                                                                             " \
                     "C 9                                                                             " \
                     "C10                                                                             " \
                     "C11 TRACE HEADER POSITION:                                                      " \
                     "C12   INLINE BYTES 189-193    | OFFSET BYTES 037-041                            " \
                     "C13   CROSSLINE BYTES 193-197 |                                                 " \
                     "C14                                                                             " \
                     "C15 END EBCDIC HEADER                                                           " \
                     "C16                                                                             " \
                     "C17                                                                             " \
                     "C18                                                                             " \
                     "C19                                                                             " \
                     "C20                                                                             " \
                     "C21                                                                             " \
                     "C22                                                                             " \
                     "C23                                                                             " \
                     "C24                                                                             " \
                     "C25                                                                             " \
                     "C26                                                                             " \
                     "C27                                                                             " \
                     "C28                                                                             " \
                     "C29                                                                             " \
                     "C30                                                                             " \
                     "C31                                                                             " \
                     "C32                                                                             " \
                     "C33                                                                             " \
                     "C34                                                                             " \
                     "C35                                                                             " \
                     "C36                                                                             " \
                     "C37                                                                             " \
                     "C38                                                                             " \
                     "C39                                                                             " \
                     "C40                                                                            \x80"


class _segyioTests(TestCase):
    def setUp(self):
        self.filename = "test-data/small.sgy"

    def test_binary_header_size(self):
        self.assertEqual(400, _segyio.binheader_size())

    def test_textheader_size(self):
        self.assertEqual(3200, _segyio.textheader_size())

    def test_open_non_existing_file(self):
        with self.assertRaises(IOError):
            f = _segyio.open("non-existing", "r")

    def test_close_non_existing_file(self):
        with self.assertRaises(TypeError):
            _segyio.close(None)

    def test_open_and_close_file(self):
        f = _segyio.open(self.filename, "r")
        _segyio.close(f)

    def test_open_flush_and_close_file(self):
        _segyio.flush(None)
        f = _segyio.open(self.filename, "r")
        _segyio.flush(f)
        _segyio.close(f)

        # This does not fail for some reason.
        # with self.assertRaises(IOError):
        #     _segyio.flush(f)

    def test_read_text_header(self):
        f = _segyio.open(self.filename, "r")

        self.assertEqual(_segyio.read_textheader(f, 0), ACTUAL_TEXT_HEADER)

        with self.assertRaises(Exception):
            _segyio.read_texthdr(None, 0)

        _segyio.close(f)

    def test_write_text_header(self):
        fname = self.filename.replace("small", "txt_hdr_wrt")
        shutil.copyfile(self.filename, fname)
        f = _segyio.open(fname, "r+")

        with self.assertRaises(ValueError):
            _segyio.write_textheader(f, 0, "")

        self.assertEqual(_segyio.read_textheader(f, 0), ACTUAL_TEXT_HEADER)

        _segyio.write_textheader(f, 0, "yolo" * 800)

        self.assertEqual(_segyio.read_textheader(f, 0), "yolo" * 800)

        _segyio.close(f)

    def test_read_and_write_binary_header(self):
        with self.assertRaises(Exception):
            hdr = _segyio.read_binaryheader(None)

        with self.assertRaises(Exception):
            _segyio.write_binaryheader(None, None)

        fname = self.filename.replace("small", "bin_hdr_wrt")
        shutil.copyfile(self.filename, fname)
        f = _segyio.open(fname, "r+")

        binary_header = _segyio.read_binaryheader(f)

        with self.assertRaises(Exception):
            _segyio.write_binaryheader(f, "Not the correct type")

        _segyio.write_binaryheader(f, binary_header)
        _segyio.close(f)


    def test_read_binary_header_fields(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)

        with self.assertRaises(TypeError):
            value = _segyio.get_field("s", 0)

        with self.assertRaises(IndexError):
            value = _segyio.get_field(binary_header, -1)

        self.assertEqual(_segyio.get_field(binary_header, 3225), 1)
        self.assertEqual(_segyio.get_field(binary_header, 3221), 50)

        _segyio.close(f)

    def test_line_metrics(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193
        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)
        _segyio.close(f)

        sorting = metrics['sorting']
        trace_count = metrics['trace_count']
        inline_count = metrics['iline_count']
        crossline_count = metrics['xline_count']
        offset_count = metrics['offset_count']

        metrics = _segyio.init_line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

        self.assertEqual(metrics['xline_length'], 5)
        self.assertEqual(metrics['xline_stride'], 5)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 1)

        # (sorting, trace_count, inline_count, crossline_count, offset_count)
        metrics = _segyio.init_line_metrics(1, 15, 3, 5, 1)

        self.assertEqual(metrics['xline_length'], 3)
        self.assertEqual(metrics['xline_stride'], 1)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 3)

        metrics = _segyio.init_line_metrics(2, 15, 3, 5, 1)

        self.assertEqual(metrics['xline_length'], 3)
        self.assertEqual(metrics['xline_stride'], 5)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 1)


    def test_metrics(self):
        f = _segyio.open(self.filename, "r")
        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193

        with self.assertRaises(TypeError):
            metrics = _segyio.init_metrics("?", binary_header, ilb, xlb)

        with self.assertRaises(TypeError):
            metrics = _segyio.init_metrics(f, "?", ilb, xlb)

        with self.assertRaises(IndexError):
            metrics = _segyio.init_metrics(f, binary_header, ilb + 1, xlb)

        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)

        self.assertEqual(metrics['iline_field'], ilb)
        self.assertEqual(metrics['xline_field'], xlb)
        self.assertEqual(metrics['trace0'], _segyio.textheader_size() + _segyio.binheader_size())
        self.assertEqual(metrics['sample_count'], 50)
        self.assertEqual(metrics['format'], 1)
        self.assertEqual(metrics['trace_bsize'], 200)
        self.assertEqual(metrics['sorting'], 2) # inline sorting = 2, crossline sorting = 1
        self.assertEqual(metrics['trace_count'], 25)
        self.assertEqual(metrics['offset_count'], 1)
        self.assertEqual(metrics['iline_count'], 5)
        self.assertEqual(metrics['xline_count'], 5)

        _segyio.close(f)

        with self.assertRaises(IOError):
            metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)

    def test_line_indices(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193
        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)
        dmy = numpy.zeros(2, dtype=numpy.uintc)

        dummy_metrics = {'xline_count': 2, 'iline_count': 2}

        with self.assertRaises(TypeError):
            _segyio.init_line_indices(".", {}, dmy, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_line_indices(f, "-", dmy, dmy)

        # with self.assertRaises(KeyError):
        #     _segyio.init_line_indices(f, {}, dmy, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_line_indices(f, dummy_metrics, 1, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_line_indices(f, dummy_metrics, dmy, 2)

        with self.assertRaises(TypeError):
            _segyio.init_line_indices(f, dummy_metrics, dmy, 2)

        with self.assertRaises(TypeError):
            fdmy = numpy.zeros(1, dtype=numpy.single)
            _segyio.init_line_indices(f, dummy_metrics, fdmy, dmy)

        one = numpy.zeros(1, dtype=numpy.uintc)
        two = numpy.zeros(2, dtype=numpy.uintc)
        with self.assertRaises(ValueError):
            _segyio.init_line_indices(f, dummy_metrics, one, two)

        with self.assertRaises(ValueError):
            _segyio.init_line_indices(f, dummy_metrics, two, one)

        # Happy Path
        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.uintc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.uintc)
        _segyio.init_line_indices(f, metrics, iline_indexes, xline_indexes)

        self.assertListEqual([1, 2, 3, 4, 5], list(iline_indexes))
        self.assertListEqual([20, 21, 22, 23, 24], list(xline_indexes))

        _segyio.close(f)

    def test_fread_trace0(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193

        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)

        sorting = metrics['sorting']
        trace_count = metrics['trace_count']
        inline_count = metrics['iline_count']
        crossline_count = metrics['xline_count']
        offset_count = metrics['offset_count']

        line_metrics = _segyio.init_line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.uintc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.uintc)
        _segyio.init_line_indices(f, metrics, iline_indexes, xline_indexes)

        with self.assertRaises(KeyError):
            _segyio.fread_trace0(0, len(xline_indexes), line_metrics['iline_stride'], iline_indexes, "inline")

        with self.assertRaises(KeyError):
            _segyio.fread_trace0(2, len(iline_indexes), line_metrics['xline_stride'], xline_indexes, "crossline")

        value = _segyio.fread_trace0(1, len(xline_indexes), line_metrics['iline_stride'], iline_indexes, "inline")
        self.assertEqual(value, 0)

        value = _segyio.fread_trace0(2, len(xline_indexes), line_metrics['iline_stride'], iline_indexes, "inline")
        self.assertEqual(value, 5)

        value = _segyio.fread_trace0(21, len(iline_indexes), line_metrics['xline_stride'], xline_indexes, "crossline")
        self.assertEqual(value, 1)

        value = _segyio.fread_trace0(22, len(iline_indexes), line_metrics['xline_stride'], xline_indexes, "crossline")
        self.assertEqual(value, 2)

        _segyio.close(f)

    def test_get_and_set_field(self):
        hdr = _segyio.empty_traceheader()

        with self.assertRaises(TypeError):
            _segyio.get_field(".", 0)

        with self.assertRaises(TypeError):
            _segyio.set_field(".", 0, 1)

        with self.assertRaises(IndexError):
            _segyio.get_field(hdr, 0)

        with self.assertRaises(IndexError):
            _segyio.set_field(hdr, 0, 1)

        _segyio.set_field(hdr, 1, 127)
        _segyio.set_field(hdr, 5, 67)
        _segyio.set_field(hdr, 9, 19)

        self.assertEqual(_segyio.get_field(hdr, 1), 127)
        self.assertEqual(_segyio.get_field(hdr, 5), 67)
        self.assertEqual(_segyio.get_field(hdr, 9), 19)

    def test_read_and_write_traceheader(self):
        fname = self.filename.replace("small", "bin_hdr_wrt")
        shutil.copyfile(self.filename, fname)
        f = _segyio.open(fname, "r+")
        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193
        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)

        empty = _segyio.empty_traceheader()

        with self.assertRaises(TypeError):
            trace_header = _segyio.read_traceheader("+", )

        with self.assertRaises(TypeError):
            trace_header = _segyio.read_traceheader(f, 0, None)

        trace_header = _segyio.read_traceheader(f, 0, _segyio.empty_traceheader(), metrics['trace0'], metrics['trace_bsize'])

        self.assertEqual(_segyio.get_field(trace_header, ilb), 1)
        self.assertEqual(_segyio.get_field(trace_header, xlb), 20)

        trace_header = _segyio.read_traceheader(f, 1, _segyio.empty_traceheader(), metrics['trace0'], metrics['trace_bsize'])

        self.assertEqual(_segyio.get_field(trace_header, ilb), 1)
        self.assertEqual(_segyio.get_field(trace_header, xlb), 21)

        _segyio.set_field(trace_header, ilb, 99)
        _segyio.set_field(trace_header, xlb, 42)

        _segyio.write_traceheader(f, 0, trace_header, metrics['trace0'], metrics['trace_bsize'])

        trace_header = _segyio.read_traceheader(f, 0, _segyio.empty_traceheader(), metrics['trace0'], metrics['trace_bsize'])

        self.assertEqual(_segyio.get_field(trace_header, ilb), 99)
        self.assertEqual(_segyio.get_field(trace_header, xlb), 42)

        _segyio.close(f)

    def test_read_and_write_trace(self):
        f = _segyio.open("test-data/trace-wrt.sgy", "w+")

        buf = numpy.ones(25, dtype=numpy.single)
        buf[11] = 3.1415
        _segyio.write_trace(f, 0, buf, 0, 100, 1, 25)
        buf[:] = 42.0
        _segyio.write_trace(f, 1, buf, 0, 100, 1, 25)

        _segyio.flush(f)

        buf = numpy.zeros(25, dtype=numpy.single)

        _segyio.read_trace(f, 0, 25, buf, 0, 100, 1, 25)

        self.assertAlmostEqual(buf[10], 1.0, places=4)
        self.assertAlmostEqual(buf[11], 3.1415, places=4)

        _segyio.read_trace(f, 1, 25, buf, 0, 100, 1, 25)

        self.assertAlmostEqual(sum(buf), 42.0 * 25, places=4)

        _segyio.close(f)

    def read_small(self, mmap = False):
        f = _segyio.open(self.filename, "r")

        if mmap: _segyio.mmap(f)

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193

        metrics = _segyio.init_metrics(f, binary_header, ilb, xlb)

        sorting = metrics['sorting']
        trace_count = metrics['trace_count']
        inline_count = metrics['iline_count']
        crossline_count = metrics['xline_count']
        offset_count = metrics['offset_count']

        line_metrics = _segyio.init_line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

        metrics.update(line_metrics)

        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.uintc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.uintc)
        _segyio.init_line_indices(f, metrics, iline_indexes, xline_indexes)

        return f, metrics, iline_indexes, xline_indexes

    def test_read_line(self):
        f, metrics, iline_idx, xline_idx = self.read_small()

        tr0 = metrics['trace0']
        bsz = metrics['trace_bsize']
        samples = metrics['sample_count']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        _segyio.read_line(f, xline_trace0, len(iline_idx), xline_stride, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        _segyio.read_line(f, iline_trace0, len(xline_idx), iline_stride, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        _segyio.close(f)

    def test_read_line_mmap(self):
        f, metrics, iline_idx, xline_idx = self.read_small(True)

        tr0 = metrics['trace0']
        bsz = metrics['trace_bsize']
        samples = metrics['sample_count']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        _segyio.read_line(f, xline_trace0, len(iline_idx), xline_stride, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        _segyio.read_line(f, iline_trace0, len(xline_idx), iline_stride, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        _segyio.close(f)

    def test_fread_trace0_for_depth(self):
        elements = list(range(25))
        indices = numpy.asarray(elements, dtype=numpy.uintc)

        for index in indices:
            d = _segyio.fread_trace0(index, 1, 1, indices, "depth")
            self.assertEqual(d, index)

        with self.assertRaises(KeyError):
            d = _segyio.fread_trace0(25, 1, 1, indices, "depth")

