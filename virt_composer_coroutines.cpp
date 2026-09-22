/*! @file
 * Implements the parts of virt_composer_coroutines.h whose bodies need the virt-state, and
 * registers what this component brings onto a state and onto the `vc` Lua table.
 *
 * The state is reached only through the accessors virt_composer.h publishes, so this file compiles
 * without seeing `virt_state_t`'s members. That is what lets it be a translation unit of its own
 * while `virt_state_t` is still private to virt_composer.cpp.
 *
 * LUA_IMPL is not defined here: virt_composer.cpp is the one translation unit that carries the Lua
 * implementation, and a second copy would be a second set of symbols.
 *
 * @date 2026-09-21 20:49
 */

#include "virt_composer.h"
/* Included by name as well as through virt_composer.h, so this file keeps compiling while
the split is only half done and virt_composer.h does not yet collect it. 2026-09-21 20:49 */
#include "virt_composer_coroutines.h"

#include "co_utils.h"
#include "yaml.h"

namespace virt_composer
{

namespace vo = virt_object;
namespace vc = virt_composer;

/* ------------------------------------------------------------------------------------------- */
/* lua_coro_t                                                                                   */
/* ------------------------------------------------------------------------------------------- */

/* See lua_coro_t::create()'s declaration in virt_composer_coroutines.h for its doc comment. */
vc::ref_t<lua_coro_t> lua_coro_t::create(virt_state_t *vs) {
    auto ret = std::make_shared<lua_coro_t>(vc::object_t::Private{type_id_static()});
    auto L   = luaw_get_lua_state(vs);

    ret->thread = lua_newthread(L);         /* pushed onto the main state */
    ret->held   = lua_object_t::create();
    ret->held->capture_ref(L);              /* pops it into the state's reference table */
    ret->done   = co::create_sem(luaw_get_pool(vs), 0);

    /* The way back, from a running thread to the object driving it. A thread Lua makes for itself
    inherits the extra space of the state that made it, which coroutines_register_meta() set to
    null on the main state, and that is how lua_await tells the two apart. 2026-09-22 03:32 */
    *(lua_coro_t **)lua_getextraspace(ret->thread) = ret.get();

    return ret;
}

/* Everything create() does except making the thread, since Lua already made this one. The status
is taken from the thread rather than assumed: an adopted coroutine may be freshly made or already
part-way through. 2026-09-21 20:49 */
vc::ref_t<lua_coro_t> lua_coro_t::adopt(virt_state_t *vs, lua_State *L, int idx) {
    auto ret = std::make_shared<lua_coro_t>(vc::object_t::Private{type_id_static()});

    ret->thread = lua_tothread(L, idx);
    ret->held   = lua_object_t::create();
    lua_pushvalue(L, idx);
    ret->held->capture_ref(L);              /* pops the copy into the reference table */
    ret->done   = co::create_sem(luaw_get_pool(vs), 0);
    ret->status = lua_status(ret->thread);

    *(lua_coro_t **)lua_getextraspace(ret->thread) = ret.get();

    return ret;
}

/* See lua_coro_t::ready_thread()'s declaration in virt_composer_coroutines.h for its doc comment.
The reset belongs here, and not in run(), because close() truncates the stack: a thread cleared at
the start of a run would lose the call the caller had just set on it. 2026-09-21 20:49 */
err_e lua_coro_t::ready_thread() {
    if (is_running()) {
        DBG("This coroutine is in the middle of a call");
        return VC_ERROR_FAILED_CALL;
    }
    if (status != LUA_OK)
        close();                            /* an errored thread is dead until it is closed */
    else
        lua_settop(thread, 0);              /* the previous call's results */
    return VC_ERROR_OK;
}

/* See lua_coro_t::run()'s declaration in virt_composer_coroutines.h for its doc comment. */
co::task<err_e> lua_coro_t::run() {
    if (is_running()) {
        DBG("This coroutine is in the middle of a call");
        co_return VC_ERROR_FAILED_CALL;
    }
    if (status != LUA_OK) {
        DBG("The last call errored, set a new call before running again");
        co_return VC_ERROR_FAILED_CALL;
    }
    if (lua_gettop(thread) < 1) {
        DBG("Nothing to call, set_call() first");
        co_return VC_ERROR_FAILED_CALL;
    }

    self = to_related<lua_coro_t>();        /* alive until the script ends */
    co::FnScope let_go([this]{ self = nullptr; });

    luaw_resume_coro(thread, lua_gettop(thread) - 1);
    if (status == LUA_YIELD)
        co_await done->wait();

    if (status != LUA_OK) {
        DBG("The script failed: %s", lua_tostring(thread, -1));
        co_return VC_ERROR_FAILED_CALL;
    }
    co_return VC_ERROR_OK;
}

/* See lua_coro_t::wait_done()'s declaration in virt_composer_coroutines.h for its doc comment. */
co::task<err_e> lua_coro_t::wait_done() {
    if (is_running())
        co_await done->wait();
    co_return status == LUA_OK ? VC_ERROR_OK : VC_ERROR_FAILED_CALL;
}

/* See lua_coro_t::close()'s declaration in virt_composer_coroutines.h for its doc comment. */
void lua_coro_t::close() {
    if (!thread)
        return;
    auto *vs = luaw_get_virt_state(thread);

    lua_closethread(thread, vs ? luaw_get_lua_state(vs) : nullptr);
    /* A thread closed after an error keeps that error on its stack, since that is what
    lua_closethread answers with (minilua.h:6614). A thread about to take a new call has no use for
    it, and a callee pushed above it would not be the one resumed. 2026-09-21 20:49 */
    lua_settop(thread, 0);
    status   = LUA_OK;
    nres     = 0;
    wait_err = {};
}

/* See luaw_resume_coro()'s declaration in virt_composer_coroutines.h for its doc comment.
`lua_resume` is given a null `from` on purpose: Lua uses it only to inherit the C-call depth, and
each resume stands on a different stack. 2026-09-21 20:49 */
void luaw_resume_coro(lua_State *thread, int n) {
    lua_coro_t *co = luaw_get_coro(thread);
    if (!co) {
        DBG("No coroutine object on this thread, dropping the resume");
        return;
    }
    co->status = lua_resume(thread, nullptr, n, &co->nres);
    if (co->status != LUA_YIELD)
        co->done->signal_all();
}

/* ------------------------------------------------------------------------------------------- */
/* The Lua-visible side                                                                         */
/* ------------------------------------------------------------------------------------------- */

/* [INTERNAL] Answers the coroutine object a Lua argument holds, or null with the error already
raised. The cast is done by hand rather than through to_related<T>(), which throws on a mismatch,
because a script passing the wrong object is an ordinary Lua error and not an exception.
2026-09-21 20:49 */
static vc::ref_t<lua_coro_t> luaw_coro_arg(lua_State *L, int idx) {
    auto *obj = vc::get_object_from_lua(L, idx);
    auto  co  = obj ? std::dynamic_pointer_cast<lua_coro_t>(obj->shared_this()) : nullptr;
    if (!co)
        vc::luaw_push_error(L, "expected a vc coroutine");
    return co;
}

/* vc.coro_create(f, ...) -- makes a coroutine and sets `f` and its arguments as its next call. The
callee and the arguments are moved rather than copied: this function is about to return and its own
stack goes with it. 2026-09-21 20:49 */
static int luaw_coro_create(lua_State *L) {
    auto *vs = vc::luaw_get_virt_state(L);
    if (!vs) {
        vc::luaw_push_error(L, "vc.coro_create: no virt state behind this lua state");
        return 0;
    }
    if (!lua_isfunction(L, 1)) {
        vc::luaw_push_error(L, "vc.coro_create: expects a function");
        return 0;
    }

    auto co = lua_coro_t::create(vs);
    int  n  = lua_gettop(L);                /* the callee and its arguments */

    lua_settop(co->thread, 0);
    lua_xmove(L, co->thread, n);            /* off this stack, onto the coroutine's, in order */

    if (vc::push_vc_object(L, co->to_related<vc::object_t>()) < 0) {
        vc::luaw_push_error(L, "vc.coro_create: could not answer the coroutine");
        return 0;
    }
    return 1;
}

/* vc.coro_adopt(co) -- takes a thread Lua made and drives it from here on. The guards catch what
can be caught: the main state, a thread already driven, one that is dead, and the one running this
very call. What cannot be caught is somebody else resuming it later, which is the whole of the
risk. 2026-09-21 20:49 */
static int luaw_coro_adopt(lua_State *L) {
    auto *vs = vc::luaw_get_virt_state(L);
    if (!vs) {
        vc::luaw_push_error(L, "vc.coro_adopt: no virt state behind this lua state");
        return 0;
    }
    if (!lua_isthread(L, 1)) {
        vc::luaw_push_error(L, "vc.coro_adopt: expects a coroutine");
        return 0;
    }

    lua_State *th = lua_tothread(L, 1);
    if (th == L || th == vc::luaw_get_lua_state(vs)) {
        vc::luaw_push_error(L, "vc.coro_adopt: a coroutine cannot adopt itself or the main state");
        return 0;
    }
    if (luaw_get_coro(th)) {
        vc::luaw_push_error(L, "vc.coro_adopt: this coroutine is already driven by the actor");
        return 0;
    }
    int st = lua_status(th);
    if (st != LUA_OK && st != LUA_YIELD) {
        vc::luaw_push_error(L, "vc.coro_adopt: this coroutine is dead");
        return 0;
    }

    auto co = lua_coro_t::adopt(vs, L, 1);
    if (vc::push_vc_object(L, co->to_related<vc::object_t>()) < 0) {
        vc::luaw_push_error(L, "vc.coro_adopt: could not answer the coroutine");
        return 0;
    }
    return 1;
}

/* co:start() -- schedules the call and answers at once. 2026-09-21 20:49 */
static int luaw_coro_start(lua_State *L) {
    auto co = luaw_coro_arg(L, 1);
    if (!co)
        return 0;

    auto *vs = vc::luaw_get_virt_state(L);
    vc::luaw_get_pool(vs)->sched(co->run());
    return 0;
}

/* [INTERNAL] Copies onto `L` the results the ended script left on its own thread. They are copied
and not moved, because every waiter is woken and every one of them takes its own.
2026-09-21 20:49 */
static int luaw_coro_push_results(lua_State *L, vc::ref_t<lua_coro_t> co) {
    lua_State *src  = co->thread;
    int        base = lua_gettop(src) - co->nres;

    for (int i = 1; i <= co->nres; i++) {
        lua_pushvalue(src, base + i);
        lua_xmove(src, L, 1);
    }
    return co->nres;
}

/* [INTERNAL] Pushes the error table the actors speak, `{errid, errstr}`. 2026-09-21 20:49 */
static int luaw_coro_push_err(lua_State *L, err_e err, const char *what) {
    lua_newtable(L);
    lua_pushinteger(L, err);
    lua_setfield(L, -2, "errid");
    lua_pushstring(L, what);
    lua_setfield(L, -2, "errstr");
    return 1;
}

/* co:wait() -- waits for another script to end and answers what it returned, or the standard error
table. The callback holds the coroutine by ref_t, so the one being waited on cannot die while
somebody waits for it, whatever else drops its handle meanwhile. 2026-09-21 20:49 */
static int luaw_coro_wait(lua_State *L) {
    auto co = luaw_coro_arg(L, 1);
    if (!co)
        return 0;

    return vc::lua_await(L, co->wait_done(),
            std::function<int(lua_State *, err_e &)>(
                [co](lua_State *L, err_e &err) -> int {
                    if (err != vc::VC_ERROR_OK)
                        return luaw_coro_push_err(L, err, "the script failed");
                    return luaw_coro_push_results(L, co);
                }));
}

/* vc.spawn(f, ...) -- the two common ones together. coro_create() moves the callee and the
arguments away, so the coroutine it answers is the only thing left on this stack and is exactly the
argument start() reads. 2026-09-21 20:49 */
static int luaw_coro_spawn(lua_State *L) {
    if (luaw_coro_create(L) != 1)
        return 0;
    luaw_coro_start(L);             /* raises rather than answering, if it fails at all */
    return 1;
}

/* ------------------------------------------------------------------------------------------- */
/* Registration                                                                                 */
/* ------------------------------------------------------------------------------------------- */

/* [INTERNAL] The `vc::lua_coro_t` builder a config names. What it builds has a thread and no
callee, which is the state a fresh vc::lua_object_t is in as well: something gives it one later.
2026-09-21 20:49 */
static co::task<vc::ref_t<vc::object_t>> build_lua_coro(vc::virt_state_t *vs,
        const std::string& name, fkyaml::node& node)
{
    (void)node;
    auto obj = lua_coro_t::create(vs);
    vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
    co_return obj->to_related<vc::object_t>();
}

/* See coroutines_register_meta()'s declaration in virt_composer_coroutines.h for its doc
comment. */
err_e coroutines_register_meta(virt_state_t *vs) {
    /* Every thread Lua makes for itself inherits this, so a thread nobody drives answers null and
    lua_await refuses it. 2026-09-21 20:49 */
    *(lua_coro_t **)lua_getextraspace(luaw_get_lua_state(vs)) = nullptr;

    if (err_e err = add_lua_tab_funcs(vs, {
            {"coro_create", luaw_coro_create},
            {"coro_adopt",  luaw_coro_adopt},
            {"spawn",       luaw_coro_spawn},
        }); err != VC_ERROR_OK)
    {
        DBG("Failed to add the coroutine functions to the vc table");
        return err;
    }

    /* Registered as raw lua_CFunctions rather than through VC_REGISTER_MEMBER_FUNCTION, for the
    reason lua_object_t::push already is: they need the real calling L. For `wait` that is not a
    detail - the L it must suspend is the waiting script's thread, and the one it reads results
    from is the other one. 2026-09-21 20:49 */
    set_lua_class_member(vs, lua_coro_t::type_id_static(), "start",
            luaw_coro_start, LUAW_MEMBER_FUNCTION);
    set_lua_class_member(vs, lua_coro_t::type_id_static(), "wait",
            luaw_coro_wait, LUAW_MEMBER_FUNCTION);

    return add_named_builder_callback(vs, "vc::lua_coro_t", build_lua_coro);
}

}; /* namespace virt_composer */
