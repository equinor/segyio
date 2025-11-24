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

        layout_index = -1
        for i, stanza_name in enumerate(src.stanza.names()):
            if stanza_name.lower().startswith('seg:layout'):
                layout_index = i
        layout = src.stanza[layout_index]

        with segyio.create(dstfile, spec, layout_xml=layout) as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            dst.bin = src.bin
            dst.trace = src.trace
            dst.traceheader = src.traceheader


if __name__ == '__main__':
    main()
