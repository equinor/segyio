from ctypes import pointer, c_long, c_int, c_bool, c_float, c_double, c_byte, c_short, c_char, c_ubyte, c_ushort, c_uint, c_ulong, c_size_t

from cwrap import MetaCWrap


class BaseCValue(object):
    __metaclass__ = MetaCWrap
    DATA_TYPE = None
    LEGAL_TYPES = [c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint, c_long, c_ulong, c_bool, c_char, c_float, c_double, c_size_t]
    
    def __init__(self, value):
        super(BaseCValue, self).__init__()
        
        if not self.DATA_TYPE in self.LEGAL_TYPES:
            raise ValueError("DATA_TYPE must be one of these CTypes classes: %s" % BaseCValue.LEGAL_TYPES)
        
        self.__value = self.cast(value)


    def value(self):
        return self.__value.value

    @classmethod
    def storageType(cls):
        return cls.type()

    @classmethod
    def type(cls):
        return cls.DATA_TYPE

    @classmethod
    def cast(cls, value):
        return cls.DATA_TYPE(value)

    def setValue(self, value):
        self.__value = self.cast(value)

    def asPointer(self):
        return pointer(self.__value)

    @classmethod
    def from_param(cls, c_value_object):
        if c_value_object is not None and not isinstance(c_value_object, BaseCValue):
            raise ValueError("c_class_object must be a BaseCValue instance!")

        return c_value_object.__value

