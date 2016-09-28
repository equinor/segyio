from cwrap import BaseCEnum


class SegySampleFormat(BaseCEnum):
    TYPE_NAME = "SegySampleFormat"

    IBM_FLOAT_4_BYTE = None
    SIGNED_INTEGER_4_BYTE = None
    SIGNED_SHORT_2_BYTE = None
    FIXED_POINT_WITH_GAIN_4_BYTE = None
    IEEE_FLOAT_4_BYTE = None
    NOT_IN_USE_1 = None
    NOT_IN_USE_2 = None
    SIGNED_CHAR_1_BYTE = None

SegySampleFormat.addEnum("IBM_FLOAT_4_BYTE", 1)
SegySampleFormat.addEnum("SIGNED_INTEGER_4_BYTE", 2)
SegySampleFormat.addEnum("SIGNED_SHORT_2_BYTE", 3)
SegySampleFormat.addEnum("FIXED_POINT_WITH_GAIN_4_BYTE", 4)
SegySampleFormat.addEnum("IEEE_FLOAT_4_BYTE", 5)
SegySampleFormat.addEnum("NOT_IN_USE_1", 6)
SegySampleFormat.addEnum("NOT_IN_USE_2", 7)
SegySampleFormat.addEnum("SIGNED_CHAR_1_BYTE", 8)

