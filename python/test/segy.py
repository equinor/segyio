from __future__ import absolute_import

from types import GeneratorType

import itertools
import numpy as np
import unittest
from .test_context import TestContext

import segyio
from segyio import TraceField, BinField
import filecmp

from segyio._field import Field
from segyio._line import Line
from segyio._header import Header
from segyio._trace import Trace

try:
    from itertools import izip as zip
    from itertools import imap as map
except ImportError:  # will be 3.x series
    pass


def mklines(fname):
    spec = segyio.spec()
    spec.format  = 5
    spec.sorting = 2
    spec.samples = range(10)
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
                       dtype = np.single).reshape(5, 10)

        for il in spec.ilines:
            ln += 1

            dst.header.iline[il] = { TraceField.INLINE_3D: il }
            dst.iline[il] = ln

        for xl in spec.xlines:
            dst.header.xline[xl] = { TraceField.CROSSLINE_3D: xl }


class TestSegy(unittest.TestCase):

    def setUp(self):
        self.filename = "test-data/small.sgy"
        self.prestack = "test-data/small-ps.sgy"
        self.fileMx1  = "test-data/Mx1.sgy"
        self.file1xN  = "test-data/1xN.sgy"
        self.file1x1  = "test-data/1x1.sgy"

    def test_inline_4(self):
        with segyio.open(self.filename, "r") as f:

            sample_count = len(f.samples)
            self.assertEqual(50, sample_count)

            data = f.iline[4]

            self.assertAlmostEqual(4.2, data[0, 0], places = 6)
            # middle sample
            self.assertAlmostEqual(4.20024, data[0, sample_count//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.20049, data[0, -1], places = 6)

            # middle xline
            middle_line = 2
            # first sample
            self.assertAlmostEqual(4.22, data[middle_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(4.22024, data[middle_line, sample_count//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.22049, data[middle_line, -1], places = 6)

            # last xline
            last_line = (len(f.xlines)-1)
            # first sample
            self.assertAlmostEqual(4.24, data[last_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(4.24024, data[last_line, sample_count//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(4.24049, data[last_line, sample_count-1], places = 6)

    def test_xline_22(self):
        with segyio.open(self.filename, "r") as f:

            data = f.xline[22]

            size = len(f.samples)

            # first iline
            # first sample
            self.assertAlmostEqual(1.22, data[0, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(1.22024, data[0, size//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(1.22049, data[0, size-1], places = 6)

            # middle iline
            middle_line = 2
            # first sample
            self.assertAlmostEqual(3.22, data[middle_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(3.22024, data[middle_line, size//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(3.22049, data[middle_line, size-1], places = 6)

            # last iline
            last_line = len(f.ilines)-1
            # first sample
            self.assertAlmostEqual(5.22, data[last_line, 0], places = 5)
            # middle sample
            self.assertAlmostEqual(5.22024, data[last_line, size//2-1], places = 6)
            # last sample
            self.assertAlmostEqual(5.22049, data[last_line, size-1], places = 6)

    def test_iline_slicing(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline))
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline[1:6]))
            self.assertEqual(len(f.ilines), sum(1 for _ in f.iline[5:0:-1]))
            self.assertEqual(len(f.ilines) // 2, sum(1 for _ in f.iline[0::2]))
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
            self.assertListEqual(list(il), list(f.xlines))
            self.assertListEqual(list(xl), list(f.ilines))
            pass

    def test_file_info(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(2, f.sorting)
            self.assertEqual(1, f.offsets)
            self.assertEqual(1, int(f.format))

            xlines = list(range(20, 25))
            ilines = list(range(1, 6))
            self.assertEqual(xlines, list(f.xlines))
            self.assertEqual(ilines, list(f.ilines))
            self.assertEqual(25, f.tracecount)
            self.assertEqual(len(f.trace), f.tracecount)
            self.assertEqual(50, len(f.samples))

    def test_open_nostrict(self):
        with segyio.open(self.filename, strict = False):
            pass

    def test_open_ignore_geometry(self):
        with segyio.open(self.filename, ignore_geometry = True) as f:
            with self.assertRaises(ValueError):
                f.iline[0]

    def test_traces_slicing(self):
        with segyio.open(self.filename, "r") as f:

            traces = list(map(np.copy, f.trace[0:6:2]))
            self.assertEqual(len(traces), 3)
            self.assertEqual(traces[0][49], f.trace[0][49])
            self.assertEqual(traces[1][49], f.trace[2][49])
            self.assertEqual(traces[2][49], f.trace[4][49])

            rev_traces = list(map(np.copy, f.trace[4::-2]))
            self.assertEqual(rev_traces[0][49], f.trace[4][49])
            self.assertEqual(rev_traces[1][49], f.trace[2][49])
            self.assertEqual(rev_traces[2][49], f.trace[0][49])

            # make sure buffers can be reused
            buf = None
            for i, trace in enumerate(f.trace[0:6:2, buf]):
                self.assertTrue(np.array_equal(trace, traces[i]))

    def test_traces_offset(self):
        with segyio.open(self.prestack, "r") as f:

            self.assertEqual(2, len(f.offsets))
            self.assertListEqual([1, 2], list(f.offsets))

            # traces are laid out |l1o1 l1o2 l2o1 l2o2...|
            # where l = iline number and o = offset number
            # traces are not re-indexed according to offsets
            # see make-ps-file.py for value formula
            self.assertAlmostEqual(101.01, f.trace[0][0], places = 4)
            self.assertAlmostEqual(201.01, f.trace[1][0], places = 4)
            self.assertAlmostEqual(101.02, f.trace[2][0], places = 4)
            self.assertAlmostEqual(201.02, f.trace[3][0], places = 4)
            self.assertAlmostEqual(102.01, f.trace[6][0], places = 4)

    def test_headers_offset(self):
        with segyio.open(self.prestack, "r") as f:
            il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
            self.assertEqual(f.header[0][il], f.header[1][il])
            self.assertEqual(f.header[1][il], f.header[2][il])

            self.assertEqual(f.header[0][xl], f.header[1][xl])
            self.assertNotEqual(f.header[1][xl], f.header[2][xl])

    def test_header_dict_methods(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(89, len(list(f.header[0].keys())))
            self.assertEqual(89, len(list(f.header[1].values())))
            self.assertEqual(89, len(list(f.header[2].items())))
            self.assertEqual(89, len(list(f.header[3])))
            self.assertTrue(0 not in f.header[0])
            self.assertTrue(1 in f.header[0])
            self.assertTrue(segyio.su.cdpx in f.header[0])
            iter(f.header[0])

            self.assertEqual(30, len(f.bin.keys()))
            self.assertEqual(30, len(list(f.bin.values())))
            self.assertEqual(30, len(list(f.bin.items())))
            self.assertEqual(30, len(f.bin))
            iter(f.bin)

    def test_headers_line_offset(self):
        with TestContext("headers_line_offset") as context:
            context.copy_file(self.prestack)
            il, xl = TraceField.INLINE_3D, TraceField.CROSSLINE_3D
            with segyio.open("small-ps.sgy", "r+") as f:
                f.header.iline[1,2] = { il: 11 }
                f.header.iline[1,2] = { xl: 13 }

            with segyio.open("small-ps.sgy", "r", strict = False) as f:
                self.assertEqual(f.header[0][il], 1)
                self.assertEqual(f.header[1][il], 11)
                self.assertEqual(f.header[2][il], 1)

                self.assertEqual(f.header[0][xl], 1)
                self.assertEqual(f.header[1][xl], 13)
                self.assertEqual(f.header[2][xl], 2)

    def test_attributes(self):
        with segyio.open(self.filename) as f:
            il = TraceField.INLINE_3D
            xl = TraceField.CROSSLINE_3D

            self.assertEqual(1,  f.attributes(il)[0])
            self.assertEqual(20, f.attributes(xl)[0])

            ils = [(i // 5) + 1 for i in range(25)]
            attrils = list(map(int, f.attributes(il)[:]))
            self.assertListEqual(ils, attrils)

            xls = [(i % 5) + 20 for i in range(25)]
            attrxls = list(map(int, f.attributes(xl)[:]))
            self.assertListEqual(xls, attrxls)

            ils = [(i // 5) + 1 for i in range(25)][::-1]
            attrils = list(map(int, f.attributes(il)[::-1]))
            self.assertListEqual(ils, attrils)

            xls = [(i % 5) + 20 for i in range(25)][::-1]
            attrxls = list(map(int, f.attributes(xl)[::-1]))
            self.assertListEqual(xls, attrxls)

            self.assertEqual(f.header[0][il], f.attributes(il)[0])
            f.mmap()
            self.assertEqual(f.header[0][il], f.attributes(il)[0])

            ils = [(i // 5) + 1 for i in range(25)][1:21:3]
            attrils = list(map(int, f.attributes(il)[1:21:3]))
            self.assertListEqual(ils, attrils)

            xls = [(i % 5) + 20 for i in range(25)][2:17:5]
            attrxls = list(map(int, f.attributes(xl)[2:17:5]))
            self.assertListEqual(xls, attrxls)

            ils = [1, 2, 3, 4, 5]
            attrils = list(map(int, f.attributes(il)[[0, 5, 11, 17, 23]]))
            self.assertListEqual(ils, attrils)

            ils = [1, 2, 3, 4, 5]
            indices = np.asarray([0, 5, 11, 17, 23])
            attrils = list(map(int, f.attributes(il)[indices]))
            self.assertListEqual(ils, attrils)

    def test_iline_offset(self):
        with segyio.open(self.prestack, "r") as f:

            line1 = f.iline[1, 1]
            self.assertAlmostEqual(101.01, line1[0][0], places = 4)
            self.assertAlmostEqual(101.02, line1[1][0], places = 4)
            self.assertAlmostEqual(101.03, line1[2][0], places = 4)

            self.assertAlmostEqual(101.01001, line1[0][1], places = 4)
            self.assertAlmostEqual(101.01002, line1[0][2], places = 4)
            self.assertAlmostEqual(101.02001, line1[1][1], places = 4)

            line2 = f.iline[1, 2]
            self.assertAlmostEqual(201.01, line2[0][0], places = 4)
            self.assertAlmostEqual(201.02, line2[1][0], places = 4)
            self.assertAlmostEqual(201.03, line2[2][0], places = 4)

            self.assertAlmostEqual(201.01001, line2[0][1], places = 4)
            self.assertAlmostEqual(201.01002, line2[0][2], places = 4)
            self.assertAlmostEqual(201.02001, line2[1][1], places = 4)

            with self.assertRaises(KeyError):
                f.iline[1, 0]

            with self.assertRaises(KeyError):
                f.iline[1, 3]

            with self.assertRaises(KeyError):
                f.iline[100, 1]

            with self.assertRaises(TypeError):
                f.iline[1, {}]

    def test_iline_slice_fixed_offset(self):
        with segyio.open(self.prestack, "r") as f:

            for i, ln in enumerate(f.iline[:, 1], 1):
                self.assertAlmostEqual(i + 100.01, ln[0][0], places = 4)
                self.assertAlmostEqual(i + 100.02, ln[1][0], places = 4)
                self.assertAlmostEqual(i + 100.03, ln[2][0], places = 4)

                self.assertAlmostEqual(i + 100.01001, ln[0][1], places = 4)
                self.assertAlmostEqual(i + 100.01002, ln[0][2], places = 4)
                self.assertAlmostEqual(i + 100.02001, ln[1][1], places = 4)

    def test_iline_slice_fixed_line(self):
        with segyio.open(self.prestack, "r") as f:

            for i, ln in enumerate(f.iline[1, :], 1):
                off = i * 100
                self.assertAlmostEqual(off + 1.01, ln[0][0], places = 4)
                self.assertAlmostEqual(off + 1.02, ln[1][0], places = 4)
                self.assertAlmostEqual(off + 1.03, ln[2][0], places = 4)

                self.assertAlmostEqual(off + 1.01001, ln[0][1], places = 4)
                self.assertAlmostEqual(off + 1.01002, ln[0][2], places = 4)
                self.assertAlmostEqual(off + 1.02001, ln[1][1], places = 4)

    def test_iline_slice_all_offsets(self):
        with segyio.open(self.prestack, "r") as f:
            offs, ils = len(f.offsets), len(f.ilines)
            self.assertEqual(offs * ils, sum(1 for _ in f.iline[:,:]))
            self.assertEqual(offs * ils, sum(1 for _ in f.iline[:,::-1]))
            self.assertEqual(offs * ils, sum(1 for _ in f.iline[::-1,:]))
            self.assertEqual(offs * ils, sum(1 for _ in f.iline[::-1,::-1]))
            self.assertEqual(0,  sum(1 for _ in f.iline[:, 10:12]))
            self.assertEqual(0,  sum(1 for _ in f.iline[10:12, :]))

            self.assertEqual((offs // 2) * ils, sum(1 for _ in f.iline[::2, :]))
            self.assertEqual(offs * (ils // 2), sum(1 for _ in f.iline[:, ::2]))

            self.assertEqual((offs // 2) * ils, sum(1 for _ in f.iline[::-2, :]))
            self.assertEqual(offs * (ils // 2), sum(1 for _ in f.iline[:, ::-2]))

            self.assertEqual((offs // 2) * (ils // 2), sum(1 for _ in f.iline[::2, ::2]))
            self.assertEqual((offs // 2) * (ils // 2), sum(1 for _ in f.iline[::2, ::-2]))
            self.assertEqual((offs // 2) * (ils // 2), sum(1 for _ in f.iline[::-2, ::2]))


    def test_gather_mode(self):
        with segyio.open(self.prestack) as f:
            empty = np.empty(0, dtype = np.single)
            # should raise
            with self.assertRaises(KeyError):
                self.assertTrue(np.array_equal(empty, f.gather[2, 3, 3]))

            with self.assertRaises(KeyError):
                self.assertTrue(np.array_equal(empty, f.gather[2, 5, 1]))

            with self.assertRaises(KeyError):
                self.assertTrue(np.array_equal(empty, f.gather[5, 2, 1]))

            self.assertTrue(np.array_equal(f.trace[10], f.gather[2, 3, 1]))
            self.assertTrue(np.array_equal(f.trace[11], f.gather[2, 3, 2]))

            traces = segyio.tools.collect(f.trace[10:12])
            gather = f.gather[2,3,:]
            self.assertTrue(np.array_equal(traces, gather))
            self.assertTrue(np.array_equal(traces, f.gather[2,3]))
            self.assertTrue(np.array_equal(empty,  f.gather[2,3,1:0]))
            self.assertTrue(np.array_equal(empty,  f.gather[2,3,3:4]))

            for g, line in zip(f.gather[1:3, 3, 1], f.iline[1:3]):
                self.assertEqual(10, len(g))
                self.assertEqual((10,), g.shape)
                self.assertTrue(np.array_equal(line[2], g))

            for g, line in zip(f.gather[1:3, 3, :], f.iline[1:3]):
                self.assertEqual(2, len(g))
                self.assertEqual((2, 10), g.shape)
                self.assertTrue(np.array_equal(line[2], g[0]))

            for g, line in zip(f.gather[1, 1:3, 1], f.xline[1:3]):
                self.assertEqual(10, len(g))
                self.assertEqual((10,), g.shape)
                self.assertTrue(np.array_equal(line[0], g))

            for g, line in zip(f.gather[1, 1:3, :], f.xline[1:3]):
                self.assertEqual(2, len(g))
                self.assertEqual((2, 10), g.shape)
                self.assertTrue(np.array_equal(line[0], g[0]))


    def test_line_generators(self):
        with segyio.open(self.filename, "r") as f:
            for line in f.iline:
                pass

            for line in f.xline:
                pass

    def test_fast_slow_dimensions(self):
        with segyio.open(self.filename, 'r') as f:
            for iline, fline in zip(f.iline, f.fast):
                self.assertTrue(np.array_equal(iline, fline))

            for xline, sline in zip(f.xline, f.slow):
                self.assertTrue(np.array_equal(iline, fline))

    def test_traces_raw(self):
        with segyio.open(self.filename, "r") as f:
            gen_traces = np.array(list(map( np.copy, f.trace )), dtype = np.single)

            raw_traces = f.trace.raw[:]
            self.assertTrue(np.array_equal(gen_traces, raw_traces))

            self.assertEqual(len(gen_traces), f.tracecount)
            self.assertEqual(len(raw_traces), f.tracecount)

            self.assertEqual(gen_traces[0][49], raw_traces[0][49])
            self.assertEqual(gen_traces[1][49], f.trace.raw[1][49])
            self.assertEqual(gen_traces[2][49], raw_traces[2][49])

            self.assertTrue(np.array_equal(f.trace[10], f.trace.raw[10]))

            for raw, gen in zip(f.trace.raw[::2], f.trace[::2]):
                self.assertTrue(np.array_equal(raw, gen))

            for raw, gen in zip(f.trace.raw[::-1], f.trace[::-1]):
                self.assertTrue(np.array_equal(raw, gen))

    def test_read_header(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(1, f.header[0][189])
            self.assertEqual(1, f.header[1][TraceField.INLINE_3D])
            self.assertEqual(1, f.header[1][segyio.su.iline])
            self.assertEqual(5, f.header[-1][segyio.su.iline])
            self.assertEqual(5, f.header[24][segyio.su.iline])
            self.assertEqual(dict(f.header[-1]), dict(f.header[24]))

            with self.assertRaises(IndexError):
                f.header[30]

            with self.assertRaises(IndexError):
                f.header[-30]

            with self.assertRaises(IndexError):
                f.header[0][188] # between byte offsets

            with self.assertRaises(IndexError):
                f.header[0][-1]

            with self.assertRaises(IndexError):
                f.header[0][700]

    def test_write_header(self):
        with TestContext("write_header") as context:
            context.copy_file(self.filename)
            with segyio.open("small.sgy", "r+") as f:
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
                self.assertEqual(11, f.header[1][segyio.su.xline])
                self.assertEqual(15, f.header[1][segyio.su.offset])

                # looking up multiple values at once returns a { TraceField: value } dict
                self.assertEqual(d, f.header[1][TraceField.INLINE_3D, TraceField.CROSSLINE_3D, TraceField.offset])

                # slice-support over headers (similar to trace)
                for th in f.header[0:10]:
                    pass

                self.assertEqual(6, len(list(f.header[10::-2])))
                self.assertEqual(5, len(list(f.header[10:5:-1])))
                self.assertEqual(0, len(list(f.header[10:5])))

                d = { TraceField.INLINE_3D: 45,
                      TraceField.CROSSLINE_3D: 10,
                      TraceField.offset: 16 }

                # assign multiple values using alternative syntax
                f.header[5].update(d)
                f.flush()
                self.assertEqual(45, f.header[5][TraceField.INLINE_3D])
                self.assertEqual(10, f.header[5][segyio.su.xline])
                self.assertEqual(16, f.header[5][segyio.su.offset])

                # accept anything with a key-value structure
                f.header[5].update( [(segyio.su.ns, 12), (segyio.su.dt, 4)] )
                f.header[5].update( ((segyio.su.muts, 3), (segyio.su.mute, 7)) )

                with self.assertRaises(TypeError):
                    f.header[0].update(10)

                with self.assertRaises(TypeError):
                    f.header[0].update(None)

                with self.assertRaises(ValueError):
                    f.header[0].update('foo')

                f.flush()
                self.assertEqual(12, f.header[5][segyio.su.ns])
                self.assertEqual( 4, f.header[5][segyio.su.dt])
                self.assertEqual( 3, f.header[5][segyio.su.muts])
                self.assertEqual( 7, f.header[5][segyio.su.mute])

                # for-each support
                for th in f.header:
                    pass

                # copy a header
                f.header[2] = f.header[1]
                f.flush()

                # don't use this interface in production code, it's only for testing
                # i.e. don't access buf of treat it as a list
                #self.assertEqual(list(f.header[2].buf), list(f.header[1].buf))

    def test_write_binary(self):
        with TestContext("write_binary") as context:
            context.copy_file(self.filename)
            with segyio.open("small.sgy", "r+") as f:
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
                self.assertEqual(43, f.bin[segyio.su.ntrpr])
                self.assertEqual(11, f.bin[segyio.su.hsfs])

                d = { BinField.Traces: 45,
                      BinField.SweepFrequencyStart: 10 }

                # assign multiple values using alternative syntax
                f.bin.update(d)
                f.flush()
                self.assertEqual(45, f.bin[segyio.su.ntrpr])
                self.assertEqual(10, f.bin[segyio.su.hsfs])

                # accept anything with a key-value structure
                f.bin.update( [(segyio.su.jobid, 12), (segyio.su.lino, 4)] )
                f.bin.update( ((segyio.su.reno, 3), (segyio.su.hdt, 7)) )

                f.flush()
                self.assertEqual(12, f.bin[segyio.su.jobid])
                self.assertEqual( 4, f.bin[segyio.su.lino])
                self.assertEqual( 3, f.bin[segyio.su.reno])
                self.assertEqual( 7, f.bin[segyio.su.hdt])

                # looking up multiple values at once returns a { TraceField: value } dict
                self.assertEqual(d, f.bin[BinField.Traces, BinField.SweepFrequencyStart])

                # copy a header
                f.bin = f.bin

    def test_fopen_error(self):
        # non-existent file
        with self.assertRaises(IOError):
            segyio.open("no_dir/no_file", "r")

        # non-existant mode
        with self.assertRaises(IOError):
            segyio.open(self.filename, "foo")

    def test_wrong_lineno(self):
        with self.assertRaises(KeyError):
            with segyio.open(self.filename, "r") as f:
                f.iline[3000]

        with self.assertRaises(KeyError):
            with segyio.open(self.filename, "r") as f:
                f.xline[2]

    def test_open_wrong_inline(self):
        with self.assertRaises(IndexError):
            with segyio.open(self.filename, "r", 2) as f:
                pass

        with segyio.open(self.filename, "r", 2, strict = False) as f:
            pass

    def test_open_wrong_crossline(self):
        with self.assertRaises(IndexError):
            with segyio.open(self.filename, "r", 189, 2) as f:
                pass

        with segyio.open(self.filename, "r", 189, 2, strict = False) as f:
            pass

    def test_wonky_dimensions(self):
        with segyio.open(self.fileMx1) as f:
            pass

        with segyio.open(self.file1xN) as f:
            pass

        with segyio.open(self.file1x1) as f:
            pass

    def test_open_fails_unstructured(self):
        with segyio.open(self.filename, "r", 37, strict = False) as f:
            with self.assertRaises(ValueError):
                f.iline[10]

            with self.assertRaises(ValueError):
                f.iline[:,:]

            with self.assertRaises(ValueError):
                f.xline[:,:]

            with self.assertRaises(ValueError):
                f.depth_slice[2]

            # operations that don't rely on geometry still works
            self.assertEqual(f.header[2][189], 1)
            self.assertListEqual(list(f.attributes(189)[:]),
                                 [(i // 5) + 1 for i in range(len(f.trace))])

    def test_assign_all_traces(self):
        with TestContext("assign_all_traces") as context:
            context.copy_file(self.filename, target = 'dst')
            context.copy_file(self.filename)
            with segyio.open('small.sgy') as f:
                traces = f.trace.raw[:] * 2.0

            with segyio.open('dst/small.sgy', 'r+') as f:
                f.trace[:] = traces[:]

            with segyio.open('dst/small.sgy') as f:
                self.assertTrue(np.array_equal(f.trace.raw[:], traces))

    def test_traceaccess_from_array(self):
        a = np.arange(10, dtype = np.int)
        b = np.arange(10, dtype = np.int32)
        c = np.arange(10, dtype = np.int64)
        d = np.arange(10, dtype = np.intc)
        with segyio.open(self.filename) as f:
            f.trace[a[0]]
            f.trace[b[1]]
            f.trace[c[2]]
            f.trace[d[3]]

    def test_create_sgy(self):
        with TestContext("create_sgy") as context:
            context.copy_file(self.filename)
            src_file = "small.sgy"
            dst_file = "small_created.sgy"
            with segyio.open(src_file, "r") as src:
                spec = segyio.spec()
                spec.format     = int(src.format)
                spec.sorting    = int(src.sorting)
                spec.samples    = src.samples
                spec.ilines     = src.ilines
                spec.xlines     = src.xlines

                with segyio.create(dst_file, spec) as dst:
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

            self.assertTrue(filecmp.cmp(src_file, dst_file))

    def test_create_sgy_shorter_traces(self):
        with TestContext("create_sgy_shorter_traces") as context:
            context.copy_file(self.filename)
            src_file = "small.sgy"
            dst_file = "small_created_shorter.sgy"

            with segyio.open(src_file, "r") as src:
                spec = segyio.spec()
                spec.format  = int(src.format)
                spec.sorting = int(src.sorting)
                spec.samples = src.samples[:20] # reduces samples per trace
                spec.ilines  = src.ilines
                spec.xlines  = src.xlines

                with segyio.create(dst_file, spec) as dst:
                    for i, srch in enumerate(src.header):
                        dst.header[i] = srch
                        d = { TraceField.INLINE_3D: srch[TraceField.INLINE_3D] + 100 }
                        dst.header[i] = d

                    for lineno in dst.ilines:
                        dst.iline[lineno] = src.iline[lineno]

                    # alternative form using left-hand-side slices
                    dst.iline[2:4] = src.iline

                    for lineno in dst.xlines:
                        dst.xline[lineno] = src.xline[lineno]

                with segyio.open(dst_file, "r") as dst:
                    self.assertEqual(20, len(dst.samples))
                    self.assertEqual([x + 100 for x in src.ilines], list(dst.ilines))

    def test_create_from_naught(self):
        with TestContext("create_from_naught") as context:
            fname = "mk.sgy"
            spec = segyio.spec()
            spec.format  = 5
            spec.sorting = 2
            spec.samples = range(150)
            spec.ilines  = range(1, 11)
            spec.xlines  = range(1, 6)

            with segyio.create(fname, spec) as dst:
                tr = np.arange( start = 1.000, stop = 1.151, step = 0.001, dtype = np.single)

                for i in range( len( dst.trace ) ):
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

    def test_create_from_naught_prestack(self):
        with TestContext("create_from_naught_prestack") as context:
            fname = "mk-ps.sgy"
            spec = segyio.spec()
            spec.format  = 5
            spec.sorting = 2
            spec.samples = range(7)
            spec.ilines  = range(1, 4)
            spec.xlines  = range(1, 3)
            spec.offsets = range(1, 6)

            cube_size = len(spec.ilines) * len(spec.xlines)

            with segyio.create(fname, spec) as dst:
                arr = np.arange( start = 0.000,
                                 stop = 0.007,
                                 step = 0.001,
                                 dtype = np.single)

                arr = np.concatenate([[arr + 0.01], [arr + 0.02]], axis = 0)
                lines = [arr + i for i in spec.ilines]
                cube = [(off * 100) + line for line in lines for off in spec.offsets]

                dst.iline[:,:] = cube

                for of in spec.offsets:
                    for il in spec.ilines:
                        dst.header.iline[il,of] = { TraceField.INLINE_3D: il,
                                                    TraceField.offset: of
                                                  }
                    for xl in spec.xlines:
                        dst.header.xline[xl,of] = { TraceField.CROSSLINE_3D: xl }

            with segyio.open(fname, "r") as f:
                self.assertAlmostEqual(101.010, f.trace[0][0],  places = 4)
                self.assertAlmostEqual(101.011, f.trace[0][1],  places = 4)
                self.assertAlmostEqual(101.016, f.trace[0][-1], places = 4)
                self.assertAlmostEqual(503.025, f.trace[-1][5], places = 4)
                self.assertNotEqual(f.header[0][TraceField.offset], f.header[1][TraceField.offset])
                self.assertEqual(1, f.header[0][TraceField.offset])
                self.assertEqual(2, f.header[1][TraceField.offset])

                for x, y in zip(f.iline[:,:], cube):
                    self.assertListEqual(list(x.flatten()), list(y.flatten()))

    def test_create_write_lines(self):
        fname = "mklines.sgy"

        with TestContext("create_write_lines") as context:
            mklines(fname)

            with segyio.open(fname, "r") as f:
                self.assertAlmostEqual(1,     f.iline[1][0][0], places = 4)
                self.assertAlmostEqual(2.004, f.iline[2][0][4], places = 4)
                self.assertAlmostEqual(2.014, f.iline[2][1][4], places = 4)
                self.assertAlmostEqual(8.043, f.iline[8][4][3], places = 4)

    def test_create_sgy_skip_lines(self):
        fname = "lines.sgy"
        dstfile = "lines-halved.sgy"

        with TestContext("create_sgy_skip_lines") as context:
            mklines(fname)

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
                self.assertListEqual(list(f.ilines), list(spec.ilines))
                self.assertListEqual(list(f.xlines), [1, 3, 6])
                self.assertAlmostEqual(1,     f.iline[1][0][0], places = 4)
                self.assertAlmostEqual(3.004, f.iline[3][0][4], places = 4)
                self.assertAlmostEqual(3.014, f.iline[3][1][4], places = 4)
                self.assertAlmostEqual(7.023, f.iline[7][2][3], places = 4)

    def test_segyio_types(self):
        with segyio.open(self.filename, "r") as f:
            self.assertIsInstance(f.sorting, int)
            self.assertIsInstance(f.ext_headers, int)
            self.assertIsInstance(f.tracecount, int)
            self.assertIsInstance(f.samples, np.ndarray)

            self.assertIsInstance(f.depth_slice, Line)
            self.assertIsInstance(f.depth_slice[1], np.ndarray)
            self.assertIsInstance(f.depth_slice[1:23], GeneratorType)

            self.assertIsInstance(f.ilines, np.ndarray)
            self.assertIsInstance(f.iline, Line)
            self.assertIsInstance(f.iline[1], np.ndarray)
            self.assertIsInstance(f.iline[1:3], GeneratorType)
            self.assertIsInstance(f.iline[1][0], np.ndarray)
            self.assertIsInstance(f.iline[1][0:2], np.ndarray)
            self.assertIsInstance(float(f.iline[1][0][0]), float)
            self.assertIsInstance(f.iline[1][0][0:3], np.ndarray)

            self.assertIsInstance(f.xlines, np.ndarray)
            self.assertIsInstance(f.xline, Line)
            self.assertIsInstance(f.xline[21], np.ndarray)
            self.assertIsInstance(f.xline[21:23], GeneratorType)
            self.assertIsInstance(f.xline[21][0], np.ndarray)
            self.assertIsInstance(f.xline[21][0:2], np.ndarray)
            self.assertIsInstance(float(f.xline[21][0][0]), float)
            self.assertIsInstance(f.xline[21][0][0:3], np.ndarray)

            self.assertIsInstance(f.header, Header)
            self.assertIsInstance(f.header.iline, Line)
            self.assertIsInstance(f.header.iline[1], GeneratorType)
            self.assertIsInstance(next(f.header.iline[1]), Field)
            self.assertIsInstance(f.header.xline, Line)
            self.assertIsInstance(f.header.xline[21], GeneratorType)
            self.assertIsInstance(next(f.header.xline[21]), Field)

            self.assertIsInstance(f.trace, Trace)
            self.assertIsInstance(f.trace[0], np.ndarray)

            self.assertIsInstance(f.bin, Field)
            self.assertIsInstance(f.text, object)  # inner TextHeader instance

    def test_depth_slice_reading(self):
        with segyio.open(self.filename, "r") as f:
            self.assertEqual(len(f.depth_slice), len(f.samples))

            for depth_sample in range(len(f.samples)):
                depth_slice = f.depth_slice[depth_sample]
                self.assertIsInstance(depth_slice, np.ndarray)
                self.assertEqual(depth_slice.shape, (5, 5))

                for x, y in itertools.product(f.ilines, f.xlines):
                    i, j = x - f.ilines[0], y - f.xlines[0]
                    self.assertAlmostEqual(depth_slice[i][j], f.iline[x][j][depth_sample], places=6)

            for index, depth_slice in enumerate(f.depth_slice):
                self.assertIsInstance(depth_slice, np.ndarray)
                self.assertEqual(depth_slice.shape, (5, 5))

                for x, y in itertools.product(f.ilines, f.xlines):
                    i, j = x - f.ilines[0], y - f.xlines[0]
                    self.assertAlmostEqual(depth_slice[i][j], f.iline[x][j][index], places=6)

        with self.assertRaises(KeyError):
            slice = f.depth_slice[len(f.samples)]

    def test_depth_slice_writing(self):
        with TestContext("depth_slice_writing") as context:
            context.copy_file(self.filename)
            fname = ("small.sgy")

            buf = np.empty(shape=(5, 5), dtype=np.single)

            def value(x, y):
                return x + (1.0 // 5) * y

            for x, y in itertools.product(range(5), range(5)):
                buf[x][y] = value(x, y)

            with segyio.open(fname, "r+") as f:
                f.depth_slice[7] = buf * 3.14 # assign to depth 7
                self.assertTrue(np.allclose(f.depth_slice[7], buf * 3.14))

                f.depth_slice = [buf * i for i in range(len(f.depth_slice))]  # assign to all depths
                for index, depth_slice in enumerate(f.depth_slice):
                    self.assertTrue(np.allclose(depth_slice, buf * index))

if __name__ == '__main__':
    unittest.main()
