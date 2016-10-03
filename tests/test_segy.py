import numpy as np
from unittest import TestCase
import segyio
from segyio import TraceField, BinField
import shutil
import filecmp
import itertools


class TestSegy(TestCase):

    def setUp(self):
        self.filename = "test-data/small.sgy"

    def test_inline_4(self):
        with segyio.open(self.filename, "r") as f:

            sample_count = f.samples
            self.assertEqual(50, sample_count)

            data = f.iline[4]

            self.assertAlmostEqual(4.2, data[0, 0], places = 6)
            # middle sample
            self.assertAlmostEqual(4.20024, data[0, sample_count/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.20049, data[0, -1], places = 6)

            # middle xline
            middle_line = 2
            # first sample
            self.assertAlmostEqual(4.22, data[middle_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(4.22024, data[middle_line, sample_count/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.22049, data[middle_line, -1], places = 6)

            # last xline
            last_line = (len(f.xlines)-1)
            # first sample
            self.assertAlmostEqual(4.24, data[last_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(4.24024, data[last_line, sample_count/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.24049, data[last_line, sample_count-1], places = 6)

    def test_xline_22(self):
        with segyio.open(self.filename, "r") as f:

            data = f.xline[22]

            # first iline
            # first sample
            self.assertAlmostEqual(1.22, data[0, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(1.22024, data[0, f.samples/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(1.22049, data[0, f.samples-1], places = 6)

            # middle iline
            middle_line = 2
            # first sample
            self.assertAlmostEqual(3.22, data[middle_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(3.22024, data[middle_line, f.samples/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(3.22049, data[middle_line, f.samples-1], places = 6)

            # last iline
            last_line = len(f.ilines)-1
            # first sample
            self.assertAlmostEqual(5.22, data[last_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(5.22024, data[last_line, f.samples/2-1], places = 6)
            # last sample
            self.assertAlmostEqual(5.22049, data[last_line, f.samples-1], places = 6)

    @staticmethod
    def make_file(filename, samples, first_iline, last_iline, first_xline, last_xline):

        spec = segyio.spec()
        # to create a file from nothing, we need to tell segyio about the structure of
        # the file, i.e. its inline numbers, crossline numbers, etc. You can also add
        # more structural information, but offsets etc. have sensible defaults. This is
        # the absolute minimal specification for a N-by-M volume
        spec.sorting = 2
        spec.format = 1
        spec.samples = samples
        spec.ilines = range(*map(int, [first_iline, last_iline]))
        spec.xlines = range(*map(int, [first_xline, last_xline]))

        with segyio.create(filename, spec) as f:
            start = 0.0
            step = 0.00001
            # fill a trace with predictable values: left-of-comma is the inline
            # number. Immediately right of comma is the crossline number
            # the rightmost digits is the index of the sample in that trace meaning
            # looking up an inline's i's jth crosslines' k should be roughly equal
            # to i.j0k
            trace = np.arange(start = start,
                              stop  = start + step * spec.samples,
                              step  = step,
                              dtype = np.float32)

            # one inline is N traces concatenated. We fill in the xline number
            line = np.concatenate([trace + (xl / 100.0) for xl in spec.xlines])

            # write the line itself to the file
            # write the inline number in all this line's headers
            for ilno in spec.ilines:
                f.iline[ilno] = (line + ilno)
                f.header.iline[ilno] = { segyio.TraceField.INLINE_3D: ilno,
                                         segyio.TraceField.offset: 1
                                         }

            # then do the same for xlines
            for xlno in spec.xlines:
                f.header.xline[xlno] = { segyio.TraceField.CROSSLINE_3D: xlno }

    @staticmethod
    def il_sample(s):
        return int(s)

    @staticmethod
    def xl_sample(s):
        return int(round((s-int(s))*100))

    @classmethod
    def depth_sample(cls, s):
        return int(round((s - cls.il_sample(s) - cls.xl_sample(s)/100.0)*10e2,2)*100)

    def test_make_file(self):
        filename = "test.segy"
        samples = 10
        self.make_file(filename, samples, 0, 2, 10, 13)

        with segyio.open(filename, "r") as f:
            for xlno, xl in itertools.izip(f.xlines, f.xline):
                for ilno, trace in itertools.izip(f.ilines, xl):
                    for sample_index, sample in itertools.izip(range(samples), trace):
                        self.assertEqual(self.il_sample(sample), ilno,
                                         ("sample: {0}, ilno {1}".format(self.il_sample(sample), ilno)))
                        self.assertEqual(self.xl_sample(sample), xlno,
                                         ("sample: {0}, xlno {1}, sample {2}".format(
                                             self.xl_sample(sample), xlno, sample)))
                        self.assertEqual(self.depth_sample(sample), sample_index,
                                         ("sample: {0}, sample_index {1}, real_sample {2}".format(
                                             self.depth_sample(sample), sample_index, sample)))

    def test_read_all_depth_planes(self):
        filename = "test.segy"
        samples = 10
        self.make_file(filename, samples, 0, 2, 10, 13)

        with segyio.open(filename, "r") as f:
            for i, plane in enumerate(f.depth_plane):
                for ilno, xlno in itertools.product(range(len(f.ilines)), range(len(f.xlines))):
                    self.assertEqual(self.depth_sample(plane[xlno, ilno]), i,
                                     "plane[{0},{1}] == {2}, should be 0".format(
                                         ilno, xlno, self.depth_sample(plane[xlno, ilno])))

    def test_iline_slicing(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline))
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline[1:6]))
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline[5:0:-1]))
            self.assertEqual(len(f.ilines) / 2, sum(1 for _ in f.iline[0::2]))
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline[1:]))
            self.assertEqual(3,  sum(1 for _ in f.iline[::2]))
            self.assertEqual(0,  sum(1 for _ in f.iline[12:24]))
            self.assertEqual(3,  sum(1 for _ in f.iline[:4]))
            self.assertEqual(2,  sum(1 for _ in f.iline[2:6:2]))

    def test_xline_slicing(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(len(f.xlines), sum(1 for _ in f.xline))
            self.assertEqual(len(f.xlines), sum(1 for _ in f.xline[20:25]))
            self.assertEqual(len(f.xlines), sum(1 for _ in f.xline[25:19:-1]))
            self.assertEqual(3, sum(1 for _ in f.xline[0::2]))
            self.assertEqual(3, sum(1 for _ in f.xline[::2]))
            self.assertEqual(len(f.xlines), sum(1 for _ in f.xline[20:]))
            self.assertEqual(0,  sum(1 for _ in f.xline[12:18]))
            self.assertEqual(5,  sum(1 for _ in f.xline[:25]))
            self.assertEqual(2,  sum(1 for _ in f.xline[:25:3]))

    def test_open_transposed_lines(self):
        il, xl = [], []
        with segyio.open(self.filename, "r") as f:
            il = f.ilines
            xl = f.xlines

        with segyio.open(self.filename, "r", segyio.TraceField.CROSSLINE_3D, segyio.TraceField.INLINE_3D) as f:
            self.assertEqual(il, f.xlines)
            self.assertEqual(xl, f.ilines)
            pass


    def test_file_info(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(2, f.sorting)
            self.assertEqual(1, f.offsets)
            self.assertEqual(1, int(f.format))

            xlines = list(xrange(20, 25))
            ilines = list(xrange(1, 6))
            self.assertEqual(xlines, f.xlines)
            self.assertEqual(ilines, f.ilines)
            self.assertEqual(25, f.tracecount)
            self.assertEqual(len(f.trace), f.tracecount)
            self.assertEqual(50, f.samples)

    def native_conversion(self):
        arr1 = np.random.rand(10, dtype=np.float32)
        arr2 = np.copy(arr1)
        self.assertTrue(np.array_equal(arr1, arr2))

        # round-trip should not modify data
        segyio.file._from_native(1, f.samples, segyio.asfloatp(arr1))
        self.assertFalse(np.array_equal(arr1, arr2))
        segyio.file._to_native(1, f.samples, segyio.asfloatp(arr1))
        self.assertTrue(np.array_equal(arr1, arr2))

    def test_traces_slicing(self):
        with segyio.open(self.filename, "r") as f:

            traces = map(np.copy, f.trace[0:6:2])
            self.assertEqual(len(traces), 3)
            self.assertEqual(traces[0][49], f.trace[0][49])
            self.assertEqual(traces[1][49], f.trace[2][49])
            self.assertEqual(traces[2][49], f.trace[4][49])

            rev_traces = map(np.copy, f.trace[4::-2])
            self.assertEqual(rev_traces[0][49], f.trace[4][49])
            self.assertEqual(rev_traces[1][49], f.trace[2][49])
            self.assertEqual(rev_traces[2][49], f.trace[0][49])

            # make sure buffers can be reused
            buf = None
            for i, trace in enumerate(f.trace[0:6:2, buf]):
                self.assertTrue(np.array_equal(trace, traces[i]))

    def test_line_generators(self):
        with segyio.open(self.filename, "r") as f:
            for line in f.iline:
                pass

            for line in f.xline:
                pass

    def test_read_header(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(1, f.header[0][189])
            self.assertEqual(1, f.header[1][TraceField.INLINE_3D])

            with self.assertRaises(IndexError):
                f.header[0][188] # between byte offsets

            with self.assertRaises(IndexError):
                f.header[0][-1]

            with self.assertRaises(IndexError):
                f.header[0][700]

    def test_write_header(self):
        fname = self.filename + "write-header"
        shutil.copyfile(self.filename, fname)
        with segyio.open(fname, "r+") as f:
            # assign to a field in a header, write immediately
            f.header[0][189] = 42
            f.flush()

            self.assertEqual(42, f.header[0][189])
            self.assertEqual(1, f.header[1][189])

            # accessing non-existing offsets raises exceptions
            with self.assertRaises(IndexError):
                f.header[0][188] = 1 # between byte offsets

            with self.assertRaises(IndexError):
                f.header[0][-1] = 1

            with self.assertRaises(IndexError):
                f.header[0][700] = 1

            d = { TraceField.INLINE_3D: 43,
                  TraceField.CROSSLINE_3D: 11,
                  TraceField.offset: 15 }

            # assign multiple fields at once by using a dict
            f.header[1] = d

            f.flush()
            self.assertEqual(43, f.header[1][TraceField.INLINE_3D])
            self.assertEqual(11, f.header[1][TraceField.CROSSLINE_3D])
            self.assertEqual(15, f.header[1][TraceField.offset])

            # looking up multiple values at once returns a { TraceField: value } dict
            self.assertEqual(d, f.header[1][TraceField.INLINE_3D, TraceField.CROSSLINE_3D, TraceField.offset])

            # slice-support over headers (similar to trace)
            for th in f.header[0:10]:
                pass

            self.assertEqual(6, len(list(f.header[10::-2])))
            self.assertEqual(5, len(list(f.header[10:5:-1])))
            self.assertEqual(0, len(list(f.header[10:5])))

            # for-each support
            for th in f.header:
                pass

            # copy a header
            f.header[2] = f.header[1]
            f.flush()
            # don't use this interface in production code, it's only for testing
            # i.e. don't access buf of treat it as a list
            self.assertEqual(list(f.header[2].buf), list(f.header[1].buf))

    def test_write_binary(self):
        fname = self.filename.replace( ".sgy", "-binary.sgy")
        shutil.copyfile(self.filename, fname)

        with segyio.open(fname, "r+") as f:
            f.bin[3213] = 5
            f.flush()

            self.assertEqual(5, f.bin[3213])

            # accessing non-existing offsets raises exceptions
            with self.assertRaises(IndexError):
                f.bin[0]

            with self.assertRaises(IndexError):
                f.bin[50000]

            with self.assertRaises(IndexError):
                f.bin[3214]

            d = { BinField.Traces: 43,
                  BinField.SweepFrequencyStart: 11 }

            # assign multiple fields at once by using a dict
            f.bin = d

            f.flush()
            self.assertEqual(43, f.bin[BinField.Traces])
            self.assertEqual(11, f.bin[BinField.SweepFrequencyStart])

            # looking up multiple values at once returns a { TraceField: value } dict
            self.assertEqual(d, f.bin[BinField.Traces, BinField.SweepFrequencyStart])

            # copy a header
            f.bin = f.bin

    def test_fopen_error(self):
        # non-existent file
        with self.assertRaises(OSError):
            segyio.open("no_dir/no_file", "r")

        # non-existant mode
        with self.assertRaises(OSError):
            segyio.open(self.filename, "foo")

    def test_wrong_lineno(self):
        with self.assertRaises(KeyError):
            with segyio.open(self.filename, "r") as f:
                f.iline[3000]

        with self.assertRaises(KeyError):
            with segyio.open(self.filename, "r") as f:
                f.xline[2]

    def test_open_wrong_inline(self):
        with self.assertRaises(ValueError):
            with segyio.open(self.filename, "r", 2) as f:
                pass

    def test_open_wrong_crossline(self):
        with self.assertRaises(ValueError):
            with segyio.open(self.filename, "r", 189, 2) as f:
                pass

    def test_create_sgy(self):
        dstfile = self.filename.replace(".sgy", "-created.sgy")

        with segyio.open(self.filename, "r") as src:
            spec = segyio.spec()
            spec.format     = int(src.format)
            spec.sorting    = int(src.sorting)
            spec.samples    = src.samples
            spec.ilines     = src.ilines
            spec.xlines     = src.xlines

            with segyio.create(dstfile, spec) as dst:
                dst.text[0] = src.text[0]
                dst.bin     = src.bin

                # copy all headers
                dst.header = src.header

                for i, srctr in enumerate(src.trace):
                    dst.trace[i] = srctr

                dst.trace = src.trace

                # this doesn't work yet, some restructuring is necessary
                # if it turns out to be a desired feature it's rather easy to do
                #for dsth, srch in zip(dst.header, src.header):
                #    dsth = srch

                #for dsttr, srctr in zip(dst.trace, src.trace):
                #    dsttr = srctr

        self.assertTrue(filecmp.cmp(self.filename, dstfile))

    def test_create_sgy_shorter_traces(self):
        dstfile = self.filename.replace(".sgy", "-shorter.sgy")

        with segyio.open(self.filename, "r") as src:
            spec = segyio.spec()
            spec.format     = int(src.format)
            spec.sorting    = int(src.sorting)
            spec.samples    = 20 # reduces samples per trace
            spec.ilines     = src.ilines
            spec.xlines     = src.xlines

            with segyio.create(dstfile, spec) as dst:
                for i, srch in enumerate(src.header):
                    dst.header[i] = srch
                    d = { TraceField.INLINE_3D: srch[TraceField.INLINE_3D] + 100 }
                    dst.header[i] = d

                for lineno in dst.ilines:
                    dst.iline[lineno] = src.iline[lineno]

                # alternative form using left-hand-side slices
                dst.iline[2:4] = src.iline


                buf = None # reuse buffer for speed
                for lineno in dst.xlines:
                    dst.xline[lineno] = src.xline[lineno, buf]

            with segyio.open(dstfile, "r") as dst:
                self.assertEqual(20, dst.samples)
                self.assertEqual([x + 100 for x in src.ilines], dst.ilines)

    def test_create_from_naught(self):
        fname = "test-data/mk.sgy"
        spec = segyio.spec()
        spec.format  = 5
        spec.sorting = 2
        spec.samples = 150
        spec.ilines  = range(1, 11)
        spec.xlines  = range(1, 6)

        with segyio.create(fname, spec) as dst:
            tr = np.arange( start = 1.000, stop = 1.151, step = 0.001, dtype = np.float32 )

            for i in xrange( len( dst.trace ) ):
                dst.trace[i] = tr
                tr += 1.000

            for il in spec.ilines:
                dst.header.iline[il] = { TraceField.INLINE_3D: il }

            for xl in spec.xlines:
                dst.header.xline[xl] = { TraceField.CROSSLINE_3D: xl }

            # Set header field 'offset' to 1 in all headers
            dst.header = { TraceField.offset: 1 }

        with segyio.open(fname, "r") as f:
            self.assertAlmostEqual(1,      f.trace[0][0],   places = 4)
            self.assertAlmostEqual(1.001,  f.trace[0][1],   places = 4)
            self.assertAlmostEqual(1.149,  f.trace[0][-1],   places = 4)
            self.assertAlmostEqual(50.100, f.trace[-1][100], places = 4)
            self.assertEqual(f.header[0][TraceField.offset], f.header[1][TraceField.offset])
            self.assertEqual(1, f.header[1][TraceField.offset])

    @staticmethod
    def mklines(fname):
        spec = segyio.spec()
        spec.format  = 5
        spec.sorting = 2
        spec.samples = 10
        spec.ilines  = range(1, 11)
        spec.xlines  = range(1, 6)

# create a file with 10 inlines, with values on the form l.0tv where
# l = line no
# t = trace number (within line)
# v = trace value
# i.e. 2.043 is the value at inline 2's fourth trace's third value
        with segyio.create(fname, spec) as dst:
            ln = np.arange(start = 0,
                           stop = 0.001 * (5 * 10),
                           step = 0.001,
                           dtype = np.float32).reshape(5, 10)

            for il in spec.ilines:
                ln += 1

                dst.header.iline[il] = { TraceField.INLINE_3D: il }
                dst.iline[il] = ln

            for xl in spec.xlines:
                dst.header.xline[xl] = { TraceField.CROSSLINE_3D: xl }

    def test_create_write_lines(self):
        fname = "test-data/mklines.sgy"

        self.mklines(fname)

        with segyio.open(fname, "r") as f:
            self.assertAlmostEqual(1,     f.iline[1][0][0], places = 4)
            self.assertAlmostEqual(2.004, f.iline[2][0][4], places = 4)
            self.assertAlmostEqual(2.014, f.iline[2][1][4], places = 4)
            self.assertAlmostEqual(8.043, f.iline[8][4][3], places = 4)

    def test_create_sgy_skip_lines(self):
        fname = "test-data/lines.sgy"
        dstfile = fname.replace(".sgy", "-halved.sgy")

        self.mklines(fname)

        with segyio.open(fname, "r") as src:
            spec = segyio.spec()
            spec.format     = int(src.format)
            spec.sorting    = int(src.sorting)
            spec.samples    = src.samples
            spec.ilines     = src.ilines[::2]
            spec.xlines     = src.xlines[::2]

            with segyio.create(dstfile, spec) as dst:
                # use the inline headers as base
                dst.header.iline = src.header.iline[::2]
                # then update crossline numbers from the crossline headers
                for xl in dst.xlines:
                    f = next(src.header.xline[xl])[TraceField.CROSSLINE_3D]
                    dst.header.xline[xl] = { TraceField.CROSSLINE_3D: f }

                # but we override the last xline to be 6, not 5
                dst.header.xline[5] = { TraceField.CROSSLINE_3D: 6 }
                dst.iline = src.iline[::2]

        with segyio.open(dstfile, "r") as f:
            self.assertEqual(f.ilines, spec.ilines)
            self.assertEqual(f.xlines, [1,3,6])
            self.assertAlmostEqual(1,     f.iline[1][0][0], places = 4)
            self.assertAlmostEqual(3.004, f.iline[3][0][4], places = 4)
            self.assertAlmostEqual(3.014, f.iline[3][1][4], places = 4)
            self.assertAlmostEqual(7.023, f.iline[7][2][3], places = 4)
