"""Tests for the pyjava module.

This package contains tests for the interface of PyJava, directly accessed by
the user.
"""


import math
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
        self.assertTrue(isinstance(Math.sin, pyjava._UnboundJavaMethod))

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


class Test_call(unittest.TestCase):
    def test_constructor(self):
        """Constructs a Java object from a constructor.
        """
        Vector = pyjava.getclass('java.util.Vector')
        vector = Vector(10)
        self.assertIsNotNone(vector)
        self.assertEqual(vector.capacity(), 10)

    def test_method(self):
        """Calls a well-known method on a wrapper returned by a static method.
        """
        Collections = pyjava.getclass('java.util.Collections')
        li = Collections.emptyList()
        self.assertEqual(li.size(), 0)

    def test_staticmethod(self):
        """Calls a well-known static method.
        """
        Math = pyjava.getclass('java.lang.Math')
        self.assertAlmostEqual(Math.sin(math.pi/2), 1.0)

    def test_badoverload(self):
        """Calls an existing method but with wrong argument types.
        """
        Math = pyjava.getclass('java.lang.Math')
        self.assertRaises(
                pyjava.NoMatchingOverload,
                Math.sin,
                4, 2)
        self.assertRaises(
                pyjava.NoMatchingOverload,
                Math.sin)
