import collections
import itertools
try: from future_builtins import zip
except ImportError: pass

import segyio
from .line import Line as LineBase
from .field import Field

class Line(LineBase):
    # a lot of implementation details are shared between reading data traces
    # line-by-line and trace headers line-by-line, so (ab)use inheritance for
    # __len__, keys() etc., however, the __getitem__ is  way different and is re-implemented

    def __init__(self, header, base, direction):
        super(Line, self).__init__(header.segy,
                                   base.lines,
                                   base.length,
                                   base.stride,
                                   sorted(base.offsets.keys()),
                                   'header.' + direction,
                                  )
        self.header = header

    def __getitem__(self, index):
        """line[i] or line[i, o]

        The line `i`, or the line `i` at a specific offset `o`. ``line[i]``
        returns an iterable of `Field` objects, and changes to these *will* be
        reflected on disk.

        The `i` and `o` are *keys*, and should correspond to the line- and
        offset labels in your file, and in the `ilines`, `xlines`, and
        `offsets` attributes.

        Slices can contain lines and offsets not in the file, and like with
        list slicing, these are handled gracefully and ignored.

        When `i` or `o` is a slice, a generator of iterables of headers are
        returned.

        When both `i` and `o` are slices, one generator is returned for the
        product `i` and `o`, and the lines are yielded offsets-first, roughly
        equivalent to the double for loop::

            >>> for line in lines:
            ...     for off in offsets:
            ...         yield line[line, off]
            ...

        Parameters
        ----------

        i : int or slice
        o : int or slice

        Returns
        -------

        line : iterable of Field or generator of iterator of Field

        Raises
        ------

        KeyError
            If `i` or `o` don't exist

        Notes
        -----

        .. versionadded:: 1.1

        """
        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        try:
            start = self.heads[index] + self.offsets[offset]
        except TypeError:
            # index is either unhashable (because it's a slice), or offset is a
            # slice.
            pass

        else:
            step = self.stride * len(self.offsets)
            return self.header[start::step]

        def gen():
            irange, orange = self.ranges(index, offset)
            for line in irange:
                for off in orange:
                    yield self[line, off]

        return gen()

    def __setitem__(self, index, val):
        """line[i] = val or line[i, o] = val

        Follows the same rules for indexing and slicing as ``line[i]``. If `i`
        is an int, and `val` is a dict or Field, that value is replicated and
        assigned to every trace header in the line, otherwise it's treated as
        an iterable, and each trace in the line is assigned the ``next()``
        yielded value.

        If `i` or `o` is a slice, `val` must be an iterable.

        In either case, if the `val` iterable is exhausted before the line(s),
        assignment stops with whatever is written so far.

        Parameters
        ----------

        i : int or slice
        offset : int or slice
        val : field or dict_like or iterable of field or iterable of dict_like

        Raises
        ------

        KeyError
            If `i` or `o` don't exist

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists.

        """
        offset = self.default_offset
        try: index, offset = index
        except TypeError: pass

        try: start = self.heads[index] + self.offsets[offset]
        except TypeError: pass

        else:
            try:
                if hasattr(val, 'keys'):
                    val = itertools.repeat(val)
            except TypeError:
                # already an iterable
                pass

            step = self.stride * len(self.offsets)
            self.header[start::step] = val
            return

        irange, orange = self.ranges(index, offset)
        val = iter(val)
        for line in irange:
            for off in orange:
                try:
                    self[line, off] = next(val)
                except StopIteration:
                    return

class Header(collections.Sequence):
    def __init__(self, segy):
        self.segy = segy
        self.length = segy.tracecount

    @staticmethod
    def _header_buffer(buf=None):
        if buf is None:
            buf = bytearray(segyio._segyio.thsize())
        return buf

    def __getitem__(self, i):
        """header[i]

        *i*th header of the file, starting at 0

        Parameters
        ----------

        i : int or slice

        Returns
        -------

        field : Field
            dict_like header

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists

        """
        try:
            if i < 0:
                i += len(self)

            if not 0 <= i < len(self):
                # in python2, int-slice comparison does not raise a type error,
                # (but returns False), so force a type-error if this still
                # isn't an int-like.
                _ = i + 0
                msg = 'Header out of range: 0 <= {} < {}'
                raise IndexError(msg.format(i, len(self)))

            return Field.trace(traceno = i, segy = self.segy)

        except TypeError:
            def gen():
                # double-buffer the header. when iterating over a range, we
                # want to make sure the visible change happens as late as
                # possible, and that in the case of exception the last valid
                # header was untouched. this allows for some fancy control
                # flow, and more importantly helps debugging because you can
                # fully inspect and interact with the last good value.
                x = Field.trace(None, self.segy)
                buf = bytearray(x.buf)
                for j in range(*i.indices(len(self))):
                    # skip re-invoking __getitem__, just update the buffer
                    # directly with fetch, and save some initialisation work
                    buf = x.fetch(buf, j)
                    x.buf[:] = buf
                    x.traceno = j
                    yield x

            return gen()

    def __setitem__(self, i, val):
        """header[i] = val

        Write *i*th header of the file, starting at 0

        Parameters
        ----------

        i : int or slice
        val : Field or array_like of dict_like

        Notes
        -----

        .. versionadded:: 1.1

        Behaves like [] for lists

        """

        x = self[i]

        try:
            x.update(val)
        except AttributeError:
            if isinstance(val, Field) or isinstance(val, dict):
                val = itertools.repeat(val)

            for h, v in zip(x, val):
                h.update(v)

    def __repr__(self):
        return 'Header(traces = {})'.format(len(self))

    def __len__(self):
        return self.length

    @property
    def iline(self):
        return Line(self, self.segy.iline, 'inline')

    @iline.setter
    def iline(self, value):
        """Write iterables to lines

        Examples:
            Supports writing to *all* crosslines via assignment, regardless of
            data source and format. Will respect the sample size and structure
            of the file being assigned to, so if the argument traces are longer
            than that of the file being written to the surplus data will be
            ignored. Uses same rules for writing as `f.iline[i] = x`.
        """
        for i, src in zip(self.segy.ilines, value):
            self.iline[i] = src

    @property
    def xline(self):
        return Line(self, self.segy.xline, 'crossline')

    @xline.setter
    def xline(self, value):
        """Write iterables to lines

        Examples:
            Supports writing to *all* crosslines via assignment, regardless of
            data source and format. Will respect the sample size and structure
            of the file being assigned to, so if the argument traces are longer
            than that of the file being written to the surplus data will be
            ignored. Uses same rules for writing as `f.xline[i] = x`.
        """

        for i, src in zip(self.segy.xlines, value):
            self.xline[i] = src
