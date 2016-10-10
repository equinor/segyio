import itertools

import segyio


class Line:
    """ Line mode for traces and trace headers. Internal.

    The _line class provides an interface for line-oriented operations. The
    line reading operations themselves are not streaming - it's assumed than
    when a line is queried it's somewhat limited in size and will comfortably
    fit in memory, and that the full line is interesting. This also applies to
    line headers; however, all returned values support the iterable protocol so
    they work fine together with the streaming bits of this library.

    _line should not be instantiated directly by users, but rather returned
    from the iline/xline properties of file or from the header mode. Any
    direct construction of this should be considered an error.
    """

    def __init__(self, segy, length, stride, lines, other_lines, buffn, readfn, writefn, name):
        self.segy = segy
        self.len = length
        self.stride = stride
        self.lines = lines
        self.other_lines = other_lines
        self.name = name
        self.buffn = buffn
        self.readfn = readfn
        self.writefn = writefn

    def __getitem__(self, lineno, buf=None):
        """ :rtype: numpy.ndarray|collections.Iterable[numpy.ndarray]"""
        if isinstance(lineno, tuple):
            return self.__getitem__(lineno[0], lineno[1])

        buf = self.buffn(buf)

        if isinstance(lineno, slice):
            # in order to support [:end] syntax, we must make sure
            # start has a non-None value. lineno.indices() would set it
            # to 0, but we don't know if that's a reasonable value or
            # not. If start is None we set it to the first line
            if lineno.start is None:
                lineno = slice(self.lines[0], lineno.stop, lineno.step)

            def gen():
                s = set(self.lines)
                rng = xrange(*lineno.indices(self.lines[-1] + 1))

                # use __getitem__ lookup to avoid tuple
                # construction and unpacking and fast-forward
                # into the interesting code path
                for i in itertools.ifilter(s.__contains__, rng):
                    yield self.__getitem__(i, buf)

            return gen()

        else:
            try:
                lineno = int(lineno)
            except TypeError:
                raise TypeError("Must be int or slice")
            else:
                t0 = segyio._segyio.fread_trace0(lineno, len(self.other_lines), self.stride, self.lines, self.name)
                return self.readfn(t0, self.len, self.stride, buf)

    def __setitem__(self, lineno, val):
        if isinstance(lineno, slice):
            if lineno.start is None:
                lineno = slice(self.lines[0], lineno.stop, lineno.step)

            rng = xrange(*lineno.indices(self.lines[-1] + 1))
            s = set(self.lines)

            for i, x in itertools.izip(filter(s.__contains__, rng), val):
                self.__setitem__(i, x)

            return

        t0 = segyio._segyio.fread_trace0(lineno, len(self.other_lines), self.stride, self.lines, self.name)
        self.writefn(t0, self.len, self.stride, val)

    def __len__(self):
        return len(self.lines)

    def __iter__(self):
        buf = self.buffn()
        for i in self.lines:
            yield self.__getitem__(i, buf)
