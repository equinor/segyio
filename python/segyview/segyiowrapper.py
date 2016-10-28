import sys
import math
import numpy as np
import segyio
import itertools


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
    def __init__(self, segy=None, file_name=None, file_activity_monitor=None):
        self.segy = segy
        self.file_name = file_name
        self.file_activity_monitor = file_activity_monitor
        self.iline_slices, self.xline_slices, self.depth_slices, self.min_max = None, None, None, None

    @classmethod
    def wrap(cls, segy, file_activity_monitor=None):
        """
        Wraps an existing segyio instance. Caller is responsible for closing the file.
        :param segy:
        :param file_activity_monitor:
        :return: SegyIOWrapper
        """
        wrapped = cls(segy=segy, file_activity_monitor=file_activity_monitor)
        wrapped._wrap_segyio_slices()
        return wrapped

    @classmethod
    def open_file_and_wrap(cls, file_name, file_activity_monitor=None):
        """
        Creates and wrap a segyio instance for a given filename.
        :param file_name:
        :param file_activity_monitor:
        :return: SegyIOWrapper
        """
        wrapped = cls(file_name=file_name, file_activity_monitor=file_activity_monitor)
        wrapped.segy = segyio.open(str(file_name))
        wrapped._wrap_segyio_slices()

        return wrapped

    def _wrap_segyio_slices(self):
        self.iline_slices = SlicesWrapper(self, self.segy.ilines.tolist(), self.segy.iline, read_from_file=True)
        self.xline_slices = SlicesWrapper(self, self.segy.xlines.tolist(), self.segy.xline, read_from_file=True)
        self.depth_slices = SlicesWrapper(self, range(self.segy.samples), self.segy.depth_slice, read_from_file=True)
        self.min_max = None

    def close(self):
        """
        Closing the referenced segy file
        :return:
        """
        if self.segy is not None:
            try:
                self.segy.close()
            except Exception:
                raise

    def __del__(self):
        self.close()

    def read_all_traces_to_memory(self, progress_callback=None, number_of_read_iterations=100):
        """ Read all traces into memory and identify global min and max values.

        Utility method to handle the challenge of navigating up and down in depth slices,
        as each depth slice consist of samples from all traces in the segy file.

        The cubes created are transposed in aspect of the iline, xline and depth plane. Where each slice of the
        returned depth cube consists of all samples for the given depth, oriented by [iline, xline]

        Returns True if operation was able to run to completion. False if actively cancelled, or disrupted in any
        other way.
        """

        all_traces = np.empty(shape=((len(self.segy.ilines) * len(self.segy.xlines)), self.segy.samples), dtype=np.single)

        # signal file read start to anyone listening to the monitor
        if self.file_activity_monitor is not None:
            self.file_activity_monitor.set_file_read_started()

        step_size = int(math.ceil(float(self.segy.tracecount)/number_of_read_iterations))

        # read traces into memory in step_size
        for i, in itertools.izip(range(0, self.segy.tracecount, step_size)):

            if self.file_activity_monitor is not None and self.file_activity_monitor.cancelled_operation:
                self.file_activity_monitor.set_file_read_finished()
                return False

            all_traces[i: i+step_size] = self.segy.trace.raw[i:i+step_size]

            if progress_callback is not None:
                progress_callback((float(i+step_size) / self.segy.tracecount) * 100)

        if self.file_activity_monitor is not None:
            self.file_activity_monitor.set_file_read_finished()

        ils = all_traces.reshape(len(self.segy.ilines), len(self.segy.xlines), self.segy.samples)
        xls = ils.transpose(1, 0, 2)

        self.depth_slices = ils.transpose(2, 0, 1)

        self.iline_slices = SlicesWrapper(self, self.segy.ilines.tolist(), ils, read_from_file=False)
        self.xline_slices = SlicesWrapper(self, self.segy.xlines.tolist(), xls, read_from_file=False)
        self.min_max = self.identify_min_max(all_traces)
        return True

    @staticmethod
    def identify_min_max(all_traces):

        # removing positive and negative infinite numbers
        all_traces[all_traces == np.inf] = 0
        all_traces[all_traces == -np.inf] = 0

        min_value = np.nanmin(all_traces)
        max_value = np.nanmax(all_traces)

        return min_value, max_value

    @property
    def samples(self):
        return self.segy.samples

    @property
    def xlines(self):
        return self.segy.xlines.tolist()

    @property
    def ilines(self):
        return self.segy.ilines.tolist()

    @property
    def iline(self):
        return self.iline_slices

    @property
    def xline(self):
        return self.xline_slices

    @property
    def depth_slice(self):
        return self.depth_slices
