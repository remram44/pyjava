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
        """Wraps a well-known class.
        """
        String = _pyjava.getclass('java/lang/String')
        self.assertIsNotNone(String)


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

        This should be detected when accessing.
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
        self.assertIsNotNone(vector)
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

    def test_badoverload(self):
        """Calls an existing method but with wrong argument types.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.getmethod('sin')
        self.assertRaises(
                _pyjava.NoMatchingOverload,
                sin.call,
                4, 2)
        self.assertRaises(
                _pyjava.NoMatchingOverload,
                sin.call)


class Test_getfield(unittest.TestCase):
    def test_field(self):
        """TODO
        """

    def test_staticfield(self):
        """Requests a well-known static field.
        """
        Collections = _pyjava.getclass('java/util/Collections')
        empty_list = Collections.getfield('EMPTY_LIST')
        self.assertIsNotNone(empty_list)

    def test_nonexistent(self):
        """Requests a wrapper for an unknown field.

        This should be detected immediately.
        """
        Math = _pyjava.getclass('java/lang/Math')
        self.assertRaises(
                AttributeError,
                Math.getfield,
                'nonexistentfield')


class Test_accessfield(unittest.TestCase):
    def test_staticfield(self):
        """Requests a well-known static field.
        """
        Integer = _pyjava.getclass('java/lang/Integer')
        size = Integer.getfield('SIZE')
        self.assertEqual(size.get(), 32)

        String = _pyjava.getclass('java/lang/String')
        comparator = String.getfield('CASE_INSENSITIVE_ORDER')
        self.assertIsNotNone(comparator.get())

    def test_testclass(self):
        cl = _pyjava.getclass(
                'pyjavatest/test_fields/AccessField')
        obj = cl.create()

        a = cl.getfield('a')
        self.assertEqual(a.get(), 7)
        b = cl.getfield('b')
        self.assertEqual(b.get(), 'test')
        c = cl.getfield('c')
        self.assertEqual(c.get(), None)
        d = cl.getfield('d')
        self.assertEqual(d.get(obj), -7)
        e = cl.getfield('e')
        self.assertEqual(e.get(obj), None)
        f = cl.getfield('f')
        self.assertEqual(f.get(obj), '4')


class Test_conversions(unittest.TestCase):
    """Big set of method calls to cover the conversions.
    """
    def setUp(self):
        self._jcl = _pyjava.getclass(
                'pyjavatest/test_conversions/CallMethod_Conversions')
        self._jo = self._jcl.create()

    def test_v_ii(self):
        m = self._jcl.getmethod('v_ii')
        self.assertIsNone(m.call(self._jo, 12, -5))

    def test_i_fc(self):
        m = self._jcl.getmethod('i_fc')
        self.assertEqual(m.call(self._jo, 12.5, u'\u05D0'), -7)

    def test_b_Bs(self):
        m = self._jcl.getmethod('_b_Bs')
        self.assertEqual(m.call(0x42, 13042), False)

    def test_c_lS(self):
        m = self._jcl.getmethod('c_lS')
        self.assertEqual(m.call(self._jo, -70458L, u'R\xE9mi'), u'\u05D0')

    def test_d_iSb(self):
        m = self._jcl.getmethod('d_iSb')
        self.assertAlmostEqual(m.call(self._jo, 0, u'', True), 197.9986e17)

    def test_f_(self):
        m = self._jcl.getmethod('_f_')
        self.assertAlmostEqual(m.call(), -0.07)

    def test_S_(self):
        m = self._jcl.getmethod('S_')
        self.assertEqual(m.call(self._jo), u'\xE9\xEA\x00\xE8')

    def test_B_loi(self):
        g = self._jcl.getmethod('o_b')
        o = g.call(self._jo, False)
        self.assertIsNotNone(o)
        self.assertTrue(isinstance(o, _pyjava.JavaInstance))
        m = self._jcl.getmethod('B_loi')
        self.assertEqual(m.call(self._jo, 142005L, o, -100), 0x20)

    def test_s_So(self):
        g = self._jcl.getmethod('o_b')
        o = g.call(self._jo, False)
        self.assertIsNotNone(o)
        self.assertTrue(isinstance(o, _pyjava.JavaInstance))
        m = self._jcl.getmethod('_s_So')
        self.assertEqual(m.call(u'\x00\u252C\u2500\u2500\u252C', o), -15)

    def test_o_S(self):
        m = self._jcl.getmethod('_o_S')
        self.assertEqual(m.call(None), None)

    def test_v_o(self):
        m = self._jcl.getmethod('v_o')
        self.assertIsNone(m.call(self._jo, None))

    def test_C_(self):
        g = self._jcl.getmethod('_C_')
        C = g.call() # this returns a Class, which should be wrapped as
                # JavaClass instead of JavaInstance automatically
        self.assertIsNotNone(C)
        o = C.create(17)
        m = C.getmethod('i_')
        self.assertEqual(m.call(o), 42)
