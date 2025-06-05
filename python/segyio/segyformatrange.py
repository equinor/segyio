from . import Enum

class SegyFormatRange(Enum):
    IBM_FLOAT_4_BYTE = None
    SIGNED_INTEGER_4_BYTE = (-(2**31), 2**31 - 1)
    SIGNED_SHORT_2_BYTE = (-(2**15), 2**15 - 1)
    FIXED_POINT_WITH_GAIN_4_BYTE = None
    IEEE_FLOAT_4_BYTE = None
    IEEE_FLOAT_8_BYTE = (-(2**50), 2**50)
    SIGNED_CHAR_3_BYTE = (-(2**23), 2**23 - 1)
    SIGNED_CHAR_1_BYTE = (-(2**7), 2**7 - 1)
    SIGNED_INTEGER_8_BYTE = (-(2**63), 2**63 - 1)
    UNSIGNED_INTEGER_4_BYTE = (0, 2**32 - 1)
    UNSIGNED_SHORT_2_BYTE = (0, 0xFFFF - 1)
    UNSIGNED_INTEGER_8_BYTE = (0, 2**64 - 1)
    UNSIGNED_INTEGER_3_BYTE = (0, 2**24 - 1)
    UNSIGNED_CHAR_1_BYTE = (0, 0xFF - 1)
    NOT_IN_USE_1 = None
    NOT_IN_USE_2 = None
