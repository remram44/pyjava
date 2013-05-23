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


typedef struct _S_java_Method {
    jmethodID id;
    char is_static;
    size_t nb_args;
    jclass *args;
    jclass returntype;
} java_Method;

typedef struct _S_java_Methods {
    size_t nb_methods;
    java_Method methods[1];
} java_Methods;


/**
 * Initialization method.
 *
 * To be called AFTER java_start_vm() has succeeded.
 */
void java_init(void);


#define LIST_STATIC         0x1
#define LIST_NONSTATIC      0x2
#define LIST_ALL            (LIST_STATIC | LIST_NONSTATIC)

/**
 * Returns all the Java methods with a given name, or NULL if none is found.
 */
java_Methods *java_list_methods(jclass javaclass, const char *method,
        int what);

/**
 * Returns all the Java constructors, or NULL if none is found.
 *
 * Constructors are special static methods named "<init>". No return type is
 * set on the java_Method objects returned.
 */
java_Methods *java_list_constructors(jclass javaclass);

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
 * Gets the name of a Java class.
 */
const char *java_getclassname(jclass javaclass, size_t *size);


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
char *java_to_utf8(jstring str, size_t *newsize);


extern jstring str_utf8; /* "UTF-8" */

/* java.lang.Class */
extern jclass class_Class;
    extern jmethodID meth_Class_getConstructors;
    extern jmethodID meth_Class_getField;
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
    extern jmethodID meth_Method_getModifiers;
    extern jmethodID meth_Method_getName;
    extern jmethodID meth_Method_getParameterTypes;
    extern jmethodID meth_Method_getReturnType;

/* java.lang.reflect.Field */
    extern jmethodID meth_Field_getModifiers;
    extern jmethodID meth_Field_getType;

/* java.lang.reflect.Constructor */
    extern jmethodID meth_Constructor_getParameterTypes;

/* java.lang.reflect.Modifier */
extern jclass class_Modifier;
    extern jmethodID meth_Modifier_isStatic;

#endif
