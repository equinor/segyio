import sys
import numpy as np
import segyio

# Purpose is to check that all fields behave correctly if set to values from the
# range of largest, positive values (sign bit is 0) or of smallest, negative for
# signed types values (sign bit is 1).
#
# Values in all integer fields are set using increasing or decreasing numbers
# respecting the bounds of the field type (32-bit signed integer, 64-bit
# unsigned integer, etc.). The first field is using the smallest allowed value
# of its type, if numbers are increased, or the largest allowed value, if
# numbers are decreased. See actual code for the details.


def main():
    if len(sys.argv) < 3:
        sys.exit("Usage: {} [file] [type]".format(sys.argv[0]))

    filename = sys.argv[1]
    type = sys.argv[2]

    if type == 'inc':
        # negative values are increasing
        value_sign = -1
        diff_sign = 1
    elif type == 'dec':
        # positive values are decreasing
        value_sign = 1
        diff_sign = -1
    else:
        sys.exit("Type must be 'inc' for increase, 'dec' for decrease")

    spec = segyio.spec()
    samples = 4
    spec.sorting = 2
    spec.format = segyio.SegySampleFormat.IBM_FLOAT_4_BYTE
    spec.samples = range(samples)
    spec.ilines = range(1)
    spec.xlines = range(5)

    def calculate_value(field, field_size, diff):
        defaulted = [
            segyio.BinField.Unassigned1,
            segyio.BinField.Unassigned2,
            segyio.BinField.Format,
        ]
        if field in defaulted:
            return None
        if field == segyio.BinField.ExtEnsembleTraces:
            return 5
        if field == segyio.BinField.ExtSamples:
            return samples
        if field == segyio.BinField.ExtendedHeaders:
            return 0
        if field == segyio.BinField.IntConstant:
            return 16909060
        if field == segyio.BinField.FirstTraceOffset:
            return 3600

        if field == segyio.BinField.ExtInterval:
            return 1125899906842594.0 * value_sign
        if field == segyio.BinField.ExtIntervalOriginal:
            return 1125899906842593.0 * value_sign

        sizes = [1, 2, 4, 8]
        if field_size not in sizes:
            raise ValueError(
                "Unsupported field {} / field size {}".format(field, field_size))

        unsigned = [
            segyio.BinField.Samples,
            segyio.BinField.SamplesOriginal,
            segyio.BinField.SEGYRevision,
            segyio.BinField.SEGYRevisionMinor,
            segyio.BinField.FirstTraceOffset,
            segyio.BinField.NrTracesInStream,
            segyio.TraceField.TRACE_SEQUENCE_LINE,
            segyio.TraceField.TRACE_SEQUENCE_FILE,
            segyio.TraceField.TRACE_SAMPLE_COUNT
        ]
        if value_sign < 0:
            if field in unsigned:
                # negative values can't happen, calculations go from 0 and up
                base = 0
            else:
                # in 2-complements notation lowest value is binary 1000...0000 = 0x8000...0000
                base = 2 ** (field_size * 8 - 1)
        else:
            if field in unsigned:
                # all bits can be used to represent number
                base = 2 ** (field_size * 8) - 1

                # note: logic slip in the current implementation
                inconsistent = [
                    segyio.BinField.Samples,
                    segyio.BinField.SamplesOriginal,
                    segyio.BinField.SEGYRevision,
                    segyio.BinField.SEGYRevisionMinor,
                    segyio.TraceField.TRACE_SAMPLE_COUNT,
                ]
                if field in inconsistent:
                    base = base - 1
            else:
                # highest bit is reserved for sign
                base = 2 ** (field_size * 8 - 1) - 1

        return base * value_sign + diff * diff_sign

    def calculate_header(header_dict, header_field_offsets, value_diff_step):
        prev_field = None
        value_diff = 0
        for field in header_field_offsets:
            if prev_field:
                field_size = field - prev_field
                value = calculate_value(prev_field, field_size, value_diff)
                if value is not None:
                    header_dict.update({prev_field: value})
                value_diff += value_diff_step
            prev_field = field

    binheader = {}
    binvalues = [int(field) for field in segyio.BinField.enums()]
    binvalues.append(3601)
    calculate_header(binheader, binvalues, 1)

    with segyio.create(filename, spec) as f:
        tracevalues = [int(field) for field in segyio.TraceField.enums()]
        tracevalues.append(241)
        tr = 0
        for _ in spec.ilines:
            for _ in spec.xlines:
                traceheader = {}
                calculate_header(traceheader, tracevalues, tr + 1)

                f.header[tr] = traceheader
                f.trace[tr] = np.zeros(samples, dtype=np.single)
                tr += 1

        f.bin = binheader
        f.text[0] = ""


if __name__ == '__main__':
    main()
