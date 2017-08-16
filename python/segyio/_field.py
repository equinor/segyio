import segyio
from segyio import BinField
from segyio import TraceField

class Field(object):
    _bin_keys = [x for x in BinField.enums()
                 if  x != BinField.Unassigned1
                 and x != BinField.Unassigned2]

    _tr_keys = [x for x in TraceField.enums()
                if  x != TraceField.UnassignedInt1
                and x != TraceField.UnassignedInt2]


    def __init__(self, buf, write, field_type, traceno=None, keys = _tr_keys):
        self.buf = buf
        self.traceno = traceno
        self._field_type = field_type
        self._keys = keys
        self._write = write

    def _get_field(self, *args):
        return segyio._segyio.get_field(self.buf, *args)

    def _set_field(self, *args):
        return segyio._segyio.set_field(self.buf, *args)

    def __getitem__(self, field):
        # add some structure so we can always iterate over fields
        if isinstance(field, int) or isinstance(field, self._field_type):
            field = [field]

        d = {self._field_type(f): self._get_field(f) for f in field}

        # unpack the dictionary. if header[field] is requested, a
        # plain, unstructed output is expected, but header[f1,f2,f3]
        # yields a dict
        if len(d) == 1:
            return d.popitem()[1]

        return d

    def keys(self):
        return list(self._keys)

    def values(self):
        return map(self._get_field, self.keys())

    def items(self):
        return zip(self.keys(), self.values())

    def __contains__(self, key):
        return key in self._keys

    def __len__(self):
        return len(self._keys)

    def __iter__(self):
        return iter(self._keys)

    def __setitem__(self, field, val):
        self._set_field(field, val)
        self._write(self.buf, self.traceno)

    def update(self, value):
        if type(self) is type(value):
            buf = value.buf
        else:
            buf = self.buf

            # iter() on a dict only gives values, need key-value pairs
            try: value = value.items()
            except AttributeError: pass

            for k, v in value:
                self._set_field(int(k), v)

        self._write(buf, self.traceno)

    @classmethod
    def binary(cls, segy):
        try:
            buf = segyio._segyio.read_binaryheader(segy.xfd)
        except IOError:
            # the file was probably newly created and the binary header hasn't
            # been written yet.  if this is the case we want to try and write
            # it. if the file was broken, permissions were wrong etc writing
            # will fail too
            buf = segyio._segyio.empty_binaryheader()

        def wr(buf, *_):
            segyio._segyio.write_binaryheader(segy.xfd, buf)

        return Field(buf, write=wr, field_type=BinField, keys = Field._bin_keys)

    @classmethod
    def trace(cls, buf, traceno, segy):
        if traceno < 0:
            traceno = segy.tracecount + traceno

        if traceno >= segy.tracecount or traceno < 0:
            raise IndexError("Header out of range: 0 <= %d < %d" % (traceno, segy.tracecount))

        if buf is None:
            buf = segyio._segyio.empty_traceheader()

        try:
            segyio._segyio.read_traceheader(segy.xfd, traceno, buf, segy._tr0, segy._bsz)
        except IOError:
            # the file was probably newly created and the trace header hasn't
            # been written yet.  if this is the case we want to try and write
            # it. if the file was broken, permissions were wrong etc writing
            # will fail too
            pass

        def wr(buf, traceno):
            segyio._segyio.write_traceheader(segy.xfd, traceno, buf, segy._tr0, segy._bsz)

        return Field(buf, traceno=traceno,
                          write=wr,
                          field_type=TraceField,
                          keys=Field._tr_keys)

    def __repr__(self):
        return self[self.keys()].__repr__()
