from unittest import TestCase

from segyio import BinField
from segyio import TraceField
import numpy as np
from test_context import TestContext
import segyio


class ToolsTest(TestCase):
    def setUp(self):
        self.filename = "test-data/small.sgy"

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
                np.testing.assert_almost_equal(segyio.dt(f), dt_us/1000)

    def test_sample_indexes(self):
        with segyio.open(self.filename, "r") as f:
            indexes = segyio.sample_indexes(f)
            self.assertListEqual(indexes, [t * 4.0 for t in range(f.samples)])

            indexes = segyio.sample_indexes(f, t0=1.5)
            self.assertListEqual(indexes, [1.5 + t * 4.0 for t in range(f.samples)])

            indexes = segyio.sample_indexes(f, t0=1.5, dt_override=3.21)
            self.assertListEqual(indexes, [1.5 + t * 3.21 for t in range(f.samples)])

