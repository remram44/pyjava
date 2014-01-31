import _pyjava
from _pyjava import Error, ClassNotFound, NoMatchingOverload


__all__ = [
        'Error', 'ClassNotFound', 'NoMatchingOverload',
        'start', 'getclass']


def start(path=None, *args):
    if path is None:
        from pyjava.find_dll import find_dll
        path = find_dll()
    try:
        if len(args) == 0:
            _pyjava.start(path, [])
        elif len(args) == 1 and (isinstance(args[0], list)):
            _pyjava.start(path, args[0])
        else:
            _pyjava.start(path, args)
    except Error:
        raise Error("Unable to start Java VM with path %s" % path)


def getclass(classname):
    # Convert from the 'usual' syntax to the 'JNI' syntax
    jni_classname = classname.replace('.', '/')

    cls = _pyjava.getclass(jni_classname)  # might raise ClassNotFound
    return cls
