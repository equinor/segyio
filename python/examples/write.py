import sys
import segyio
import numpy as np

def main():
    if len( sys.argv ) < 2:
        sys.exit("Usage: write.py [file]")

    filename = sys.argv[1]

    # the mode parameter is passed directly to C's fopen
    # opening the file for writing requires r+, not rw because rw would
    # truncate (effectively destroy) the file, and r+ would preserve the size
    with segyio.open( filename, "r+" ) as src:

        # read trace 0, then double all values
        trace = src.trace[0]
        trace *= 2

        # write trace 0 back to disk
        src.trace[0] = trace

        # read trace 1, but re-use the memory for speed
        trace = src.trace[1]
        # square all values. the trace is a plain numpy array
        trace = np.square(trace, trace)
        # write the trace back to disk, but at trace 2
        src.trace[2] = trace

        # read every other trace, from 10 through 20
        # then write them to every third step from 40 through 52
        # i.e. 40, 43, 46...
        # slices yield a generator, so only one numpy array is created
        for tr, i in zip(src.trace[10:20:2], range(2,13,3)):
            src.trace[i] = tr

        # iterate over all traces in a file. this is a generator with a shared
        # buffer, so it's quite efficient
        tracesum = 0
        for tr in src.trace:
            # accumulate the traces' 30th value
            tracesum += tr[30]

        print("Trace sum: {}".format(tracesum))

        # write the iline at 2 to the iline at 3
        sum3 = np.sum(src.iline[3])
        src.iline[2] = src.iline[3]
        # flush to make sure our changes to the file are visible
        src.flush()
        sum2 = np.sum(src.iline[2])

        print("Accumulates of inlines 2 and 3: {} -- {}".format(sum2, sum3))

        # ilines too are plain numpy ndarrays, with trace-major addressing
        # i.e. iline[2,40] would be yield trace#2's 40th value
        iline = src.iline[2]
        # since ilines are numpy arrays they also support numpy operations
        iline = np.add(iline, src.iline[4])

        # lines too have generator support, so we accumulate the 2nd trace's
        # 22nd value.
        linesum = 0
        for line in src.iline:
            linesum += line[2,22]

        print("Inline sum: {}".format(linesum))

        # xline access is identical to iline access
        linesum = 0
        for line in src.xline:
            linesum += line[2,22]

        print("Crossline sum: {}".format(linesum))

        # accessing a non-existing inline will raise a KeyError
        try:
            _ = src.iline[5000]
            sys.exit("Was able to access non-existing inline")
        except KeyError as e:
            print(str(e))

if __name__ == '__main__':
    main()
