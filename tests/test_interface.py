"""Tests for the pyjava module.

This package contains tests for the interface of PyJava, directly accessed by
the user. Tests are also available, in the 'internals' package, for the C
components.
"""


import unittest

import pyjava


class Test_getclass(unittest.TestCase):
    def test_nonexistent(self):
        """Tests that ClassNotFound is raised when getting an unknown class.

        No other error should happen, and the problem should be detected
        immediately.
        """
        self.assertRaises(
                pyjava.ClassNotFound,
                pyjava.getclass,
                'java.lang.Nonexistent')

    def test_String(self):
        """Wraps a well-known class.
        """
        String = pyjava.getclass('java.lang.String')
        self.assertIsNotNone(String)


class Test_getmethod(unittest.TestCase):
    def test_method(self):
        """Accesses a well-known method.
        """
        String = pyjava.getclass('java.lang.String')
        self.assertTrue(isinstance(String.length, pyjava._UnboundJavaMethod))

    def test_staticmethod(self):
        """Accesses a well-known static method.
        """
        Math = pyjava.getclass('java.lang.Math')
        self.assertTrue(isinstance(Math.sin, pyjava._StaticJavaMethod))

    def test_nonexistent(self):
        """Accesses an unknown method.

        This should be detected when accessing.
        """
        String = pyjava.getclass('java.lang.String')
        def test_func():
            m = String.nonexistentMethod
        self.assertRaises(
                AttributeError,
                test_func)
