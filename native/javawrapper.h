#ifndef JAVAWRAPPER_H
#define JAVAWRAPPER_H

#include <Python.h>
#include <jni.h>


/**
 * Initialize the module (creates the types).
 */
void javawrapper_init(PyObject *mod);


/**
 * Build a Python wrapper for a Java class.
 */
PyObject *javawrapper_wrap_class(jclass javaclass);


/**
 * Unwraps a JavaInstance object.
 *
 * @param javaobject Where to store the Java object, or NULL.
 * @param javaclass Where to store the Java class, or NULL.
 * @returns 1 on success, 0 on error (for instance, pyobj wasn't a JavaInstance
 * Python object).
 */
int javawrapper_unwrap_instance(PyObject *pyobject,
        jobject *javaobject, jclass *javaclass);


/**
 * Wraps a JavaInstance object.
 */
PyObject *javawrapper_wrap_instance(jobject javaobject);


/**
 * The _pyjava.issubclass() function.
 */
PyObject *javawrapper_issubclass(PyObject *self, PyObject *args);

#endif
