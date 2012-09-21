import _pyjava


# Exception classes
from _pyjava import Error, ClassNotFound, NoMatchingMethod


__all__ = [
        'Error', 'ClassNotFound', 'NoMatchingMethod',
        'start', 'getclass',
        'JavaMethod']


# This metaclass adds nothing to the standard one (type), other than not being
# implemented in C. This allows the __class__ field on it's meta-instance to
# be changed.
# I consider this to be a bug in Python.
class _Meta(type):
    def __new__(*args):
        return type.__new__(*args)


# TODO : this could be a descriptor whose __get__ method return a
# _BoundJavaMethod... I think
class _UnboundJavaMethod(object):
    """An unbound Java method.

    You can call it like a regular method. It might raise NoMatchingMethod if
    the parameters number or types are not compatible with one of the
    overloads.
    """
    def __init__(self, method):
        self.__method = method

    def __call__(self, jself, *args):
        # TODO : check type of first argument, like Python does, or
        # raise TypeError
        return self.__method.call(jself, *args) # might raise NoMatchingMethod


class _BoundJavaMethod(object):
    """A bound Java method.

    You can call it like a regular method. It might raise NoMatchingMethod if
    the parameters number or types are not compatible with one of the
    overloads.
    """
    def __init__(self, method, obj):
        self.__method = method
        self.__obj = obj

    def __call__(self, *args):
        return self.__method.call(self.__obj, *args)
                # might raise NoMatchingMethod


class _StaticJavaMethod(object):
    """A static Java method.

    You can call it like a regular method. It might raise NoMatchingMethod if
    the parameters number or types are not compatible with one of the
    overloads.
    """
    def __init__(self, method, obj):
        self.__method = method

    def __call__(self, *args):
        return self.__method.call(*args) # might raise NoMatchingMethod


# This is the root Java class, java.lang.Object
# It is also instance of the meta-class java.lang.Class
class _JavaObject(object):
    __metaclass__ = _Meta

    def __init__(self, *args, **kwargs):
        print '+_JavaObject#__init__'
        # We accept a keyword-only argument _javaobject=None
        try:
            _javaobject = kwargs.pop('_javaobject')
        except KeyError:
            _javaobject = None
        if kwargs:
            raise TypeError('_JavaObject.__init__ got an unexpected '
                            'keyword argument %s' % kwargs.keys()[0])

        if _javaobject is not None:
            self._pyjava_javaobject = _javaobject
        else:
            # This might raise NoMatchingMethod
            self._pyjava_javaobject = (
                    self.__class__ # this is a _JavaClass
                    .__dict__['_pyjava_javaobject'] # the JavaClass
                    .create(*args))
        print '-_JavaObject#__init__'

    # This is called when an attribute is assigned
    # If it matches a Java field, update it, else call object.__setattr__
    def __setattr__(self, attr, value):
        # TODO : non-static fields
        pass

    # Contrary to __getattribute__, this will only be called if nothing is
    # found with the usual mechanism
    # We attempt to find a field or a method in the Java class or raise
    # AttributeError
    def __getattr__(self, attr):
        # Method
        try:
            return _BoundJavaMethod(self.__class__ # this is a _JavaClass
                    .__dict__['_pyjava_javaobject'] # the JavaClass
                    .getmethod(attr),
                    self.__dict__['_pyjava_javaobject']) # the _JavaInstance
        except NoMatchingMethod:
            pass
        # TODO : non-static fields
        raise AttributeError


# This is the "special" java.lang.Class class
class _JavaClass(type, _JavaObject):
    __metaclass__ = _Meta

    def __init__(self, *args, **kwargs):
        print '+_JavaClass#__init__'
        _JavaObject.__init__(self, *args, **kwargs)
        print '-_JavaClass#__init__'

    # This is called when an attribute is assigned
    # If it matches a Java field, update it, else call object.__setattr__
    def __setattr__(self, attr, value):
        # TODO : static fields
        pass

    # Because of how Python works, we can access both static and non-static
    # methods either from the instance or the class
    # This overload of _JavaObject#__getattr__ searches on this class before
    # searching on this class's class (which is always _JavaClass)
    def __getattr__(self, attr):
        # Method of this class
        try:
            return _UnboundJavaMethod(self
                    .__dict__['_pyjava_javaobject'] # the JavaClass
                    .getmethod(attr))
        except NoMatchingMethod:
            pass
        # TODO : static fields
        return _JavaObject.__getattr__(self, attr) # might raise AttributeError


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


_known_classes = {
        'java.lang.Object': _JavaObject,
        'java.lang.Class': _JavaClass}


def getclass(classname):
    try:
        return _known_classes[classname]
    except KeyError:
        # Convert from the 'usual' syntax to the 'JNI' syntax
        classname = classname.replace('.', '/')

        cls = _pyjava.getclass(classname) # might raise ClassNotFound
        cls = _JavaClass(_javaobject=cls)
        _known_classes[classname] = cls
        return cls
