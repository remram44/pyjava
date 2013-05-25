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
     * we found.
     * The Java compiler shouldn't let this happen... may be a bug in the
     * convert module?
     */
    if(nb_matches > 1)
        PyErr_Warn(
                PyExc_RuntimeWarning,
                "Multiple Java methods matching Python parameters");

    if(matching_method == -1)
        return NULL;

    return &overloads->methods[matching_method];
}


PyObject *_method_call(java_Methods *overloads, int bound,
        jclass javaclass, PyObject *args)
{
    size_t nbargs = PyTuple_Size(args);
    PyObject *ret;
    jvalue *java_parameters;

    size_t nonmatchs;
    java_Method *matching_method = find_matching_overload(overloads,
            args, &nonmatchs);
    if(matching_method == NULL)
    {
        size_t nbargs = PyTuple_Size(args);
        PyErr_Format(
                Err_NoMatchingOverload,
                "%zu methods with %zd parameters (no match)",
                nonmatchs, nbargs);
        return NULL;
    }

    java_parameters = malloc(sizeof(jvalue) * nbargs);
    {
        size_t i;
        for(i = 0; i < nbargs; ++i)
            convert_py2jav(
                    PyTuple_GET_ITEM(args, i),
                    matching_method->args[i],
                    &java_parameters[i]);
    }

    if(matching_method->is_static)
    {
        assert(!bound);
        ret = convert_calljava_static(
                javaclass, matching_method->id,
                java_parameters,
                matching_method->returntype);
    }
    else
    {
        ret = convert_calljava(
                java_parameters[0].l, matching_method->id,
                java_parameters+1,
                matching_method->returntype);
    }

    free(java_parameters);

    return ret;
}


/*==============================================================================
 * UnboundMethod type.
 *
 * This represents an unbound method, i.e. a method obtained from the class.
 * It has no instance associated with it. When called, it will be matched with
 * both the static and nonstatic methods of that class. It contains the jclass
 * and the name of the method.
 * There is no jmethodID here because this object wraps all the Java methods
 * with the same name, and the actual decision will occur when the call is
 * made (and the parameter types are known).
 */

typedef struct _S_UnboundMethod {
    PyObject_VAR_HEAD
    jclass javaclass;
    java_Methods *overloads;
    char name[1];
} UnboundMethod;

static PyObject *UnboundMethod_call(PyObject *v_self,
        PyObject *args, PyObject *kwargs)
{
    UnboundMethod *self = (UnboundMethod*)v_self;

    return _method_call(self->overloads, 0, self->javaclass, args);
}

static void UnboundMethod_dealloc(PyObject *v_self)
{
    UnboundMethod *self = (UnboundMethod*)v_self;

    if(self->overloads != NULL)
        java_free_methods(self->overloads);
    if(self->javaclass != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javaclass);

    self->ob_type->tp_free(self);
}

static PyTypeObject UnboundMethod_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.UnboundMethod",    /*tp_name*/
    sizeof(UnboundMethod),     /*tp_basicsize*/
    1,                         /*tp_itemsize*/
    UnboundMethod_dealloc,     /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    UnboundMethod_call,        /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Java unbound method",     /*tp_doc*/
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
 * BoundMethod type.
 *
 * This represents a bound method, i.e. a method obtained from an instance.
 * It is associated with the instance it was retrieved from. When called, it
 * will be matched only with the nonstatic methods of that class, and will use
 * the instance it is bound to as the self argument. It contains the jclass,
 * the jobject, and the name of the method.
 * There is no jmethodID here because this object wraps all the Java methods
 * with the same name, and the actual decision will occur when the call is
 * made (and the parameter types are known).
 */

typedef struct _S_BoundMethod {
    PyObject_VAR_HEAD
    jclass javaclass;
    jobject javainstance;
    java_Methods *overloads;
    char name[1];
} BoundMethod;

static PyObject *BoundMethod_call(PyObject *v_self,
        PyObject *args, PyObject *kwargs)
{
    BoundMethod *self = (BoundMethod*)v_self;

    /* args = (self,) + args */
    {
        PyObject *wrapped_obj = javawrapper_wrap_instance(self->javainstance);
        PyObject *first_arg = Py_BuildValue("(O)", wrapped_obj);
        Py_DECREF(wrapped_obj);
        args = PySequence_Concat(first_arg, args);
        /* the original 'args' object is deleted by Python anyway */
        Py_DECREF(first_arg);
    }

    return _method_call(self->overloads, 1, self->javaclass, args);
}

static void BoundMethod_dealloc(PyObject *v_self)
{
    BoundMethod *self = (BoundMethod*)v_self;

    if(self->overloads != NULL)
        java_free_methods(self->overloads);
    if(self->javaclass != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javaclass);
    if(self->javainstance != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javainstance);

    self->ob_type->tp_free(self);
}

static PyTypeObject BoundMethod_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.BoundMethod",      /*tp_name*/
    sizeof(BoundMethod),       /*tp_basicsize*/
    1,                         /*tp_itemsize*/
    BoundMethod_dealloc,       /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    BoundMethod_call,          /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Java bound method",       /*tp_doc*/
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
 * JavaInstance type.
 *
 * This is the wrapper for Java instance objects; it contains a jobject.
 */

typedef struct _S_JavaInstance {
    PyObject_HEAD
    jobject javaobject;
} JavaInstance;

static PyObject *JavaInstance_getattr(PyObject *v_self, PyObject *attr_name)
{
    JavaInstance *self = (JavaInstance*)v_self;
    Py_ssize_t namelen;
    jclass javaclass;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return NULL; /* TypeError from PyString_AsString() */
    namelen = PyString_GET_SIZE(attr_name);
    javaclass = java_getclass(self->javaobject);

    /* First, try to find a method with that name, in that class.
     * If at least one such method exists, we return a BoundMethod. */
    {
        java_Methods *methods = java_list_methods(javaclass, name,
                                                  FIELD_BOTH);
        if(methods != NULL)
        {
            BoundMethod *wrapper = PyObject_NewVar(BoundMethod,
                    &BoundMethod_type, namelen);
            wrapper->javaclass = (*penv)->NewGlobalRef(penv, javaclass);
            wrapper->javainstance = (*penv)->NewGlobalRef(penv,
                                                          self->javaobject);
            wrapper->overloads = methods;
            memcpy(wrapper->name, name, namelen);
            wrapper->name[namelen] = '\0';

            return (PyObject*)wrapper;
        }
    }

    /* Then, try a field (static or nonstatic) */
    {
        PyObject *field = convert_getjavafield(javaclass, self->javaobject,
                                               name, FIELD_BOTH);
        if(field != NULL)
        {
            (*penv)->DeleteLocalRef(penv, javaclass);
            return field;
        }
    }

    /* We didn't find anything, raise AttributeError */
    PyErr_Format(
            PyExc_AttributeError,
            "Java instance has no attribute %s",
            name);
    (*penv)->DeleteLocalRef(penv, javaclass);
    return NULL;
}

static int JavaInstance_setattr(PyObject *v_self, PyObject *attr_name,
        PyObject *value)
{
    JavaInstance *self = (JavaInstance*)v_self;
    jclass javaclass;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return -1; /* TypeError from PyString_AsString() */
    javaclass = java_getclass(self->javaobject);

    if(convert_setjavafield(javaclass, self->javaobject, name, FIELD_NONSTATIC,
                            value))
    {
        (*penv)->DeleteLocalRef(penv, javaclass);
        return 0;
    }
    else
    {
        PyErr_Format(
                PyExc_AttributeError,
                "Java class has no nonstatic attribute %s",
                name);
        (*penv)->DeleteLocalRef(penv, javaclass);
        return -1;
    }
}

static void JavaInstance_dealloc(PyObject *v_self)
{
    JavaInstance *self = (JavaInstance*)v_self;

    if(self->javaobject != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javaobject);

    self->ob_type->tp_free(self);
}

static PyTypeObject JavaInstance_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.JavaInstance",     /*tp_name*/
    sizeof(JavaInstance),      /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    JavaInstance_dealloc,      /*tp_dealloc*/
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
    JavaInstance_getattr,      /*tp_getattro*/
    JavaInstance_setattr,      /*tp_setattro*/
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
 * JavaClass type.
 *
 * This is the wrapper returned by getclass().
 * It can be called to make a new instance of that Java type, or accessed for
 * either static fields or unbound methods (static or not).
 */

typedef struct _S_JavaClass {
    PyObject_HEAD
    jobject javaclass;
    /* the struct until here is the same as JavaInstance, as we inherit! */
    java_Methods *constructors;
} JavaClass;

static PyObject *JavaClass_create(PyObject *v_self,
        PyObject *args, PyObject *kwargs)
{
    JavaClass *self = (JavaClass*)v_self;
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
                "%zu constructors with %zd parameters (no match)",
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
        inst->javaobject = (*penv)->NewGlobalRef(penv, javaobject);

        return (PyObject*)inst;
    }
}

static PyObject *JavaClass_getattr(PyObject *v_self, PyObject *attr_name)
{
    JavaClass *self = (JavaClass*)v_self;
    Py_ssize_t namelen;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return NULL; /* TypeError from PyString_AsString() */
    namelen = PyString_GET_SIZE(attr_name);

    /* First, try to find a method with that name, in that class.
     * If at least one such method exists, we return an UnboundMethod. */
    {
        java_Methods *methods = java_list_methods(self->javaclass, name,
                                                  FIELD_BOTH);
        if(methods != NULL)
        {
            UnboundMethod *wrapper = PyObject_NewVar(UnboundMethod,
                    &UnboundMethod_type, namelen);
            wrapper->javaclass = (*penv)->NewGlobalRef(penv, self->javaclass);
            wrapper->overloads = methods;
            memcpy(wrapper->name, name, namelen);
            wrapper->name[namelen] = '\0';

            return (PyObject*)wrapper;
        }
    }

    /* Then, try a field (static) */
    {
        PyObject *field = convert_getjavafield(self->javaclass, NULL, name,
                                               FIELD_STATIC);
        if(field != NULL)
            return field;
    }

    /* Finally, act on the Class object (reflection) */
    /* TODO */

    /* We didn't find anything, raise AttributeError */
    PyErr_Format(
            PyExc_AttributeError,
            "Java class has no attribute %s",
            name);
    return NULL;
}

static int JavaClass_setattr(PyObject *v_self, PyObject *attr_name,
        PyObject *value)
{
    JavaClass *self = (JavaClass*)v_self;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return -1; /* TypeError from PyString_AsString() */

    if(convert_setjavafield(self->javaclass, NULL, name, FIELD_STATIC, value))
        return 0;
    else
    {
        PyErr_Format(
                PyExc_AttributeError,
                "Java class has no static attribute %s",
                name);
        return -1;
    }
}

static void JavaClass_dealloc(PyObject *v_self)
{
    JavaClass *self = (JavaClass*)v_self;

    if(self->constructors != NULL)
        java_free_methods(self->constructors);
    if(self->javaclass != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javaclass);

    self->ob_type->tp_free(self);
}

static PyTypeObject JavaClass_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.JavaClass",        /*tp_name*/
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
    JavaClass_create,          /*tp_call*/
    0,                         /*tp_str*/
    JavaClass_getattr,         /*tp_getattro*/
    JavaClass_setattr,         /*tp_setattro*/
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

    JavaClass_type.tp_base = &JavaInstance_type;
    if(PyType_Ready(&JavaClass_type) < 0)
        return;
    Py_INCREF(&JavaClass_type);
    PyModule_AddObject(mod, "JavaClass", (PyObject*)&JavaClass_type);

    if(PyType_Ready(&UnboundMethod_type) < 0)
        return;
    Py_INCREF(&UnboundMethod_type);
    PyModule_AddObject(mod, "UnboundMethod", (PyObject*)&UnboundMethod_type);

    if(PyType_Ready(&BoundMethod_type) < 0)
        return;
    Py_INCREF(&BoundMethod_type);
    PyModule_AddObject(mod, "BoundMethod", (PyObject*)&BoundMethod_type);
}

PyObject *javawrapper_wrap_class(jclass javaclass)
{
    JavaClass *wrapper = PyObject_New(JavaClass, &JavaClass_type);
    wrapper->javaclass = (*penv)->NewGlobalRef(penv, javaclass);
    wrapper->constructors = java_list_constructors(javaclass);

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
        inst->javaobject = (*penv)->NewGlobalRef(penv, javaobject);
        return (PyObject*)inst;
    }
}
