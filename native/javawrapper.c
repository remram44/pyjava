#include <Python.h>
#include <jni.h>

typedef struct {
    PyObject_HEAD
    jclass javaclass;
} JavaClass;

static PyObject *JavaClass_getmethod(JavaClass *self, PyObject *args)
{
    /* TODO */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef JavaClass_methods[] = {
    {"getmethod", (PyCFunction)JavaClass_getmethod, METH_VARARGS,
     "Returns a wrapper for a Java method."
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject JavaClass_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.JavaClass",        /*tp_name*/
    sizeof(JavaClass),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Java class wrapper",      /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    JavaClass_methods,         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    PyType_GenericNew,         /* tp_new */
};

/**
 * Initialize the module (creates the JavaClass type).
 */
void javawrapper_init(PyObject *mod)
{
    if(PyType_Ready(&JavaClass_type) < 0)
        return;

    Py_INCREF(&JavaClass_type);
    PyModule_AddObject(mod, "JavaClass", (PyObject*)&JavaClass_type);
}

/**
 * Build a Python wrapper for a Java class.
 */
PyObject *javawrapper_build(jclass javaclass)
{
    PyObject *args = Py_BuildValue("()");
    PyObject *kwargs = Py_BuildValue("{}");
    JavaClass* wrapper = PyObject_New(JavaClass, &JavaClass_type);
    Py_DECREF(args);
    Py_DECREF(kwargs);

    return (PyObject*)wrapper;
}
