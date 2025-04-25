import tempfile
import numpy as np
import segyio

# requires Python >3.12


def test_reading_past_4GB():
    # reading from position that can't be stored as 32 bit int
    with tempfile.NamedTemporaryFile(delete_on_close=False) as temp_file:
        temp_path = temp_file.name

        # each trace is 4 bytes * 250 samples = 1000 bytes
        # 5 million traces = 5 GB
        fmt = segyio.SegySampleFormat.IEEE_FLOAT_4_BYTE
        nsamples = 250
        trace_count = 5000000

        spec = segyio.spec()
        spec.format = fmt
        spec.samples = range(nsamples)
        spec.tracecount = trace_count

        zero_trace = np.zeros(nsamples, dtype=np.float32)
        last_trace = np.arange(1, nsamples + 1, dtype=np.float32)

        with segyio.create(temp_path, spec) as f:
            for i in range(trace_count - 1):
                f.trace[i] = zero_trace

            f.trace[trace_count - 1] = last_trace

        with segyio.open(temp_path, "r", strict=False) as f:
            assert np.array_equal(f.trace[trace_count - 1], last_trace)
