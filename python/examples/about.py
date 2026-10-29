import sys
from segyio import TraceField
import segyio


""""
about.py -  A command line tool to inspect SEG-Y file metadata using segyio.

This script reads SEG-Y files and prints key attributes like inline and crossline counts, format, and offsets. If insufficient arguments are provided, it lists valied 
trace fields and their byte offsets.

Usage:
    python about.py <segy-file> <inline-field> <crossline-field>

Arguments:
    seg-file        Path to the SEG-Y file
    inline-field    Inline field name
    crossline-field Crossline field name

Example:
    python about.py mydata.segy iline xline
"""


def list_byte_offset_names():
    print("Available offsets and their corresponding byte value:")
    for x in TraceField.enums():
        print("  {}: {}".format(str(x), x))


if __name__ == '__main__':
    if len(sys.argv) < 4:
        list_byte_offset_names()
        sys.exit("Usage: about.py [file] [inline] [crossline]")

    # we need a way to convert from run-time inline/crossline argument (as
    # text) to the internally used TraceField enum. Make a string -> TraceField
    # map and look up into that. this dictionary comprehension creates that
    fieldmap = {str(x).lower(): x for x in TraceField.enums()}

    filename = sys.argv[1]
    inline_name, crossline_name = sys.argv[2].lower(), sys.argv[3].lower()

    # exit if inline or crossline are unknown
    if inline_name not in fieldmap:
        list_byte_offset_names()
        sys.exit("Unknown inline field '{}'".format(sys.argv[2]))

    if crossline_name not in fieldmap:
        list_byte_offset_names()
        sys.exit("Unknown crossline field '{}'".format(sys.argv[3]))

    inline, crossline = fieldmap[inline_name], fieldmap[crossline_name]

    with segyio.open(filename, "r", inline, crossline) as f:
        print("About '{}':".format(filename))
        print("Format type: {}".format(f.format))
        print("Offset count: {}".format(f.offsets))
        print("ilines: {}".format(", ".join(map(str, f.ilines))))
        print("xlines: {}".format(", ".join(map(str, f.xlines))))

    print("+------+")

    with segyio.open(filename, "r", crossline, inline) as f:
        # with swapped inline/crossline
        print("About '{}':".format(filename))
        print("Format type: {}".format(f.format))
        print("Offset count: {}".format(f.offsets))
        print("ilines: {}".format(", ".join(map(str, f.ilines))))
        print("xlines: {}".format(", ".join(map(str, f.xlines))))
