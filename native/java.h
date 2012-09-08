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
    size_t nb_args;
    jclass *args;
} java_Method;

typedef struct _java_Methods {
    size_t nb_methods;
    java_Method methods[1];
} java_Methods;

java_Methods *java_list_overloads(jclass javaclass, const char *method,
                                  size_t nb_args);

void java_free_methods(java_Methods *methods);

#endif
