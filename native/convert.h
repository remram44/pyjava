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


typedef struct _S_JavaFieldDescr {
    jfieldID id;
    char is_static;
    int type; /* this is really a enum CVT_JType */
} JavaFieldDescr;


/**
 * Gets a field descriptor.
 *
 * Gets a descriptor from a field of a class, which contains both the jfieldID
 * and the type of the field (obtained through reflection).
 *
 * @return 1 on success, 0 if the field doesn't exist.
 */
int convert_getfielddescriptor(JavaFieldDescr *field,
        jclass javaclass, const char *name);


/**
 * Convert the value of a Java class field as a Python object.
 *
 * This function takes the field description; it can be either an object or a
 * POD, and the correct Get<type>Field() function will be used.
 */
PyObject *convert_getjavafield(jobject object, const JavaFieldDescr *field);


/**
 * Convert the value of a Java class static field as a Python object.
 *
 * This function takes the field description; it can be either an object or a
 * POD, and the correct GetStatic<type>Field() function will be used.
 */
PyObject *convert_getstaticjavafield(jclass javaclass,
        const JavaFieldDescr *field);

#endif
