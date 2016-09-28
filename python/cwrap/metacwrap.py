import re
from types import MethodType

from cwrap.prototype import registerType, Prototype


def snakeCase(name):
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    return re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1).lower()


class MetaCWrap(type):
    def __init__(cls, name, bases, attrs):
        super(MetaCWrap, cls).__init__(name, bases, attrs)

        is_return_type = False
        storage_type = None

        if "TYPE_NAME" in attrs:
            type_name = attrs["TYPE_NAME"]
        else:
            type_name = snakeCase(name)

        if hasattr(cls, "DATA_TYPE") or hasattr(cls, "enums"):
            is_return_type = True

        if hasattr(cls, "storageType"):
            storage_type = cls.storageType()

        registerType(type_name, cls, is_return_type=is_return_type, storage_type=storage_type)

        if hasattr(cls, "createCReference"):
            registerType("%s_ref" % type_name, cls.createCReference, is_return_type=True, storage_type=storage_type)

        if hasattr(cls, "createPythonObject"):
            registerType("%s_obj" % type_name, cls.createPythonObject, is_return_type=True, storage_type=storage_type)


        for key, attr in attrs.items():
            if isinstance(attr, Prototype):
                attr.resolve()
                attr.__name__ = key

                if attr.shouldBeBound():
                    method = MethodType(attr, None, cls)
                    setattr(cls, key, method)
