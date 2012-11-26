import _pyjava


# Exception classes
from _pyjava import Error, ClassNotFound, NoMatchingMethod


__all__ = [
        'Error', 'ClassNotFound', 'NoMatchingMethod',
        'start', 'getclass']


# TODO : this could be a descriptor whose __get__ method return a
# _BoundJavaMethod... I think
class _UnboundJavaMethod(object):
    """An unbound Java method.

    You can call it like a regular method. It might raise NoMatchingMethod if
    the parameters number or types are not compatible with one of the
    overloads.

    Note that this type represents both object methods before they are bound
    and static methods.
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


# TODO : define __instancecheck__ and __subclasscheck__ to override Python's
# isinstance() and issubclass() builtins
# PEP 3119:
#   The primary mechanism proposed here is to allow overloading the built-in
#   functions isinstance() and issubclass(). The overloading works as follows:
#     The call isinstance(x, C) first checks whether C.__instancecheck__
#     exists, and if so, calls C.__instancecheck__(x) instead of its normal
#     implementation.
#     Similarly, the call issubclass(D, C) first checks whether
#     C.__subclasscheck__ exists, and if so, calls C.__subclasscheck__(D)
#     instead of its normal implementation.

# This is the "special" java.lang.Class class
class _JavaClass(type):
    _JavaObject_constructed = False

    def __new__(*args, **kwargs):
        if len(args) == 4 and not _JavaClass._JavaObject_constructed:
            # This class may only be used once as __metaclass__:
            # to construct _JavaObject (just below)
            _JavaClass._JavaObject_constructed = True
            args[3].update(dict(_pyjava__javaclass = None)) # initialization of
                    # this field happens in start(), as getclass() needs a
                    # running VM
            cls = type.__new__(
                    _JavaClass,
                    args[1],
                    (),
                    args[3])
        elif len(args) == 4 and _JavaClass._JavaObject_constructed:
            raise Error("_JavaClass shouldn't be used as __metaclass__")
        elif len(args) != 1:
            raise TypeError("_JavaClass only accepts keyword arguments "
                            "_javaclass and _classname")
        elif set(kwargs.keys()) == set(['_javaclass', '_classname']):
            # It is used with the keyword argument _javaclass to construct
            # classes
            cls = type.__new__(
                    _JavaClass,
                    kwargs['_classname'],
                    (),
                    dict(_pyjava__javaclass=kwargs['_javaclass']))
        else:
            raise TypeError("Invalid keyword arguments to _JavaClass")

        return cls

    def __init__(self, *args, **kwargs):
        # Will be called after __new__
        # We just override type.__init__ so that it accepts the arguments meant
        # for __new__
        pass

    def __getattr__(self, attr):
        # Method
        try:
            return _UnboundJavaMethod(
                    self.__dict__['_pyjava__javaclass'].getmethod(attr))
        except NoMatchingMethod:
            pass
        # TODO : static fields
        raise AttributeError

    def __setattr__(self, attr, value):
        # TODO : static fields
        pass


# This is the root Java class, java.lang.Object
# It is also instance of the meta-class java.lang.Class
class _JavaObject(object):
    __metaclass__ = _JavaClass

    def __init__(self, *args, **kwargs):
        # We accept a keyword-only argument _javaobject=None
        try:
            _javaobject = kwargs.pop('_javaobject')
        except KeyError:
            _javaobject = None
        if kwargs:
            raise TypeError('_JavaObject.__init__ got an unexpected '
                            'keyword argument %s' % kwargs.keys()[0])

        if _javaobject is not None:
            self._pyjava__javaobject = _javaobject
        else:
            # This might raise NoMatchingMethod
            self._pyjava__javaobject = (
                    self._pyjava__javaclass.create(*args))

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
            return _BoundJavaMethod(
                    self._pyjava__javaclass.getmethod(attr),
                    self._pyjava__javaobject)
        except NoMatchingMethod:
            pass
        # TODO : non-static fields
        raise AttributeError


def start(path, *args):
    try:
        if len(args) == 0:
            _pyjava.start(path, [])
        elif len(args) == 1 and (isinstance(args[0], list)):
            _pyjava.start(path, args[0])
        else:
            _pyjava.start(path, args)
    except Error:
        raise Error("Unable to start Java VM with path %s" % path)

    _JavaObject._pyjava__javaclass = _pyjava.getclass('java/lang/Object')


_known_classes = {
        'java.lang.Object': _JavaObject,
        'java.lang.Class': _JavaClass}


def getclass(classname):
    try:
        return _known_classes[classname]
    except KeyError:
        # Convert from the 'usual' syntax to the 'JNI' syntax
        jni_classname = classname.replace('.', '/')

        cls = _pyjava.getclass(jni_classname) # might raise ClassNotFound
        cls = _JavaClass(_classname=classname, _javaclass=cls)
        _known_classes[classname] = cls
        return cls
