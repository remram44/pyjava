from distutils.core import setup, Extension
import os, os.path

jdk = 'C:\\Program Files (x86)\\Java\\jdk1.6.0_25'

pyjava = Extension('_pyjava',
                   sources=['native/pyjava.c', 'native/javawrapper.c'],
                   include_dirs=[os.path.join(jdk, 'include'),
                                 os.path.join(jdk, 'include', 'win32')])

setup(name='PyJava',
      version='0.0',
      description='Python-Java bridge',
      ext_modules=[pyjava])
