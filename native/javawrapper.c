#include "javawrapper.h"

#include "convert.h"
#include "java.h"
#include "pyjava.h"


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
    java_Methods *overloads;
    char name[1];
} JavaMethod;

static PyObject *JavaMethod_call(JavaMethod *self, PyObject *args)
{
    size_t nbargs;
    size_t nonmatchs = 0;
    size_t i;
    int matching_method = -1;
    int nb_matches = 1;

    nbargs = PyTuple_Size(args);

    for(i = 0; i < self->overloads->nb_methods; ++i)
    {
        /* Attempt to match the arguments with the ones we got from Python. */
        size_t a;
        char matches = 1;
        java_Method *m = &self->overloads->methods[i];

        if(m->nb_args != nbargs)
            continue;

        for(a = 0; a < m->nb_args; ++a)
        {
            jclass javatype = m->args[a];
            PyObject *pyarg = PyTuple_GET_ITEM(args, a);
            if(!convert_check_py2jav(pyarg, javatype))
            {
                matches = 0;
                break;
            }
        }

        if(matches)
        {
            if(matching_method == -1)
                matching_method = i;
            else
                nb_matches++;
        }
        else
            nonmatchs++;
    }

    /*
     * Several methods matched the given arguments. We'll use the first method
     * we found The Java compiler shouldn't let this happen... may be a bug in
     * the convert module?
     */
    if(nb_matches > 1)
        PyErr_Warn(
                PyExc_RuntimeWarning,
                "Multiple Java methods matching Python parameters");

    if(matching_method == -1)
    {
        PyErr_Format(
                Err_NoMatchingMethod,
                "%d methods \"%s\" with %d parameters (no match)\n",
                nonmatchs, self->name, nbargs);
        return NULL;
    }

    {
        jvalue *java_parameters;
        PyObject *ret;
        java_parameters = malloc(sizeof(jvalue) * nbargs);
        java_Method *m = &self->overloads->methods[matching_method];
        for(i = 0; i < nbargs; ++i)
            convert_py2jav(
                    PyTuple_GET_ITEM(args, i),
                    m->args[i],
                    &java_parameters[i]);

        if(m->is_static)
            ret = convert_calljava_static(
                    self->javaclass, m->id,
                    java_parameters,
                    m->returntype);
        else
            ret = convert_calljava(
                    java_parameters[0].l, m->id,
                    java_parameters+1,
                    m->returntype);

        free(java_parameters);

        return ret;
    }
}

static void JavaMethod_dealloc(void *v_self)
{
    JavaMethod *self = (JavaMethod*)v_self;

    java_free_methods(self->overloads);
}

static PyMethodDef JavaMethod_methods[] = {
    {"call", (PyCFunction)JavaMethod_call, METH_VARARGS,
    "call(*args) -> [object]\n"
    "\n"
    "Calls the Java method. Which overload gets called depends on the type\n"
    "of the parameters."
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject JavaMethod_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.JavaMethod",       /*tp_name*/
    sizeof(JavaMethod),        /*tp_basicsize*/
    1,                         /*tp_itemsize*/
    JavaMethod_dealloc,        /*tp_dealloc*/
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
    "Java method wrapper",     /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    JavaMethod_methods,        /*tp_methods*/
    0,                         /*tp_members*/
    0,                         /*tp_getset*/
    0,                         /*tp_base*/
    0,                         /*tp_dict*/
    0,                         /*tp_descr_get*/
    0,                         /*tp_descr_set*/
    0,                         /*tp_dictoffset*/
    0,                         /*tp_init*/
    0,                         /*tp_alloc*/
    PyType_GenericNew,         /*tp_new*/
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
    java_Methods *methods;
    JavaMethod* wrapper;

    if(!(PyArg_ParseTuple(args, "s", &name)))
        return NULL;

    /* Find the methods of that class with that name */
    methods = java_list_overloads(self->javaclass, name);
    if(methods == NULL)
    {
        PyErr_SetString(
                PyExc_AttributeError,
                name);
        return NULL;
    }

    namelen = strlen(name);

    wrapper = PyObject_NewVar(JavaMethod, &JavaMethod_type, namelen);
    wrapper->javaclass = self->javaclass;
    wrapper->overloads = methods;
    strcpy(wrapper->name, name);

    return (PyObject*)wrapper;
}

static PyObject *JavaClass_create(JavaClass *self, PyObject *args)
{
    /*JavaInstance* wrapper;

    wrapper = PyObject_New(JavaInstance, &JavaInstance_type);
    wrapper->javaclass = self->javaclass;
    wrapper->javaobject = ???;

    return (PyObject*)wrapper;*/
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef JavaClass_methods[] = {
    {"getmethod", (PyCFunction)JavaClass_getmethod, METH_VARARGS,
    "getmethod(str) -> JavaMethod\n"
    "\n"
    "Returns a wrapper for a Java method.\n"
    "The actual method with this name to call is chosen at call time, from\n"
    "the type of the parameters."
    },
    {"create", (PyCFunction)JavaClass_create, METH_VARARGS,
    "create(str) -> JavaInstance\n"
    "\n"
    "Builds an object.\n"
    "This allocates a new object, selecting the correct <init> method from\n"
    "the type of the parameters, and returns a wrapper for the Java\n"
    "instance."
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
    "Java class wrapper",      /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    JavaClass_methods,         /*tp_methods*/
    0,                         /*tp_members*/
    0,                         /*tp_getset*/
    0,                         /*tp_base*/
    0,                         /*tp_dict*/
    0,                         /*tp_descr_get*/
    0,                         /*tp_descr_set*/
    0,                         /*tp_dictoffset*/
    0,                         /*tp_init*/
    0,                         /*tp_alloc*/
    PyType_GenericNew,         /*tp_new*/
};


/*==============================================================================
 * Public functions of javawrapper.
 *
 * javawrapper_init() is called by pyjava_start() to create the types.
 *
 * javawrapper_build() is called by pyjava_getclass() to obtain a wrapper.
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

PyObject *javawrapper_build(jclass javaclass)
{
    JavaClass* wrapper = PyObject_New(JavaClass, &JavaClass_type);
    wrapper->javaclass = javaclass;

    return (PyObject*)wrapper;
}
