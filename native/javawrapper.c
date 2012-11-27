#include "javawrapper.h"

#include "convert.h"
#include "java.h"
#include "pyjava.h"


static java_Method *find_matching_overload(java_Methods *overloads,
        PyObject *args, size_t *nonmatchs)
{
    size_t nbargs;
    size_t i;
    int matching_method = -1;
    int nb_matches = 1;

    nbargs = PyTuple_Size(args);
    *nonmatchs = 0;

    for(i = 0; i < overloads->nb_methods; ++i)
    {
        /* Attempt to match the arguments with the ones we got from Python. */
        size_t a;
        char matches = 1;
        java_Method *m = &overloads->methods[i];

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
            (*nonmatchs)++;
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
        return NULL;

    return &overloads->methods[matching_method];
}


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
    size_t nonmatchs;
    size_t i;

    java_Method *matching_method;

    if(self->overloads == NULL)
    {
        PyErr_Format(
                Err_NoMatchingOverload,
                "no visible method \"%s\"",
                self->name);
        return NULL;
    }

    matching_method = find_matching_overload(self->overloads,
            args, &nonmatchs);

    nbargs = PyTuple_Size(args);

    if(matching_method == NULL)
    {
        PyErr_Format(
                Err_NoMatchingOverload,
                "%u methods \"%s\" with %d parameters (no match)",
                nonmatchs, self->name, nbargs);
        return NULL;
    }

    {
        jvalue *java_parameters;
        PyObject *ret;
        java_parameters = malloc(sizeof(jvalue) * nbargs);
        for(i = 0; i < nbargs; ++i)
            convert_py2jav(
                    PyTuple_GET_ITEM(args, i),
                    matching_method->args[i],
                    &java_parameters[i]);

        if(matching_method->is_static)
            ret = convert_calljava_static(
                    self->javaclass, matching_method->id,
                    java_parameters,
                    matching_method->returntype);
        else
        {
            /* TODO : check that first parameter has type self->javaclass, or
             * raise TypeError */
            ret = convert_calljava(
                    java_parameters[0].l, matching_method->id,
                    java_parameters+1,
                    matching_method->returntype);
        }

        free(java_parameters);

        return ret;
    }
}

static void JavaMethod_dealloc(PyObject *v_self)
{
    JavaMethod *self = (JavaMethod*)v_self;

    if(self->overloads)
        java_free_methods(self->overloads);

    self->ob_type->tp_free(self);
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
    "_pyjava.JavaMethod",      /*tp_name*/
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
 * JavaInstance type.
 *
 * This is the wrapper for Java instance objects; it contains a jobject.
 */

typedef struct {
    PyObject_HEAD
    jobject javaobject;
} JavaInstance;

static PyTypeObject JavaInstance_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyjava.JavaInstance",    /*tp_name*/
    sizeof(JavaInstance),      /*tp_basicsize*/
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
    "Java object wrapper",     /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    0,                         /*tp_methods*/
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
 * JavaField type.
 *
 * This is the wrapper returned by JavaClass.getfield(). It contains the
 * jclass and the jfieldID of the field.
 */

typedef struct {
    PyObject_HEAD
    jclass javaclass;
    JavaFieldDescr field;
} JavaField;

static PyObject *JavaField_get(JavaField *self, PyObject *args)
{
    JavaInstance *obj;
    if(self->field.is_static)
    {
        if(!PyArg_ParseTuple(args, "", &JavaInstance_type, &obj))
            return NULL;
        return convert_getstaticjavafield(self->javaclass, &self->field);
    }
    else
    {
        if(!PyArg_ParseTuple(args, "O!", &JavaInstance_type, &obj))
            return NULL;
        if(java_equals(java_getclass(obj->javaobject), self->javaclass))
            return convert_getjavafield(obj->javaobject, &self->field);
        else
        {
            PyErr_SetString(
                    Err_Base,
                    "field getter used on object of different type");
            return NULL;
        }
    }
}

static PyMethodDef JavaField_methods[] = {
    {"get", (PyCFunction)JavaField_get, METH_VARARGS,
    "get([object]) -> [object]\n"
    "\n"
    "Gets this class field on this object."
    },
    {NULL}  /* Sentinel */
};

static PyTypeObject JavaField_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "_pyjava.JavaField",       /*tp_name*/
    sizeof(JavaField),         /*tp_basicsize*/
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
    "Java field wrapper",      /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    0,                         /*tp_richcompare*/
    0,                         /*tp_weaklistoffset*/
    0,                         /*tp_iter*/
    0,                         /*tp_iternext*/
    JavaField_methods,         /*tp_methods*/
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
    jobject javaclass;
    /* the struct until here is the same as JavaInstance, as we inherit! */
    java_Methods *constructors;
} JavaClass;

static PyObject *JavaClass_getmethod(JavaClass *self, PyObject *args)
{
    const char *name;
    size_t namelen;
    java_Methods *methods;
    JavaMethod *wrapper;

    if(!(PyArg_ParseTuple(args, "s", &name)))
        return NULL;

    /* Find the methods of that class with that name */
    methods = java_list_overloads(self->javaclass, name, 0);
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

static PyObject *JavaClass_getfield(JavaClass *self, PyObject *args)
{
    const char *name;
    JavaField *wrapper;

    if(!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    wrapper = PyObject_New(JavaField, &JavaField_type);
    wrapper->javaclass = self->javaclass;
    if(convert_getfielddescriptor(&wrapper->field, self->javaclass, name) == 1)
        return (PyObject*)wrapper;
    else
    {
        PyErr_SetString(
                PyExc_AttributeError,
                name);
        return NULL;
    }
}

static PyObject *JavaClass_create(JavaClass *self, PyObject *args)
{
    jobject javaobject;
    size_t nbargs;
    size_t nonmatchs;
    size_t i;

    java_Method *matching_method;

    if(self->constructors == NULL)
    {
        PyErr_SetString(
                Err_NoMatchingOverload,
                "no visible constructor");
        return NULL;
    }

    matching_method = find_matching_overload(self->constructors,
            args, &nonmatchs);

    nbargs = PyTuple_Size(args);

    if(matching_method == NULL)
    {
        PyErr_Format(
                Err_NoMatchingOverload,
                "%u constructors with %d parameters (no match)",
                nonmatchs, nbargs);
        return NULL;
    }

    {
        jvalue *java_parameters;
        java_parameters = malloc(sizeof(jvalue) * nbargs);
        for(i = 0; i < nbargs; ++i)
            convert_py2jav(
                    PyTuple_GET_ITEM(args, i),
                    matching_method->args[i],
                    &java_parameters[i]);

        javaobject = (*penv)->NewObjectA(
                penv,
                self->javaclass, matching_method->id,
                java_parameters);

        free(java_parameters);
    }

    {
        JavaInstance *inst;

        inst = PyObject_New(JavaInstance, &JavaInstance_type);
        inst->javaobject = javaobject;

        return (PyObject*)inst;
    }
}

static void JavaClass_dealloc(PyObject *v_self)
{
    JavaClass *self = (JavaClass*)v_self;

    if(self->constructors != NULL)
        java_free_methods(self->constructors);

    self->ob_type->tp_free(self);
}

static PyMethodDef JavaClass_methods[] = {
    {"getmethod", (PyCFunction)JavaClass_getmethod, METH_VARARGS,
    "getmethod(str) -> JavaMethod\n"
    "\n"
    "Returns a wrapper for a Java method.\n"
    "The actual method with this name to call is chosen at call time, from\n"
    "the type of the parameters."
    },
    {"getfield", (PyCFunction)JavaClass_getfield, METH_VARARGS,
    "getfield(str) -> JavaField\n"
    "\n"
    "Returns a wrapper for a Java field."
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
    "_pyjava.JavaClass",       /*tp_name*/
    sizeof(JavaClass),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    JavaClass_dealloc,         /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT |
      Py_TPFLAGS_BASETYPE,     /*tp_flags*/
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
 * javawrapper_wrap_class() is called by pyjava_getclass() to obtain a wrapper.
 */

void javawrapper_init(PyObject *mod)
{
    if(PyType_Ready(&JavaInstance_type) < 0)
        return;
    Py_INCREF(&JavaInstance_type);
    PyModule_AddObject(mod, "JavaInstance", (PyObject*)&JavaInstance_type);

    if(PyType_Ready(&JavaField_type) < 0)
        return;
    Py_INCREF(&JavaField_type);
    PyModule_AddObject(mod, "JavaField", (PyObject*)&JavaField_type);

    JavaClass_type.tp_base = &JavaInstance_type;
    if(PyType_Ready(&JavaClass_type) < 0)
        return;
    Py_INCREF(&JavaClass_type);
    PyModule_AddObject(mod, "JavaClass", (PyObject*)&JavaClass_type);

    if(PyType_Ready(&JavaMethod_type) < 0)
        return;
    Py_INCREF(&JavaMethod_type);
    PyModule_AddObject(mod, "JavaMethod", (PyObject*)&JavaMethod_type);
}

PyObject *javawrapper_wrap_class(jclass javaclass)
{
    JavaClass* wrapper = PyObject_New(JavaClass, &JavaClass_type);
    wrapper->javaclass = javaclass;
    wrapper->constructors = java_list_overloads(javaclass, "<init>", 1);

    return (PyObject*)wrapper;
}

int javawrapper_unwrap_instance(PyObject *pyobject,
        jobject *javaobject, jclass *javaclass)
{
    if(PyObject_IsInstance(pyobject, (PyObject*)&JavaInstance_type))
    {
        JavaInstance *inst = (JavaInstance*)pyobject;
        if(javaobject != NULL)
            *javaobject = inst->javaobject;
        if(javaclass != NULL)
            *javaclass = java_getclass(inst->javaobject);
        return 1;
    }
    return 0;
}

PyObject *javawrapper_wrap_instance(jobject javaobject)
{
    jclass javaclass = java_getclass(javaobject);
    if(java_equals(javaclass, class_Class))
        return javawrapper_wrap_class(javaobject);
    else
    {
        JavaInstance *inst = PyObject_New(JavaInstance, &JavaInstance_type);
        inst->javaobject = javaobject;
        return (PyObject*)inst;
    }
}
