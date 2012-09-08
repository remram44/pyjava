#include "convert.h"

#include <stdlib.h>

#include "java.h"

enum CVT_PType {
    CVT_BOOLEAN,
    CVT_BYTE,
    CVT_CHAR,
    CVT_SHORT,
    CVT_INT,
    CVT_LONG,
    CVT_FLOAT,
    CVT_DOUBLE,
    NB_PTYPES
};

static jclass ptypes[NB_PTYPES];
static const char *ptypes_classes[NB_PTYPES] = {
    "java/lang/Boolean",
    "java/lang/Byte",
    "java/lang/Character",
    "java/lang/Short",
    "java/lang/Integer",
    "java/lang/Long",
    "java/lang/Float",
    "java/lang/Double"
};

static jmethodID meth_Class_isPrimitive; /* java.lang.Class.isPrimitive */
static jmethodID meth_Object_equals; /* java.lang.Object.equals */

void convert_init(void)
{
    jclass class_Class, class_Object;
    size_t i;

    class_Class = (*penv)->FindClass(penv, "java/lang/Class");
    meth_Class_isPrimitive = (*penv)->GetMethodID(
            penv, class_Class, "isPrimitive", "()Z");
    class_Object = (*penv)->FindClass(penv, "java/lang/Object");
    meth_Object_equals = (*penv)->GetMethodID(
            penv, class_Object, "equals", "(Ljava/lang/Object;)Z");

    for(i = 0; i < NB_PTYPES; ++i)
    {
        jclass clasz = (*penv)->FindClass(penv, ptypes_classes[i]);
        jfieldID field = (*penv)->GetStaticFieldID(
                penv, clasz, "TYPE", "Ljava/lang/Class;");
        ptypes[i] = (*penv)->GetStaticObjectField(penv, clasz, field);
    }
}

static enum CVT_PType convert_id_type(jclass javatype)
{
    size_t i;
    for(i = 0; i < NB_PTYPES; ++i)
    {
        if((*penv)->CallBooleanMethod(
                penv,
                javatype, meth_Object_equals, ptypes[i]) != JNI_FALSE)
            return i;
    }
    assert(0); /* can't happen */
}

int convert_check_py2jav(PyObject *pyobj, jclass javatype)
{
    char primitive = (*penv)->CallBooleanMethod(
            penv,
            javatype, meth_Class_isPrimitive) == JNI_TRUE;

    if(primitive)
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
        switch(convert_id_type(javatype))
        {
        case CVT_BOOLEAN:
            /* We don't implicitly cast other types to boolean here */
            return PyBool_Check(pyobj);
        case CVT_BYTE:
        {
            long value;
            if(!PyInt_Check(pyobj) && long_status != 1)
                return 0;
            value = PyInt_AsLong(pyobj);
            return 0 <= value && value <= 255;
        }
        case CVT_CHAR:
        {
            if(!PyUnicode_Check(pyobj) && !PyString_Check(pyobj))
                return 0;
            return PySequence_Length(pyobj) == 1;
        }
        case CVT_SHORT:
        {
            long value;
            if(!PyInt_Check(pyobj) && long_status != 1)
                return 0;
            value = PyInt_AsLong(pyobj);
            return -32768 <= value && value <= 32767;
        }
        case CVT_INT:
            return PyInt_Check(pyobj) || long_status == 1;
        case CVT_LONG:
            /* TODO */
            return 1;
        case CVT_FLOAT:
        case CVT_DOUBLE:
            return PyNumber_Check(pyobj);
        default:
            assert(0); /* can't happen */
        }
    }
    else
    {
        /* TODO */
        return 0;
    }
}

void convert_py2jav(PyObject *pyobj, jclass javatype, jvalue *javavalue)
{
    char primitive = (*penv)->CallBooleanMethod(
            penv,
            javatype, meth_Class_isPrimitive) == JNI_TRUE;

    if(primitive)
    {
        switch(convert_id_type(javatype))
        {
        case CVT_BOOLEAN:
            javavalue->z = (pyobj == Py_True)?JNI_TRUE:JNI_FALSE;
            break;
        case CVT_BYTE:
            javavalue->b = PyInt_AsLong(pyobj);
            break;
        case CVT_CHAR:
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
        case CVT_SHORT:
            javavalue->s = PyInt_AsLong(pyobj);
            break;
        case CVT_INT:
            javavalue->i = PyInt_AsLong(pyobj);
            break;
        case CVT_LONG:
            javavalue->j = PyInt_AsLong(pyobj);
            break;
        case CVT_FLOAT:
            javavalue->f = PyFloat_AsDouble(pyobj);
            break;
        case CVT_DOUBLE:
            javavalue->d = PyFloat_AsDouble(pyobj);
            break;
        default:
            assert(0);
        }
    }
    else
    {
        /* TODO */
        javavalue->l = NULL;
    }
}

PyObject *convert_jav2py(jobject javaobj)
{
    Py_INCREF(Py_None);
    return Py_None;
}
