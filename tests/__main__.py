import os
import sys
import unittest


top_level = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
start_dir = os.path.join(top_level, 'tests')
if not top_level in sys.path:
    sys.path.append(top_level)


import _pyjava
from tests.find_dll import find_dll


dll = find_dll()
if not dll:
    sys.stderr.write("No suitable JVM DLL found. Please set your JAVA_HOME "
                     "environment variable.\n")
    sys.exit(1)
else:
    sys.stderr.write("Using JVM DLL: %s\n" % dll)


# This need to be called once, before running the test suite
_pyjava.start(dll, ['-Djava.class.path=tests/java-tests.jar'])


class Program(unittest.TestProgram):
    def createTests(self):
        # Is there really no way to load the tests from the package, using
        # TestLoader#discover(), while still using main()?
        # Why would you provide TestLoader, TestRunner and all that nonsense if
        # I can't do that without overriding part of main itself?
        if self.testNames is None:
            self.test = self.testLoader.discover(
                    start_dir=start_dir,
                    pattern='test_*.py',
                    top_level_dir=top_level)
        else:
            self.test = self.testLoader.loadTestsFromNames(self.testNames)

Program()
