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


/**
 * Clear a local or global ref to a Java object.
 */
void java_clear_ref(jobject ref);


/**
 * Create a Java string from standard UTF-8.
 *
 * Java's string functions expect a modified UTF-8 encoding which noone else
 * uses. These functions allow to use regular UTF-8, like Python does.
 *
 * @param utf8 Standard UTF-8 representation of the string; might contain NULL
 * bytes.
 * @param size Size of the string, in bytes. Might be larger than strlen(utf8)
 * because of embedded zeros.
 * @return A reference to a String object, which you might want to clear.
 */
jstring java_from_utf8(const char *utf8, size_t size);


/**
 * Extracts a standard UTF-8 representation of a Java string.
 *
 * Java's string functions expect a modified UTF-8 encoding which noone else
 * uses. These functions allow to use regular UTF-8, like Python does.
 *
 * @param str A reference to a String object.
 * @param newsize If not NULL, the size of the new representation, in bytes,
 * will be written at that address. It is necessary as the returned string
 * cannot be safely NULL-terminated; it might contain embedded NULL bytes.
 */
const char *java_to_utf8(jstring str, size_t *newsize);


extern jstring str_utf8; /* "UTF-8" */

/* java.lang.Class */
extern jclass class_Class;
    extern jmethodID meth_Class_getConstructors;
    extern jmethodID meth_Class_getMethods;
    extern jmethodID meth_Class_isPrimitive;

/* java.lang.Object */
extern jclass class_Object;
    extern jmethodID meth_Object_equals;

/* java.lang.String */
extern jclass class_String;
    extern jmethodID cstr_String_bytes;
    extern jmethodID meth_String_getBytes;

/* java.lang.reflect.Method */
    extern jmethodID meth_Method_getParameterTypes;
    extern jmethodID meth_Method_getName;
    extern jmethodID meth_Method_getReturnType;
    extern jmethodID meth_Method_getModifiers;

/* java.lang.reflect.Constructor */
    extern jmethodID meth_Constructor_getParameterTypes;

/* java.lang.reflect.Modifier */
extern jclass class_Modifier;
    extern jmethodID meth_Modifier_isStatic;

#endif
