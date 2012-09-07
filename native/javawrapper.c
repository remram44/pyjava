#include <Python.h>
#include <jni.h>


/*==============================================================================
 * JavaMethod type.
 *
 * This is the wrapper returned by JavaClass.getmethod(). It contains the
 * jclass and the name of the method.
 * There is no jmethodID here because this object wraps all the Java methods
 * with the same name, and the actual decision will occur when the call is
 * made (and the parameter types are known).
 */

typedef struct {
    PyObject_HEAD
    jclass javaclass;
    char name[1];
} JavaMethod;

static PyMethodDef JavaMethod_methods[] = {
    {NULL}  /* Sentinel */
};

static PyTypeObject JavaMethod_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.JavaMethod",       /*tp_name*/
    sizeof(JavaMethod),        /*tp_basicsize*/
    1,                         /*tp_itemsize*/
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
    "Java method wrapper",     /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    JavaMethod_methods,        /* tp_methods */
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


/*==============================================================================
 * JavaClass type.
 *
 * This is the wrapper returned by _pyjava.getclass(). It contains the jclass
 * and a getmethod() method that returns a wrapper for a specific method.
 */

typedef struct {
    PyObject_HEAD
    jclass javaclass;
} JavaClass;

static PyObject *JavaClass_getmethod(JavaClass *self, PyObject *args)
{
    const char *name;
    size_t namelen;
    JavaMethod* wrapper;

    if(!(PyArg_ParseTuple(args, "s", &name)))
        return NULL;

    namelen = strlen(name);

    wrapper = PyObject_NewVar(JavaMethod, &JavaMethod_type, namelen);
    wrapper->javaclass = self->javaclass;
    strcpy(wrapper->name, name);

    return (PyObject*)wrapper;
}

static PyMethodDef JavaClass_methods[] = {
    {"getmethod", (PyCFunction)JavaClass_getmethod, METH_VARARGS,
    "getmethod(str) -> JavaMethod\n"
    "\n"
    "Returns a wrapper for a Java method.\n"
    "The actual method with this name to call is chosen at call time, from\n"
    "the type of the parameters."
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


/*==============================================================================
 * Public functions of javawrapper.
 *
 * javawrapper_init() is called by pyjava_start() to create the types.
 *
 * javawrapper_build() is called by pyjava_getclass() to obtain a wrapper.
 */

/**
 * Initialize the module (creates the types).
 */
void javawrapper_init(PyObject *mod)
{
    if(PyType_Ready(&JavaClass_type) < 0)
        return;
    Py_INCREF(&JavaClass_type);

    if(PyType_Ready(&JavaMethod_type) < 0)
        return;
    Py_INCREF(&JavaMethod_type);
}

/**
 * Build a Python wrapper for a Java class.
 */
PyObject *javawrapper_build(jclass javaclass)
{
    JavaClass* wrapper = PyObject_New(JavaClass, &JavaClass_type);

    return (PyObject*)wrapper;
}
