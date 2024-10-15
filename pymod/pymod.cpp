#define PY_SSIZE_T_CLEAN
#include "Python.h"

// - include this file first because python needs to be included first
// - with the respective version do:
// INCLCUDES += -I/usr/include/python3.8
// LIBS      += -lpython3.8

#include "pymod.h"

#include "debug.h"
#include "misc_utils.h"

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

static PyMethodDef pymod_methods[] = {
    {"sset_cbk",  pymod_sset_cbk, METH_VARARGS, "Adds a callback with a context to a string trigger"},
    {"iset_cbk",  pymod_iset_cbk, METH_VARARGS, "Adds a callback with a context to a integer trigger"},
    {"unset_cbk",  pymod_unset_cbk, METH_VARARGS, "Removes a callback"},
    {"trigger_str", pymod_str_trigger, METH_VARARGS, "Forces the trigger of a string callback"},
    {"trigger_int", pymod_int_trigger, METH_VARARGS, "Forces the trigger of an integer callback"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef pymod_cfg = {
    PyModuleDef_HEAD_INIT,
    "pymod",        /* name of module */
    NULL,           /* module documentation, may be NULL */
    -1,             /* size of per-interpreter state of the module,
                    or -1 if the module keeps state in global variables. */
    pymod_methods
};

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

PyMODINIT_FUNC PyInit_pymod() {
    DBG_SCOPE();
    PyObject *m;

    m = PyModule_Create(&pymod_cfg);
    if (m == NULL)
        return NULL;

    pymod_error = PyErr_NewException("pymod.error", NULL, NULL);
    Py_INCREF(pymod_error);
    if (PyModule_AddObject(m, "error", pymod_error) < 0) {
        Py_XDECREF(pymod_error);
        Py_CLEAR(pymod_error);
        Py_DECREF(m);
        return NULL;
    }

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
