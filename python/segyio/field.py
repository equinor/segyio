try:
    from collections.abc import Mapping # noqa
    from collections.abc import MutableMapping # noqa
except ImportError:
    from collections import Mapping # noqa
    from collections import MutableMapping # noqa

import segyio
from .binfield import BinField
from .tracefield import TraceField

class Field(MutableMapping):
    """
    The Field implements the dict interface, with a fixed set of keys. It's
    used for both binary- and trace headers. Any modifications to this
    dict_like object will be reflected on disk.

    The keys can be integers, int_likes, or enumerations such as BinField,
    TraceField, and su. If raw, numerical offsets are used they must align with
    the defined byte offsets by the SEGY specification.

    Notes
    -----
    .. versionadded:: 1.1

    .. versionchanged:: 1.3
        common dict operations (update, keys, values)

    .. versionchanged:: 1.6
        more common dict operations (MutableMapping)
    """
    _bin_keys = [x for x in BinField.enums()
                 if  x != BinField.Unassigned1
                 and x != BinField.Unassigned2]

    _tr_keys = [x for x in TraceField.enums()
                if  x != TraceField.UnassignedInt1
                and x != TraceField.UnassignedInt2]

    _kwargs = {
        'tracl' : TraceField.TRACE_SEQUENCE_LINE,
        'tracr' : TraceField.TRACE_SEQUENCE_FILE,
        'fldr'  : TraceField.FieldRecord,
        'tracf' : TraceField.TraceNumber,
        'ep'    : TraceField.EnergySourcePoint,
        'cdp'   : TraceField.CDP,
        'cdpt'  : TraceField.CDP_TRACE,
        'trid'  : TraceField.TraceIdentificationCode,
        'nvs'   : TraceField.NSummedTraces,
        'nhs'   : TraceField.NStackedTraces,
        'duse'  : TraceField.DataUse,
        'offset': TraceField.offset,
        'gelev' : TraceField.ReceiverGroupElevation,
        'selev' : TraceField.SourceSurfaceElevation,
        'sdepth': TraceField.SourceDepth,
        'gdel'  : TraceField.ReceiverDatumElevation,
        'sdel'  : TraceField.SourceDatumElevation,
        'swdep' : TraceField.SourceWaterDepth,
        'gwdep' : TraceField.GroupWaterDepth,
        'scalel': TraceField.ElevationScalar,
        'scalco': TraceField.SourceGroupScalar,
        'sx'    : TraceField.SourceX,
        'sy'    : TraceField.SourceY,
        'gx'    : TraceField.GroupX,
        'gy'    : TraceField.GroupY,
        'counit': TraceField.CoordinateUnits,
        'wevel' : TraceField.WeatheringVelocity,
        'swevel': TraceField.SubWeatheringVelocity,
        'sut'   : TraceField.SourceUpholeTime,
        'gut'   : TraceField.GroupUpholeTime,
        'sstat' : TraceField.SourceStaticCorrection,
        'gstat' : TraceField.GroupStaticCorrection,
        'tstat' : TraceField.TotalStaticApplied,
        'laga'  : TraceField.LagTimeA,
        'lagb'  : TraceField.LagTimeB,
        'delrt' : TraceField.DelayRecordingTime,
        'muts'  : TraceField.MuteTimeStart,
        'mute'  : TraceField.MuteTimeEND,
        'ns'    : TraceField.TRACE_SAMPLE_COUNT,
        'dt'    : TraceField.TRACE_SAMPLE_INTERVAL,
        'gain'  : TraceField.GainType,
        'igc'   : TraceField.InstrumentGainConstant,
        'igi'   : TraceField.InstrumentInitialGain,
        'corr'  : TraceField.Correlated,
        'sfs'   : TraceField.SweepFrequencyStart,
        'sfe'   : TraceField.SweepFrequencyEnd,
        'slen'  : TraceField.SweepLength,
        'styp'  : TraceField.SweepType,
        'stat'  : TraceField.SweepTraceTaperLengthStart,
        'stae'  : TraceField.SweepTraceTaperLengthEnd,
        'tatyp' : TraceField.TaperType,
        'afilf' : TraceField.AliasFilterFrequency,
        'afils' : TraceField.AliasFilterSlope,
        'nofilf': TraceField.NotchFilterFrequency,
        'nofils': TraceField.NotchFilterSlope,
        'lcf'   : TraceField.LowCutFrequency,
        'hcf'   : TraceField.HighCutFrequency,
        'lcs'   : TraceField.LowCutSlope,
        'hcs'   : TraceField.HighCutSlope,
        'year'  : TraceField.YearDataRecorded,
        'day'   : TraceField.DayOfYear,
        'hour'  : TraceField.HourOfDay,
        'minute': TraceField.MinuteOfHour,
        'sec'   : TraceField.SecondOfMinute,
        'timbas': TraceField.TimeBaseCode,
        'trwf'  : TraceField.TraceWeightingFactor,
        'grnors': TraceField.GeophoneGroupNumberRoll1,
        'grnofr': TraceField.GeophoneGroupNumberFirstTraceOrigField,
        'grnlof': TraceField.GeophoneGroupNumberLastTraceOrigField,
        'gaps'  : TraceField.GapSize,
        'otrav' : TraceField.OverTravel,
        'cdpx'  : TraceField.CDP_X,
        'cdpy'  : TraceField.CDP_Y,
        'iline' : TraceField.INLINE_3D,
        'xline' : TraceField.CROSSLINE_3D,
        'sp'    : TraceField.ShotPoint,
        'scalsp': TraceField.ShotPointScalar,
        'trunit': TraceField.TraceValueMeasurementUnit,
        'tdcm'  : TraceField.TransductionConstantMantissa,
        'tdcp'  : TraceField.TransductionConstantPower,
        'tdunit': TraceField.TransductionUnit,
        'triden': TraceField.TraceIdentifier,
        'sctrh' : TraceField.ScalarTraceHeader,
        'stype' : TraceField.SourceType,
        'sedm'  : TraceField.SourceEnergyDirectionMantissa,
        'sede'  : TraceField.SourceEnergyDirectionExponent,
        'smm'   : TraceField.SourceMeasurementMantissa,
        'sme'   : TraceField.SourceMeasurementExponent,
        'smunit': TraceField.SourceMeasurementUnit,
        'uint1' : TraceField.UnassignedInt1,
        'uint2' : TraceField.UnassignedInt2,

        'jobid' : BinField.JobID,
        'lino'  : BinField.LineNumber,
        'reno'  : BinField.ReelNumber,
        'ntrpr' : BinField.Traces,
        'nart'  : BinField.AuxTraces,
        'hdt'   : BinField.Interval,
        'dto'   : BinField.IntervalOriginal,
        'hns'   : BinField.Samples,
        'nso'   : BinField.SamplesOriginal,
        'format': BinField.Format,
        'fold'  : BinField.EnsembleFold,
        'tsort' : BinField.SortingCode,
        'vscode': BinField.VerticalSum,
        'hsfs'  : BinField.SweepFrequencyStart,
        'hsfe'  : BinField.SweepFrequencyEnd,
        'hslen' : BinField.SweepLength,
        'hstyp' : BinField.Sweep,
        'schn'  : BinField.SweepChannel,
        'hstas' : BinField.SweepTaperStart,
        'hstae' : BinField.SweepTaperEnd,
        'htatyp': BinField.Taper,
        'hcorr' : BinField.CorrelatedTraces,
        'bgrcv' : BinField.BinaryGainRecovery,
        'rcvm'  : BinField.AmplitudeRecovery,
        'mfeet' : BinField.MeasurementSystem,
        'polyt' : BinField.ImpulseSignalPolarity,
        'vpol'  : BinField.VibratoryPolarity,
        'unas1' : BinField.Unassigned1,
        'rev'   : BinField.SEGYRevision,
        'trflag': BinField.TraceFlag,
        'exth'  : BinField.ExtendedHeaders,
        'unas2' : BinField.Unassigned2,
    }

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
        buf     : bytearray
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
        """
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

        Read the associated value of `key`.

        `key` can be any iterable, to retrieve multiple keys at once. In this
        case, a mapping of key -> value is returned.

        Parameters
        ----------
        key : int, or iterable of int

        Returns
        -------
        value : int or dict_like

        Notes
        -----
        .. versionadded:: 1.1

        .. note::
            Since version 1.6, KeyError is appropriately raised on key misses,
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
        Read a single value:

        >>> d[3213]
        15000

        Read multiple values at once:

        >>> d[37, 189]
        { 37: 5, 189: 2484 }
        >>> d[37, TraceField.INLINE_3D]
        { 37: 5, 189: 2484 }
        """

        try: return self.getfield(self.buf, int(key))
        except TypeError: pass

        return {self.kind(k): self.getfield(self.buf, int(k)) for k in key}

    def __setitem__(self, key, val):
        """d[key] = val

        Set d[key] to val. Setting keys commits changes to disk, although the
        changes may not be visible until the kernel schedules the write.

        Unlike d[key], this method does not support assigning multiple values
        at once. To set multiple values at once, use the `update` method.

        Parameters
        ----------
        key : int_like
        val : int_like

        Returns
        -------
        val : int
            The value set

        Notes
        -----
        .. versionadded:: 1.1

        .. note::
            Since version 1.6, KeyError is appropriately raised on key misses,
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

        Examples
        --------
        Set a value and keep in a variable:

        >>> x = header[189] = 5
        >>> x
        5
        """

        self.putfield(self.buf, key, val)
        self.flush()

        return val

    def __delitem__(self, key):
        """del d[key]

        'Delete' the key by setting value to zero. Equivalent to ``d[key] =
        0``.

        Notes
        -----
        .. versionadded:: 1.6
        """

        self[key] = 0

    def keys(self):
        """D.keys() -> a set-like object providing a view on D's keys"""
        return list(self._keys)

    def __len__(self):
        """x.__len__() <==> len(x)"""
        return len(self._keys)

    def __iter__(self):
        """x.__iter__() <==> iter(x)"""
        return iter(self._keys)

    def __eq__(self, other):
        """x.__eq__(y) <==> x == y"""

        if not isinstance(other, Mapping):
            return NotImplemented

        if len(self) != len(other):
            return False

        def intkeys(d):
            return { int(k): v for k, v in d.items() }

        return intkeys(self) == intkeys(other)


    def update(self, *args, **kwargs):
        """d.update([E, ]**F) -> None.  Update D from mapping/iterable E and F.

        Overwrite the values in `d` with the keys from `E` and `F`. If any key
        in `value` is invalid in `d`, ``KeyError`` is raised.

        This method is atomic - either all values in `value` are set in `d`, or
        none are. ``update`` does not commit a partially-updated version to
        disk.

        For kwargs, Seismic Unix-style names are supported. `BinField` and
        `TraceField` are not, because there are name collisions between them,
        although this restriction may be lifted in the future.

        Notes
        -----
        .. versionchanged:: 1.3
            Support for common dict operations (update, keys, values)

        .. versionchanged:: 1.6
            Atomicity guarantee

        .. versionchanged:: 1.6
            `**kwargs` support

        Examples
        --------
        >>> e = { 1: 10, 9: 5 }
        >>> d.update(e)
        >>> l = [ (105, 11), (169, 4) ]
        >>> d.update(l)
        >>> d.update(e, iline=189, xline=193, hour=5)
        >>> d.update(sx=7)

        """

        if len(args) > 1:
            msg = 'update expected at most 1 non-keyword argument, got {}'
            raise TypeError(msg.format(len(args)))

        buf = bytearray(self.buf)

        # Implementation largely borrowed from Mapping
        # If E present and has a .keys() method: for k in E: D[k] = E[k]
        # If E present and lacks .keys() method: for (k, v) in E: D[k] = v
        # In either case, this is followed by: for k, v in F.items(): D[k] = v
        if len(args) == 1:
            other = args[0]
            if isinstance(other, Mapping):
                for key in other:
                    self.putfield(buf, int(key), other[key])
            elif hasattr(other, "keys"):
                for key in other.keys():
                    self.putfield(buf, int(key), other[key])
            else:
                for key, value in other:
                    self.putfield(buf, int(key), value)

        for key, value in kwargs.items():
            self.putfield(buf, int(self._kwargs[key]), value)

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
