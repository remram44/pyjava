#include <Python.h>
#include <jni.h>
#include <stdio.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "javawrapper.h"

JNIEnv *penv = NULL;

typedef jint (JNICALL *type_JNI_CreateJavaVM)(JavaVM**, void**, JavaVMInitArgs*);

PyObject *Err_Base;
PyObject *Err_ClassNotFound;

/**
 * Starts a JVM using the invocation API.
 *
 * The Java DLL is loaded dynamically from the specified path.
 *
 * @param path Path to the JVM DLL, to be passed to JNI_CreateJavaVM().
 * @param options The option strings to pass to the JVM.
 * @param nbopts The number of option strings to be passed.
 */
static JNIEnv *start_jvm(const char *path, const char **opts, size_t nbopts)
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

/**
 * _pyjava.start function: dynamically load a JVM DLL and start it.
 */
static PyObject *pyjava_start(PyObject *self, PyObject *args)
{
    const char *path;
    PyObject *options;
    size_t size;
    const char **option_array;
    size_t i;

    if(!(PyArg_ParseTuple(args, "sO!", &path, &PyList_Type, &options)))
        return NULL;

    size = PyList_GET_SIZE(options);
    option_array = malloc(size * sizeof(char *));
    for(i = 0; i < size; ++i)
    {
        PyObject *option = PyList_GET_ITEM(options, i);
        if(!PyString_Check(option))
        {
            PyErr_SetString(
                    PyExc_TypeError,
                    "Options list contained non-string objects.");
            free(option_array);
            return NULL;
        }

        option_array[i] = PyString_AS_STRING(option);
    }

    penv = start_jvm(path, option_array, size);
    free(option_array);

    if(penv != NULL)
    {
        Py_INCREF(Py_True);
        return Py_True;
    }
    else
    {
        Py_INCREF(Py_False);
        return Py_False;
    }
}

/**
 * _pyjava.getclass function: get a wrapper for a Java class.
 */
static PyObject *pyjava_getclass(PyObject *self, PyObject *args)
{
    const char *classname;
    jclass javaclass;

    if(!(PyArg_ParseTuple(args, "s", &classname)))
        return NULL;

    if(penv == NULL)
    {
        PyErr_SetString(
                Err_Base,
                "Java VM is not running.");
        return NULL;
    }

    javaclass = (*penv)->FindClass(penv, classname);
    if(javaclass == NULL)
    {
        PyErr_SetString(Err_ClassNotFound, classname);
        return NULL;
    }

    return javawrapper_build(javaclass);
}

static PyMethodDef methods[] = {
    {"start",  pyjava_start, METH_VARARGS,
    "start(bytestring, list) -> bool\n"
    "\n"
    "Starts a Java Virtual Machine. The first argument is the path of the\n"
    "library to be dynamically loaded. The second is a list of strings that\n"
    "are passed to the JVM as options."},
    {"getclass",  pyjava_getclass, METH_VARARGS,
    "getclass(str) -> JavaClass\n"
    "\n"
    "Find the desired class and returns a wrapper."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyjava(void)
{
    PyObject *mod;

    mod = Py_InitModule("_pyjava", methods);
    if(mod == NULL)
        return ;

    Err_Base = PyErr_NewException("pyjava.Error", NULL, NULL);
    Py_INCREF(Err_Base);
    PyModule_AddObject(mod, "Error", Err_Base);

    Err_ClassNotFound = PyErr_NewException("pyjava.ClassNotFound", Err_ClassNotFound, NULL);
    Py_INCREF(Err_ClassNotFound);
    PyModule_AddObject(mod, "ClassNotFound", Err_ClassNotFound);

    javawrapper_init(mod);
}
