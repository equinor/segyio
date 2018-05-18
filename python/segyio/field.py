import collections

import segyio
from segyio import BinField
from segyio import TraceField

class Field(collections.MutableMapping):
    _bin_keys = [x for x in BinField.enums()
                 if  x != BinField.Unassigned1
                 and x != BinField.Unassigned2]

    _tr_keys = [x for x in TraceField.enums()
                if  x != TraceField.UnassignedInt1
                and x != TraceField.UnassignedInt2]

    def __init__(self, buf, kind, traceno = None, filehandle = None, readonly = True):
        # do setup of kind/keys first, so that keys() work. if this method
        # throws, we want repr() to be well-defined for backtrace, and that
        # requires _keys
        if kind == 'binary':
            self._keys = self._bin_keys
            self.kind = BinField
        elif kind == 'trace':
            self._keys = self._tr_keys
            self.kind = TraceField
        else:
            raise ValueError('Unknown header type {}'.format(kind))

        self.buf = buf
        self.traceno = traceno
        self.filehandle = filehandle
        self.getfield = segyio._segyio.getfield
        self.putfield = segyio._segyio.putfield

        self.readonly = readonly

    def fetch(self, buf = None, traceno = None):
        """Fetch the header from disk

        This object will read header when it is constructed, which means it
        might be out-of-date if the file is updated through some other handle.
        This method is largely meant for internal use - if you need to reload
        disk contents, use ``reload``.

        Fetch does not update any internal state (unless `buf` is ``None`` on a
        trace header, and the read succeeds), but returns the fetched header
        contents.

        This method can be used to reposition the trace header, which is useful
        for constructing generators.

        If this is called on a writable, new file, and this header has not yet
        been written to, it will successfully return an empty buffer that, when
        written to, will be reflected on disk.

        Parameters
        ----------

        buf : bytearray
            buffer to read into instead of ``self.buf``
        traceno : int

        Returns
        -------

        buf : bytearray

        Notes
        -----

        .. versionadded:: 1.6

        This method is not intended as user-oriented functionality, but might
        be useful in high-performance code.

        """

        if buf is None:
            buf = self.buf

        if traceno is None:
            traceno = self.traceno

        try:
            if self.kind == TraceField:
                if traceno is None: return buf
                return self.filehandle.getth(traceno, buf)
            else:
                return self.filehandle.getbin()
        except IOError:
            if not self.readonly:
                # the file was probably newly created and the trace header
                # hasn't been written yet, and we set the buffer to zero. if
                # this is the case we want to try and write it later, and if
                # the file was broken, permissions were wrong etc writing will
                # fail too
                #
                # if the file is opened read-only and this happens, there's no
                # way to actually write and the error is an actual error
                return bytearray(len(self.buf))
            else: raise

    def reload(self):
        """Reload the header from disk

        This object will read header when it is constructed, which means it
        might be out-of-date if the file is updated through some other handle.

        It's rarely required to call this method, and it's a symptom of fragile
        code. However, if you have multiple handles to the same header, it
        might be necessary. Consider the following example::

            >>> x = f.header[10]
            >>> y = f.header[10]
            >>> x[1, 5]
            { 1: 5, 5: 10 }
            >>> y[1, 5]
            { 1: 5, 5: 10 }
            >>> x[1] = 6
            >>> x[1], y[1] # write to x[1] is invisible to y
            6, 5
            >>> y.reload()
            >>> x[1], y[1]
            6, 6
            >>> x[1] = 5
            >>> x[1], y[1]
            5, 6
            >>> y[5] = 1
            >>> x.reload()
            >>> x[1], y[1, 5] # the write to x[1] is lost
            6, { 1: 6; 5: 1 }

        In segyio, headers writes are atomic, and the write to disk writes the
        full cache. If this cache is out of date, some writes might get lost,
        even though the updates are compatible.

        The fix to this issue is either to use ``reload`` and maintain buffer
        consistency, or simply don't let header handles alias and overlap in
        lifetime.

        Notes
        -----

        .. versionadded:: 1.6

        """

        self.buf = self.fetch(buf = self.buf)
        return self

    def flush(self):
        """Commit backing storage to disk

        This method is largely internal, and it is not necessary to call this
        from user code. It should not be explicitly invoked and may be removed
        in future versions.
        """

        if self.kind == TraceField:
            self.filehandle.putth(self.traceno, self.buf)

        elif self.kind == BinField:
            self.filehandle.putbin(self.buf)

        else:
            msg = 'Object corrupted: kind {} not valid'
            raise RuntimeError(msg.format(self.kind))

    def __getitem__(self, key):
        """d[key]

        Read the associated value of `key`. Raises ``KeyError`` if `key` is not
        in the header.

        `key` can be any iterable, to retrieve multiple keys at once. In this
        case, a mapping of key -> value is returned.

        Parameters
        ----------
        key : int, or iterable of int

        Returns
        -------

        value : int

        Notes
        -----

        .. versionadded:: 1.1

        .. note::
            Since version 1.6, KeyError is approperiately raised on key misses,
            whereas ``IndexError`` was raised before. This is an old bug, since
            header types were documented to be dict-like. If you rely on
            catching key-miss errors in your code, you might want to handle
            both ``IndexError`` and ``KeyError`` for multi-version robustness.

        .. warning::
            segyio considers reads/writes full headers, not individual fields,
            and does the read from disk when this class is constructed. If the
            file is updated through some other handle, including a secondary
            access via `f.header`, this cache might be out-of-date.

        Examples
        --------

        >>> d[3213]
        15000

        >>> d[37, 189]
        { 37: 5, 189: 2484 }

        """
        try: return self.getfield(self.buf, int(key))
        except TypeError: pass

        return {self.kind(k): self.getfield(self.buf, int(k)) for k in key}

    def __setitem__(self, key, val):
        """d[key] = value

        Set ``d[key]`` to `value`. Raises ``KeyError`` if `key` is not in the
        header. Setting keys commits changes to disk, although the changes may
        not be visible until the kernel schedules the write.

        Unlike ``d[key]``, this method does not support assigning multiple values
        at once. To set multiple values at once, use the `update` method.

        Parameters
        ----------

        key : int
        val : int

        Returns
        -------

        val : int
            The value set

        Notes
        -----

        .. versionadded:: 1.1

        .. note::
            Since version 1.6, KeyError is approperiately raised on key misses,
            whereas ``IndexError`` was raised before. This is an old bug, since
            header types were documented to be dict-like. If you rely on
            catching key-miss errors in your code, you might want to handle
            both ``IndexError`` and ``KeyError`` for multi-version robustness.

        .. warning::
            segyio considers reads/writes full headers, not individual fields,
            and does the read from disk when this class is constructed. If the
            file is updated through some other handle, including a secondary
            access via `f.header`, this cache might be out-of-date. That means
            writing an individual field will write the full header to disk,
            possibly overwriting previously set values.

        """

        self.putfield(self.buf, key, val)
        self.flush()

        return val

    def __delitem__(self, key):
        """del d[key]

        'Delete' the `key` by setting value to zero. Equivalent
        to ``d[key] = 0``.
        """

        self[key] = 0

    def keys(self):
        return list(self._keys)

    def __len__(self):
        return len(self._keys)

    def __iter__(self):
        return iter(self._keys)

    def __eq__(self, other):
        if not isinstance(other, collections.Mapping):
            return NotImplemented

        if len(self) != len(other):
            return False

        def intkeys(d):
            return { int(k): v for k, v in d.items() }

        return intkeys(self) == intkeys(other)


    def update(self, value):
        """d.update([value])

        Overwrite the values in `d` with the keys from `value`. If any key in
        `value` is invalid in `d`, ``KeyError`` is raised.

        This method is atomic - either all values in `value` are set in `d`, or
        none are. ``update`` does not commit a partially-updated version to
        disk.

        Notes
        -----

        .. versionchanged:: 1.3
            Support for common dict operations (update, keys, values)

        .. versionchanged:: 1.6
            Atomicity guarantee

        Examples
        --------

        >>> e = { 1: 10, 9: 5 }
        >>> d.update(e)
        >>> l = [ (105, 11), (169, 4) ]
        >>> d.update(l)

        """

        buf = bytearray(self.buf)

        # iter() on a dict only gives values, need key-value pairs
        try: value = value.items()
        except AttributeError: pass

        for k, v in value:
            self.putfield(buf, int(k), v)

        self.buf = buf

        self.flush()

    @classmethod
    def binary(cls, segy):
        buf = bytearray(segyio._segyio.binsize())
        return Field(buf, kind='binary',
                          filehandle=segy.xfd,
                          readonly=segy.readonly,
                    ).reload()

    @classmethod
    def trace(cls, traceno, segy):
        buf = bytearray(segyio._segyio.thsize())
        return Field(buf, kind='trace',
                          traceno=traceno,
                          filehandle=segy.xfd,
                          readonly=segy.readonly,
                    ).reload()

    def __repr__(self):
        return repr(self[self.keys()])
