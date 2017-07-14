#!/usr/bin/env python
import argparse
import segyio

# Create ArgPasrer object with usage and a example
parser = argparse.ArgumentParser(description='Concatenate the binary header from FILE to standard output.', epilog='Example:'+'\n'+'python segycatb.py /your/path/to/file.sgy\n')

# Prints the version based on the segyio build
parser.add_argument('--version', action='version', version='Segycatb (SegyIO: version '+segyio.__version__+')', help='Output version information and exit')

# Takes path as a positionl argument
parser.add_argument('filepath', help='filepath to the segy file you to view binary header for')


args = parser.parse_args()

# Opens given file and prints the binary header
try:
    with segyio.open(args.filepath, 'r') as segyfile:
        segyfile.mmap()

        # Reads binary content and prints to readable
        binh = segyfile.bin
        print "{:<25} | {:<6}".format("Key", "Number")
        for i in binh:
            print "{:<25} | {:<6}".format(i, binh[i])

except RuntimeError:
    print "Error: Provided path is not to a .sgy file"