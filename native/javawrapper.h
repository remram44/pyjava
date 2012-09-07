#ifndef JAVAWRAPPER_H
#define JAVAWRAPPER_H

void javawrapper_init(PyObject *mod);
PyObject *javawrapper_build(jclass javaclass);

#endif
