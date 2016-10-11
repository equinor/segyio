import numpy as np
from unittest import TestCase
import segyio
import segyviewer

from segyio import TraceField, BinField
import shutil
import filecmp
import itertools
from test_segy import TestSegy

class TestSegyViewer(TestCase):

    def setUp(self):
        pass

    def test_read_traces_to_memory_and_verify_cube_rotation(self):
        '''an alternate way of reading in the depth slices by reading all traces and rotating the cube '''

        filename = "test.segy"
        samples = 10
        TestSegy.make_file(filename, samples, 0, 2, 10, 13)

        with segyio.open(filename, "r") as segy:
            depth_slices, min_max = segyviewer.read_traces_to_memory(segy)

            for i, slice in enumerate(depth_slices):
                for ilno, xlno in itertools.product(range(len(segy.ilines)), range(len(segy.xlines))):
                    self.assertEqual(TestSegy.depth_sample(slice[ilno, xlno]), i,
                                     "slice[{0},{1}] == {2}, should be {3}"
                                     .format(ilno, xlno, TestSegy.depth_sample(slice[ilno, xlno]), i))






