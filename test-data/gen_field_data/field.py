import struct


class Field:

    MIN_MAX_VALUES = {
        "i": (-(2**31), 2**31 - 1),
        "h": (-(2**15), 2**15 - 1),
        "B": (0, 0xFF - 1),
        "H": (0, 0xFFFF - 1),
        "d": (-(2**50), 2**50),
        "s": (0, 0),
        "Q": (0, 2**64 - 1),
    }

    def __init__(self, name: str, d_type: str, endian: str):
        self.f_struct = struct.Struct(endian + d_type)
        self.name = name
        self.type = d_type
        self.min_value = Field.MIN_MAX_VALUES[d_type[-1]][0]
        self.max_value = Field.MIN_MAX_VALUES[d_type[-1]][1]
        self.min_max = (
            0,
            self.min_value,
            self.max_value,
        )  # 1: min value, -1: max value
        self.value = None
        self.length = self.f_struct.size

    def __str__(self):
        return f"{self.name} ({self.length} bytes) - {self.type}"
