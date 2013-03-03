#include "convert.h"

#include <stdlib.h>

#include "java.h"
#include "javawrapper.h"

enum CVT_JType {
    CVT_J_VOID,
    CVT_J_BOOLEAN,
    CVT_J_BYTE,
    CVT_J_CHAR,
    CVT_J_SHORT,
    CVT_J_INT,
    CVT_J_LONG,
    CVT_J_FLOAT,
    CVT_J_DOUBLE,
    CVT_J_OBJECT
};
#define NB_JPTYPES 9
#define NB_JTYPES 10
#define JTYPE_PRIMITIVE(t) ((t) != CVT_J_OBJECT)

static jclass jptypes[NB_JPTYPES];
static const char *jptypes_classes[NB_JPTYPES] = {
    "java/lang/Void",
    "java/lang/Boolean",
    "java/lang/Byte",
    "java/lang/Character",
    "java/lang/Short",
    "java/lang/Integer",
    "java/lang/Long",
    "java/lang/Float",
    "java/lang/Double"
};

void convert_init(void)
{
    size_t i;
    for(i = 0; i < NB_JPTYPES; ++i)
    {
        jclass clasz = (*penv)->FindClass(penv, jptypes_classes[i]);
        jfieldID field = (*penv)->GetStaticFieldID(
                penv, clasz, "TYPE", "Ljava/lang/Class;");
        jptypes[i] = (*penv)->GetStaticObjectField(penv, clasz, field);
    }
}

static enum CVT_JType convert_id_type(jclass javatype)
{
    char primitive = (*penv)->CallBooleanMethod(
            penv,
            javatype, meth_Class_isPrimitive) == JNI_TRUE;

    if(primitive)
    {
        size_t i;
        for(i = 0; i < NB_JPTYPES; ++i)
        {
            if((*penv)->CallBooleanMethod(
                    penv,
                    javatype, meth_Object_equals, jptypes[i]) != JNI_FALSE)
                return i;
        }
        assert(0); /* can't happen */
        return 0;
    }
    else
        return CVT_J_OBJECT;
}

int convert_check_py2jav(PyObject *pyobj, jclass javatype)
{
    enum CVT_JType type = convert_id_type(javatype);

    if(JTYPE_PRIMITIVE(type))
    {
        char long_status;
        if(!PyLong_Check(pyobj))
            long_status = 0;
        else
        {
            PyObject *asint = PyNumber_Int(pyobj);
            char toobig = !PyInt_Check(asint); /* still a long: too big */
            Py_DECREF(asint);
            if(toobig)
                long_status = 2;
            else
                long_status = 1; /* can fit in an int */
        }
        switch(type)
        {
        case CVT_J_BOOLEAN:
            /* We don't implicitly cast other types to boolean here */
            return PyBool_Check(pyobj);
        case CVT_J_BYTE:
            {
                long value;
                if(!PyInt_Check(pyobj) && long_status != 1)
                    return 0;
                value = PyInt_AsLong(pyobj);
                return 0 <= value && value <= 255;
            }
        case CVT_J_CHAR:
            {
                if(!PyUnicode_Check(pyobj) && !PyString_Check(pyobj))
                    return 0;
                return PySequence_Length(pyobj) == 1;
            }
        case CVT_J_SHORT:
            {
                long value;
                if(!PyInt_Check(pyobj) && long_status != 1)
                    return 0;
                value = PyInt_AsLong(pyobj);
                return -32768 <= value && value <= 32767;
            }
        case CVT_J_INT:
            return PyInt_Check(pyobj) || long_status == 1;
        case CVT_J_LONG:
            return PyInt_Check(pyobj) || long_status == 1;
        case CVT_J_FLOAT:
        case CVT_J_DOUBLE:
            return PyNumber_Check(pyobj);
        default:
            assert(0); /* can't happen */
        }
    }
    else if(pyobj == Py_None)
        return 1;
    else
    {
        /* Checks that pyobj is a JavaInstance and unwraps the jclass */
        jclass passed_class;
        if(javawrapper_unwrap_instance(pyobj, NULL, &passed_class))
        {
            /* Check that the passed object has a class that is subclass of the
             * wanted class */
            return java_is_subclass(passed_class, javatype);
        }
        else
        {
            /* Special case: We can convert a unicode object to String */
            if(java_equals(javatype, class_String) && PyUnicode_Check(pyobj))
                return 1;
        }

        return 0;
    }
    return 0;
}

void convert_py2jav(PyObject *pyobj, jclass javatype, jvalue *javavalue)
{
    enum CVT_JType type = convert_id_type(javatype);

    if(JTYPE_PRIMITIVE(type))
    {
        switch(type)
        {
        case CVT_J_BOOLEAN:
            javavalue->z = (pyobj == Py_True)?JNI_TRUE:JNI_FALSE;
            break;
        case CVT_J_BYTE:
            javavalue->b = PyInt_AsLong(pyobj);
            break;
        case CVT_J_CHAR:
            if(PyString_Check(pyobj))
                javavalue->c = PyString_AsString(pyobj)[0];
            else /* PyUnicode_Check(pyobj) */
            {
                /*
                 * Hmm, this is supposed to be UCS2... but Java provides only
                 * 16 bits as well...
                 */
                javavalue->c = PyUnicode_AS_UNICODE(pyobj)[0];
            }
            break;
        case CVT_J_SHORT:
            javavalue->s = PyInt_AsLong(pyobj);
            break;
        case CVT_J_INT:
            javavalue->i = PyInt_AsLong(pyobj);
            break;
        case CVT_J_LONG:
            javavalue->j = PyInt_AsLong(pyobj);
            break;
        case CVT_J_FLOAT:
            javavalue->f = PyFloat_AsDouble(pyobj);
            break;
        case CVT_J_DOUBLE:
            javavalue->d = PyFloat_AsDouble(pyobj);
            break;
        default:
            assert(0); /* can't happen */
        }
    }
    else if(pyobj == Py_None)
        javavalue->l = NULL;
    else
    {
        if(javawrapper_unwrap_instance(pyobj, &javavalue->l, NULL))
            return ;
        else
        {
            /* Special case: String objects can be created from unicode, which
             * makes sense. They can get converted back when received from
             * Java. */
            PyObject *pyutf8 = PyUnicode_AsUTF8String(pyobj);
            const char *utf8 = PyString_AsString(pyutf8);
            size_t size = PyString_GET_SIZE(pyutf8);
            javavalue->l = java_from_utf8(utf8, size);
            Py_DECREF(pyutf8);
        }
    }
}

PyObject *convert_calljava(jobject self, jmethodID method,
        jvalue *parameters, jclass returntype)
{
    enum CVT_JType type = convert_id_type(returntype);

    switch(type)
    {
    case CVT_J_VOID:
        (*penv)->CallVoidMethodA(
                    penv,
                    self, method,
                    parameters);
        Py_INCREF(Py_None);
        return Py_None;
    case CVT_J_BOOLEAN:
        {
            jboolean ret = (*penv)->CallBooleanMethodA(
                    penv,
                    self, method,
                    parameters);
            if(ret == JNI_FALSE)
            {
                Py_INCREF(Py_False);
                return Py_False;
            }
            else
            {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }
    case CVT_J_BYTE:
        {
            jbyte ret = (*penv)->CallByteMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_CHAR:
        {
            jchar ret = (*penv)->CallCharMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyUnicode_FromFormat("%c", (int)ret);
        }
    case CVT_J_SHORT:
        {
            jshort ret = (*penv)->CallShortMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_INT:
        {
            jint ret = (*penv)->CallIntMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_LONG:
        {
            jlong ret = (*penv)->CallLongMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyLong_FromLongLong(ret);
        }
    case CVT_J_FLOAT:
        {
            jfloat ret = (*penv)->CallFloatMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_DOUBLE:
        {
            jdouble ret = (*penv)->CallDoubleMethodA(
                    penv,
                    self, method,
                    parameters);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_OBJECT:
        {
            jobject ret = (*penv)->CallObjectMethodA(
                    penv,
                    self, method,
                    parameters);
            if(ret == NULL)
            {
                Py_INCREF(Py_None);
                return Py_None;
            }
            else if(java_equals(java_getclass(ret), class_String))
            {
                /* Special case: String objects get converted to unicode, which
                 * makes sense. They can get converted back if need be. */
                size_t size;
                char *utf8 = java_to_utf8(ret, &size);
                PyObject *unicode = PyUnicode_FromStringAndSize(
                        utf8,
                        size);
                free(utf8);
                return unicode;
            }
            else
                return javawrapper_wrap_instance(ret);
        }
    default:
        assert(0); /* can't happen */
        return Py_None;
    }
}

PyObject *convert_calljava_static(jclass javaclass, jmethodID method,
        jvalue *parameters, jclass returntype)
{
    enum CVT_JType type = convert_id_type(returntype);

    switch(type)
    {
    case CVT_J_VOID:
        (*penv)->CallStaticVoidMethodA(
                    penv,
                    javaclass, method,
                    parameters);
        Py_INCREF(Py_None);
        return Py_None;
    case CVT_J_BOOLEAN:
        {
            jboolean ret = (*penv)->CallStaticBooleanMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            if(ret == JNI_FALSE)
            {
                Py_INCREF(Py_False);
                return Py_False;
            }
            else
            {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }
    case CVT_J_BYTE:
        {
            jbyte ret = (*penv)->CallStaticByteMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_CHAR:
        {
            jchar ret = (*penv)->CallStaticCharMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyUnicode_FromFormat("%c", (int)ret);
        }
    case CVT_J_SHORT:
        {
            jshort ret = (*penv)->CallStaticShortMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_INT:
        {
            jint ret = (*penv)->CallStaticIntMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyInt_FromLong(ret);
        }
    case CVT_J_LONG:
        {
            jlong ret = (*penv)->CallStaticLongMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyLong_FromLongLong(ret);
        }
    case CVT_J_FLOAT:
        {
            jfloat ret = (*penv)->CallStaticFloatMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_DOUBLE:
        {
            jdouble ret = (*penv)->CallStaticDoubleMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_OBJECT:
        {
            jobject ret = (*penv)->CallStaticObjectMethodA(
                    penv,
                    javaclass, method,
                    parameters);
            if(ret == NULL)
            {
                Py_INCREF(Py_None);
                return Py_None;
            }
            else if(java_equals(java_getclass(ret), class_String))
            {
                /* Special case: String objects get converted to unicode, which
                 * makes sense. They can get converted back if need be. */
                size_t size;
                char *utf8 = java_to_utf8(ret, &size);
                PyObject *unicode = PyUnicode_FromStringAndSize(
                        utf8,
                        size);
                free(utf8);
                return unicode;
            }
            else
                return javawrapper_wrap_instance(ret);
        }
    default:
        assert(0); /* can't happen */
        return Py_None;
    }
}

int convert_getfielddescriptor(JavaFieldDescr *field,
        jclass javaclass, const char *name)
{
    jobject javafield;
    jclass javatype;
    jint modifiers;
    char is_static;
    jstring javaname = java_from_utf8(name, strlen(name));

    javafield = (*penv)->CallObjectMethod(
            penv,
            javaclass,
            meth_Class_getField,
            javaname);
    if(javafield == NULL)
    {
        (*penv)->ExceptionClear(penv);
        return 0;
    }

    modifiers = (*penv)->CallIntMethod(
            penv,
            javafield, meth_Field_getModifiers);
    is_static = (*penv)->CallStaticBooleanMethod(
            penv,
            class_Modifier, meth_Modifier_isStatic,
            modifiers) != JNI_FALSE;

    javatype = (*penv)->CallObjectMethod(
            penv,
            javafield,
            meth_Field_getType);

    java_clear_ref(javaname);

    field->id = (*penv)->FromReflectedField(penv, javafield);
    field->type = (int)convert_id_type(javatype);
    field->is_static = is_static;

    java_clear_ref(javafield);
    java_clear_ref(javatype);

    return 1;
}

PyObject *convert_getjavafield(jobject object, const JavaFieldDescr *field)
{
    switch((enum CVT_JType)field->type)
    {
    case CVT_J_BOOLEAN:
        {
            jboolean ret = (*penv)->GetBooleanField(
                    penv,
                    object,
                    field->id);
            if(ret == JNI_FALSE)
            {
                Py_INCREF(Py_False);
                return Py_False;
            }
            else
            {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }
    case CVT_J_BYTE:
        {
            jbyte ret = (*penv)->GetByteField(
                    penv,
                    object,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_CHAR:
        {
            jchar ret = (*penv)->GetCharField(
                    penv,
                    object,
                    field->id);
            return PyUnicode_FromFormat("%c", (int)ret);
        }
    case CVT_J_SHORT:
        {
            jshort ret = (*penv)->GetShortField(
                    penv,
                    object,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_INT:
        {
            jint ret = (*penv)->GetIntField(
                    penv,
                    object,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_LONG:
        {
            jlong ret = (*penv)->GetLongField(
                    penv,
                    object,
                    field->id);
            return PyLong_FromLongLong(ret);
        }
    case CVT_J_FLOAT:
        {
            jfloat ret = (*penv)->GetFloatField(
                    penv,
                    object,
                    field->id);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_DOUBLE:
        {
            jdouble ret = (*penv)->GetDoubleField(
                    penv,
                    object,
                    field->id);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_OBJECT:
        {
            jobject ret = (*penv)->GetObjectField(
                    penv,
                    object,
                    field->id);
            if(ret == NULL)
            {
                Py_INCREF(Py_None);
                return Py_None;
            }
            else if(java_equals(java_getclass(ret), class_String))
            {
                /* Special case: String objects get converted to unicode, which
                 * makes sense. They can get converted back if need be. */
                size_t size;
                char *utf8 = java_to_utf8(ret, &size);
                PyObject *unicode = PyUnicode_FromStringAndSize(
                        utf8,
                        size);
                free(utf8);
                return unicode;
            }
            else
                return javawrapper_wrap_instance(ret);
        }
    case CVT_J_VOID:
    default:
        assert(0); /* can't happen */
    }
    return Py_None;
}

PyObject *convert_getstaticjavafield(jclass javaclass,
        const JavaFieldDescr *field)
{
    switch((enum CVT_JType)field->type)
    {
    case CVT_J_BOOLEAN:
        {
            jboolean ret = (*penv)->GetStaticBooleanField(
                    penv,
                    javaclass,
                    field->id);
            if(ret == JNI_FALSE)
            {
                Py_INCREF(Py_False);
                return Py_False;
            }
            else
            {
                Py_INCREF(Py_True);
                return Py_True;
            }
        }
    case CVT_J_BYTE:
        {
            jbyte ret = (*penv)->GetStaticByteField(
                    penv,
                    javaclass,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_CHAR:
        {
            jchar ret = (*penv)->GetStaticCharField(
                    penv,
                    javaclass,
                    field->id);
            return PyUnicode_FromFormat("%c", (int)ret);
        }
    case CVT_J_SHORT:
        {
            jshort ret = (*penv)->GetStaticShortField(
                    penv,
                    javaclass,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_INT:
        {
            jint ret = (*penv)->GetStaticIntField(
                    penv,
                    javaclass,
                    field->id);
            return PyInt_FromLong(ret);
        }
    case CVT_J_LONG:
        {
            jlong ret = (*penv)->GetStaticLongField(
                    penv,
                    javaclass,
                    field->id);
            return PyLong_FromLongLong(ret);
        }
    case CVT_J_FLOAT:
        {
            jfloat ret = (*penv)->GetStaticFloatField(
                    penv,
                    javaclass,
                    field->id);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_DOUBLE:
        {
            jdouble ret = (*penv)->GetStaticDoubleField(
                    penv,
                    javaclass,
                    field->id);
            return PyFloat_FromDouble(ret);
        }
    case CVT_J_OBJECT:
        {
            jobject ret = (*penv)->GetStaticObjectField(
                    penv,
                    javaclass,
                    field->id);
            if(ret == NULL)
            {
                Py_INCREF(Py_None);
                return Py_None;
            }
            else if(java_equals(java_getclass(ret), class_String))
            {
                /* Special case: String objects get converted to unicode, which
                 * makes sense. They can get converted back if need be. */
                size_t size;
                char *utf8 = java_to_utf8(ret, &size);
                PyObject *unicode = PyUnicode_FromStringAndSize(
                        utf8,
                        size);
                free(utf8);
                return unicode;
            }
            else
                return javawrapper_wrap_instance(ret);
        }
    case CVT_J_VOID:
    default:
        assert(0); /* can't happen */
        return Py_None;
    }
}
