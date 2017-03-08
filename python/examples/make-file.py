import sys
import numpy as np
import segyio


def main():
    if len(sys.argv) < 7:
        sys.exit("Usage: {} [file] [samples] [first iline] [last iline] [first xline] [last xline]".format(sys.argv[0]))

    spec = segyio.spec()
    filename = sys.argv[1]

    # to create a file from nothing, we need to tell segyio about the structure of
    # the file, i.e. its inline numbers, crossline numbers, etc. You can also add
    # more structural information, but offsets etc. have sensible defautls. This is
    # the absolute minimal specification for a N-by-M volume
    spec.sorting = 2
    spec.format = 1
    spec.samples = range(int(sys.argv[2]))
    spec.ilines = range(*map(int, sys.argv[3:5]))
    spec.xlines = range(*map(int, sys.argv[5:7]))

    with segyio.create(filename, spec) as f:
        # one inline consists of 50 traces
        # which in turn consists of 2000 samples
        start = 0.0
        step = 0.00001
        # fill a trace with predictable values: left-of-comma is the inline
        # number. Immediately right of comma is the crossline number
        # the rightmost digits is the index of the sample in that trace meaning
        # looking up an inline's i's jth crosslines' k should be roughly equal
        # to i.j0k
        trace = np.arange(start = start,
                          stop  = start + step * len(spec.samples),
                          step  = step,
                          dtype = np.single)

        # one inline is N traces concatenated. We fill in the xline number
        line = np.concatenate([trace + (xl / 100.0) for xl in spec.xlines])
        line = line.reshape( (len(spec.xlines), len(spec.samples)) )

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


if __name__ == '__main__':
    main()
