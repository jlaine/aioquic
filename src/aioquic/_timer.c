#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <sys/timerfd.h>

#define MODULE_NAME "aioquic._timer"

typedef struct {
    PyObject_HEAD
    int fd;
} TimerObject;

static int
Timer_init(TimerObject *self, PyObject *args, PyObject *kwargs)
{
    self->fd = timerfd_create(CLOCK_MONOTONIC, 0);
    return 0;
}

static void
Timer_dealloc(TimerObject *self)
{
}

static PyObject*
Timer_file(TimerObject *self)
{
    return PyFile_FromFd(self->fd, "timer", "rb", -1, NULL, NULL, NULL, 0);
}

static PyObject*
Timer_settime(TimerObject *self, PyObject *args)
{
    double value;
    struct itimerspec spec;

    if (!PyArg_ParseTuple(args, "d", &value))
        return NULL;

    if (value < 0) {
        PyErr_SetString(PyExc_ValueError, "time must be positive or zero");
        return NULL;
    }

    spec.it_interval.tv_sec = 0;
    spec.it_interval.tv_nsec = 0;
    spec.it_value.tv_sec = value;
    spec.it_value.tv_nsec = (value - spec.it_value.tv_sec) * 1e9;

    if (timerfd_settime(self->fd, 0, &spec, NULL)) {
        PyErr_Format(PyExc_OSError, "timerfd_settime() failed: %d", errno);
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
Timer_time(TimerObject *self, PyObject *args)
{
    struct timespec spec;

    if (clock_gettime(CLOCK_MONOTONIC, &spec)) {
        PyErr_Format(PyExc_OSError, "clock_gettime() failed: %d", errno);
        return NULL;
    }

    return PyFloat_FromDouble((double)spec.tv_sec + (double)spec.tv_nsec / (double)1e9);
}

static PyMethodDef Timer_methods[] = {
    {"file", (PyCFunction)Timer_file, METH_VARARGS, ""},
    {"settime", (PyCFunction)Timer_settime, METH_VARARGS, ""},
    {"time", (PyCFunction)Timer_time, METH_VARARGS, ""},
    {NULL}
};

static PyTypeObject TimerType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    MODULE_NAME ".Timer",               /* tp_name */
    sizeof(TimerObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)Timer_dealloc,          /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash  */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    "Timer objects",                    /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    Timer_methods,                      /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    (initproc)Timer_init,               /* tp_init */
    0,                                  /* tp_alloc */
};


static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    MODULE_NAME,                        /* m_name */
    "A faster timer.",                  /* m_doc */
    -1,                                 /* m_size */
    NULL,                               /* m_methods */
    NULL,                               /* m_reload */
    NULL,                               /* m_traverse */
    NULL,                               /* m_clear */
    NULL,                               /* m_free */
};


PyMODINIT_FUNC
PyInit__timer(void)
{
    PyObject* m;

    m = PyModule_Create(&moduledef);
    if (m == NULL)
        return NULL;

    TimerType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&TimerType) < 0)
        return NULL;
    Py_INCREF(&TimerType);
    PyModule_AddObject(m, "Timer", (PyObject *)&TimerType);

    return m;
}
