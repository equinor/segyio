import sys
import segyio

# this program creates a new subcube, taking the first 5 lines in both
# directions, and reduces the trace size to 20 samples
def main():
    if len(sys.argv) < 3:
        sys.exit("Usage: {} [source-file] [destination-file]".format(sys.argv[0]))

    sourcefile = sys.argv[1]
    destfile = sys.argv[2]

    with segyio.open(sourcefile) as src:
        spec = segyio.spec()
        spec.sorting = int(src.sorting)
        spec.format  = int(src.format)
        spec.samples = range(50)
        spec.ilines = src.ilines[:5]
        spec.xlines = src.xlines[:5]

        with segyio.create(destfile, spec) as dst:
            # Copy all textual headers, including possible extended
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            # copy the binary header, then insert the modifications needed for
            # the new shape
            dst.bin = src.bin
            dst.bin = { segyio.BinField.Samples: 50,
                        segyio.BinField.Traces: 5 * 5
                      }

            # Copy all headers in the new inlines. Since we know we're copying
            # the five first we don't have to take special care to update
            # headers
            dst.header.iline = src.header.iline
            # the copy traces (in line mode)
            dst.iline = src.iline

if __name__ == '__main__':
    main()
