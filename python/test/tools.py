from __future__ import absolute_import
from __future__ import division
import unittest

import math
from segyio import BinField
from segyio import TraceField
import numpy as np
from .test_context import TestContext
import segyio


class ToolsTest(unittest.TestCase):
    def setUp(self):
        self.filename = "test-data/small.sgy"
        self.prestack = "test-data/small-ps.sgy"

    def test_dt_fallback(self):
        with TestContext("dt_fallback") as context:
            context.copy_file(self.filename)
            with segyio.open("small.sgy", "r+") as f:
                # Both zero
                f.bin[BinField.Interval] = 0
                f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = 0
                f.flush()
                fallback_dt = 4
                np.testing.assert_almost_equal(segyio.dt(f, fallback_dt), fallback_dt)

                # dt in bin header different from first trace
                f.bin[BinField.Interval] = 6000
                f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = 1000
                f.flush()
                fallback_dt = 4
                np.testing.assert_almost_equal(segyio.dt(f, fallback_dt), fallback_dt)

    def test_dt_no_fallback(self):
        with TestContext("dt_no_fallback") as context:
            context.copy_file(self.filename)
            dt_us = 6000
            with segyio.open("small.sgy", "r+") as f:
                f.bin[BinField.Interval] = dt_us
                f.header[0][TraceField.TRACE_SAMPLE_INTERVAL] = dt_us
                f.flush()
                np.testing.assert_almost_equal(segyio.dt(f), dt_us)

    def test_sample_indexes(self):
        with segyio.open(self.filename, "r") as f:
            indexes = segyio.sample_indexes(f)
            step = 4000.0
            self.assertListEqual(indexes, [t * step for t in range(len(f.samples))])

            indexes = segyio.sample_indexes(f, t0=1.5)
            self.assertListEqual(indexes, [1.5 + t * step for t in range(len(f.samples))])

            indexes = segyio.sample_indexes(f, t0=1.5, dt_override=3.21)
            self.assertListEqual(indexes, [1.5 + t * 3.21 for t in range(len(f.samples))])

    def test_empty_text_header_creation(self):
        text_header = segyio.create_text_header({})

        for line_no in range(0, 40):
            line = text_header[line_no * 80: (line_no + 1) * 80]
            self.assertEqual(line, "C{0:>2} {1:76}".format(line_no + 1, ""))

    def test_wrap(self):
        with segyio.open(self.filename, "r") as f:
            segyio.tools.wrap(f.text[0])
            segyio.tools.wrap(f.text[0], 90)

    def test_values_text_header_creation(self):
        lines = {i + 1: chr(64 + i) * 76 for i in range(40)}
        text_header = segyio.create_text_header(lines)

        for line_no in range(0, 40):
            line = text_header[line_no * 80: (line_no + 1) * 80]
            self.assertEqual(line, "C{0:>2} {1:76}".format(line_no + 1, chr(64 + line_no) * 76))

    def test_native(self):
        with open(self.filename, 'rb') as f, segyio.open(self.filename) as sgy:
            f.read(3600+240)
            filetr = f.read(4 * len(sgy.samples))
            segytr = sgy.trace[0]

            filetr = np.frombuffer(filetr, dtype = np.single)
            self.assertFalse(np.array_equal(segytr, filetr))
            self.assertTrue(np.array_equal(segytr, segyio.tools.native(filetr)))

    def test_cube_filename(self):
        with segyio.open(self.filename) as f:
            c1 = segyio.tools.cube(f)
            c2 = segyio.tools.cube(self.filename)
            self.assertTrue(np.all(c1 == c2))

    def test_cube_identity(self):
        with segyio.open(self.filename) as f:
            x = segyio.tools.collect(f.trace[:])
            x = x.reshape((len(f.ilines), len(f.xlines), len(f.samples)))
            self.assertTrue(np.all(x == segyio.tools.cube(f)))

    def test_cube_identity_prestack(self):
        with segyio.open(self.prestack) as f:
            dims = (len(f.ilines), len(f.xlines), len(f.offsets), len(f.samples))
            x = segyio.tools.collect(f.trace[:]).reshape(dims)
            self.assertTrue(np.all(x == segyio.tools.cube(f)))

    def test_unstructured_rotation(self):
        with self.assertRaises(ValueError):
            with segyio.open(self.filename, ignore_geometry = True) as f:
                segyio.tools.rotation(f)

    def test_rotation(self):
        names = ['normal', 'acute', 'right', 'obtuse',
                 'straight', 'reflex', 'left', 'inv-acute']
        angles = [0.000, 0.785, 1.571, 2.356,
                  3.142, 3.927, 4.712, 5.498]
        rights = [1.571, 2.356, 3.142, 3.927,
                  4.712, 5.498, 0.000, 0.785 ]

        def rotation(x, **kwargs): return segyio.tools.rotation(x, **kwargs)[0]
        close = self.assertAlmostEqual

        for name, angle, right in zip(names, angles, rights):
            src = self.filename.replace('/', '/' + name + '-')

            with segyio.open(src) as f:
                close(angle, rotation(f), places = 3)
                close(angle, rotation(f, line = 'fast'),  places = 3)
                close(angle, rotation(f, line = 'iline'), places = 3)
                close(right, rotation(f, line = 'slow'),  places = 3)
                close(right, rotation(f, line = 'xline'), places = 3)

if __name__ == '__main__':
    unittest.main()
