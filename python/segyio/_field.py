import segyio
from segyio import BinField
from segyio import TraceField


class Field:
    def __init__(self, buf, write, field_type, traceno=None):
        self.buf = buf
        self.traceno = traceno
        self._get_field = segyio._segyio.get_field
        self._set_field = segyio._segyio.set_field
        self._field_type = field_type
        self._write = write

    def __getitem__(self, field):

        # add some structure so we can always iterate over fields
        if isinstance(field, int) or isinstance(field, self._field_type):
            field = [field]

        d = {self._field_type(f): self._get_field(self.buf, f) for f in field}

        # unpack the dictionary. if header[field] is requested, a
        # plain, unstructed output is expected, but header[f1,f2,f3]
        # yields a dict
        if len(d) == 1:
            return d.values()[0]

        return d

    def __setitem__(self, field, val):
        self._set_field(self.buf, field, val)
        self._write(self.buf, self.traceno)

    def update(self, value):
        buf = self.buf
        if isinstance(value, dict):
            for k, v in value.items():
                self._set_field(buf, int(k), v)
        else:
            buf = value.buf

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

        return Field(buf, write=wr, field_type=BinField)

    @classmethod
    def trace(cls, buf, traceno, segy):
        if traceno >= segy.tracecount:
            raise IndexError("Trace number out of range: 0 <= %d < %d" % (traceno, segy.tracecount))

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

        return Field(buf, traceno=traceno, write=wr, field_type=TraceField)