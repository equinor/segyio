import collections
import numpy as np
try: from future_builtins import zip
except ImportError: pass

from .tracesortingformat import TraceSortingFormat as Sorting

class Depth(collections.Sequence):

    def __init__(self, fd):
        self.filehandle = fd.xfd
        self.dtype = fd.dtype

        if fd.unstructured:
            self.shape = fd.tracecount
        elif fd.sorting == Sorting.CROSSLINE_SORTING:
            self.shape = (len(fd.ilines), len(fd.xlines))
        else:
            self.shape = (len(fd.xlines), len(fd.ilines))

        self.length = len(fd.samples)
        self.offsets = len(fd.offsets)

    def __getitem__(self, i):
        """depth[i]

        *i*th depth, a horizontal cross-section of the file, starting at 0.
        ``depth[i]`` returns a numpy array, and changes to this array will
        *not* be reflected on disk.

        When i is a slice, a generator of numpy arrays is returned.

        Parameters
        ----------

        i : int or slice

        Returns
        -------

        depth : numpy.ndarray of dtype or generator of numpy.ndarray of dtype

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

        """

        try:
            if i < 0:
                i += len(self)

            if not 0 <= i < len(self):
                # in python2, int-slice comparison does not raise a type error,
                # (but returns False), so force a type-error if this still
                # isn't an int-like.
                i += 0
                msg = 'Trace out of range: 0 <= {} < {}'
                raise IndexError(msg.format(i, len(self)))

            buf = np.empty(self.shape, dtype=self.dtype)
            return self.filehandle.getdepth(i, buf.size, self.offsets, buf)

        except TypeError:
            def gen():
                x = np.empty(self.shape, dtype=self.dtype)
                y = np.copy(x)

                for j in range(*i.indices(len(self))):
                    self.filehandle.getdepth(j, x.size, self.offsets, x)
                    x, y = y, x
                    yield y

            return gen()

    def __setitem__(self, depth, val):
        """depth[i] = val

        Write the *i*th depth, a horizontal cross-section, of the file,
        starting at 0. It accepts any array_like, but `val` must be at least as
        big as the underlying data slice.

        If `val` is longer than the underlying trace, `val` is essentially truncated.

        Parameters
        ----------

        i : int or slice
        val : array_like

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

        """
        try:
            depth += 0
            flat = self.shape[0] * self.shape[1]
            x = np.asarray(val)
            x = np.ascontiguousarray(x.reshape(flat), dtype=self.dtype)
            self.filehandle.putdepth(depth, x.size, self.offsets, x)

        except TypeError:
            for i, x in zip(range(*depth.indices(len(self))), val):
                self[i] = x

    def __len__(self):
        return self.length

    def __iter__(self):
        return self[:]
