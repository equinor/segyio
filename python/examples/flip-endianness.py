import sys
import segyio


def main():
    """
    Flips file endianness. Assumes sources is big endian and dst is little endian.
    """
    if len(sys.argv) < 3:
        sys.exit(
            "Usage: {} [source-file] [destination-file]".format(sys.argv[0]))

    srcfile = sys.argv[1]
    dstfile = sys.argv[2]

    with segyio.open(srcfile) as src:
        spec = segyio.spec()
        spec.sorting = int(src.sorting)
        spec.format = int(src.format)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines
        spec.ext_headers = src.ext_headers
        spec.traceheader_count = src.traceheader_count
        spec.tracecount = src.tracecount
        spec.endian = 'little'

        with segyio.create(dstfile, spec) as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            dst.bin = src.bin
            dst.trace = src.trace

            # intention:
            # dst.traceheader = src.traceheader

        # traceheader mapping was copied after open. At the moment of creation
        # of this example file code can't deal with that, so we must reopen the
        # file after bin and ext_headers are copied so that mapping is
        # recognized.
        with segyio.open(dstfile, "r+", endian="little", ignore_geometry=True) as dst:
            for trace_index in range(src.tracecount):
                for traceheader_index in range(src.traceheader_count):
                    traceheader = src.segyfd.getth(
                        trace_index, traceheader_index, bytearray(240))
                    dst.segyfd.putth(
                        trace_index, traceheader_index, traceheader)


if __name__ == '__main__':
    main()
