import sys
import pandas as pd
import segyio


def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: trace_headers_to_dataframe.py [file]")

    filename = sys.argv[1]

    # Open file
    with segyio.open(filename, ignore_geometry=True) as f:
        # Get all header keys:
        header_keys = segyio.tracefield.keys
        # Initialize df with trace id as index and headers as columns
        trace_headers = pd.DataFrame(index=range(1, f.tracecount+1),
                                     columns=header_keys.keys())
        # Fill dataframe with all trace headers values
        for k, v in header_keys.items():
            trace_headers[k] = f.attributes(v)[:]

    print(trace_headers.head())


if __name__ == '__main__':
    main()
