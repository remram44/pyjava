try:
    from setuptools import setup, Extension
except ImportError:
    from distutils.core import setup, Extension
import os
import re
import sys


# Number of bits of Python
# We have to build for the same architecture if we want to link with Python...
if sys.maxsize <= 2**32:
    bits = 32
else:
    bits = 64


USING_WINDOWS = sys.platform == 'win32'


# Attempt to find the location of the JDK
java_home = os.getenv('JAVA_HOME')
if java_home:
    sys.stderr.write("Using JDK from JAVA_HOME environment variable\n")
elif USING_WINDOWS:
    JDK_NAME = re.compile(r'^jdk1\.([0-9])\.([0-9])_([0-9]+)$')

    def find_jdk(java_dir):
        if not os.path.exists(java_dir):
            return None
        choice = None
        for subdir in os.listdir(java_dir):
            m = JDK_NAME.match(subdir)
            if m:
                version = m.groups()
                if choice is None or version > choice[1]:
                    choice = subdir, version
        if choice is not None:
            return os.path.join(java_dir, choice[0])
        else:
            return None

    if os.path.exists('C:\\Program Files (x86)'):
        if bits == 32:
            java_home = find_jdk('C:\\Program Files (x86)\\Java')
        else:
            java_home = find_jdk('C:\\Program Files\\Java')
    else:
        java_home = find_jdk('C:\\Program Files\\Java')
else:
    # FIXME : This is a stub for Linux
    java_home = '/usr/lib/jvm/java-6-sun'

if not java_home:
    sys.stderr.write("No suitable JDK found. Please set your "
                     "JAVA_HOME environment variable.\n")
    sys.exit(1)
else:
    sys.stderr.write("Using JDK: %s\n" % java_home)


# List the source files
sources = []
for dirpath, dirnames, filenames in os.walk('native'):
    for f in filenames:
        if f[-2:] == '.c':
            sources.append(os.path.join(dirpath, f))


# Setup the include directories
if USING_WINDOWS:
    include_dirs = [os.path.join(java_home, 'include'),
                    os.path.join(java_home, 'include', 'win32')]
    # Yes, it is also 'win32' on 64 bits versions of Windows
else:
    include_dirs = [os.path.join(java_home, 'include'),
                    os.path.join(java_home, 'include', 'linux')]

# Setup the libraries
libraries = []
if not USING_WINDOWS:
    libraries.append('dl')


# Build the C module
pyjava = Extension('_pyjava',
                   sources=sources,
                   include_dirs=include_dirs,
                   libraries=libraries)

description = """
PyJava is a bridge allowing to use Java classes in regular Python code.

It is a C extension that uses JNI to access a Java virtual machine. The
integration with Java code is meant to be as complete as possible, allowing to
use Java and Python objects seemlessly and converting objects back and forth
when Java code is called. Furthermore, subclassing Java classes or interfaces
in Python code to allow callback from Java is planned for the 0.2 version.

Please note that this extension is still at a very early stage of development
and probably shouldn't be used for anything.

Don't hesitate to contact me if you have questions or remarks, or to fork the
project on Github if you can help.
"""
setup(name='PyJava',
      version='0.0',
      ext_modules=[pyjava],
      package_dir={'': 'python'},
      packages=['pyjava'],
      description='Python-Java bridge',
      author="Remi Rampin",
      author_email='remirampin@gmail.com',
      url='http://github.com/remram44/pyjava',
      long_description=description,
      license='MIT',
      keywords=['java', 'bridge', 'jni'],
      classifiers=[
        "Development Status :: 2 - Pre-Alpha",
        "License :: OSI Approved :: MIT License",
        'Programming Language :: C',
        'Programming Language :: Java',
        'Programming Language :: Python'])
