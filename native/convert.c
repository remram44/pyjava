#include "convert.h"

int convert_check_py2jav(PyObject *pyobj, jclass javatype)
{
    return 1;
}

void convert_py2jav(PyObject *pyobj, jclass javatype, jvalue *javavalue)
{
}

PyObject *convert_jav2py(jobject javaobj)
{
    Py_INCREF(Py_None);
    return Py_None;
}
