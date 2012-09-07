#ifndef JAVAWRAPPER_H
#define JAVAWRAPPER_H

#include <Python.h>
#include "java.h"


void javawrapper_init(PyObject *mod);
PyObject *javawrapper_build(jclass javaclass);

#endif
