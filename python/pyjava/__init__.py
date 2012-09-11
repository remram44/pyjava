import _pyjava


# Exception classes
from _pyjava import Error, ClassNotFound, NoMatchingMethod


__all__ = ['Error', 'ClassNotFound', 'NoMatchingMethod', 'start']


# This metaclass adds nothing to the standard one (type), other than not being
# implemented in C. This allows the __class__ field on it's meta-instance to
# be changed.
# I consider this to be a bug in Python.
class _Meta(type):
    pass


# This is the root Java class, java.lang.Object
# It is also instance of the meta-class java.lang.Class
class _JavaObject(object):
    __metaclass__ = _Meta

    def __init__(self, *args):
        # This might raise NoMatchingMethod
        self.__javaobject__ = self.__javaclass__.create(*args)

    # This is called when an attribute is assigned
    # If it matches a Java field, update it, else call object.__setattr__
    def __setattr__(self, attr, value):
        # TODO
        pass

    # Contrary to __getattribute__, this will only be called if nothing is
    # found with the usual mechanism
    # We attempt to find a field in the Java class or raise AttributeError
    def __getattr__(self, attr, value):
        # TODO
        pass


# This is the "special" java.lang.Class class
class _JavaClass(type, _JavaObject):
    __metaclass__ = _Meta

    def __init__(self, fullclassname):
        # This might raise ClassNotFound
        self.__javaclass__ = _pyjava.getclass(fullclassname)

_JavaClass.__class__ = _JavaClass
_JavaObject.__class__ = _JavaClass


def start(path, *args):
    if len(args) == 0:
        _pyjava.start(path, [])
    elif len(args) == 1 and (isinstance(args[0], list)):
        _pyjava.start(path, args[0])
    else:
        _pyjava.start(path, args)

    _JavaObject.__javaclass__ = _pyjava.getclass('java/lang/Class')
