#include "java.h"

#include <stdio.h>
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
            fprintf(stderr, "Loading jvm.dll\n");
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
            options[i].optionString = opts[i];
        vm_args.version = 0x00010002;
        vm_args.options = options;
        vm_args.nOptions = nbopts;
        vm_args.ignoreUnrecognized = JNI_TRUE;
        /* Create the Java VM */
        fprintf(stderr, "JNI_CreateJavaVM(<%d options>)\n", nbopts);
        res = dyn_JNI_CreateJavaVM(&jvm, (void**)&env, &vm_args);
        fprintf(stderr, "res = %ld\nenv = 0x%p\n", res, env);

        free(options);
    }
    #else
    #error "JNI 1.1 is not supported"
    #endif

    return (res >= 0)?env:NULL;
}

static jclass class_Class = NULL; /* java.lang.Class */
static jmethodID meth_Class_getMethods; /* java.lang.Class#getMethods() */
static jmethodID meth_Method_getParameterTypes;
        /* java.lang.reflect.Method.getParameterTypes */
static jmethodID meth_Method_getName;
        /* java.lang.reflect.Method.getName */

java_Methods *java_list_overloads(jclass javaclass, const char *methodname,
                                  size_t nb_args)
{
    jarray method_array;
    size_t nb_methods;
    size_t i;
    java_Methods *methods;

    fprintf(stderr, "java_list_overloads(0x%p, \"%s\", %u)\n",
            javaclass, methodname, nb_args);

    if(class_Class == NULL)
    {
        class_Class = (*penv)->FindClass(
                penv, "java/lang/Class");
        meth_Class_getMethods = (*penv)->GetMethodID(
                penv,
                class_Class, "getMethods",
                "()[Ljava/lang/reflect/Method;");

        jclass class_Method = (*penv)->FindClass(
                penv, "java/lang/reflect/Method");
        meth_Method_getParameterTypes = (*penv)->GetMethodID(
                penv,
                class_Method, "getParameterTypes",
                "()[Ljava/lang/Class;");
        meth_Method_getName = (*penv)->GetMethodID(
                penv,
                class_Method, "getName",
                "()Ljava/lang/String;");
    }

    /* Method[] method_array = javaclass.getMethods() */
    method_array = (*penv)->CallObjectMethod(
            penv,
            javaclass, meth_Class_getMethods);
    nb_methods = (*penv)->GetArrayLength(penv, method_array);

    /* Create the list of methods. It will not necessarily be entirely filled,
     * but whatever. */
    methods = malloc(sizeof(java_Methods) +
                     sizeof(java_Method) * (nb_methods - 1));
    methods->nb_methods = 0;

    for(i = 0; i < nb_methods; ++i)
    {
        size_t c, j;
        jobject parameter_types;

        /* Method method = method_array[i] */
        jobject method = (*penv)->GetObjectArrayElement(
                penv,
                method_array, i);

        {
            /* String name = method.getName() */
            jobject oname = (*penv)->CallObjectMethod(
                    penv,
                    method, meth_Method_getName);
            const char *name = (*penv)->GetStringUTFChars(
                    penv, oname, NULL);
            char correctname;
            correctname = strcmp(name, methodname) == 0;
            (*penv)->ReleaseStringUTFChars(penv, oname, name);
            if(!correctname)
                continue;
        }

        /* Class[] parameter_types = method.getParameterTypes() */
        parameter_types = (*penv)->CallObjectMethod(
                penv,
                method, meth_Method_getParameterTypes);

        if((*penv)->GetArrayLength(penv, parameter_types) != nb_args)
            continue;

        c = methods->nb_methods;
        methods->methods[c].id = (*penv)->FromReflectedMethod(
                penv, method);
        methods->methods[c].nb_args = nb_args;
        methods->methods[c].args = malloc(sizeof(jclass) * nb_args);
        for(j = 0; j < nb_args; ++j)
            methods->methods[c].args[j] = (*penv)->GetObjectArrayElement(
                    penv,
                    parameter_types, j);
        methods->nb_methods++;
    }

    fprintf(stderr, "Returning %d matching methods\n", methods->nb_methods);

    return methods;
}

void java_free_methods(java_Methods *methods)
{
    size_t i;
    for(i = 0; i < methods->nb_methods; ++i)
        free(methods->methods[i].args);
    free(methods);
}
