import sys
import segyio
import numpy as np


def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: {} [segyfile] ".format(sys.argv[0]))

    segyfile = sys.argv[1]

    min_value = sys.float_info.max
    max_value = sys.float_info.min

    with segyio.open(segyfile) as f:
        for trace in f.trace:

            local_min = np.nanmin(trace)
            local_max = np.nanmax(trace)

            if np.isfinite(local_min):
                min_value = min(local_min, min_value)

            if np.isfinite(local_max):
                max_value = max(local_max, max_value)

    print("min: {}".format(min_value))
    print("max: {}".format(max_value))

if __name__ == '__main__':
    main()
