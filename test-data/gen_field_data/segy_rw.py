import argparse
import sys
from textual_file_header import TextualFileHeader
from binary_file_header import BinaryFileHeader
from trace_ensemble import TraceEnsemble
from trace_header import TraceHeader
from segy_data import SEGYData


def generate_data(args):

    if args.gen_type is None:
        print(
            "No generation type specified. Use 'inc' for increasing or 'dec' for decreasing."
        )
        sys.exit(1)

    args.gen_type = args.gen_type.lower()
    if args.gen_type not in ["inc", "dec"]:
        print(
            "Invalid type specified. Use 'inc' for increasing or 'dec' for decreasing."
        )
        sys.exit(1)

    factor = 1 if args.gen_type == "inc" else -1
    textual_file_header = TextualFileHeader()
    binary_file_header = BinaryFileHeader()
    for i, field in enumerate(binary_file_header.fields):
        if field.name not in ["Unassigned1", "Unassigned2"]:
            binary_file_header.values[field.name] = field.min_max[factor] + (i * factor)

    # Set values for specific fields
    binary_file_header.values["ExtendedHeaders"] = 0
    binary_file_header.values["ExtEnsembleTraces"] = 5
    binary_file_header.values["ExtSamples"] = 4
    binary_file_header.values["Format"] = 1

    # Create trace ensembles
    trace_ensembles = []
    for i in range(binary_file_header.values["ExtEnsembleTraces"]):
        trace_header = TraceHeader()

        for j, th_field in enumerate(trace_header.fields):
            trace_header.values[th_field.name] = th_field.min_max[factor] + (
                (i + 1) * j * factor
            )

        sample_data = bytearray(binary_file_header.values["ExtSamples"] * binary_file_header.format_size)
        trace_ensembles.append(TraceEnsemble(trace_header, sample_data, i))

    segy_data = SEGYData(textual_file_header, binary_file_header, trace_ensembles)
    segy_data.to_file(args.path)


def get_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "type", choices=["read", "write"], help="Operation type: read or write"
    )
    parser.add_argument(
        "gen_type",
        nargs="?",
        default=None,
        choices=["inc", "dec"],
        help="Type of data to generate: inc for increasing, dec for decreasing",
    )
    parser.add_argument("path", help="Path to save the generated SEG-Y file")

    args = parser.parse_args()
    if not args.path.endswith(".sgy"):
        print("Path must end with '.sgy'.")
        sys.exit(1)

    return args


def main():

    args = get_parser()
    if args.type.lower() == "write":
        generate_data(args)
    else:
        segy_file = SEGYData.from_file(args.path)
        print(segy_file)


if __name__ == "__main__":
    main()
