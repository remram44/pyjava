#ifndef CONVERT_H
#define CONVERT_H

#include <Python.h>
#include <jni.h>


/**
 * Initialization method.
 *
 * To be called AFTER java_start_vm() has succeeded.
 */
void convert_init(void);

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
 * Convert the return value of a Java method as a Python object.
 *
 * This function takes the return type as a jclass; it can be an object or a
 * POD, and the correct Call<type>MethodA() function will be used.
 */
PyObject *convert_calljava(jobject self, jmethodID method,
        jvalue *params, jclass returntype);


/**
 * Convert the return value of a Java static method as a Python object.
 *
 * This function takes the return type as a jclass; it can be an object or a
 * POD, and the correct CallStatic<type>MethodA() function will be used.
 */
PyObject *convert_calljava_static(jclass javaclass, jmethodID method,
        jvalue *params, jclass returntype);


/**
 * Convert the value of a Java field as a Python object.
 *
 * This function takes the class, the object (or NULL) and the name of the
 * field. It can return either an object or a POD, obtained using the correct
 * Get<type>Field() function.
 *
 * If there is no field by that name, returns NULL (doesn't set an exception).
 */
PyObject *convert_getjavafield(jclass javaclass, jobject object,
        const char *name, int type);


/**
 * Sets a field on a Java class or instance to the given Python object.
 *
 * This function takes the class, the object (or NULL), the name of the field
 * and the Python object, and sets it using the correct Get<type>Field()
 * function.
 *
 * If there is no field by that name, or if the type doesn't match, returns
 * 0 (doesn't set an exception). Else, returns 1.
 */
int convert_setjavafield(jclass javaclass, jobject javaobject,
        const char *name, int type, PyObject *value);

#endif
