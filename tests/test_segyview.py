from unittest import TestCase
import segyio
from segyview import SegyIOWrapper
import itertools


class TestSegyView(TestCase):

    def setUp(self):
        self.filename = "test-data/small.sgy"

    def test_read_all_traces_to_memory_compare_with_depth_slice_and_verify_cube_rotation(self):


        with segyio.open(self.filename, "r") as segy:
            swrap = SegyIOWrapper.wrap(segy)
            swrap.read_all_traces_to_memory()
            for i, depth_slice in enumerate(swrap.depth_slices):
                for ilno, xlno in itertools.product(range(len(segy.ilines)), range(len(segy.xlines))):

                    self.assertEqual(depth_slice[ilno, xlno], segy.depth_slice[i][ilno, xlno],
                                     "the cube values from read_all_traces and depth_slice differ {0} != {1}"
                                     .format(depth_slice[ilno, xlno], segy.depth_slice[i][ilno, xlno]))











