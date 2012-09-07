#ifndef JAVAWRAPPER_H
#define JAVAWRAPPER_H

#include <Python.h>
#include "java.h"


/**
 * Initialize the module (creates the types).
 */
void javawrapper_init(PyObject *mod);

/**
 * Build a Python wrapper for a Java class.
 */
PyObject *javawrapper_build(jclass javaclass);

#endif
