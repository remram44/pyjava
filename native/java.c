#include "java.h"

#include <stdlib.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif


JNIEnv *penv = NULL;

typedef jint (JNICALL *type_JNI_CreateJavaVM)(JavaVM**, void**, JavaVMInitArgs*);

JNIEnv *java_start_vm(const char *path, const char **opts, size_t nbopts)
{
    JavaVM *jvm = NULL;
    jint res;
    JNIEnv *env;

    #ifdef JNI_VERSION_1_2
    {
        size_t i;
        JavaVMInitArgs vm_args;
        type_JNI_CreateJavaVM dyn_JNI_CreateJavaVM;
        JavaVMOption *options = malloc(nbopts * sizeof(JavaVMOption));

        #if defined(_WIN32) || defined(_WIN64)
        {
            HINSTANCE jvm_dll;
            jvm_dll = LoadLibrary(path);
            if(jvm_dll == NULL)
            {
                free(options);
                return NULL;
            }
            dyn_JNI_CreateJavaVM = (type_JNI_CreateJavaVM)GetProcAddress(jvm_dll, "JNI_CreateJavaVM");
            if(dyn_JNI_CreateJavaVM == NULL)
            {
                FreeLibrary(jvm_dll);
                free(options);
                return NULL;
            }
        }
        #else
        {
            void *jvm_dll = dlopen(path, RTLD_LAZY);
            if(jvm_dll == NULL)
            {
                free(options);
                return NULL;
            }
            dyn_JNI_CreateJavaVM = dlsym(jvm_dll, "JNI_CreateJavaVM");
            if(dyn_JNI_CreateJavaVM == NULL)
            {
                dlclose(jvm_dll);
                free(options);
                return NULL;
            }
        }
        #endif

        for(i = 0; i < nbopts; ++i)
            options[i].optionString = (char*)opts[i];
        vm_args.version = 0x00010002;
        vm_args.options = options;
        vm_args.nOptions = nbopts;
        vm_args.ignoreUnrecognized = JNI_TRUE;
        /* Create the Java VM */
        res = dyn_JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);

        free(options);
    }
    #else
    #error "JNI 1.1 is not supported"
    #endif

    return (res >= 0)?env:NULL;
}

jstring str_utf8; /* "UTF-8" */

/* java.lang.Class */
jclass class_Class;
    jmethodID meth_Class_getConstructors;
    jmethodID meth_Class_getField;
    jmethodID meth_Class_getMethods;
    jmethodID meth_Class_getName;
    jmethodID meth_Class_isPrimitive;

/* java.lang.Object */
jclass class_Object;
    jmethodID meth_Object_equals;

/* java.lang.String */
jclass class_String;
    jmethodID cstr_String_bytes;
    jmethodID meth_String_getBytes;

/* java.lang.reflect.Method */
    jmethodID meth_Method_getModifiers;
    jmethodID meth_Method_getName;
    jmethodID meth_Method_getParameterTypes;
    jmethodID meth_Method_getReturnType;

/* java.lang.reflect.Field */
    jmethodID meth_Field_getModifiers;
    jmethodID meth_Field_getType;

/* java.lang.reflect.Constructor */
    jmethodID meth_Constructor_getParameterTypes;

/* java.lang.reflect.Modifier */
jclass class_Modifier;
    jmethodID meth_Modifier_isStatic;

void java_init(void)
{
    jclass class_Method, class_Field, class_Constructor;

    class_Class = (*penv)->FindClass(
            penv, "java/lang/Class");
    meth_Class_getConstructors = (*penv)->GetMethodID(
            penv, class_Class, "getConstructors",
            "()[Ljava/lang/reflect/Constructor;");
    meth_Class_getField = (*penv)->GetMethodID(
            penv, class_Class, "getField",
            "(Ljava/lang/String;)Ljava/lang/reflect/Field;");
    meth_Class_getMethods = (*penv)->GetMethodID(
            penv, class_Class, "getMethods",
            "()[Ljava/lang/reflect/Method;");
    meth_Class_getName = (*penv)->GetMethodID(
            penv, class_Class, "getName",
            "()Ljava/lang/String;");
    meth_Class_isPrimitive = (*penv)->GetMethodID(
            penv, class_Class, "isPrimitive",
            "()Z");

    class_Object = (*penv)->FindClass(
            penv, "java/lang/Object");
    meth_Object_equals = (*penv)->GetMethodID(
            penv, class_Object, "equals",
            "(Ljava/lang/Object;)Z");

    class_String = (*penv)->FindClass(
            penv, "java/lang/String");
    cstr_String_bytes = (*penv)->GetMethodID(
            penv, class_String, "<init>",
            "([BLjava/lang/String;)V");
    meth_String_getBytes = (*penv)->GetMethodID(
            penv, class_String, "getBytes",
            "(Ljava/lang/String;)[B");

    class_Method = (*penv)->FindClass(
            penv, "java/lang/reflect/Method");
    meth_Method_getModifiers = (*penv)->GetMethodID(
            penv, class_Method, "getModifiers",
            "()I");
    meth_Method_getName = (*penv)->GetMethodID(
            penv, class_Method, "getName",
            "()Ljava/lang/String;");
    meth_Method_getParameterTypes = (*penv)->GetMethodID(
            penv, class_Method, "getParameterTypes",
            "()[Ljava/lang/Class;");
    meth_Method_getReturnType = (*penv)->GetMethodID(
            penv, class_Method, "getReturnType",
            "()Ljava/lang/Class;");

    class_Field = (*penv)->FindClass(
            penv, "java/lang/reflect/Field");
    meth_Field_getModifiers = (*penv)->GetMethodID(
            penv, class_Field, "getModifiers",
            "()I");
    meth_Field_getType = (*penv)->GetMethodID(
            penv, class_Field, "getType",
            "()Ljava/lang/Class;");

    class_Constructor = (*penv)->FindClass(
            penv, "java/lang/reflect/Constructor");
    meth_Constructor_getParameterTypes = (*penv)->GetMethodID(
            penv, class_Constructor, "getParameterTypes",
            "()[Ljava/lang/Class;");

    class_Modifier = (*penv)->FindClass(
            penv, "java/lang/reflect/Modifier");
    meth_Modifier_isStatic = (*penv)->GetStaticMethodID(
            penv, class_Modifier, "isStatic",
            "(I)Z");

    str_utf8 = (*penv)->NewStringUTF(penv, "UTF-8");
}

java_Methods *java_list_overloads(jclass javaclass, const char *methodname,
        int constructors)
{
    jarray method_array;
    size_t nb_methods;
    size_t nb_args;
    size_t i;
    java_Methods *methods;

    if(!constructors)
    {
        /* Method[] method_array = javaclass.getMethods() */
        method_array = (*penv)->CallObjectMethod(
                penv,
                javaclass, meth_Class_getMethods);
    }
    else
    {
        /* Constructor[] method_array = javaclass.getConstructors() */
        method_array = (*penv)->CallObjectMethod(
                penv,
                javaclass, meth_Class_getConstructors);
    }
    nb_methods = (*penv)->GetArrayLength(penv, method_array);

    /* Create the list of methods. */
    /* FIXME : we could count the exact number of methods we'll need to store.
     * This structure is now kept in memory forever... */
    methods = malloc(sizeof(java_Methods) +
                     sizeof(java_Method) * (nb_methods - 1));
    methods->nb_methods = 0;

    for(i = 0; i < nb_methods; ++i)
    {
        size_t j;
        char is_static = 1;
        jobject parameter_types;
        size_t py_nb_args;
        java_Method *m;

        /* Method method = method_array[i] */
        jobject method = (*penv)->GetObjectArrayElement(
                penv,
                method_array, i);

        if(!constructors)
        {
            /* String name = method.getName() */
            jobject oname = (*penv)->CallObjectMethod(
                    penv,
                    method, meth_Method_getName);
            const char *name = (*penv)->GetStringUTFChars(
                    penv, oname, NULL);
            char correctname = strcmp(name, methodname) == 0;
            (*penv)->ReleaseStringUTFChars(penv, oname, name);
            if(!correctname)
                continue;
        }

        if(!constructors)
        {
            /*
             * Is the method static ?
             * If not, we'll add a first parameter of this class's type.
             */
            jint modifiers = (*penv)->CallIntMethod(
                    penv,
                    method, meth_Method_getModifiers);
            is_static = (*penv)->CallStaticBooleanMethod(
                    penv,
                    class_Modifier, meth_Modifier_isStatic,
                    modifiers) != JNI_FALSE;
        }

        /* Class[] parameter_types = method.getParameterTypes() */
        if(!constructors)
            parameter_types = (*penv)->CallObjectMethod(
                    penv,
                    method, meth_Method_getParameterTypes);
        else
            parameter_types = (*penv)->CallObjectMethod(
                    penv,
                    method, meth_Constructor_getParameterTypes);

        /* In Python, non-static methods take a first "self" parameter that
         * can be made implicit through the "binding" mecanism */
        nb_args = (*penv)->GetArrayLength(penv, parameter_types);
        if(is_static) /* also if constructors */
            py_nb_args = nb_args;
        else
            py_nb_args = nb_args + 1;

        m = &methods->methods[methods->nb_methods];
        m->id = (*penv)->FromReflectedMethod(
                penv, method);
        m->is_static = is_static;

        /* Store the parameters */
        m->nb_args = py_nb_args;
        m->args = malloc(sizeof(jclass) * py_nb_args);
        if(is_static) /* also if constructors */
        {
            for(j = 0; j < nb_args; ++j)
                m->args[j] = (*penv)->GetObjectArrayElement(
                        penv,
                        parameter_types, j);
        }
        else
        {
            m->args[0] = javaclass;
            for(j = 0; j < nb_args; ++j)
                m->args[j+1] = (*penv)->GetObjectArrayElement(
                        penv,
                        parameter_types, j);
        }

        /* Store the return type */
        if(!constructors)
        {
            m->returntype = (*penv)->CallObjectMethod(
                    penv,
                    method, meth_Method_getReturnType);
        }

        methods->nb_methods++;
    }

    if(methods->nb_methods == 0)
    {
        free(methods);
        return NULL;
    }

    return methods;
}

void java_free_methods(java_Methods *methods)
{
    size_t i;
    for(i = 0; i < methods->nb_methods; ++i)
        free(methods->methods[i].args);
    free(methods);
}

jclass java_getclass(jobject javaobject)
{
    return (*penv)->GetObjectClass(penv, javaobject);
}

int java_equals(jobject a, jobject b)
{
    return (*penv)->CallBooleanMethod(penv, a, meth_Object_equals, b) != JNI_FALSE;
}

int java_is_subclass(jclass sub, jclass klass)
{
    return (*penv)->IsAssignableFrom(penv, sub, klass) != JNI_FALSE;
}

const char *java_getclassname(jclass javaclass, size_t *size)
{
    const char *utf8;
    jstring classname = (jstring)(*penv)->CallObjectMethod(
            penv,
            javaclass,
            meth_Class_getName);
    utf8 = java_to_utf8(classname, size);
    java_clear_ref(classname);
    return utf8;
}

void java_clear_ref(jobject ref)
{
    switch((*penv)->GetObjectRefType(penv, ref))
    {
    case JNILocalRefType:
        (*penv)->DeleteLocalRef(penv, ref);
        break;
    case JNIGlobalRefType:
        (*penv)->DeleteGlobalRef(penv, ref);
        break;
    default:
        /* do nothing */
        break;
    }
}

jstring java_from_utf8(const char *utf8, size_t size)
{
    /* string = new String(utf8, "UTF-8"); */

    jstring str;
    jobject bytes = (*penv)->NewByteArray(penv, size);

    (*penv)->SetByteArrayRegion(
            penv, bytes,
            0, size, (jbyte*)utf8);

    str = (*penv)->NewObject(
            penv, class_String, cstr_String_bytes,
            bytes, str_utf8);

    /* Clear reference */
    java_clear_ref(bytes);

    return str;
}

const char *java_to_utf8(jstring str, size_t *newsize)
{
    /* byte[] utf8 = string.getBytes("UTF-8"); */

    jobject bytes = (*penv)->CallObjectMethod(
            penv, str, meth_String_getBytes,
            str_utf8);

    size_t len = (*penv)->GetArrayLength(penv, bytes);
    char *utf8 = malloc(len);
    (*penv)->GetByteArrayRegion(penv, bytes, 0, len, (jbyte*)utf8);

    /* Clear reference */
    java_clear_ref(bytes);

    if(newsize != NULL)
        *newsize = len;

    return utf8;
}
