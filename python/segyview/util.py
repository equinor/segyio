import sys
import numpy as np


def read_traces_to_memory(segy):
    """ Read all traces into memory and identify global min and max values.

    Utility method to handle the challenge of navigating up and down in depth slices,
    as each depth slice consist of samples from all traces in the segy file.

    The cube returned is transposed in aspect of the depth plane. Where each slice of the returned array
    consists of all samples for the given depth, oriented by [iline, xline]
    """

    all_traces = np.empty(shape=((len(segy.ilines) * len(segy.xlines)), segy.samples), dtype=np.float32)

    min_value = sys.float_info.max
    max_value = sys.float_info.min

    for i, t in enumerate(segy.trace):
        all_traces[i] = t

        local_min = np.nanmin(t)
        local_max = np.nanmax(t)

        if np.isfinite(local_min):
            min_value = min(local_min, min_value)

        if np.isfinite(local_max):
            max_value = max(local_max, max_value)

    reshaped_traces = all_traces.reshape(len(segy.ilines), len(segy.xlines), segy.samples)

    transposed_traces = reshaped_traces.transpose(2, 0, 1)

    return transposed_traces, (min_value, max_value)
