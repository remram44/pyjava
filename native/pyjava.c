#include <Python.h>
#include <jni.h>
#include <stdio.h>

JNIEnv *penv = NULL;

/**
 * Starts a JVM using the invocation API.
 *
 * @param path Path to the JVM DLL, to be passed to JNI_CreateJavaVM().
 * @param options The option strings to pass to the JVM.
 * @param nbopts The number of option strings to be passed.
 */
static JNIEnv *start_jvm(const char *path, const char **opts, size_t nbopts)
{
    JavaVM *jvm = NULL;
    jint res;
    #ifdef JNI_VERSION_1_2
    {
        size_t i;
        JavaVMInitArgs vm_args;
        JavaVMOption *options = malloc(nbopts * sizeof(JavaVMOption));
        for(i = 0; i < nbopts; ++i)
            options[i].optionString = opts[i];
        vm_args.version = 0x00010002;
        vm_args.options = options;
        vm_args.nOptions = nbopts;
        vm_args.ignoreUnrecognized = JNI_TRUE;
        /* Create the Java VM */
        /* TODO : Load jvm.dll dynamically */
        printf("JNI_CreateJavaVM(<%d options>)\n", nbopts);
        res = JNI_CreateJavaVM(&jvm, (void**)&penv, &vm_args);
        free(options);
    }
    #else
        /* Unsupported */
        return NULL;
    #endif

    return (res >= 0)?penv:NULL;
}

static PyObject *pyjava_start(PyObject *self, PyObject *args)
{
    const char *path;
    PyObject *options;
    size_t size;
    const char **option_array;
    size_t i;

    printf("pyjava_start()\n");

    if(!(PyArg_ParseTuple(args, "sO!", &path, &PyList_Type, &options)))
        return NULL;

    printf("read parameters\n");

    size = PyList_GET_SIZE(options);
    printf("  size = %u\n", size);
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
        printf("  options[%d] = %s\n", i, option_array[i]);
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

static PyMethodDef methods[] = {
    {"start",  pyjava_start, METH_VARARGS, "Starts a Java Virtual Machine"},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_pyjava(void)
{
    PyObject *mod;

    mod = Py_InitModule("_pyjava", methods);
    if(mod == NULL)
        return ;
}
