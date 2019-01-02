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

        # Write the file trace-by-trace and update headers with iline, xline
        # and offset
        tr = 0
        for il in spec.ilines:
            for xl in spec.xlines:
                f.header[tr] = {
                    segyio.su.offset : 1,
                    segyio.su.iline  : il,
                    segyio.su.xline  : xl
                }
                f.trace[tr] = trace + (xl / 100.0) + il
                tr += 1

        f.bin.update(
            tsort=segyio.TraceSortingFormat.INLINE_SORTING
        )


if __name__ == '__main__':
    main()
