import sys
import numpy as np
import segyio

def main():
    if len(sys.argv) < 2:
        sys.exit('Usage: {} [output]'.format(sys.argv[0]))

    spec = segyio.spec()
    filename = sys.argv[1]

    spec.format = 1
    spec.samples = range(25)
    spec.tracecount = 61

    with segyio.create(filename, spec) as f:

        trno = 0
        keys = [2, 3, 5, 8]
        lens = [10, 12, 13, 26]

        for key, length in zip(keys, lens):
            for i in range(length):

                f.header[trno].update(
                    offset = 1,
                    fldr = key,
                    grnofr = (i % 2) + 1,
                    tracf = spec.tracecount - i,
                )

                trace = np.linspace(start = key,
                                    stop  = key + 1,
                                    num   = len(spec.samples))
                f.trace[trno] = trace
                trno += 1

        f.bin.update()

if __name__ == '__main__':
    main()
