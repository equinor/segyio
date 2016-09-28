import ctypes
import inspect
import re

import sys

class TypeDefinition(object):
    def __init__(self, type_class_or_function, is_return_type, storage_type):
        self.storage_type = storage_type
        self.is_return_type = is_return_type
        self.type_class_or_function = type_class_or_function


REGISTERED_TYPES = {}
""":type: dict[str,TypeDefinition]"""


def registerType(type_name, type_class_or_function, is_return_type=True, storage_type=None):
    if type_name in REGISTERED_TYPES:
        raise PrototypeError("Type: '%s' already registered!" % type_name)

    REGISTERED_TYPES[type_name] = TypeDefinition(type_class_or_function, is_return_type, storage_type)

    # print("Registered: %s for class: %s" % (type_name, repr(type_class_or_function)))

registerType("void", None)
registerType("void*", ctypes.c_void_p)
registerType("uint", ctypes.c_uint)
registerType("uint*", ctypes.POINTER(ctypes.c_uint))
registerType("int", ctypes.c_int)
registerType("int*", ctypes.POINTER(ctypes.c_int))
registerType("int64", ctypes.c_int64)
registerType("int64*", ctypes.POINTER(ctypes.c_int64))
registerType("size_t", ctypes.c_size_t)
registerType("size_t*", ctypes.POINTER(ctypes.c_size_t))
registerType("bool", ctypes.c_bool)
registerType("bool*", ctypes.POINTER(ctypes.c_bool))
registerType("long", ctypes.c_long)
registerType("long*", ctypes.POINTER(ctypes.c_long))
registerType("char", ctypes.c_char)
registerType("char*", ctypes.c_char_p)
registerType("char**", ctypes.POINTER(ctypes.c_char_p))
registerType("float", ctypes.c_float)
registerType("float*", ctypes.POINTER(ctypes.c_float))
registerType("double", ctypes.c_double)
registerType("double*", ctypes.POINTER(ctypes.c_double))
registerType("py_object", ctypes.py_object)

PROTOTYPE_PATTERN = "(?P<return>[a-zA-Z][a-zA-Z0-9_*]*) +(?P<function>[a-zA-Z]\w*) *[(](?P<arguments>[a-zA-Z0-9_*, ]*)[)]"

class PrototypeError(Exception):
    pass


class Prototype(object):
    pattern = re.compile(PROTOTYPE_PATTERN)

    def __init__(self, lib, prototype, bind=False):
        super(Prototype, self).__init__()
        self._lib = lib
        self._prototype = prototype
        self._bind = bind
        self._func = None
        self.__name__ = prototype
        self._resolved = False


    def _parseType(self, type_name):
        """Convert a prototype definition type from string to a ctypes legal type."""
        type_name = type_name.strip()

        if type_name in REGISTERED_TYPES:
            type_definition = REGISTERED_TYPES[type_name]
            return type_definition.type_class_or_function, type_definition.storage_type
        raise ValueError("Unknown type: %s" % type_name)


    def shouldBeBound(self):
        return self._bind

    def resolve(self):
        match = re.match(Prototype.pattern, self._prototype)
        if not match:
            raise PrototypeError("Illegal prototype definition: %s\n" % self._prototype)
        else:
            restype = match.groupdict()["return"]
            function_name = match.groupdict()["function"]
            self.__name__ = function_name
            arguments = match.groupdict()["arguments"].split(",")

            try:
                func = getattr(self._lib, function_name)
            except AttributeError:
                raise PrototypeError("Can not find function: %s in library: %s" % (function_name , self._lib))

            if not restype in REGISTERED_TYPES or not REGISTERED_TYPES[restype].is_return_type:
                sys.stderr.write("The type used as return type: %s is not registered as a return type.\n" % restype)

                return_type = self._parseType(restype)

                if inspect.isclass(return_type):
                    sys.stderr.write("  Correct type may be: %s_ref or %s_obj.\n" % (restype, restype))

                return None

            return_type, storage_type = self._parseType(restype)

            func.restype = return_type

            if storage_type is not None:
                func.restype = storage_type

                def returnFunction(result, func, arguments):
                    return return_type(result)

                func.errcheck = returnFunction

            if len(arguments) == 1 and arguments[0].strip() == "":
                func.argtypes = []
            else:
                argtypes = [self._parseType(arg)[0] for arg in arguments]
                if len(argtypes) == 1 and argtypes[0] is None:
                    argtypes = []
                func.argtypes = argtypes

            self._func = func


    def __call__(self, *args):
        if not self._resolved:
            self.resolve()
            self._resolved = True

        if self._func is None:
            raise PrototypeError("Prototype has not been properly resolved!")
        return self._func(*args)

    def __repr__(self):
        bound = ""
        if self.shouldBeBound():
            bound = ", bind=True"

        return 'Prototype("%s"%s)' % (self._prototype, bound)

    @classmethod
    def registerType(cls, type_name, type_class_or_function, is_return_type=True, storage_type=None):
        registerType(type_name, type_class_or_function, is_return_type=is_return_type, storage_type=storage_type)
