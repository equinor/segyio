import sys
import segyio
import argparse

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: {} [source-file]".format(sys.argv[0]))

    parser = argparse.ArgumentParser(description="Show a range of trace headers from -t <header_nr> to -T <header_nr>. To print just one header, there is no need to set the flag -T")
    parser.add_argument("src", help="Path to sourcefile")
    parser.add_argument("-t", "--RANGE_START", action="store", required=True, type=int, help="First traceheader to be printed")
    parser.add_argument("-T", "--RANGE_END", action="store", type=int, help="Last traceheader to be printed")

    sourcefile = parser.parse_args().src
    range_start = parser.parse_args().RANGE_START
    range_end = parser.parse_args().RANGE_END

    if range_end is None:
        range_end = range_start + 1
    else:
        range_end = range_end + 1

    with segyio.open(sourcefile) as src:
        src.mmap()

        for i in range(range_start, range_end):
            print("Traceheader: ", src.header[i].traceno )
            print("{:<40} | {:<6}".format("Key", "Value"))
            print("------------------------------------------------------")
            for j in src.header[i]:
                print("{:<40} | {:<6}".format(j, src.header[i][j]))
            print("------------------------------------------------------")

if __name__ == '__main__':
    main()
