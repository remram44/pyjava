PyJava -- A Python to Java bridge

# Description

PyJava is a bridge allowing to use Java classes in regular Python code. It is
similar to [JPype] [1].

It is a C extension that uses JNI to access a Java virtual machine, meaning
that it can be used anywhere Python is available. It is not a different
interpreter like [Jython] [2] and does not require anything. The JVM dynamic
library is load dynamically through pyjava.start() (some basic logic for
locating this library on major platforms will be provided).

The integration with Java code is meant to be as complete as possible, allowing
to use Java and Python objects seemlessly and converting objects back and forth
when Java code is called. Furthermore, subclassing Java classes or interfaces
in Python code to allow callback from Java is planned for the 0.2 version.

Please note that this extension is still at a very early stage of development
and probably shouldn't be used for anything.

  [1]: http://jpype.sourceforge.net/
  [2]: http://jython.org/
