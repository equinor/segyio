from segyio import Enum


class SegySampleFormat(Enum):
    IBM_FLOAT_4_BYTE = 1
    SIGNED_INTEGER_4_BYTE = 2
    SIGNED_SHORT_2_BYTE = 3
    FIXED_POINT_WITH_GAIN_4_BYTE = 4
    IEEE_FLOAT_4_BYTE = 5
    NOT_IN_USE_1 = 6
    NOT_IN_USE_2 = 7
    SIGNED_CHAR_1_BYTE = 8
