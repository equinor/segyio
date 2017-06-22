import sys
import os
import shutil
import itertools as itr
import segyio
import segyio.su as su

def product(f):
    return itr.product(range(len(f.ilines)), range(len(f.xlines)))

def pathjoin(prefix, path):
    dir, base = os.path.split(path)
    return os.path.join(dir, '-'.join((prefix, base)))

# this program copies the source-file and creates eight copies, each with a
# modified set of CDP-X and CDP-Y coordinates, rotating the field around the
# north (increasing CDP-Y) axis.
def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: {} [source-file] [destination-file]".format(sys.argv[0]))

    srcfile = sys.argv[1]
    dstfile = sys.argv[2] if len(sys.argv) > 2 else srcfile

    for pre in ['normal', 'acute', 'right', 'obtuse', 'straight', 'reflex', 'left', 'inv-acute']:
        fname = pathjoin(pre, dstfile)
        shutil.copyfile(srcfile, fname)

        with segyio.open(srcfile) as src, segyio.open(fname, 'r+') as dst:
            for i in range(1 + src.ext_headers):
                dst.text[i] = src.text[i]

            dst.bin = src.bin
            dst.trace = src.trace
            dst.header = src.header

    with segyio.open(pathjoin('normal', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = x
            trh[su.cdpy] = y
            trh[su.scalco] = 10

    with segyio.open(pathjoin('acute', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[y + x * len(src.ilines)]
            trh[su.cdpx] = x + y
            trh[su.cdpy] = (100 - x) + y
            trh[su.scalco] = -10

    with segyio.open(pathjoin('right', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = y
            trh[su.cdpy] = 100 - x
            trh[su.scalco] = 1

    with segyio.open(pathjoin('obtuse', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = (100 - x) + y
            trh[su.cdpy] = (100 - x) - y
            trh[su.scalco] = 2

    with segyio.open(pathjoin('straight', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = 100 - x
            trh[su.cdpy] = 100 - y
            trh[su.scalco] = -7

    with segyio.open(pathjoin('reflex', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = 100 - (x + y)
            trh[su.cdpy] = 100 + (x - y)
            trh[su.scalco] = 7

    with segyio.open(pathjoin('left', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = 100 - y
            trh[su.cdpy] = x
            trh[su.scalco] = 21

    with segyio.open(pathjoin('inv-acute', dstfile), 'r+') as dst:
        for i, (x, y) in enumerate(product(src)):
            trh = dst.header[i]
            trh[su.cdpx] = 100 + x - y
            trh[su.cdpy] = x + y
            trh[su.scalco] = 100

if __name__ == '__main__':
    main()
