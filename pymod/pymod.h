#ifndef PYMOD_H
#define PYMOD_H

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#include <functional>
#include <memory>

/* TODO: Should I add mutexes in this implementation? */
struct pymod_cbk_t;
using pymod_cbk_wp = std::weak_ptr<pymod_cbk_t>;

/* Those can be called from the code to trigger the respective callback.

Int triggers can be also compatible with pointers and this code is meant to be part of a bigger
module, such that the python part will call other functions from those modules.
Str triggers can also be used in conjunction with jsons to pass the trigger information directly
to the c++ part and the reverse should also be possible */
int pymod_trigger_int(uint64_t trig, const std::string& strval, int64_t intval);
int pymod_trigger_str(const std::string &trig, const std::string& strval, int64_t intval);
int pymod_trigger_cbk(pymod_cbk_wp cbk, const std::string& strval, int64_t intval);

/* also a kind of callback, but this time those callbacks are meant to be used inside await stuff
inside python. Those awaitables will have 3 fields .intval, .strval, .usrval, where usrval can
be set iniside the constructor. */
PyObject *pymod_await_new(PyObject *ctx);
int pymod_await_trig(PyObject *aw, const std::string& strval, int64_t intval);

/* This is a function that must be provided by the implementer of this header and this function
will be called right after the python module is initialized. define PYMOD_NOINIT_FUNCTION to ignore
it */
int pymod_pre_init(std::vector<PyMethodDef> &methods, PyModuleDef *module_def);
int pymod_post_init(PyObject *m);
void pymod_uninit();

#ifdef PYMOD_NOINIT_FUNCTION
inline int pymod_pre_init(std::vector<PyMethodDef> &, PyModuleDef *) { return 0; }
inline int pymod_post_init() { return 0; }
inline void pymod_uninit() { return 0; }
#endif

/* those will be called by the python side whenever a callback is to be registered to the C++ side.
*/
inline std::function<void(uint64_t, pymod_cbk_wp)> pymod_register_int_cbk;
inline std::function<void(std::string, pymod_cbk_wp)> pymod_register_str_cbk;

inline std::function<void(pymod_cbk_wp)> pymod_unregister_int_cbk;
inline std::function<void(pymod_cbk_wp)> pymod_unregister_str_cbk;

/* 
On the Python side callbacks are registered as in the form (trigger, function, user_data)
- trigger is a descroptor of the callback context, it is a uint64_t or a const char *
- function is the callback that is to be called
- user_data is the user data associated with the callback 

The callback is called in the form (strval, intval, user_data) by the c++ side, intval and strval
are dependent on the callback trigger.
*/

#endif
