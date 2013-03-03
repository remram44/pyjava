import _pyjava


# Exception classes
from _pyjava import Error, ClassNotFound, NoMatchingOverload


__all__ = [
        'Error', 'ClassNotFound', 'NoMatchingOverload',
        'start', 'getclass']


class _UnboundJavaMethod(object):
    """An unbound Java method.

    You can call it like a regular method. It might raise NoMatchingOverload if
    the parameters number or types are not compatible with one of the
    overloads.

    Note that this type represents both object methods before they are bound
    and static methods.
    """
    def __init__(self, method):
        self._pyjava_method = method

    def __call__(self, jself, *args):
        return self._pyjava_method.call(jself, *args)
                # might raise NoMatchingMethod


class _BoundJavaMethod(object):
    """A bound Java method.

    You can call it like a regular method. It might raise NoMatchingOverload if
    the parameters number or types are not compatible with one of the
    overloads.
    """
    def __init__(self, method, obj):
        self._pyjava_method = method
        self._pyjava_obj = obj

    def __call__(self, *args):
        return self._pyjava_method.call(self.__obj, *args)
                # might raise NoMatchingOverload


# Here we define __instancecheck__ and __subclasscheck__ to override Python's
# isinstance() and issubclass() builtins
#
# PEP 3119:
#   The primary mechanism proposed here is to allow overloading the built-in
#   functions isinstance() and issubclass(). The overloading works as follows:
#     The call isinstance(x, C) first checks whether C.__instancecheck__
#     exists, and if so, calls C.__instancecheck__(x) instead of its normal
#     implementation.
#     Similarly, the call issubclass(D, C) first checks whether
#     C.__subclasscheck__ exists, and if so, calls C.__subclasscheck__(D)
#     instead of its normal implementation.

# This is the metaclass, i.e. objects of this type represent Java classes
# It is NOT java.lang.Class however
class _JavaClass(object):
    def __init__(self, classname, javaclass):
        self.__dict__['_pyjava_classname'] = classname
        self.__dict__['_pyjava_javaclass'] = javaclass

    def __getattr__(self, attr):
        # Method
        try:
            # Use __dict__ to avoid infinite recursion
            return _UnboundJavaMethod(
                    self.__dict__['_pyjava_javaclass'].getmethod(attr))
        except AttributeError:
            pass
        # TODO : static fields
        raise AttributeError

    def __setattr__(self, attr, value):
        # TODO : static fields
        pass

    def __instancecheck__(self, inst):
        """Called to check if an object is an instance of this class.
        """
        if isinstance(inst, _JavaObject):
            return _pyjava.issubclass(
                    inst.__dict__['_pyjava_class'].__dict__['_pyjava_javaclass'],
                    self.__dict__['_pyjava_javaclass'])
        else:
            return False

    def __subclasscheck__(self, other):
        """Called to check if a class is a subclass of this class
        """
        if isinstance(other, _JavaClass):
            return _pyjava.issubclass(other.__dict__['_pyjava_javaclass'],
                                      self.__dict__['_pyjava_javaclass'])
        elif isinstance(other, type): # A non-Java class
            return False
        else: # Not a class
            raise TypeError("issubclass() arg 2 must be a class")

    def __call__(self, *args):
        """Called to instanciate this class.
        """
        return _JavaObject(
                self.__dict__['_pyjava_javaclass'],
                self.__dict__['_pyjava_javaclass'].create(*args))
                # This might raise NoMatchingOverload


# This is the real class of all Java objects, even though we pretend (by
# overriding isinstance() and issubclass() behaviors) that they are instances
# of instances of _JavaClass (which we pretend are classes)
class _JavaObject(object):
    def __init__(self, klass, javaobject):
        self.__dict__['_pyjava_class'] = klass
        self.__dict__['_pyjava_javaobject'] = javaobject

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
                    self.__dict__['_pyjava_javaclass'].getmethod(attr),
                    self.__dict__['_pyjava_javaobject'])
        except AttributeError:
            pass
        # TODO : non-static fields
        raise AttributeError

    def __eq__(self, other):
        """Tests that the two objects are the same.

        This tests equality is the Java sense, i.e. that the references are
        equal (Java has no operator overriding).
        In Java, its behaviors is similar to that of Python's "is" operator,
        but here we have to use == ("is" cannot be overridden).
        """
        if isinstance(other, _JavaObject):
            return _pyjava.issameobject(
                    other.__dict__['_pyjava_javaobject'],
                    self.__dict__['_pyjava_javaobject'])
        return False


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


_known_classes = dict()

def getclass(classname):
    try:
        return _known_classes[classname]
    except KeyError:
        # Convert from the 'usual' syntax to the 'JNI' syntax
        jni_classname = classname.replace('.', '/')

        cls = _pyjava.getclass(jni_classname) # might raise ClassNotFound
        cls = _JavaClass(classname, cls)
        _known_classes[classname] = cls
        return cls
