#include "javawrapper.h"

#include "convert.h"
#include "java.h"
#include "pyjava.h"


PyObject *javawrapper_compare(PyObject *o1, PyObject *o2, int op);


static java_Method *find_matching_overload(java_Methods *overloads,
        PyObject *args, size_t *nonmatchs, int what)
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

        if( (m->is_static && !(what & FIELD_STATIC))
         || (!m->is_static && !(what & FIELD_NONSTATIC)) )
            continue;
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


static PyObject *_method_call(java_Methods *overloads, int bound,
        jclass javaclass, PyObject *args, int what)
{
    size_t nbargs = PyTuple_Size(args);
    PyObject *ret = NULL;
    jvalue *java_parameters;

    size_t nonmatchs;
    java_Method *matching_method = find_matching_overload(overloads,
            args, &nonmatchs, what);

    /* If bound, we can only call non-static methods. This happens in
     * BoundMethod_call and ClassMethod_call.
     * If unbound, we might want to call both (from UnboundMethod_call) or only
     * static (from ClassMethod_call). */
    assert(!bound || what == FIELD_NONSTATIC);

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

    if(matching_method->is_static && !bound)
    {
        ret = convert_calljava_static(
                javaclass, matching_method->id,
                java_parameters,
                matching_method->returntype);
    }
    else if(!matching_method->is_static)
    {
        ret = convert_calljava(
                java_parameters[0].l, matching_method->id,
                java_parameters+1,
                matching_method->returntype);
    }

    free(java_parameters);

    if(ret == NULL)
    {
        PyErr_Format(
                Err_NoMatchingOverload,
                "%zu methods with %zd parameters (no match)",
                nonmatchs, nbargs);
    }
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

    return _method_call(self->overloads, 0, self->javaclass, args, FIELD_BOTH);
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

    return _method_call(self->overloads, 1, self->javaclass, args,
                        FIELD_NONSTATIC);
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
 * ClassMethod type.
 *
 * This represents a Class method obtained from a JavaClass.
 * If the class is Java's Class, 'overloads' contains static methods that we
 * don't want to be bound to the class object, but if a non-static method is
 * called, we want it to be bound to the class object. It contains the jclass,
 * the jobject, and the name of the method.
 * There is no jmethodID here because this object wraps all the Java methods
 * with the same name, and the actual decision will occur when the call is
 * made (and the parameter types are known).
 */

typedef struct _S_ClassMethod {
    PyObject_VAR_HEAD
    jclass javaclass;
    java_Methods *overloads;
    char name[1];
} ClassMethod;

static PyObject *ClassMethod_call(PyObject *v_self,
        PyObject *args, PyObject *kwargs)
{
    ClassMethod *self = (ClassMethod*)v_self;

    {
        PyObject *b_args;
        PyObject *result;
        /* b_args = (self,) + args */
        {
            PyObject *wrapped_obj = javawrapper_wrap_class(self->javaclass);
            PyObject *first_arg = Py_BuildValue("(O)", wrapped_obj);
            Py_DECREF(wrapped_obj);
            b_args = PySequence_Concat(first_arg, args);
            Py_DECREF(first_arg);
        }

        /* Attempts bound method call */
        result = _method_call(self->overloads, 1, class_Class, b_args,
                              FIELD_NONSTATIC);
        if(result != NULL)
            return result;
        PyErr_Clear();
    }

    /* Attempts unbound method call (static only) */
    return _method_call(self->overloads, 0, self->javaclass, args,
                        FIELD_STATIC);
}

static void ClassMethod_dealloc(PyObject *v_self)
{
    ClassMethod *self = (ClassMethod*)v_self;

    if(self->overloads != NULL)
        java_free_methods(self->overloads);
    if(self->javaclass != NULL)
        (*penv)->DeleteGlobalRef(penv, self->javaclass);

    self->ob_type->tp_free(self);
}

static PyTypeObject ClassMethod_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "pyjava.ClassMethod",      /*tp_name*/
    sizeof(ClassMethod),       /*tp_basicsize*/
    1,                         /*tp_itemsize*/
    ClassMethod_dealloc,       /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    ClassMethod_call,          /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Java class method",       /*tp_doc*/
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
    int res;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return -1; /* TypeError from PyString_AsString() */
    javaclass = java_getclass(self->javaobject);

    res = convert_setjavafield(javaclass, self->javaobject, name,
                               FIELD_NONSTATIC, value);
    if(res == 1)
    {
        (*penv)->DeleteLocalRef(penv, javaclass);
        return 0;
    }
    else
    {
        if(res == 0)
            PyErr_Format(
                    Err_FieldTypeError,
                    "Java nonstatic attribute %s has incompatible type",
                    name);
        else /* res == -1 */
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

PyTypeObject JavaInstance_type = {
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
    PyObject_HashNotImplemented, /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    JavaInstance_getattr,      /*tp_getattro*/
    JavaInstance_setattr,      /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Java object wrapper",     /*tp_doc*/
    0,                         /*tp_traverse*/
    0,                         /*tp_clear*/
    javawrapper_compare,       /*tp_richcompare*/
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

extern PyTypeObject JavaClass_type;

typedef struct _S_JavaClass {
    PyTypeObject pytype;
    jobject javaclass;
    /* the struct until here is the same as JavaInstance, as we inherit! */
    java_Methods *constructors;
} JavaClass;

static PyObject *JavaClass_new(PyTypeObject *subtype,
        PyObject *args, PyObject *kwds)
{
    PyErr_SetString(
            PyExc_NotImplementedError,
                "Subclassing Java classes is not supported");
    return NULL;
}

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
            args, &nonmatchs, FIELD_STATIC);

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
        JavaInstance *inst = (JavaInstance*)PyObject_CallObject(
                (PyObject*)&JavaInstance_type,
                NULL);
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
    if(!(*penv)->IsSameObject(penv, self->javaclass, class_Class))
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
    {
        /* If calling from Class, we can access all methods; if calling on
         * another Class instance, don't access the static methods */
        int list_what = (*penv)->IsSameObject(penv, class_Class,
                                              self->javaclass)?
                FIELD_BOTH:FIELD_NONSTATIC;
        java_Methods *methods = java_list_methods(class_Class, name,
                                                  list_what);
        if(methods != NULL)
        {
            /* A different kind of wrapper is used here because we need a
             * non-static Class method to be bound to javaclass but a static
             * Class method not to be (obviously, it's static). */
            ClassMethod *wrapper = PyObject_NewVar(ClassMethod,
                    &ClassMethod_type, namelen);
            wrapper->javaclass = (*penv)->NewGlobalRef(penv, self->javaclass);
            wrapper->overloads = methods;
            memcpy(wrapper->name, name, namelen);
            wrapper->name[namelen] = '\0';

            return (PyObject*)wrapper;
        }
    }

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
    int res;
    const char *name = PyString_AsString(attr_name); /* UTF-8 */
    if(name == NULL)
        return -1; /* TypeError from PyString_AsString() */

    res = convert_setjavafield(self->javaclass, NULL, name,
                               FIELD_STATIC, value);
    if(res == 1)
        return 0;
    else
    {
        if(res == 0)
            PyErr_Format(
                    Err_FieldTypeError,
                    "Java static attribute %s has incompatible type",
                    name);
        else /* res == -1 */
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

    PyType_Type.tp_free((PyObject*)self);
}

static PyObject *JavaClass_subclasscheck(JavaClass *self, PyObject *args)
{
    PyObject *obj;
    JavaClass *other;
    if(!PyArg_ParseTuple(args, "O!", &PyType_Type, &obj))
        return NULL;

    if(!PyObject_IsInstance(obj, (PyObject*)&JavaClass_type))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }
    other = (JavaClass*)obj;

    if(java_is_subclass(other->javaclass, self->javaclass))
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

static PyObject *JavaClass_instancecheck(JavaClass *self, PyObject *args)
{
    PyObject *obj;
    JavaInstance *inst;
    jclass instclass;
    int res;
    if(!PyArg_ParseTuple(args, "O", &obj))
        return NULL;

    if(!PyObject_IsInstance(obj, (PyObject*)&JavaInstance_type))
    {
        Py_INCREF(Py_False);
        return Py_False;
    }
    inst = (JavaInstance*)obj;

    instclass = java_getclass(inst->javaobject);
    res = java_is_subclass(instclass, self->javaclass);
    (*penv)->DeleteLocalRef(penv, instclass);

    if(res)
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

static PyMethodDef JavaClass_methods[] = {
    {"__subclasscheck__", (PyCFunction)JavaClass_subclasscheck, METH_VARARGS,
    "__subclasscheck__(obj) -> bool"
    },
    {"__instancecheck__", (PyCFunction)JavaClass_instancecheck, METH_VARARGS,
    "__instancecheck__(obj) -> bool"
    },
    {NULL}  /* Sentinel */
};

PyTypeObject JavaClass_type = {
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
    PyObject_HashNotImplemented, /*tp_hash */
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
    javawrapper_compare,       /*tp_richcompare*/
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
    JavaClass_new,             /*tp_new*/
};


/*==============================================================================
 * JavaClass and JavaInstance comparison
 */

static int get_compared_object(PyObject *o, jobject *j)
{
    int check;

    check = PyObject_IsInstance(o, (PyObject*)&JavaInstance_type);
    if(check == -1)
        return -1;
    else if(check)
    {
        *j = ((JavaInstance*)o)->javaobject;
        return 1;
    }

    check = PyObject_IsInstance(o, (PyObject*)&JavaClass_type);
    if(check == -1)
        return -1;
    else if(check)
    {
        *j = ((JavaClass*)o)->javaclass;
        return 1;
    }

    return 0;
}

PyObject *javawrapper_compare(PyObject *o1, PyObject *o2, int op)
{
    int expectation;
    if(op == Py_EQ)
        expectation = 1;
    else if(op == Py_NE)
        expectation = 0;
    else
    {
        PyErr_SetString(
                PyExc_TypeError,
                "JavaInstance only supports == and !=");
        return NULL;
    }

    {
        jobject inst1, inst2;
        int cv1, cv2;

        cv1 = get_compared_object(o1, &inst1);
        if(cv1 == -1)
            return NULL;
        cv2 = get_compared_object(o2, &inst2);
        if(cv2 == -1)
            return NULL;

        if(!cv1 || !cv2)
        {
            Py_INCREF(Py_False);
            return Py_False;
        }

        int cmp = (*penv)->IsSameObject(penv, inst1, inst2)?1:0;
        if(cmp == expectation)
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
}


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

    JavaClass_type.tp_base = &PyType_Type;
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

    if(PyType_Ready(&ClassMethod_type) < 0)
        return;
    Py_INCREF(&ClassMethod_type);
    PyModule_AddObject(mod, "ClassMethod", (PyObject*)&ClassMethod_type);
}

PyObject *javawrapper_wrap_class(jclass javaclass)
{
    JavaClass *wrapper;
    PyObject *cstr_args;
    {
        size_t name_len;
        const char *name = java_getclassname(javaclass, &name_len);
        cstr_args = Py_BuildValue(
                "(s#(O){})",
                name, name_len, &PyBaseObject_Type);
    }

    {
        PyObject *obj = PyType_Type.tp_new(&JavaClass_type, cstr_args, NULL);
        if(PyType_Type.tp_init != NULL)
            PyType_Type.tp_init(obj, cstr_args, NULL);
        Py_DECREF(cstr_args);
        wrapper = (JavaClass*)obj;
    }

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
    else if(PyObject_IsInstance(pyobject, (PyObject*)&JavaClass_type))
    {
        JavaClass *cls = (JavaClass*)pyobject;
        if(javaobject != NULL)
            *javaobject = cls->javaclass;
        if(javaclass != NULL)
            *javaclass = class_Class;
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
        JavaInstance *inst = (JavaInstance*)PyObject_CallObject(
                (PyObject*)&JavaInstance_type,
                NULL);
        inst->javaobject = (*penv)->NewGlobalRef(penv, javaobject);
        return (PyObject*)inst;
    }
}
