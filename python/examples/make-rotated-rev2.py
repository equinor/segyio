import sys
import itertools as itr
import segyio
import segyio.su as su


def product(f):
    return itr.product(range(len(f.ilines)), range(len(f.xlines)))

# this program turns the rev1 file into rev2 file with CDP values set to
# different set of cdp-x and cdp-y coordinates in standard and ext1 headers.
# The formulas are taken from make-rotated-copies.py script

def main():
    if len(sys.argv) != 3:
        sys.exit(
            "Usage: {} [source-file] [destination-file]".format(sys.argv[0]))

    srcfile = sys.argv[1]
    dstfile = sys.argv[2] if len(sys.argv) > 2 else srcfile

    with segyio.open(srcfile) as src:
        spec = segyio.spec()
        spec.format = int(src.format)
        spec.sorting = int(src.sorting)
        spec.samples = src.samples
        spec.ilines = src.ilines
        spec.xlines = src.xlines
        spec.traceheader_count = 2
        spec.tracecount = src.tracecount

        with segyio.create(dstfile, spec) as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]
            dst.bin = src.bin
            dst.trace = src.trace
            dst.traceheader = src.traceheader

            dst.bin[segyio.BinField.SEGYRevision] = 2
            dst.bin[segyio.BinField.MaxAdditionalTraceHeaders] = 1

            for i in range(src.tracecount):
                dst.traceheader[i][1].header_name = bytes("SEG00001", "ascii")

    with segyio.open(dstfile, 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.traceheader[i][0]
            # "right" rotation formula
            trh.cdp_x = y
            trh.cdp_y = 100 - x
            trh.co_scal = 1

            trh = dst.traceheader[i][1]
            # "left" rotation formula
            trh.cdp_x = (100 - y) * 21
            trh.cdp_y = x * 21


if __name__ == '__main__':
    main()
