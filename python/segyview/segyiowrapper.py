import sys
import numpy as np
from numpy import inf
import segyio

class SlicesWrapper(object):
    """ Simple wrapper either around the Line class (when read_from_file) or a native Numpy array in memory. 
    (by. mapping index to line number).

    Will signal file read start and finished events through the FileActivityMonitor when file read operations in
    segyio api are used.
    """

    def __init__(self, segyiowrapper, indexes, cube, read_from_file=False):
        self.indexes = indexes
        self.cube = cube
        self.swrap = segyiowrapper
        self.read_from_file = read_from_file

    def __getitem__(self, item):

        if self.read_from_file:
            self.swrap.file_activity_monitor.set_file_read_started()
            slice_from_file = self.cube[item]
            self.swrap.file_activity_monitor.set_file_read_finished()
            return slice_from_file
        else:
            return self.cube[self.indexes.index(item)]

    def __len__(self):
        return len(self.indexes)


class SegyIOWrapper(object):
    """ Wraps around the functionality offered by the segyio api - and proxies read operations either towards segyio,
    or an in-memory numpy array.
    """

    def __init__(self, file_activity_monitor=None):
        self.s, self.iline_slices, self.xline_slices, self.depth_slices, self.min_max = None, None, None, None, None
        self.file_activity_monitor = file_activity_monitor

    def open_file(self, filename, read_to_memory=True, progress_callback=None):
        """ to avoid having to rescan the entire file for accessing each depth slice.
                we have the option that all traces are read once. Use the read_to_memory flag for that intention.
        """

        # close the current file.
        if self.s is not None:
            self.s.close()

        self.s = segyio.open(str(filename))

        if read_to_memory:
            return self.read_traces_to_memory(progress_callback=progress_callback)
        else:
            self.iline_slices = SlicesWrapper(self, self.s.ilines.tolist(), self.s.iline, read_from_file=True)
            self.xline_slices = SlicesWrapper(self, self.s.xlines.tolist(), self.s.xline, read_from_file=True)
            self.depth_slices = SlicesWrapper(self, range(self.s.samples), self.s.depth_slice, read_from_file=True)
            self.min_max = None
            if progress_callback is not None:
                progress_callback(100)
        return True

    def read_traces_to_memory(self, progress_callback=None):
        """ Read all traces into memory and identify global min and max values.

        Utility method to handle the challenge of navigating up and down in depth slices,
        as each depth slice consist of samples from all traces in the segy file.

        The cubes created are transposed in aspect of the iline, xline and depth plane. Where each slice of the
        returned depth cube consists of all samples for the given depth, oriented by [iline, xline]

        Returns True if operation was able to run to completion. False if actively cancelled, or disrupted in any
        other way.
        """

        all_traces = np.empty(shape=((len(self.s.ilines) * len(self.s.xlines)), self.s.samples), dtype=np.single)

        number_of_traces = self.s.tracecount

        # signal file read start to anyone listening to the monitor
        if self.file_activity_monitor is not None:
            self.file_activity_monitor.set_file_read_started()

        for i, t in enumerate(self.s.trace):

            if self.file_activity_monitor is not None and self.file_activity_monitor.cancelled_operation:
                """ Read operation is cancelled, closing segy file, and signal that file read is finished
                """
                self.s.close()
                self.file_activity_monitor.set_file_read_finished()
                return False

            all_traces[i] = t

            if progress_callback is not None:
                progress_callback((float(i + 1) / number_of_traces) * 100)

        if self.file_activity_monitor is not None:
            self.file_activity_monitor.set_file_read_finished()

        iline_slices = all_traces.reshape(len(self.s.ilines), len(self.s.xlines), self.s.samples)
        xline_slices = iline_slices.transpose(1, 0, 2)

        self.depth_slices = iline_slices.transpose(2, 0, 1)

        self.iline_slices = SlicesWrapper(self, self.s.ilines.tolist(), iline_slices, read_from_file=False)
        self.xline_slices = SlicesWrapper(self, self.s.xlines.tolist(), xline_slices, read_from_file=False)
        self.min_max = self.identify_min_max(all_traces)
        return True


    def identify_min_max(self, all_traces):

        # removing positive and negative infinite numbers
        all_traces[all_traces == inf] = 0
        all_traces[all_traces == -inf] = 0

        min_value = np.nanmin(all_traces)
        max_value = np.nanmax(all_traces)

        return (min_value, max_value)

    @property
    def samples(self):
        return self.s.samples

    @property
    def xlines(self):
        return self.s.xlines.tolist()

    @property
    def ilines(self):
        return self.s.ilines.tolist()

    @property
    def iline(self):
        return self.iline_slices

    @property
    def xline(self):
        return self.xline_slices

    @property
    def depth_slice(self):
        return self.depth_slices