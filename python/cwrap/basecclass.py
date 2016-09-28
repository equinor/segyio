import ctypes
from cwrap import MetaCWrap


class BaseCClass(object):
    __metaclass__ = MetaCWrap

    def __init__(self, c_pointer, parent=None, is_reference=False):
        if c_pointer == 0 or c_pointer is None:
            raise ValueError("Must have a valid (not null) pointer value!")

        if c_pointer < 0:
            raise ValueError("The pointer value is negative! This may be correct, but usually is not!")

        self.__c_pointer = c_pointer
        self.__parent = parent
        self.__is_reference = is_reference

    def __new__(cls, *more, **kwargs):
        obj = super(BaseCClass, cls).__new__(cls)
        obj.__c_pointer = None
        obj.__parent = None
        obj.__is_reference = False

        return obj

    @classmethod
    def from_param(cls, c_class_object):
        if c_class_object is not None and not isinstance(c_class_object, BaseCClass):
            raise ValueError("c_class_object must be a BaseCClass instance!")

        if c_class_object is None:
            return ctypes.c_void_p()
        else:
            return ctypes.c_void_p(c_class_object.__c_pointer)

    @classmethod
    def createPythonObject(cls, c_pointer):
        if c_pointer is not None:
            new_obj = cls.__new__(cls)
            BaseCClass.__init__(new_obj, c_pointer=c_pointer, parent=None, is_reference=False)
            return new_obj
        else:
            return None

    @classmethod
    def createCReference(cls, c_pointer, parent=None):
        if c_pointer is not None:
            new_obj = cls.__new__(cls)
            BaseCClass.__init__(new_obj, c_pointer=c_pointer, parent=parent, is_reference=True)
            return new_obj
        else:
            return None

    @classmethod
    def storageType(cls):
        return ctypes.c_void_p

    def convertToCReference(self, parent):
        self.__is_reference = True
        self.__parent = parent


    def setParent(self, parent=None):
        if self.__is_reference:
            self.__parent = parent
        else:
            raise UserWarning("Can only set parent on reference types!")

        return self

    def isReference(self):
        """ @rtype: bool """
        return self.__is_reference

    def parent(self):
        return self.__parent

    def __eq__(self, other):
        # This is the last resort comparison function; it will do a
        # plain pointer comparison on the underlying C object; or
        # Python is-same-object comparison.
        if isinstance(other, BaseCClass):
            return self.__c_pointer == other.__c_pointer
        else:
            return super(BaseCClass , self).__eq__(other)


    def free(self):
        raise NotImplementedError("A BaseCClass requires a free method implementation!")



    def __del__(self):
        if self.free is not None:
            if not self.__is_reference:
                # Important to check the c_pointer; in the case of failed object creation
                # we can have a Python object with c_pointer == None.
                if self.__c_pointer > 0:
                    self.free()
