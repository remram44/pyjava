#ifndef CONVERT_H
#define CONVERT_H

#include <Python.h>
#include <jni.h>


/**
 * Indicate whether a given Python object can be implicitely converted (or
 * wrapped) as a Java object as the given type.
 *
 * Will return 1 if:
 *  - Both types are the same primitive type
 *  - Both types are compatible primitive types (int can be casted to float,
 *    for example)
 *  - The Python object is a JavaInstance from a subclass of the javatype
 *    (which is either a class or an interface)
 */
int convert_check_py2jav(PyObject *pyobj, jclass javatype);

/**
 * Convert a given Python object as a Java object of the given type.
 *
 * See convert_check_py2jav() for what is acceptable.
 */
void convert_py2jav(PyObject *pyobj, jclass javatype, jvalue *javavalue);


/**
 * Convert a given Java object as a Python object.
 *
 * This is always possible as we have no static type to comply to. If the
 * object is a reference to an object, a JavaInstance wrapper will be returned.
 */
PyObject *convert_jav2py(jobject javaobj);

#endif
