"""Tests for the _pyjava module.

This package contains tests for the internal components of PyJava, implemented
in C as the _pyjava package. Even if it is not used directly but rather through
the pyjava Python interface, the package is covered by tests.
"""


import math

import _pyjava

from base import PyjavaTestCase, unittest


class Test_getclass(PyjavaTestCase):
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
        """Wraps a language class.
        """
        String = _pyjava.getclass('java/lang/String')
        self.assertIsNotNone(String)
        self.assertEqual(String.getName(), u'java.lang.String')
        self.assertIsInstance(String, _pyjava.JavaClass)

    def test_Reader(self):
        """Wraps a well-known class.
        """
        Reader = _pyjava.getclass('java/io/Reader')
        self.assertIsNotNone(Reader)
        self.assertEqual(Reader.getName(), u'java.io.Reader')
        self.assertIsInstance(Reader, _pyjava.JavaClass)


class Test_classobject(PyjavaTestCase):
    def test_subclass(self):
        """Tests subclassing a Java class.
        """
        String = _pyjava.getclass('java/lang/String')
        with self.assertRaises(NotImplementedError):
            class MyString(String):
                pass

    @unittest.skip("JavaClass is not a metaclass right now")
    def test_issubclass(self):
        """Requests well-known classes and tests issubclass().
        """
        Object = _pyjava.getclass('java/lang/Object')
        String = _pyjava.getclass('java/lang/String')
        Class = _pyjava.getclass('java/lang/Class')
        self.assertTrue(issubclass(String, String))
        self.assertTrue(issubclass(Object, Object))
        self.assertTrue(issubclass(Class, Class))
        self.assertFalse(issubclass(Object, String))
        self.assertFalse(issubclass(String, Class))
        self.assertTrue(issubclass(Class, Object))
        self.assertFalse(issubclass(Class, String))
        self.assertTrue(issubclass(String, Object))
        self.assertFalse(issubclass(Object, Class))

        self.assertFalse(issubclass(String, list))
        self.assertFalse(issubclass(int, Class))

    @unittest.skip("JavaClass is not a metaclass right now")
    def test_isinstance(self):
        """Requests well-known classes and tests isinstance().
        """
        Collections = _pyjava.getclass('java/util/Collections')
        List = _pyjava.getclass('java/util/List')
        Integer = _pyjava.getclass('java/lang/Integer')
        empty = Collections.emptyList()
        self.assertTrue(isinstance(empty, List))
        self.assertFalse(isinstance(empty, Integer))
        self.assertFalse(isinstance(2, List))
        self.assertFalse(isinstance(empty, list))

    def test_is_same_object(self):
        """Tests for equality of references.
        """
        jcl = _pyjava.getclass('pyjavatest/ObjFactory')
        makeObject = jcl.makeObject
        obj1 = makeObject(1)
        obj2 = makeObject(2)
        obj3 = makeObject(2)
        # == here tests Java reference equality, it does not call equals()
        self.assertTrue(obj1 == obj1)
        self.assertTrue(obj2 == obj2)
        self.assertTrue(obj3 == obj3)
        self.assertFalse(obj1 == obj2)
        self.assertTrue(obj2 == obj3)
        self.assertFalse(obj3 == obj1)
        self.assertFalse(obj2 == obj1)
        self.assertTrue(obj3 == obj2)
        self.assertFalse(obj1 == obj3)

        String = _pyjava.getclass('java/lang/String')
        # These shouldn't raise
        self.assertFalse(obj1 == String)
        self.assertFalse(obj2 == String)


class Test_get_method(PyjavaTestCase):
    def test_method(self):
        """Requests a well-known method.
        """
        String = _pyjava.getclass('java/lang/String')
        length = String.length
        self.assertIsNotNone(length)
        self.assertTrue(isinstance(length, _pyjava.UnboundMethod))

    def test_staticmethod(self):
        """Requests a well-known static method.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.sin
        self.assertIsNotNone(sin)
        self.assertTrue(isinstance(sin, _pyjava.UnboundMethod))


class Test_call(PyjavaTestCase):
    def test_constructor(self):
        """Constructs a Java object from a constructor.
        """
        Vector = _pyjava.getclass('java/util/Vector')
        vector = Vector(10)
        self.assertIsNotNone(vector)
        self.assertEqual(vector.capacity(), 10)

    def test_method(self):
        """Calls a well-known method on a wrapper returned by a static method.
        """
        Collections = _pyjava.getclass('java/util/Collections')
        List = _pyjava.getclass('java/util/List')
        emptyList = Collections.emptyList
        li = emptyList()
        size = List.size
        self.assertTrue(isinstance(size, _pyjava.UnboundMethod))
        b_size = li.size
        self.assertTrue(isinstance(b_size, _pyjava.BoundMethod))
        self.assertEqual(size(li), 0)
        self.assertEqual(b_size(), 0)

    def test_staticmethod(self):
        """Calls a well-known static method.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.sin
        self.assertAlmostEqual(sin(math.pi / 2), 1.0)

    def test_badoverload(self):
        """Calls an existing method but with wrong argument types.
        """
        Math = _pyjava.getclass('java/lang/Math')
        sin = Math.sin
        with self.assertRaises(_pyjava.NoMatchingOverload):
            sin(4, 2)
        with self.assertRaises(_pyjava.NoMatchingOverload):
            sin()


class Test_get_field(PyjavaTestCase):
    def test_field(self):
        """Requests a well-known field.
        """
        Dimension = _pyjava.getclass('java/awt/Dimension')
        d = Dimension()
        self.assertEqual(d.width, 0)

    def test_staticfield(self):
        """Requests a well-known static field.
        """
        Collections = _pyjava.getclass('java/util/Collections')
        empty_list = Collections.EMPTY_LIST
        self.assertIsNotNone(empty_list)
        self.assertEqual(empty_list.size(), 0)

    def test_nonexistent_instance(self):
        """Requests an unknown field/method on an instance.

        This should be detected immediately.
        """
        Dimension = _pyjava.getclass('java/awt/Dimension')
        d = Dimension()
        with self.assertRaises(AttributeError):
            d.nonExistentField

    def test_nonexistent_class(self):
        """Requests an unknown field/method on a class.

        This should be detected immediately.
        """
        Math = _pyjava.getclass('java/lang/Math')
        with self.assertRaises(AttributeError):
            Math.nonExistentField


class Test_set_field(PyjavaTestCase):
    def test_field(self):
        """Sets a well-known field.
        """
        Dimension = _pyjava.getclass('java/awt/Dimension')
        d = Dimension()
        d.width = 42
        self.assertEqual(d.width, 42)

    def test_staticfield(self):
        """Sets a static field.
        """
        SetField = _pyjava.getclass('pyjavatest/test_fields/SetField')
        SetField.a = 4
        SetField.b = u"hello"
        self.assertEqual((SetField.a, SetField.b), (4, u"hello"))

    def test_nonexistent_instance(self):
        """Sets an unknown field on an instance.
        """
        Dimension = _pyjava.getclass('java/awt/Dimension')
        d = Dimension()
        with self.assertRaises(AttributeError):
            d.nonExistentField = 42

        SetField = _pyjava.getclass('pyjavatest/test_fields/SetField')
        sf = SetField()
        sf.c = 5
        sf.d = u"r\xE9mi is out of ideas"
        sf2 = SetField()
        self.assertEqual(sf.c, 5)
        self.assertEqual(sf2.c, 2)
        self.assertEqual(len(sf.d), 20)
        self.assertEqual(len(sf2.d), 22)

    def test_nonexistent_class(self):
        """Sets an unknown field on a class.
        """
        Dimension = _pyjava.getclass('java/awt/Dimension')
        with self.assertRaises(AttributeError):
            Dimension.nonExistentField = 42

    def test_wrongtype(self):
        """Assigns values of different types to fields.
        """
        SetField = _pyjava.getclass('pyjavatest/test_fields/SetField')
        sf = SetField()
        with self.assertRaises(TypeError):
            SetField.a = 6.87
        with self.assertRaises(TypeError):
            SetField.b = sf
        with self.assertRaises(TypeError):
            sf.c = u"test"
        with self.assertRaises(TypeError):
            sf.d = 1


class Test_accessfield(PyjavaTestCase):
    def test_staticfield(self):
        """Requests a well-known static field.
        """
        Integer = _pyjava.getclass('java/lang/Integer')
        size = Integer.SIZE
        self.assertEqual(size, 32)

        String = _pyjava.getclass('java/lang/String')
        comparator = String.CASE_INSENSITIVE_ORDER
        self.assertIsNotNone(comparator)

    def test_testclass(self):
        cl = _pyjava.getclass(
                'pyjavatest/test_fields/AccessField')
        obj = cl()

        self.assertEqual(cl.a, 7)
        self.assertEqual(cl.b, 'test')
        self.assertEqual(cl.c, None)
        self.assertEqual(obj.d, -7)
        self.assertEqual(obj.e, None)
        self.assertEqual(obj.f, '4')


class Test_reflection(PyjavaTestCase):
    def test_forname(self):
        """Uses Class.forName().
        """
        Class = _pyjava.getclass('java/lang/Class')
        String = Class.forName(u'java.lang.String')
        self.assertIsInstance(String, _pyjava.JavaClass)
        self.assertEqual(String(u'lala').length(), 4)
        String2 = _pyjava.getclass('java/lang/String')
        self.assertEqual(String, String2)

        with self.assertRaises(AttributeError):
            # forName() is static and can only be accessed from Class
            String2.forName

    def test_nonstatic_method(self):
        """Access a non-static Class method.
        """
        String = _pyjava.getclass('java/lang/String')
        self.assertEqual(String.getName(), u'java.lang.String')
        Class = _pyjava.getclass('java/lang/Class')
        self.assertEqual(Class.getName(), u'java.lang.Class')
        self.assertEqual(Class.getName(String), u'java.lang.String')


class Test_conversions(PyjavaTestCase):
    """Big set of method calls to cover the conversions.
    """
    def setUp(self):
        self._jcl = _pyjava.getclass(
                'pyjavatest/test_conversions/CallMethod_Conversions')
        self._jo = self._jcl()

    def test_v_ii(self):
        m = self._jcl.v_ii
        self.assertIsNone(m(self._jo, 12, -5))

    def test_i_fc(self):
        m = self._jcl.i_fc
        self.assertEqual(m(self._jo, 12.5, u'\u05D0'), -7)

    def test_b_Bs(self):
        m = self._jcl._b_Bs
        self.assertEqual(m(0x42, 13042), False)

    def test_c_lS(self):
        m = self._jcl.c_lS
        self.assertEqual(m(self._jo, -70458L, u'R\xE9mi'), u'\u05D0')

    def test_d_iSb(self):
        m = self._jcl.d_iSb
        self.assertAlmostEqual(m(self._jo, 0, u'', True), 197.9986e17)

    def test_f_(self):
        m = self._jcl._f_
        self.assertAlmostEqual(m(), -0.07)

    def test_S_(self):
        m = self._jcl.S_
        self.assertEqual(m(self._jo), u'\xE9\xEA\x00\xE8')

    def test_B_loi(self):
        g = self._jcl.o_b
        o = g(self._jo, False)
        self.assertIsNotNone(o)
        self.assertTrue(isinstance(o, _pyjava.JavaInstance))
        m = self._jcl.B_loi
        self.assertEqual(m(self._jo, 142005L, o, -100), 0x20)

    def test_s_So(self):
        g = self._jcl.o_b
        o = g(self._jo, False)
        self.assertIsNotNone(o)
        self.assertTrue(isinstance(o, _pyjava.JavaInstance))
        m = self._jcl._s_So
        self.assertEqual(m(u'\x00\u252C\u2500\u2500\u252C', o), -15)

    def test_o_S(self):
        m = self._jcl._o_S
        self.assertEqual(m(None), None)

    def test_v_o(self):
        m = self._jcl.v_o
        self.assertIsNone(m(self._jo, None))

    def test_C_(self):
        g = self._jcl._C_
        C = g() # this returns a Class, which should be wrapped as
                # JavaClass instead of JavaInstance automatically
        self.assertIsNotNone(C)
        o = C(17)
        m = C.i_
        self.assertEqual(m(o), 42)
