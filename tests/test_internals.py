"""Tests for the _pyjava module.

This package contains tests for the internal components of PyJava, implemented
in C as the _pyjava package. Even if it is not used directly but rather through
the pyjava Python interface, the package is covered by tests.
"""


import math
import unittest

import _pyjava


class Test_getclass(unittest.TestCase):
    def test_nonexistent(self):
        """Tests that ClassNotFound is raised when getting an unknown class.

        No other error should happen, and the problem should be detected
        immediately.
        """
        self.assertRaises(
                _pyjava.ClassNotFound,
                _pyjava.getclass,
                'java/lang/Nonexistent')

    def test_String(self):
        """Wraps an well-known class.
        """
        String = _pyjava.getclass('java/lang/String')


class Test_getmethod(unittest.TestCase):
    def test_method(self):
        """Requests a well-known method.
        """
        String = _pyjava.getclass('java/lang/String')
        length = String.getmethod('length')
        self.assertIsNotNone(length)

    def test_staticmethod(self):
        """Requests a well-known static method.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.getmethod('sin')
        self.assertIsNotNone(sin)

    def test_nonexistent(self):
        """Requests a wrapper for an unknown method.

        This be detected when accessing.
        """
        String = _pyjava.getclass('java/lang/String')
        self.assertRaises(
                AttributeError,
                String.getmethod,
                'nonexistentMethod')


class Test_call(unittest.TestCase):
    def test_constructor(self):
        """Constructs a Java object from a constructor.
        """
        Vector = _pyjava.getclass('java/util/Vector')
        vector = Vector.create(10)
        capacity = Vector.getmethod('capacity')
        self.assertEqual(capacity.call(vector), 10)

    def test_method(self):
        """Calls a well-known method on a wrapper returned by a static method.
        """
        Collections = _pyjava.getclass('java/util/Collections')
        List = _pyjava.getclass('java/util/List')
        emptyList = Collections.getmethod('emptyList')
        li = emptyList.call()
        size = List.getmethod('size')
        self.assertEqual(size.call(li), 0)

    def test_staticmethod(self):
        """Calls a well-known static method.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.getmethod('sin')
        self.assertAlmostEqual(sin.call(math.pi/2), 1.0)
