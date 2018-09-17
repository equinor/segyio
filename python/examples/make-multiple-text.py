import sys
import segyio

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: make-multiple-text.py [file]")

    filename = sys.argv[1]

    spec = segyio.spec()
    spec.sorting = 2
    spec.format = 1
    spec.samples = [1]
    spec.ilines = [1]
    spec.xlines = [1]
    spec.ext_headers = 4

    with segyio.create(filename, spec) as f:
        for i in range(1, spec.ext_headers + 1):
            f.text[i] = f.text[0]

        f.trace[0] = [0]

if __name__ == '__main__':
    main()
