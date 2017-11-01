from __future__ import absolute_import

import unittest

import numpy
import segyio
import segyio._segyio as _segyio
from .test_context import TestContext


class _segyioTests(unittest.TestCase):
    def setUp(self):
        self.filename = "test-data/small.sgy"

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
        self.ACTUAL_TEXT_HEADER = bytes(rows)

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

        self.assertEqual(_segyio.read_textheader(f, 0), self.ACTUAL_TEXT_HEADER)

        with self.assertRaises(Exception):
            _segyio.read_texthdr(None, 0)

        _segyio.close(f)

    def test_write_text_header(self):
        with TestContext("write_text_header") as context:
            context.copy_file(self.filename)

            f = _segyio.open("small.sgy", "r+")

            with self.assertRaises(ValueError):
                _segyio.write_textheader(f, 0, "")

            self.assertEqual(_segyio.read_textheader(f, 0), self.ACTUAL_TEXT_HEADER)

            _segyio.write_textheader(f, 0, "yolo" * 800)

            textheader = _segyio.read_textheader(f, 0)
            textheader = textheader.decode('ascii')  # Because in Python 3.5 bytes are not comparable to strings
            self.assertEqual(textheader, "yolo" * 800)

            _segyio.close(f)

    def test_read_and_write_binary_header(self):
        with self.assertRaises(Exception):
            hdr = _segyio.read_binaryheader(None)

        with self.assertRaises(Exception):
            _segyio.write_binaryheader(None, None)

        with TestContext("read_and_write_bin_header") as context:
            context.copy_file(self.filename)

            f = _segyio.open("small.sgy", "r+")

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
        metrics = _segyio.init_metrics(f, binary_header)
        metrics.update(_segyio.init_cube_metrics(f, ilb, xlb,
                                                 metrics['trace_count'],
                                                 metrics['trace0'],
                                                 metrics['trace_bsize']))
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
            metrics = _segyio.init_metrics("?", binary_header)

        with self.assertRaises(TypeError):
            metrics = _segyio.init_metrics(f, "?")

        with self.assertRaises(IndexError):
            metrics = _segyio.init_metrics(f, binary_header)
            metrics.update(_segyio.init_cube_metrics(f, ilb + 1, xlb,
                                                     metrics['trace_count'],
                                                     metrics['trace0'],
                                                     metrics['trace_bsize']))

        metrics = _segyio.init_metrics(f, binary_header)
        metrics.update(_segyio.init_cube_metrics(f, ilb, xlb,
                                                 metrics['trace_count'],
                                                 metrics['trace0'],
                                                 metrics['trace_bsize']))

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
            metrics = _segyio.init_metrics(f, binary_header)

    def test_indices(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193
        metrics = _segyio.init_metrics(f, binary_header)
        dmy = numpy.zeros(2, dtype=numpy.intc)

        dummy_metrics = {'xline_count':   2,
                         'iline_count':   2,
                         'offset_count':  1}

        with self.assertRaises(TypeError):
            _segyio.init_indices(".", {}, dmy, dmy, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_indices(f, "-", dmy, dmy, dmy)

        # with self.assertRaises(KeyError):
        #     _segyio.init_indices(f, {}, dmy, dmy, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_indices(f, dummy_metrics, 1, dmy, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_indices(f, dummy_metrics, dmy, 2, dmy)

        with self.assertRaises(TypeError):
            _segyio.init_indices(f, dummy_metrics, dmy, dmy, 2)

        with self.assertRaises(TypeError):
            fdmy = numpy.zeros(1, dtype=numpy.single)
            _segyio.init_indices(f, dummy_metrics, fdmy, dmy, dmy)

        one = numpy.zeros(1, dtype=numpy.intc)
        two = numpy.zeros(2, dtype=numpy.intc)
        off = numpy.zeros(1, dtype=numpy.intc)
        with self.assertRaises(ValueError):
            _segyio.init_indices(f, dummy_metrics, one, two, off)

        with self.assertRaises(ValueError):
            _segyio.init_indices(f, dummy_metrics, two, one, off)

        metrics.update(_segyio.init_cube_metrics(f, ilb, xlb,
                                                 metrics['trace_count'],
                                                 metrics['trace0'],
                                                 metrics['trace_bsize']))

        # Happy Path
        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
        offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
        _segyio.init_indices(f, metrics, iline_indexes, xline_indexes, offsets)

        self.assertListEqual([1, 2, 3, 4, 5], list(iline_indexes))
        self.assertListEqual([20, 21, 22, 23, 24], list(xline_indexes))
        self.assertListEqual([1], list(offsets))

        _segyio.close(f)

    def test_fread_trace0(self):
        f = _segyio.open(self.filename, "r")

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193

        metrics = _segyio.init_metrics(f, binary_header)
        metrics.update(_segyio.init_cube_metrics(f, ilb, xlb,
                                                 metrics['trace_count'],
                                                 metrics['trace0'],
                                                 metrics['trace_bsize']))

        sorting = metrics['sorting']
        trace_count = metrics['trace_count']
        inline_count = metrics['iline_count']
        crossline_count = metrics['xline_count']
        offset_count = metrics['offset_count']

        line_metrics = _segyio.init_line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
        offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
        _segyio.init_indices(f, metrics, iline_indexes, xline_indexes, offsets)

        with self.assertRaises(KeyError):
            _segyio.fread_trace0(0, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")

        with self.assertRaises(KeyError):
            _segyio.fread_trace0(2, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")

        value = _segyio.fread_trace0(1, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")
        self.assertEqual(value, 0)

        value = _segyio.fread_trace0(2, len(xline_indexes), line_metrics['iline_stride'], offset_count, iline_indexes, "inline")
        self.assertEqual(value, 5)

        value = _segyio.fread_trace0(21, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")
        self.assertEqual(value, 1)

        value = _segyio.fread_trace0(22, len(iline_indexes), line_metrics['xline_stride'], offset_count, xline_indexes, "crossline")
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
        with TestContext("read_and_write_trace_header") as context:
            context.copy_file(self.filename)

            f = _segyio.open("small.sgy", "r+")
            binary_header = _segyio.read_binaryheader(f)
            ilb = 189
            xlb = 193
            metrics = _segyio.init_metrics(f, binary_header)

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
        with TestContext("read_and_write_trace") as context:
            f = _segyio.open("trace-wrt.sgy", "w+")

            buf = numpy.ones(25, dtype=numpy.single)
            buf[11] = 3.1415
            _segyio.write_trace(f, 0, buf, 0, 100, 1, 25)
            buf[:] = 42.0
            _segyio.write_trace(f, 1, buf, 0, 100, 1, 25)

            _segyio.flush(f)

            buf = numpy.zeros(25, dtype=numpy.single)

            _segyio.read_trace(f, buf, 0, 1, 1, 1, 25, 0, 100)

            self.assertAlmostEqual(buf[10], 1.0, places=4)
            self.assertAlmostEqual(buf[11], 3.1415, places=4)

            _segyio.read_trace(f, buf, 1, 1, 1, 1, 25, 0, 100)

            self.assertAlmostEqual(sum(buf), 42.0 * 25, places=4)

            _segyio.close(f)

    def read_small(self, mmap = False):
        f = _segyio.open(self.filename, "r")

        if mmap: _segyio.mmap(f)

        binary_header = _segyio.read_binaryheader(f)
        ilb = 189
        xlb = 193

        metrics = _segyio.init_metrics(f, binary_header)
        metrics.update(_segyio.init_cube_metrics(f, ilb, xlb,
                                                 metrics['trace_count'],
                                                 metrics['trace0'],
                                                 metrics['trace_bsize']))

        sorting = metrics['sorting']
        trace_count = metrics['trace_count']
        inline_count = metrics['iline_count']
        crossline_count = metrics['xline_count']
        offset_count = metrics['offset_count']

        line_metrics = _segyio.init_line_metrics(sorting, trace_count, inline_count, crossline_count, offset_count)

        metrics.update(line_metrics)

        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
        offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
        _segyio.init_indices(f, metrics, iline_indexes, xline_indexes, offsets)

        return f, metrics, iline_indexes, xline_indexes

    def test_read_line(self):
        f, metrics, iline_idx, xline_idx = self.read_small()

        tr0 = metrics['trace0']
        bsz = metrics['trace_bsize']
        samples = metrics['sample_count']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']
        offsets = metrics['offset_count']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, offsets, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, offsets, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        _segyio.read_line(f, xline_trace0, len(iline_idx), xline_stride, offsets, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        _segyio.read_line(f, iline_trace0, len(xline_idx), iline_stride, offsets, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        _segyio.close(f)

    def test_read_line_mmap(self):
        f, metrics, iline_idx, xline_idx = self.read_small(True)

        tr0 = metrics['trace0']
        bsz = metrics['trace_bsize']
        samples = metrics['sample_count']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']
        offsets = metrics['offset_count']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, offsets, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, offsets, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        _segyio.read_line(f, xline_trace0, len(iline_idx), xline_stride, offsets, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        _segyio.read_line(f, iline_trace0, len(xline_idx), iline_stride, offsets, buf, tr0, bsz, 1, samples)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        _segyio.close(f)

    def test_fread_trace0_for_depth(self):
        elements = list(range(25))
        indices = numpy.asarray(elements, dtype=numpy.intc)

        for index in indices:
            d = _segyio.fread_trace0(index, 1, 1, 1, indices, "depth")
            self.assertEqual(d, index)

        with self.assertRaises(KeyError):
            d = _segyio.fread_trace0(25, 1, 1, 1, indices, "depth")

if __name__ == '__main__':
    unittest.main()
