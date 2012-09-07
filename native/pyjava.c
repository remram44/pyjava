#include <Python.h>

#include "java.h"
#include "javawrapper.h"


PyObject *Err_Base;
PyObject *Err_ClassNotFound;

/**
 * _pyjava.start function: dynamically load a JVM DLL and start it.
 */
static PyObject *pyjava_start(PyObject *self, PyObject *args)
{
    const char *path;
    PyObject *options;
    size_t size;
    const char **option_array;
    size_t i;

    if(!(PyArg_ParseTuple(args, "sO!", &path, &PyList_Type, &options)))
        return NULL;

    size = PyList_GET_SIZE(options);
    option_array = malloc(size * sizeof(char *));
    for(i = 0; i < size; ++i)
    {
        PyObject *option = PyList_GET_ITEM(options, i);
        if(!PyString_Check(option))
        {
            PyErr_SetString(
                    PyExc_TypeError,
                    "Options list contained non-string objects.");
            free(option_array);
            return NULL;
        }

        option_array[i] = PyString_AS_STRING(option);
    }

    penv = java_start_vm(path, option_array, size);
    free(option_array);

    if(penv != NULL)
    {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

/**
 * _pyjava.getclass function: get a wrapper for a Java class.
 */
static PyObject *pyjava_getclass(PyObject *self, PyObject *args)
{
    const char *classname;
    jclass javaclass;

    if(!(PyArg_ParseTuple(args, "s", &classname)))
        return NULL;

    if(penv == NULL)
    {
        PyErr_SetString(
                Err_Base,
                "Java VM is not running.");
        return NULL;
    }

    javaclass = (*penv)->FindClass(penv, classname);
    if(javaclass == NULL)
    {
        PyErr_SetString(Err_ClassNotFound, classname);
        return NULL;
    }

    return javawrapper_build(javaclass);
}

static PyMethodDef methods[] = {
    {"start",  pyjava_start, METH_VARARGS,
    "start(bytestring, list) -> bool\n"
    "\n"
    "Starts a Java Virtual Machine. The first argument is the path of the\n"
    "library to be dynamically loaded. The second is a list of strings that\n"
    "are passed to the JVM as options."},
    {"getclass",  pyjava_getclass, METH_VARARGS,
    "getclass(str) -> JavaClass\n"
    "\n"
    "Find the desired class and returns a wrapper."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyjava(void)
{
    PyObject *mod;

    mod = Py_InitModule("_pyjava", methods);
    if(mod == NULL)
        return ;

    Err_Base = PyErr_NewException("pyjava.Error", NULL, NULL);
    Py_INCREF(Err_Base);
    PyModule_AddObject(mod, "Error", Err_Base);

    Err_ClassNotFound = PyErr_NewException("pyjava.ClassNotFound", Err_ClassNotFound, NULL);
    Py_INCREF(Err_ClassNotFound);
    PyModule_AddObject(mod, "ClassNotFound", Err_ClassNotFound);

    javawrapper_init(mod);
}
