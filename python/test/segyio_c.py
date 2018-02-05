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
        self.assertEqual(400, _segyio.binsize())

    def test_textheader_size(self):
        self.assertEqual(3200, _segyio.textsize())

    def test_open_non_existing_file(self):
        with self.assertRaises(IOError):
            _ = _segyio.segyiofd("non-existing", "r")

    def test_close_non_existing_file(self):
        with self.assertRaises(TypeError):
            _segyio.segyiofd.close(None)

    def test_open_and_close_file(self):
        f = _segyio.segyiofd(self.filename, "r")
        f.close()

    def test_open_flush_and_close_file(self):
        f = _segyio.segyiofd(self.filename, "r")
        f.flush()
        f.close()

        with self.assertRaises(IOError):
            f.flush()

    def test_read_text_header(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

        self.assertEqual(f.gettext(0), self.ACTUAL_TEXT_HEADER)

        with self.assertRaises(Exception):
            _segyio.read_texthdr(None, 0)

        f.close()

    def test_read_text_header_mmap(self):
        self.test_read_text_header(True)

    def test_write_text_header(self, mmap=False):
        with TestContext("write_text_header") as context:
            context.copy_file(self.filename)

            f = _segyio.segyiofd("small.sgy", "r+")
            if mmap: f.mmap()

            f.puttext(0, "")

            textheader = f.gettext(0)
            textheader = textheader.decode('ascii')
            self.assertEqual(textheader, "" * 3200)

            f.puttext(0, "yolo" * 800)

            textheader = f.gettext(0)
            textheader = textheader.decode('ascii')  # Because in Python 3.5 bytes are not comparable to strings
            self.assertEqual(textheader, "yolo" * 800)

            f.close()

    def test_write_text_header_mmap(self):
        self.test_write_text_header(True)

    def test_read_and_write_binary_header(self, mmap=False):
        with TestContext("read_and_write_bin_header") as context:
            context.copy_file(self.filename)

            f = _segyio.segyiofd("small.sgy", "r+")
            if mmap: f.mmap()

            binary_header = f.getbin()

            with self.assertRaises(ValueError):
                f.putbin("Buffer too small")

            f.putbin(binary_header)
            f.close()

    def test_read_and_write_binary_header_mmap(self):
        self.test_read_and_write_binary_header(True)

    def test_read_binary_header_fields(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

        binary_header = f.getbin()

        with self.assertRaises(TypeError):
            _ = _segyio.getfield([], 0)

        with self.assertRaises(IndexError):
            _ = _segyio.getfield(binary_header, -1)

        self.assertEqual(_segyio.getfield(binary_header, 3225), 1)
        self.assertEqual(_segyio.getfield(binary_header, 3221), 50)

        f.close()

    def test_read_binary_header_fields_mmap(self):
        self.test_read_binary_header_fields(True)

    def test_line_metrics(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

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

        self.assertEqual(metrics['xline_length'], 5)
        self.assertEqual(metrics['xline_stride'], 5)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 1)

        # (sorting, trace_count, inline_count, crossline_count, offset_count)
        metrics = _segyio.line_metrics(1, 15, 3, 5, 1)

        self.assertEqual(metrics['xline_length'], 3)
        self.assertEqual(metrics['xline_stride'], 1)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 3)

        metrics = _segyio.line_metrics(2, 15, 3, 5, 1)

        self.assertEqual(metrics['xline_length'], 3)
        self.assertEqual(metrics['xline_stride'], 5)
        self.assertEqual(metrics['iline_length'], 5)
        self.assertEqual(metrics['iline_stride'], 1)

    def test_line_metrics_mmap(self):
        self.test_line_metrics(True)

    def test_metrics(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

        ilb = 189
        xlb = 193

        with self.assertRaises(IndexError):
            metrics = f.metrics()
            metrics.update(f.cube_metrics(ilb + 1, xlb))

        metrics = f.metrics()
        metrics.update(f.cube_metrics(ilb, xlb))

        self.assertEqual(metrics['trace0'], _segyio.textsize() + _segyio.binsize())
        self.assertEqual(metrics['samplecount'], 50)
        self.assertEqual(metrics['format'], 1)
        self.assertEqual(metrics['trace_bsize'], 200)
        self.assertEqual(metrics['sorting'], 2) # inline sorting = 2, crossline sorting = 1
        self.assertEqual(metrics['tracecount'], 25)
        self.assertEqual(metrics['offset_count'], 1)
        self.assertEqual(metrics['iline_count'], 5)
        self.assertEqual(metrics['xline_count'], 5)

        f.close()

    def test_metrics_mmap(self):
        self.test_metrics(True)

    def test_indices(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

        ilb = 189
        xlb = 193
        metrics = f.metrics()
        dmy = numpy.zeros(2, dtype=numpy.intc)

        dummy_metrics = {'xline_count':   2,
                         'iline_count':   2,
                         'offset_count':  1}

        with self.assertRaises(TypeError):
            f.indices("-", dmy, dmy, dmy)

        with self.assertRaises(TypeError):
            f.indices(dummy_metrics, 1, dmy, dmy)

        with self.assertRaises(TypeError):
            f.indices(dummy_metrics, dmy, 2, dmy)

        with self.assertRaises(TypeError):
            f.indices(dummy_metrics, dmy, dmy, 2)

        one = numpy.zeros(1, dtype=numpy.intc)
        two = numpy.zeros(2, dtype=numpy.intc)
        off = numpy.zeros(1, dtype=numpy.intc)
        with self.assertRaises(ValueError):
            f.indices(dummy_metrics, one, two, off)

        with self.assertRaises(ValueError):
            f.indices(dummy_metrics, two, one, off)

        metrics.update(f.cube_metrics(ilb, xlb))

        # Happy Path
        iline_indexes = numpy.zeros(metrics['iline_count'], dtype=numpy.intc)
        xline_indexes = numpy.zeros(metrics['xline_count'], dtype=numpy.intc)
        offsets       = numpy.zeros(metrics['offset_count'], dtype=numpy.intc)
        f.indices(metrics, iline_indexes, xline_indexes, offsets)

        self.assertListEqual([1, 2, 3, 4, 5], list(iline_indexes))
        self.assertListEqual([20, 21, 22, 23, 24], list(xline_indexes))
        self.assertListEqual([1], list(offsets))

        f.close()

    def test_indices_mmap(self):
        self.test_indices(True)

    def test_fread_trace0(self, mmap=False):
        f = _segyio.segyiofd(self.filename, "r")
        if mmap: f.mmap()

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

        f.close()

    def test_fread_trace0_mmap(self):
        self.test_fread_trace0(True)

    def test_get_and_putfield(self):
        hdr = bytearray(_segyio.thsize())

        with self.assertRaises(BufferError):
            _segyio.getfield(".", 0)

        with self.assertRaises(TypeError):
            _segyio.getfield([], 0)

        with self.assertRaises(TypeError):
            _segyio.putfield({}, 0, 1)

        with self.assertRaises(IndexError):
            _segyio.getfield(hdr, 0)

        with self.assertRaises(IndexError):
            _segyio.putfield(hdr, 0, 1)

        _segyio.putfield(hdr, 1, 127)
        _segyio.putfield(hdr, 5, 67)
        _segyio.putfield(hdr, 9, 19)

        self.assertEqual(_segyio.getfield(hdr, 1), 127)
        self.assertEqual(_segyio.getfield(hdr, 5), 67)
        self.assertEqual(_segyio.getfield(hdr, 9), 19)

    def test_read_and_write_traceheader(self, mmap=False):
        with TestContext("read_and_write_trace_header") as context:
            context.copy_file(self.filename)

            f = _segyio.segyiofd("small.sgy", "r+")
            if mmap: f.mmap()

            ilb = 189
            xlb = 193

            def mkempty():
                return bytearray(_segyio.thsize())

            with self.assertRaises(TypeError):
                trace_header = f.getth("+")

            with self.assertRaises(TypeError):
                trace_header = f.getth(0, None)

            trace_header = f.getth(0, mkempty())

            self.assertEqual(_segyio.getfield(trace_header, ilb), 1)
            self.assertEqual(_segyio.getfield(trace_header, xlb), 20)

            trace_header = f.getth(1, mkempty())

            self.assertEqual(_segyio.getfield(trace_header, ilb), 1)
            self.assertEqual(_segyio.getfield(trace_header, xlb), 21)

            _segyio.putfield(trace_header, ilb, 99)
            _segyio.putfield(trace_header, xlb, 42)

            f.putth(0, trace_header)

            trace_header = f.getth(0, mkempty())

            self.assertEqual(_segyio.getfield(trace_header, ilb), 99)
            self.assertEqual(_segyio.getfield(trace_header, xlb), 42)

            f.close()

    def test_read_and_write_traceheader_mmap(self):
        self.test_read_and_write_traceheader(True)

    def test_read_and_write_trace(self, mmap=False):
        with TestContext("read_and_write_trace"):
            binary = bytearray(_segyio.binsize())
            _segyio.putfield(binary, 3213, 100)
            _segyio.putfield(binary, 3221, 25)
            f = _segyio.segyiofd("trace-wrt.sgy", "w+", binary)
            if mmap: f.mmap()

            buf = numpy.ones(25, dtype=numpy.single)
            buf[11] = 3.1415
            f.puttr(0, buf)
            buf[:] = 42.0
            f.puttr(1, buf)

            f.flush()

            buf = numpy.zeros(25, dtype=numpy.single)

            f.gettr(buf, 0, 1, 1)

            self.assertAlmostEqual(buf[10], 1.0, places=4)
            self.assertAlmostEqual(buf[11], 3.1415, places=4)

            f.gettr(buf, 1, 1, 1)

            self.assertAlmostEqual(sum(buf), 42.0 * 25, places=4)

            f.close()

    def test_read_and_write_trace_mmap(self):
        self.test_read_and_write_trace(True)

    def read_small(self, mmap = False):
        f = _segyio.segyiofd(self.filename, "r")

        if mmap: f.mmap()

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

    def test_read_line(self):
        f, metrics, iline_idx, xline_idx = self.read_small()

        tr0 = metrics['trace0']
        samples = metrics['samplecount']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']
        offsets = metrics['offset_count']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, offsets, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, offsets, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        f.getline(xline_trace0, len(iline_idx), xline_stride, offsets, buf)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        f.getline(iline_trace0, len(xline_idx), iline_stride, offsets, buf)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        f.close()

    def test_read_line_mmap(self):
        f, metrics, iline_idx, xline_idx = self.read_small(True)

        tr0 = metrics['trace0']
        samples = metrics['samplecount']
        xline_stride = metrics['xline_stride']
        iline_stride = metrics['iline_stride']
        offsets = metrics['offset_count']

        xline_trace0 = _segyio.fread_trace0(20, len(iline_idx), xline_stride, offsets, xline_idx, "crossline")
        iline_trace0 = _segyio.fread_trace0(1, len(xline_idx), iline_stride, offsets, iline_idx, "inline")

        buf = numpy.zeros((len(iline_idx), samples), dtype=numpy.single)

        f.getline(xline_trace0, len(iline_idx), xline_stride, offsets, buf)
        self.assertAlmostEqual(sum(sum(buf)), 800.061169624, places=6)

        f.getline(iline_trace0, len(xline_idx), iline_stride, offsets, buf)
        self.assertAlmostEqual(sum(sum(buf)), 305.061146736, places=6)

        f.close()

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
