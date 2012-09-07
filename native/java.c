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
        fprintf(stderr, "res = %d\nenv = 0x%p\n", res, env);

        free(options);
    }
    #else
    #error "JNI 1.1 is not supported"
    #endif

    return (res >= 0)?env:NULL;
}

java_Methods *java_list_overloads(jclass javaclass, const char *method,
                                  size_t nbargs)
{
    /* TODO : list all the methods with that name and that number of
     * arguments */
    return NULL;
}

void java_free_methods(java_Methods *methods)
{
    size_t i;
    for(i = 0; i < methods->nb_methods; ++i)
    {
        size_t j;
        for(j = 0; j < methods->methods[i].nb_args; ++j)
            free(methods->methods[i].args[j]);
        free(methods->methods[i].args);
    }
    free(methods);
}
