import sys
import numpy as np
import segyio


def main():
    if len(sys.argv) < 9:
        sys.exit(" ".join(["Usage: {} [file] [samples]",
                           "[first iline] [last iline]",
                           "[first xline] [last xline]",
                           "[first offset] [last offset]"]
                         ).format(sys.argv[0]))

    spec = segyio.spec()
    filename = sys.argv[1]

    # to create a file from nothing, we need to tell segyio about the structure of
    # the file, i.e. its inline numbers, crossline numbers, etc. You can also add
    # more structural information, This is the absolute minimal specification for a
    # N-by-M volume with K offsets volume
    spec.sorting = 2
    spec.format = 1
    spec.samples = range(int(sys.argv[2]))
    spec.ilines = range(*map(int, sys.argv[3:5]))
    spec.xlines = range(*map(int, sys.argv[5:7]))
    spec.offsets = range(*map(int, sys.argv[7:9]))
    if len(spec.offsets) == 0: spec.offsets = [1]

    with segyio.create(filename, spec) as f:
        # one inline consists of 50 traces
        # which in turn consists of 2000 samples
        start = 0.0
        step = 0.00001
        # fill a trace with predictable values: left-of-comma is the inline
        # number. Immediately right of comma is the crossline number
        # the rightmost digits is the index of the sample in that trace meaning
        # looking up an inline's i's jth crosslines' k should be roughly equal
        # to (offset*100) + i.j0k.
        trace = np.arange(start = start,
                          stop  = start + step * len(spec.samples),
                          step  = step,
                          dtype = np.single)

        nx, no = len(spec.xlines), len(spec.offsets)
        # one inline is N traces concatenated. We fill in the xline number
        line = np.concatenate([trace + (xl / 100.0) for xl in spec.xlines])
        line = line.reshape( (nx, len(spec.samples)) )

        for ilindex, ilno in enumerate(spec.ilines):
            iline = line + ilno
            for tr, xlno in enumerate(spec.xlines):
                for offset_index, offset in enumerate(spec.offsets):
                    ix = (ilindex * nx * no) + (tr * no) + offset_index
                    f.trace[ix] = iline[tr] + (offset * 100)
                    f.header[ix] = { segyio.TraceField.INLINE_3D: ilno,
                                     segyio.TraceField.CROSSLINE_3D: xlno,
                                     segyio.TraceField.offset: offset }

if __name__ == '__main__':
    main()
