from cwrap import MetaCWrap, Prototype

class BaseCEnum(object):
    __metaclass__ = MetaCWrap
    enum_namespace = {}

    def __init__(self, *args, **kwargs):
        if not self in self.enum_namespace[self.__class__]:
            raise NotImplementedError("Can not be instantiated directly!")

    def __new__(cls, *args, **kwargs):
        if len(args) == 1:
            enum = cls.__resolveEnum(args[0])

            if enum is None:
                raise ValueError("Unknown enum value: %i" % args[0])

            return enum
        else:
            obj = super(BaseCEnum, cls).__new__(cls, *args)
            obj.name = None
            obj.value = None
            return obj

    @classmethod
    def from_param(cls, c_class_object):
        if not isinstance(c_class_object, BaseCEnum):
            raise ValueError("c_class_object must be an BaseCEnum instance!")
        return c_class_object.value

    @classmethod
    def addEnum(cls, name, value):
        if not isinstance(value, int):
            raise ValueError("Value must be an integer!")

        enum = cls.__new__(cls)
        enum.name = name
        enum.value = value

        setattr(cls, name, enum)

        if not cls.enum_namespace.has_key(cls):
            cls.enum_namespace[cls] = []

        cls.enum_namespace[cls].append(enum)

    @classmethod
    def enums(cls):
        return list(cls.enum_namespace[cls])

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.value == other.value

        if isinstance(other, int):
            return self.value == other

        return False

    def __str__(self):
        return self.name

    def __add__(self, other):
        self.__assertOtherIsSameType(other)
        value = self.value + other.value
        return self.__resolveOrCreateEnum(value)

    def __or__(self, other):
        self.__assertOtherIsSameType(other)
        value = self.value | other.value
        return self.__resolveOrCreateEnum(value)


    def __xor__(self, other):
        self.__assertOtherIsSameType(other)
        value = self.value ^ other.value
        return self.__resolveOrCreateEnum(value)

    def __and__(self, other):
        self.__assertOtherIsSameType(other)
        value = self.value & other.value
        return self.__resolveOrCreateEnum(value)

    def __int__(self):
        return self.value

    def __contains__(self, item):
        return self & item == item

    @classmethod
    def __createEnum(cls, value):
        enum = cls.__new__(cls)
        enum.name = "Unnamed '%s' enum with value: %i" % (str(cls.__name__), value)
        enum.value = value
        return enum

    @classmethod
    def __resolveOrCreateEnum(cls, value):
        enum = cls.__resolveEnum(value)

        if enum is not None:
            return enum

        return cls.__createEnum(value)

    @classmethod
    def __resolveEnum(cls, value):
        for enum in cls.enum_namespace[cls]:
            if enum.value == value:
                return enum
        return None

    def __assertOtherIsSameType(self, other):
        assert isinstance(other, self.__class__), "Can only operate on enums of same type: %s =! %s" % (
            self.__class__.__name__, other.__class__.__name__)

    @classmethod
    def registerEnum(cls, library, enum_name):
        Prototype.registerType(enum_name, cls)
