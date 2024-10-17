/*
 * PyAwaitable - Vendored copy of version 1.2.0
 * 
 * Docs: https://awaitable.zintensity.dev
 * Source: https://github.com/ZeroIntensity/pyawaitable
 */

#include "pyawaitable.h"

PyTypeObject _PyAwaitableGenWrapperType; // Forward declaration
/* Vendor of src/_pyawaitable/genwrapper.c */
#define DONE(cb)                 \
        do { cb->done = true;    \
             Py_CLEAR(cb->coro); \
             Py_CLEAR(g->gw_current_await); } while (0)
#define AW_DONE()               \
        do {                    \
            aw->aw_done = true; \
            Py_CLEAR(g->gw_aw); \
        } while (0)

static PyObject *
gen_new(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL)
    {
        return NULL;
    }

    GenWrapperObject *g = (GenWrapperObject *) self;
    g->gw_aw = NULL;
    g->gw_current_await = NULL;

    return (PyObject *) g;
}

static void
gen_dealloc(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *) self;
    if (g->gw_current_await != NULL)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "sanity check: gw_current_await was not cleared!"
        );
        PyErr_WriteUnraisable(self);
    }
    if (g->gw_aw != NULL)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "sanity check: gw_aw was not cleared!"
        );
        PyErr_WriteUnraisable(self);
    }
    Py_TYPE(self)->tp_free(self);
}

PyObject *
genwrapper_new(PyAwaitableObject *aw)
{
    assert(aw != NULL);
    GenWrapperObject *g = (GenWrapperObject *) gen_new(
        &_PyAwaitableGenWrapperType,
        NULL,
        NULL
    );

    if (!g)
        return NULL;

    g->gw_aw = (PyAwaitableObject *) Py_NewRef((PyObject *) aw);
    return (PyObject *) g;
}

int
genwrapper_fire_err_callback(
    PyObject *self,
    PyObject *await,
    pyawaitable_callback *cb
)
{
    assert(PyErr_Occurred() != NULL);
    if (!cb->err_callback)
    {
        cb->done = true;
        return -1;
    }

    PyObject *err = PyErr_GetRaisedException();

    Py_INCREF(self);
    int e_res = cb->err_callback(self, err);
    Py_DECREF(self);
    cb->done = true;

    if (e_res < 0)
    {
        // If the res is -1, the error is restored.
        // Otherwise, it is not.
        if (e_res == -1)
        {
            PyErr_SetRaisedException(err);
        } else
            Py_DECREF(err);

        return -1;
    }

    Py_DECREF(err);
    return 0;
}

PyObject *
genwrapper_next(PyObject *self)
{
    GenWrapperObject *g = (GenWrapperObject *)self;
    PyAwaitableObject *aw = g->gw_aw;

    if (!aw)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: genwrapper used after return"
        );
        return NULL;
    }

    pyawaitable_callback *cb;
    if (aw->aw_state == CALLBACK_ARRAY_SIZE)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: object cannot handle more than 255 coroutines"
        );
        AW_DONE();
        return NULL;
    }

    if (g->gw_current_await == NULL)
    {
        if (aw->aw_callbacks[aw->aw_state].coro == NULL)
        {
            PyErr_SetObject(
                PyExc_StopIteration,
                aw->aw_result ? aw->aw_result : Py_None
            );
            AW_DONE();
            return NULL;
        }

        cb = &aw->aw_callbacks[aw->aw_state++];

        if (
            Py_TYPE(cb->coro)->tp_as_async == NULL ||
            Py_TYPE(cb->coro)->tp_as_async->am_await == NULL
        )
        {
            PyErr_Format(
                PyExc_TypeError,
                "pyawaitable: %R is not awaitable",
                cb->coro
            );
            DONE(cb);
            AW_DONE();
            return NULL;
        }

        g->gw_current_await = Py_TYPE(cb->coro)->tp_as_async->am_await(
            cb->coro
        );
        if (g->gw_current_await == NULL)
        {
            if (
                genwrapper_fire_err_callback(
                    (PyObject *)aw,
                    g->gw_current_await,
                    cb
                ) < 0
            )
            {
                DONE(cb);
                AW_DONE();
                return NULL;
            }

            DONE(cb);
            return genwrapper_next(self);
        }
    } else
    {
        cb = &aw->aw_callbacks[aw->aw_state - 1];
    }

    PyObject *result = Py_TYPE(
        g->gw_current_await
    )->tp_iternext(g->gw_current_await);

    if (result != NULL)
    {
        return result;
    }

    PyObject *occurred = PyErr_Occurred();
    if (!occurred)
    {
        // Coro is done, no result.
        if (!cb->callback)
        {
            // No callback, skip that step.
            DONE(cb);
            return genwrapper_next(self);
        }
    }

    // TODO: I wonder if the occurred check is needed here.
    if (
        occurred && !PyErr_ExceptionMatches(PyExc_StopIteration)
    )
    {
        if (
            genwrapper_fire_err_callback(
                (PyObject *) aw,
                g->gw_current_await,
                cb
            ) < 0
        )
        {
            DONE(cb);
            AW_DONE();
            return NULL;
        }

        DONE(cb);
        return genwrapper_next(self);
    }

    if (cb->callback == NULL)
    {
        // Coroutine is done, but with a result.
        // We can disregard the result if theres no callback.
        DONE(cb);
        PyErr_Clear();
        return genwrapper_next(self);
    }

    PyObject *value;
    if (occurred)
    {
        value = PyErr_GetRaisedException();
        assert(value != NULL);
        assert(PyObject_IsInstance(value, PyExc_StopIteration));
        PyObject *tmp = PyObject_GetAttrString(value, "value");
        if (tmp == NULL)
        {
            Py_DECREF(value);
            DONE(cb);
            AW_DONE();
            return NULL;
        }
        Py_DECREF(value);
        value = tmp;
    } else
    {
        value = Py_NewRef(Py_None);
    }

    Py_INCREF(aw);
    int res = cb->callback((PyObject *) aw, value);
    Py_DECREF(aw);
    Py_DECREF(value);

    if (res < -1)
    {
        // -2 or lower denotes that the error should be deferred,
        // regardless of whether a handler is present.
        DONE(cb);
        AW_DONE();
        return NULL;
    }

    if (res < 0)
    {
        if (!PyErr_Occurred())
        {
            PyErr_SetString(
                PyExc_SystemError,
                "pyawaitable: callback returned -1 without exception set"
            );
            DONE(cb);
            AW_DONE();
            return NULL;
        }
        if (
            genwrapper_fire_err_callback(
                (PyObject *) aw,
                g->gw_current_await,
                cb
            ) < 0
        )
        {
            DONE(cb);
            AW_DONE();
            return NULL;
        }
    }

    DONE(cb);
    return genwrapper_next(self);
}

PyTypeObject _PyAwaitableGenWrapperType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_genwrapper",
    .tp_basicsize = sizeof(GenWrapperObject),
    .tp_dealloc = gen_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = genwrapper_next,
    .tp_new = gen_new,
};
/* Vendor of src/_pyawaitable/coro.c */

static PyObject *
awaitable_send_with_arg(PyObject *self, PyObject *value)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    if (aw->aw_gen == NULL)
    {
        PyObject *gen = awaitable_next(self);
        if (gen == NULL)
            return NULL;

        Py_DECREF(gen);
        Py_RETURN_NONE;
    }

    return genwrapper_next(aw->aw_gen);
}

static PyObject *
awaitable_send(PyObject *self, PyObject *args)
{
    PyObject *value;

    if (!PyArg_ParseTuple(args, "O", &value))
        return NULL;

    return awaitable_send_with_arg(self, value);
}

static PyObject *
awaitable_close(PyObject *self, PyObject *args)
{
    pyawaitable_cancel(self);
    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    aw->aw_done = true;
    Py_RETURN_NONE;
}

static PyObject *
awaitable_throw(PyObject *self, PyObject *args)
{
    PyObject *type;
    PyObject *value = NULL;
    PyObject *traceback = NULL;

    if (!PyArg_ParseTuple(args, "O|OO", &type, &value, &traceback))
        return NULL;

    if (PyType_Check(type))
    {
        PyObject *err = PyObject_Vectorcall(
            type,
            (PyObject *[]){value},
            1,
            NULL
        );
        if (err == NULL)
        {
            return NULL;
        }

        if (traceback)
            if (PyException_SetTraceback(err, traceback) < 0)
            {
                Py_DECREF(err);
                return NULL;
            }

        PyErr_Restore(err, NULL, NULL);
    } else
        PyErr_Restore(
            Py_NewRef(type),
            Py_XNewRef(value),
            Py_XNewRef(traceback)
        );

    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    if ((aw->aw_gen != NULL) && (aw->aw_state != 0))
    {
        GenWrapperObject *gw = (GenWrapperObject *)aw->aw_gen;
        pyawaitable_callback *cb = &aw->aw_callbacks[aw->aw_state - 1];
        if (cb == NULL)
            return NULL;

        if (genwrapper_fire_err_callback(self, gw->gw_current_await, cb) < 0)
            return NULL;
    } else
        return NULL;

    assert(NULL);
}

#if PY_MINOR_VERSION > 9
static PySendResult
awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult)
{
    PyObject *send_res = awaitable_send_with_arg(self, arg);
    if (send_res == NULL)
    {
        if (PyErr_ExceptionMatches(PyExc_StopIteration))
        {
            PyObject *occurred = PyErr_GetRaisedException();
            PyObject *item = PyObject_GetAttrString(occurred, "value");
            Py_DECREF(occurred);

            if (item == NULL)
            {
                return PYGEN_ERROR;
            }

            *presult = item;
            return PYGEN_RETURN;
        }
        *presult = NULL;
        return PYGEN_ERROR;
    }
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    *presult = send_res;

    return PYGEN_NEXT;
}

#endif

PyMethodDef pyawaitable_methods[] =
{
    {"send", awaitable_send, METH_VARARGS, NULL},
    {"close", awaitable_close, METH_VARARGS, NULL},
    {"throw", awaitable_throw, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

PyAsyncMethods pyawaitable_async_methods =
{
#if PY_MINOR_VERSION > 9
    .am_await = awaitable_next,
    .am_send = awaitable_am_send
#else
    .am_await = awaitable_next
#endif
};
/* Vendor of src/_pyawaitable/values.c */
#define UNPACK(arr, tp, err, index)                                  \
        do {                                                         \
            assert(awaitable != NULL);                               \
            PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
            Py_INCREF(awaitable);                                    \
            if (index == 0) {                                        \
                PyErr_SetString(                                     \
    PyExc_ValueError,                                                \
    "pyawaitable: awaitable object has no stored " err               \
                );                                                   \
                Py_DECREF(awaitable);                                \
                return -1;                                           \
            }                                                        \
            va_list args;                                            \
            va_start(args, awaitable);                               \
            for (Py_ssize_t i = 0; i < index; ++i) {                 \
                tp ptr = va_arg(args, tp);                           \
                if (ptr == NULL)                                     \
                continue;                                            \
                *ptr = arr[i];                                       \
            }                                                        \
            va_end(args);                                            \
            Py_DECREF(awaitable);                                    \
            return 0;                                                \
        } while (0)

#define SAVE_ERR(err)                                     \
        "pyawaitable: " err " array has a capacity of 32" \
        ", so storing %ld more would overflow it"         \

#define SAVE(arr, index, tp, err, wrap)                              \
        do {                                                         \
            assert(awaitable != NULL);                               \
            assert(nargs != 0);                                      \
            Py_INCREF(awaitable);                                    \
            PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
            Py_ssize_t final_size = index + nargs;                   \
            if (final_size >= VALUE_ARRAY_SIZE) {                    \
                PyErr_Format(                                        \
    PyExc_SystemError,                                               \
    SAVE_ERR(err),                                                   \
    final_size                                                       \
                );                                                   \
                return -1;                                           \
            }                                                        \
            va_list vargs;                                           \
            va_start(vargs, nargs);                                  \
            for (Py_ssize_t i = index; i < final_size; ++i) {        \
                arr[i] = wrap(va_arg(vargs, tp));                    \
            }                                                        \
            index += nargs;                                          \
            va_end(vargs);                                           \
            Py_DECREF(awaitable);                                    \
            return 0;                                                \
        } while (0)

#define INDEX_HEAD(arr, idx, ret)                                \
        PyAwaitableObject *aw = (PyAwaitableObject *) awaitable; \
        if ((index >= idx) || (index < 0)) {                     \
            PyErr_Format(                                        \
    PyExc_IndexError,                                            \
    "pyawaitable: index %ld out of range for %ld stored values", \
    index,                                                       \
    idx                                                          \
            );                                                   \
            return ret;                                          \
        }


#define NOTHING

/* Normal Values */

int
pyawaitable_unpack(PyObject *awaitable, ...)
{
    UNPACK(aw->aw_values, PyObject * *, "values", aw->aw_values_index);
}

int
pyawaitable_save(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(aw->aw_values, aw->aw_values_index, PyObject *, "values", Py_NewRef);
}

int
pyawaitable_set(
    PyObject *awaitable,
    Py_ssize_t index,
    PyObject *new_value
)
{
    INDEX_HEAD(aw->aw_values, aw->aw_values_index, -1);
    Py_SETREF(aw->aw_values[index], Py_NewRef(new_value));
    return 0;
}

PyObject *
pyawaitable_get(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    INDEX_HEAD(aw->aw_values, aw->aw_values_index, NULL);
    return aw->aw_values[index];
}

/* Arbitrary Values */

int
pyawaitable_unpack_arb(PyObject *awaitable, ...)
{
    UNPACK(
        aw->aw_arb_values,
        void **,
        "arbitrary values",
        aw->aw_arb_values_index
    );
}

int
pyawaitable_save_arb(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(
        aw->aw_arb_values,
        aw->aw_arb_values_index,
        void *,
        "arbitrary values",
        NOTHING
    );
}

int
pyawaitable_set_arb(
    PyObject *awaitable,
    Py_ssize_t index,
    void *new_value
)
{
    INDEX_HEAD(aw->aw_arb_values, aw->aw_arb_values_index, -1);
    aw->aw_arb_values[index] = new_value;
    return 0;
}

void *
pyawaitable_get_arb(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    INDEX_HEAD(aw->aw_arb_values, aw->aw_arb_values_index, NULL);
    return aw->aw_arb_values[index];
}

/* Integer Values */

int
pyawaitable_unpack_int(PyObject *awaitable, ...)
{
    UNPACK(
        aw->aw_int_values,
        long *,
        "integer values",
        aw->aw_int_values_index
    );
}

int
pyawaitable_save_int(PyObject *awaitable, Py_ssize_t nargs, ...)
{
    SAVE(
        aw->aw_int_values,
        aw->aw_int_values_index,
        long,
        "integer values",
        NOTHING
    );
}

int
pyawaitable_set_int(
    PyObject *awaitable,
    Py_ssize_t index,
    long new_value
)
{
    INDEX_HEAD(aw->aw_int_values, aw->aw_int_values_index, -1);
    aw->aw_int_values[index] = new_value;
    return 0;
}

long
pyawaitable_get_int(
    PyObject *awaitable,
    Py_ssize_t index
)
{
    INDEX_HEAD(aw->aw_int_values, aw->aw_int_values_index, -1);
    return aw->aw_int_values[index];
}
/* Vendor of src/_pyawaitable/awaitable.c */
#define AWAITABLE_POOL_SIZE 256

PyDoc_STRVAR(
    awaitable_doc,
    "Awaitable transport utility for the C API."
);

static Py_ssize_t pool_index = 0;
static PyObject *pool[AWAITABLE_POOL_SIZE];

static PyObject *
awaitable_new_func(PyTypeObject *tp, PyObject *args, PyObject *kwds)
{
    assert(tp != NULL);
    assert(tp->tp_alloc != NULL);

    PyObject *self = tp->tp_alloc(tp, 0);
    if (self == NULL)
    {
        return NULL;
    }

    PyAwaitableObject *aw = (PyAwaitableObject *) self;
    aw->aw_awaited = false;
    aw->aw_done = false;
    aw->aw_used = false;

    return (PyObject *) aw;
}

PyObject *
awaitable_next(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    if (aw->aw_awaited)
    {
        PyErr_SetString(
            PyExc_RuntimeError,
            "pyawaitable: cannot reuse awaitable"
        );
        return NULL;
    }
    aw->aw_awaited = true;
    PyObject *gen = genwrapper_new(aw);
    aw->aw_gen = Py_XNewRef(gen);
    return gen;
}

static void
awaitable_dealloc(PyObject *self)
{
    PyAwaitableObject *aw = (PyAwaitableObject *)self;
    for (Py_ssize_t i = 0; i < aw->aw_values_index; ++i)
    {
        if (!aw->aw_values[i])
            break;
        Py_DECREF(aw->aw_values[i]);
    }

    Py_XDECREF(aw->aw_gen);
    Py_XDECREF(aw->aw_result);

    for (int i = 0; i < CALLBACK_ARRAY_SIZE; ++i)
    {
        pyawaitable_callback *cb = &aw->aw_callbacks[i];
        if (cb == NULL)
            break;

        if (cb->done)
        {
            if (cb->coro != NULL)
            {
                PyErr_SetString(
                    PyExc_SystemError,
                    "sanity check: coro was not cleared"
                );
                PyErr_WriteUnraisable(self);
            }
        } else
            Py_XDECREF(cb->coro);
    }

    if (!aw->aw_done && aw->aw_used)
    {
        if (
            PyErr_WarnEx(
                PyExc_RuntimeWarning,
                "pyawaitable object was never awaited",
                1
            ) < 0
        )
        {
            PyErr_WriteUnraisable(self);
        }
    }

    Py_TYPE(self)->tp_free(self);
}

void
pyawaitable_cancel(PyObject *aw)
{
    assert(aw != NULL);
    PyAwaitableObject *a = (PyAwaitableObject *) aw;

    for (int i = 0; i < CALLBACK_ARRAY_SIZE; ++i)
    {
        pyawaitable_callback *cb = &a->aw_callbacks[i];
        if (!cb)
            break;

        // Reset the callback
        Py_CLEAR(cb->coro);
        cb->done = false;
        cb->callback = NULL;
        cb->err_callback = NULL;
    }
}

int
pyawaitable_await(
    PyObject *aw,
    PyObject *coro,
    awaitcallback cb,
    awaitcallback_err err
)
{
    PyAwaitableObject *a = (PyAwaitableObject *) aw;
    if (a->aw_callback_index == CALLBACK_ARRAY_SIZE)
    {
        PyErr_SetString(
            PyExc_SystemError,
            "pyawaitable: awaitable object cannot store more than 128 coroutines"
        );
        return -1;
    }

    pyawaitable_callback *aw_c = &a->aw_callbacks[a->aw_callback_index++];
    aw_c->coro = Py_NewRef(coro);
    aw_c->callback = cb;
    aw_c->err_callback = err;
    aw_c->done = false;

    return 0;
}

int
pyawaitable_set_result(PyObject *awaitable, PyObject *result)
{
    PyAwaitableObject *aw = (PyAwaitableObject *) awaitable;
    aw->aw_result = Py_NewRef(result);
    return 0;
}

PyObject *
pyawaitable_new(void)
{
    if (pool_index == AWAITABLE_POOL_SIZE)
    {
        PyObject *aw = awaitable_new_func(&_PyAwaitableType, NULL, NULL);
        ((PyAwaitableObject *) aw)->aw_used = true;
        return aw;
    }

    PyObject *pool_obj = pool[pool_index++];
    ((PyAwaitableObject *) pool_obj)->aw_used = true;
    return pool_obj;
}

void
dealloc_awaitable_pool(void)
{
    for (Py_ssize_t i = pool_index; i < AWAITABLE_POOL_SIZE; ++i)
    {
        if (Py_REFCNT(pool[i]) != 1)
        {
            PyErr_Format(
                PyExc_SystemError,
                "expected %R to have a reference count of 1",
                pool[i]
            );
            PyErr_WriteUnraisable(NULL);
        }
        Py_DECREF(pool[i]);
    }
}

int
alloc_awaitable_pool(void)
{
    for (Py_ssize_t i = 0; i < AWAITABLE_POOL_SIZE; ++i)
    {
        pool[i] = awaitable_new_func(&_PyAwaitableType, NULL, NULL);
        if (!pool[i])
        {
            for (Py_ssize_t x = 0; x < i; ++x)
                Py_DECREF(pool[x]);
            return -1;
        }
    }

    return 0;
}

int
pyawaitable_await_function(
    PyObject *awaitable,
    PyObject *func,
    const char *fmt,
    awaitcallback cb,
    awaitcallback_err err,
    ...
)
{
    size_t len = strlen(fmt);
    size_t size = len + 3;
    char *tup_format = PyMem_Malloc(size);
    if (!tup_format)
    {
        PyErr_NoMemory();
        return -1;
    }

    tup_format[0] = '(';
    for (size_t i = 0; i < len; ++i)
    {
        tup_format[i + 1] = fmt[i];
    }

    tup_format[size - 2] = ')';
    tup_format[size - 1] = '\0';

    va_list vargs;
    va_start(vargs, err);
    PyObject *args = Py_VaBuildValue(tup_format, vargs);
    va_end(vargs);
    PyMem_Free(tup_format);

    if (!args)
        return -1;
    PyObject *coro = PyObject_Call(func, args, NULL);
    Py_DECREF(args);

    if (!coro)
        return -1;

    if (pyawaitable_await(awaitable, coro, cb, err) < 0)
    {
        Py_DECREF(coro);
        return -1;
    }

    Py_DECREF(coro);
    return 0;
}

PyTypeObject _PyAwaitableType =
{
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_PyAwaitableType",
    .tp_basicsize = sizeof(PyAwaitableObject),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &pyawaitable_async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_doc = awaitable_doc,
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new_func,
    .tp_methods = pyawaitable_methods
};
/* Vendor of src/_pyawaitable/backport.c */

#ifdef PYAWAITABLE_NEEDS_VECTORCALL
PyObject *
_PyObject_VectorcallBackport(
    PyObject *obj,
    PyObject **args,
    size_t nargsf,
    PyObject *kwargs
)
{
    PyObject *tuple = PyTuple_New(nargsf);
    if (!tuple)
        return NULL;
    for (size_t i = 0; i < nargsf; i++)
    {
        Py_INCREF(args[i]);
        PyTuple_SET_ITEM(tuple, i, args[i]);
    }
    PyObject *o = PyObject_Call(obj, tuple, kwargs);
    Py_DECREF(tuple);
    return o;
}

#endif

#if PY_VERSION_HEX < 0x030c0000
PyObject *
PyErr_GetRaisedException(void)
{
    PyObject *type, *val, *tb;
    PyErr_Fetch(&type, &val, &tb);
    PyErr_NormalizeException(&type, &val, &tb);
    Py_XDECREF(type);
    Py_XDECREF(tb);
    // technically some entry in the traceback might be lost; ignore that
    return val;
}

void
PyErr_SetRaisedException(PyObject *err)
{
    // NOTE: We need to incref the type object here, even though
    // this function steals a reference to err.
    PyErr_Restore(Py_NewRef((PyObject *) Py_TYPE(err)), err, NULL);
}

#endif

#ifdef PYAWAITABLE_NEEDS_NEWREF
PyObject *
Py_NewRef_Backport(PyObject *o)
{
    Py_INCREF(o);
    return o;
}

#endif

#ifdef PYAWAITABLE_NEEDS_XNEWREF
PyObject *
Py_XNewRef_Backport(PyObject *o)
{
    Py_XINCREF(o);
    return o;
}

#endif
