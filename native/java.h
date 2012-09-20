#ifndef JAVA_H
#define JAVA_H

#include <jni.h>


extern JNIEnv *penv;

/**
 * Starts a JVM using the invocation API.
 *
 * The Java DLL is loaded dynamically from the specified path.
 *
 * @param path Path to the JVM DLL, to be passed to JNI_CreateJavaVM().
 * @param options The option strings to pass to the JVM.
 * @param nbopts The number of option strings to be passed.
 */
JNIEnv *java_start_vm(const char *path, const char **opts, size_t nbopts);


typedef struct _java_Method {
    jmethodID id;
    char is_static;
    size_t nb_args;
    jclass *args;
    jclass returntype;
} java_Method;

typedef struct _java_Methods {
    size_t nb_methods;
    java_Method methods[1];
} java_Methods;


/**
 * Initialization method.
 *
 * To be called AFTER java_start_vm() has succeeded.
 */
void java_init(void);


/**
 * Returns all the Java methods with a given name, or NULL if none is found.
 *
 * @param constructors Indicate to list constructors instead of methods. In
 * that case, is_static is always 1 and the return type is never set.
 */
java_Methods *java_list_overloads(jclass javaclass, const char *method,
        int constructors);

void java_free_methods(java_Methods *methods);


/**
 * Returns the Java class of a Java object.
 */
jclass java_getclass(jobject javaobject);


/**
 * Calls Object.equals on two Java objects.
 *
 * @return 1 if it is equal.
 */
int java_equals(jobject a, jobject b);


/**
 * Checks whether a class is a subclass (or the same, or implement) of another.
 *
 * @return 1 if it is.
 */
int java_is_subclass(jclass sub, jclass klass);

#endif
