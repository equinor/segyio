from unittest import TestCase
import segyio
from segyview import util
import itertools
from test_segy import TestSegy


class TestSegyView(TestCase):

    def setUp(self):
        pass

    def test_read_traces_to_memory_and_verify_iline_xline_dimensions(self):

        filename = "test.segy"
        samples = 2

        number_of_ilines = 2
        number_of_xlines = 5
        TestSegy.make_file(filename, samples, 0, number_of_ilines, 10, 10 + number_of_xlines)

        with segyio.open(filename, "r") as segy:
            depth_slices, min_max = util.read_traces_to_memory(segy)

            self.assertTrue(len(segy.ilines) == number_of_ilines == len(depth_slices[0][:]),
                            " [iline] dimensions are not as expected")
            self.assertTrue(len(segy.xlines) == number_of_xlines == len(depth_slices[0][0]),
                            " [xline] dimensions are not as expected")

    def test_read_traces_to_memory_and_verify_cube_rotation(self):

        filename = "test.segy"
        samples = 10

        number_of_ilines = 2
        number_of_xlines = 5
        TestSegy.make_file(filename, samples, 0, number_of_ilines, 10, 10+number_of_xlines)

        with segyio.open(filename, "r") as segy:
            depth_slices, min_max = util.read_traces_to_memory(segy)

            for i, slice in enumerate(depth_slices):
                for ilno, xlno in itertools.product(range(len(segy.ilines)), range(len(segy.xlines))):
                    self.assertEqual(TestSegy.depth_sample(slice[ilno, xlno]), i,
                                     "slice[{0},{1}] == {2}, should be {3}"
                                     .format(ilno, xlno, TestSegy.depth_sample(slice[ilno, xlno]), i))







