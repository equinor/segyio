import sys
import os
import shutil
import segyio

def pathjoin( prefix, path):
    directory, base = os.path.split(path)
    pref, suff = os.path.splitext(base)
    filename = pref + '-' + prefix + suff
    return os.path.join(directory, filename)

# This script reverse the traces for a given file, by iline, xline and offset in
# all combinations of the three.

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: {} [source-file] [destination-file]".format(sys.argv[0]))

    sourcefile = sys.argv[1]
    destfile = sys.argv[2] if len(sys.argv) > 2 else sourcefile

    for suf in ['dec-il-xl-off',
                'dec-il-inc-xl-off',
                'dec-xl-inc-il-off',
                'dec-il-xl-inc-off',
                'dec-il-off-inc-xl',
                'dec-xl-off-inc-il',
                'dec-off-inc-il-xl']:
        fname = pathjoin(suf, destfile)
        shutil.copyfile(sourcefile, fname)

        with segyio.open(sourcefile) as src, segyio.open(fname, 'r+') as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            dst.bin = src.bin
            dst.trace = src.trace
            dst.header = src.header

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-il-xl-off", destfile), 'r+') as dst:
            dst.header = src.header[::-1]
            dst.trace = src.trace[::-1]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-il-inc-xl-off", destfile), 'r+') as dst:
            dst.header.iline[::, ::] = src.header.iline[::-1, ::]
            dst.iline[::, ::] = src.iline[::-1, ::]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-xl-inc-il-off", destfile), 'r+') as dst:
            dst.header.xline[::, ::] = src.header.xline[::-1, ::]
            dst.xline[::, ::] = src.xline[::-1, ::]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-il-xl-inc-off", destfile), 'r+') as dst:
            dst.header = src.header[::-1]
            dst.trace = src.trace[::-1]

            for i in range(0, len(dst.trace), 2):
                dst.trace[i], dst.trace[i + 1] = dst.trace[i + 1], dst.trace[i]
                dst.header[i], dst.header[i + 1] = dst.header[i + 1], dst.header[i]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-il-off-inc-xl", destfile), 'r+') as dst:
            dst.header.iline[::, ::] = src.header.iline[::-1, ::-1]
            dst.iline[::, ::] = src.iline[::-1, ::-1]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-xl-off-inc-il", destfile), 'r+') as dst:
            dst.header.iline[::, ::] = src.header.iline[::-1, ::-1]
            dst.header.xline[::, ::] = src.header.xline[::-1, ::-1]
            dst.iline[::, ::] = src.iline[::-1, ::]

    with segyio.open(sourcefile) as src:
        with segyio.open(pathjoin("dec-off-inc-il-xl", destfile), 'r+') as dst:
            dst.header.xline[::, ::] = src.header.xline[::, ::-1]
            dst.xline[::, ::] = src.xline[::, ::-1]


if __name__ == '__main__':
    main()
