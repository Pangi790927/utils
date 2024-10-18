// - include this file first because python needs to be included first
// - with the respective version do:
// INCLCUDES += -I/usr/include/python3.8
// LIBS      += -lpython3.8

#include "pymod.h"

#include "debug.h"
#include "misc_utils.h"
#include "path_utils.h"

#include <map>

enum pymod_trig_e : int {
    PYMOD_TRIG_TYPE_INT,
    PYMOD_TRIG_TYPE_STR,
};

struct pymod_cbk_t {
    PyObject        *cbk;
    PyObject        *ctx;

    pymod_trig_e    trig_type;
    uint64_t        int_trig;
    std::string     str_trig;
};

using pymod_cbk_p = std::shared_ptr<pymod_cbk_t>;

static PyObject *pymod_sset_cbk(PyObject *self, PyObject *args);
static PyObject *pymod_iset_cbk(PyObject *self, PyObject *args);
static PyObject *pymod_unset_cbk(PyObject *self, PyObject *args);
static PyObject *pymod_str_trigger(PyObject *self, PyObject *args);
static PyObject *pymod_int_trigger(PyObject *self, PyObject *args);

static std::map<uint64_t, pymod_cbk_p> int_cbks;
static std::map<std::string, pymod_cbk_p> str_cbks;
static std::map<PyObject *, pymod_cbk_wp> self_cbks;

static PyObject *pymod_error;
static std::vector<PyMethodDef> pymod_methods;
static struct PyModuleDef pymod_cfg = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "pymod",          /* name of module */
    .m_doc = NULL,              /* module documentation, may be NULL */
    .m_size = -1,               /* size of per-interpreter state of the module,
                                or -1 if the module keeps state in global variables. */
    .m_methods = NULL           /* will be filled later */
};

/* EXPERIMENT -- AWAITABLES -- !! FAILED EXPERIMENT !! -> try using the future from asyncio?
================================================================================================= */

static PyObject *aio;
static PyObject *aio_future_class;

static std::mutex future_mu;
static std::map<PyObject *, PyObject *> future_ctx;

static void print_object(PyObject *obj) {
    if (PyObject_Print(obj, stdout, 0) < 0) {
        DBG("Failed to print");
        return ;
    }
    printf("\n");
}


struct pymod_awaitable_internal_t {
    std::string strval;
    int64_t intval;
};

struct pymod_awaitable_t {
    pymod_awaitable_internal_t *p;
};

static PyObject *awaitable_await(PyObject *self);
static PyObject *awaitable_new(PyTypeObject *tp, PyObject *args, PyObject *kwds);
static PyObject *awaitable_next(PyObject *self);
static void awaitable_dealloc(PyObject *self);

#if PY_MINOR_VERSION > 9
static PySendResult awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult);
#endif /* PY_MINOR_VERSION > 9 */

static PyAsyncMethods pymod_async_methods = {
#if PY_MINOR_VERSION > 9
    .am_await = awaitable_await,
    .am_send = awaitable_am_send
#else
    .am_await = awaitable_await
#endif /* PY_MINOR_VERSION > 9 */
};

static PyTypeObject pymod_awaitable_type =
{
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "pymod.awaitable",
    .tp_basicsize = sizeof(pymod_awaitable_t),
    .tp_dealloc = awaitable_dealloc,
    .tp_as_async = &pymod_async_methods,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = awaitable_next,
    .tp_new = awaitable_new,
};

static void awaitable_dealloc(PyObject *self) {
    DBG_SCOPE();
    pymod_awaitable_t *aw = (pymod_awaitable_t *) self;
    delete aw->p;
    Py_TYPE(self)->tp_free(self);
}

static PyObject *awaitable_new(PyTypeObject *tp, PyObject *args, PyObject *kwds) {
    DBG_SCOPE();
    if (!tp || !tp->tp_alloc) {
        DBG("Invalid object descriptor");
        return NULL;
    }

    PyObject *ret = tp->tp_alloc(tp, 0);
    if (ret == NULL) {
        DBG("Failed allocation");
        return NULL;
    }

    pymod_awaitable_t *aw = (pymod_awaitable_t *) ret;
    aw->p = new pymod_awaitable_internal_t;

    if (args) {
        DBG("TODO: Constructor needs implementation");
        /* TODO: parse args and take from them intval, strval, usrval */
    }

    return ret;
}

static PyObject *awaitable_next(PyObject *self) {
    DBG_SCOPE();
    return self;
}

#if PY_MINOR_VERSION > 9
static PySendResult awaitable_am_send(PyObject *self, PyObject *arg, PyObject **presult) {
    return NULL;
}
#endif /*PY_MINOR_VERSION > 9*/

static PyObject *awaitable_await(PyObject *self) {
    DBG_SCOPE();
    return self;
}

PyObject *pymod_await_new(PyObject *ctx) {
    DBG_SCOPE();
    PyObject* args = Py_BuildValue("()");
    auto ret = PyObject_Call(aio_future_class, args, NULL);
    Py_INCREF(ret);
    Py_XINCREF(ctx);
    {
        std::lock_guard guard(future_mu);
        if (HAS(future_ctx, ret)) {
            DBG("This is absurd");
            return NULL;
        }
        future_ctx[ret] = ctx;
    }
    print_object(ret);
    return ret;
}

int pymod_await_trig(PyObject *future, const std::string& strval, int64_t intval) {
    DBG_SCOPE();
    PyObject *ctx;
    {
        std::lock_guard guard(future_mu);
        if (!HAS(future_ctx, future)) {
            DBG("You are not allowed to trigger a future twice...");
            return -1;
        }
        ctx = future_ctx[future];
        future_ctx.erase(future);
    }

    PyObject *tup;
    if (ctx) {
        tup = Py_BuildValue("(sLO)", strval.c_str(), intval, ctx);
    }
    else {
        tup = Py_BuildValue("(sL)", strval.c_str(), intval);
    }
    if (!tup) {
        DBG("Failed to create future result");
        return -1;
    }
    // self.loop.call_soon_threadsafe(self.futures[k].set_result, v)
    PyObject *loop;
    if (!(loop = PyObject_CallMethod(future, "get_loop", "()"))) {
        DBG("Failed to get futre loop");
        return -1;
    }
    DBG("Got the loop")
    print_object(loop);
    PyObject *set_result;
    if (!(set_result = PyObject_GetAttrString(future, "set_result"))) {
        DBG("Failed to get the set_result function");
        return -1;
    }
    DBG("Got the set_result")
    print_object(set_result);
    if (!PyObject_CallMethod(loop, "call_soon_threadsafe", "(OO)", set_result, tup)) {
        DBG("Failed to schedule awake future");
        return -1;
    }
    DBG("Scheduled the awaker");
    Py_XDECREF(ctx);
    Py_DECREF(future);
    return 0;
}

static int init_awaitable_type(PyObject *m, PyTypeObject *obj_type, const std::string &type_name) {
    DBG_SCOPE();
    // Py_INCREF(obj_type);
    // if (PyType_Ready(obj_type) < 0) {
    //     Py_DECREF(obj_type);
    //     return -1;
    // }
    // if (PyModule_AddObject(m, type_name.c_str(), (PyObject *)obj_type) < 0) {
    //     Py_DECREF(obj_type);
    //     return -1;
    // }

    if (!(aio = PyImport_ImportModule("asyncio"))) {
        DBG("Failed to load asyncio module");
        return -1;
    }
    Py_INCREF(aio);
    if (!(aio_future_class = PyObject_GetAttrString(aio, "Future"))) {
        DBG("Failed to get the future class");
        return -1;
    }
    Py_INCREF(aio_future_class);
    return 0;
}

/* DONE EXPERIMENT
================================================================================================= */

static void pymod_deleter(pymod_cbk_t *ptr) {
    Py_XDECREF(ptr->cbk);
    Py_XDECREF(ptr->ctx);
    delete ptr;
}

int pymod_trigger_int(uint64_t trig, const std::string& strval, int64_t intval) {
    if (!HAS(int_cbks, trig)) {
        DBG("trigger not known %ld", trig);
        return -1;
    }
    auto cbk = int_cbks[trig];
    auto tup = Py_BuildValue("(sLO)", strval.c_str(), intval, cbk->ctx);
    if (!PyObject_CallObject(cbk->cbk, tup)) {
        DBG("Failed to call object")
        return -1;
    }
    return 0;
}

int pymod_trigger_str(const std::string &trig, const std::string& strval, int64_t intval) {
    /* TODO: do this (and make it work with the trigger bellow)*/
    if (!HAS(str_cbks, trig)) {
        DBG("trigger not known [%s]", trig.c_str());
        return -1;
    }
    auto cbk = str_cbks[trig];
    auto tup = Py_BuildValue("(sLO)", strval.c_str(), intval, cbk->ctx);
    if (!PyObject_CallObject(cbk->cbk, tup)) {
        DBG("Failed to call object")
        return -1;
    }
    return 0;
}

int pymod_trigger_cbk(pymod_cbk_wp _cbk, const std::string& s, int64_t intval) {
    /* TODO: do this (and make it work with the trigger bellow)*/
    pymod_cbk_p cbk = _cbk.lock();
    if (!cbk) {
        DBG("Callback no longer exists");
        return -1;
    }
    auto tup = Py_BuildValue("(sLO)", s.c_str(), intval, cbk->ctx);
    if (!PyObject_CallObject(cbk->cbk, tup)) {
        DBG("Failed to call object")
        return -1;
    }
    return 0;
}

PyMODINIT_FUNC PYMOD_MODULE_NAME () {
    DBG_SCOPE();

    PyObject *m;
    FnScope scope;

    /* call client before anything is initialized */
    if (pymod_pre_init(pymod_methods, &pymod_cfg) < 0) {
        DBG("Implementer pre-init returned error");
        return NULL;
    }

    /* add own functions to the function list */
    pymod_methods.push_back(PyMethodDef{"sset_cbk", pymod_sset_cbk, METH_VARARGS,
            "Adds a callback with a context to a string trigger"});
    pymod_methods.push_back(PyMethodDef{"iset_cbk", pymod_iset_cbk, METH_VARARGS,
            "Adds a callback with a context to a integer trigger"});
    pymod_methods.push_back(PyMethodDef{"unset_cbk", pymod_unset_cbk, METH_VARARGS,
            "Removes a callback"});
    pymod_methods.push_back(PyMethodDef{"trigger_str", pymod_str_trigger, METH_VARARGS,
            "Forces the trigger of a string callback"});
    pymod_methods.push_back(PyMethodDef{"trigger_int", pymod_int_trigger, METH_VARARGS,
            "Forces the trigger of an integer callback"});
    pymod_methods.push_back(PyMethodDef{NULL, NULL, 0, NULL}); /* Sentinel */

    /* initialize the module object */
    pymod_cfg.m_methods = pymod_methods.data();
    m = PyModule_Create(&pymod_cfg);
    if (m == NULL) {
        DBG("Failed to create the module");
        return NULL;
    }
    scope([&m]{ Py_DECREF(m); });

    /* adding own error type */
    pymod_error = PyErr_NewException("pymod.error", NULL, NULL);
    Py_INCREF(pymod_error);
    scope([]{
        Py_XDECREF(pymod_error);
        Py_CLEAR(pymod_error);
    });
    if (PyModule_AddObject(m, "error", pymod_error) < 0) {
        DBG("Failed to add error object");
        return NULL;
    }

    /* adding own awaitable type */
    if (init_awaitable_type(m, &pymod_awaitable_type, "awaitable") < 0) {
        DBG("Failed to register awaitable type");
        return NULL;
    }

    /* call client once everything is initialized */
    if (pymod_post_init(m) < 0) {
        DBG("Implementer post-init returned error");
        return NULL;
    }

    scope.disable();
    return m;
}

static PyObject *pymod_sset_cbk(PyObject *self, PyObject *args) {
    DBG_SCOPE();
    const char *trig;
    PyObject *cbk;
    PyObject *ctx;

    if (!PyArg_ParseTuple(args, "sOO", &trig, &cbk, &ctx)) {
        DBG("Failed parse args");
        return NULL;
    }
    if (!PyCallable_Check(cbk)) {
        DBG("Invalid callback object");
        PyErr_SetString(PyExc_TypeError, "second parameter must be callable");
        return NULL;
    }
    DBG("Setting cbk for [%s] as %p(%p)", trig, cbk, ctx);
    auto ptr = pymod_cbk_p(new pymod_cbk_t {
        .cbk = cbk,
        .ctx = ctx,
        .trig_type = PYMOD_TRIG_TYPE_STR,
        .int_trig = -1ULL,
        .str_trig = trig,
    }, pymod_deleter);
    if (!ptr) {
        DBG("Failed to create callback");
        return NULL;
    }
    Py_XINCREF(ptr->cbk);         /* Add a reference to new callback */
    Py_XINCREF(ptr->ctx);         /* Add a reference to new callback */
    str_cbks[trig] = ptr;
    self_cbks[ptr->cbk] = ptr;
    if (pymod_register_str_cbk)
        pymod_register_str_cbk(trig, ptr);

    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *pymod_iset_cbk(PyObject *self, PyObject *args) {
    DBG_SCOPE();
    uint64_t trig;
    PyObject *cbk;
    PyObject *ctx;

    if (!PyArg_ParseTuple(args, "KOO", &trig, &cbk, &ctx)) {
        DBG("Failed parse args");
        return NULL;
    }
    if (!PyCallable_Check(cbk)) {
        DBG("Not a callable object...");
        PyErr_SetString(PyExc_TypeError, "second parameter must be callable");
        return NULL;
    }
    DBG("Setting cbk for %ld as %p(%p)", trig, cbk, ctx);
    auto ptr = pymod_cbk_p(new pymod_cbk_t {
        .cbk = cbk,
        .ctx = ctx,
        .trig_type = PYMOD_TRIG_TYPE_INT,
        .int_trig = trig,
        .str_trig = "",
    }, pymod_deleter);
    if (!ptr) {
        DBG("The callback couldn't be created...");
        return NULL;
    }
    Py_XINCREF(ptr->cbk);         /* Add a reference to new callback */
    Py_XINCREF(ptr->ctx);         /* Add a reference to new callback */
    int_cbks[trig] = ptr;
    self_cbks[ptr->cbk] = ptr;
    if (pymod_register_int_cbk)
        pymod_register_int_cbk(trig, ptr);

    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *pymod_int_trigger(PyObject *self, PyObject *args) {
    DBG_SCOPE();
    uint64_t target = 0;
    int64_t int_value = 0;
    const char *str_value = 0;

    if (!PyArg_ParseTuple(args, "KsL", &target, &str_value, &int_value)) {
        DBG("Failed to parse trigger params");
        return NULL;
    }
    if (!HAS(int_cbks, target)) {
        DBG("The target does not exist: %ld", target);
        return NULL;
    }
    DBG("Triggering: %ld function: %p(%p)",
            target, int_cbks[target]->cbk, int_cbks[target]->ctx);
    auto tup = Py_BuildValue("(sLO)", str_value, int_value, int_cbks[target]->ctx);
    if (!PyObject_CallObject(int_cbks[target]->cbk, tup)) {
        DBG("Failed to call object")
        return NULL;
    }

    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *pymod_str_trigger(PyObject *self, PyObject *args) {
    DBG_SCOPE();
    const char *target;
    int64_t int_value;
    const char *str_value;

    if (!PyArg_ParseTuple(args, "ssL", &target, &str_value, &int_value)) {
        DBG("Failed to parse arsgs");
        return NULL;
    }
    if (!HAS(str_cbks, target)) {
        DBG("The target does not exist: [%s]", target);
        return NULL;
    }
    DBG("Triggering: %s function: %p(%p)",
            target, str_cbks[target]->cbk, str_cbks[target]->ctx);
    auto tup = Py_BuildValue("(sLO)", str_value, int_value, str_cbks[target]->ctx);
    if (!PyObject_CallObject(str_cbks[target]->cbk, tup)) {
        DBG("Failed to call object")
        return NULL;
    }

    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *pymod_unset_cbk(PyObject *self, PyObject *args) {
    DBG_SCOPE();
    uint64_t trig_i;
    std::string trig_s;
    PyObject *cbk;
    PyObject *ctx;

    if (!PyArg_ParseTuple(args, "O", &cbk)) {
        DBG("Failed to parse unset");
        return NULL;
    }

    DBG("unsetting_cbk: %p", cbk);

    if (!HAS(self_cbks, cbk)) {
        DBG("Callback %p is not known, can't unset", cbk);
        return NULL;
    }
    if (auto ptr = self_cbks[cbk].lock()) {
        self_cbks.erase(cbk);
        if (ptr->trig_type == PYMOD_TRIG_TYPE_INT) {
            DBG("trig: %ld", ptr->int_trig);
            int_cbks.erase(ptr->int_trig);
            if (pymod_unregister_int_cbk)
                pymod_unregister_int_cbk(ptr);
        }
        else {
            DBG("trig: [%s]", ptr->str_trig.c_str());
            str_cbks.erase(ptr->str_trig);
            if (pymod_unregister_int_cbk)
                pymod_unregister_int_cbk(ptr);
        }
    }
    else {
        DBG("huh?");
    }

    /* Boilerplate to return "None" */
    Py_INCREF(Py_None);
    return Py_None;
}
