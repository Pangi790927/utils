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

/* See lua_coro_t::_ready_thread()'s declaration in virt_composer_coroutines.h for its doc comment.
The reset belongs here, and not in run(), because close() truncates the stack: a thread cleared at
the start of a run would lose the call the caller had just set on it. 2026-09-21 20:49 */
err_e lua_coro_t::_ready_thread() {
    if (is_running()) {
        DBG("This coroutine is in the middle of a call");
        return VC_ERROR_FAILED_CALL;
    }
    if (status != LUA_OK) {
        /* An errored thread is dead until it is closed. The refusal cannot fire here - an errored
        thread is not a LUA_OK one - but the answer is carried rather than dropped, because that
        stops being obvious the moment close() learns another reason to say no. 2026-09-22 10:20 */
        if (err_e err = close(); err != VC_ERROR_OK)
            return err;
    }
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

    /* A local, not a member: a coroutine's locals live in its frame, so this reference is held
    for exactly as long as the run lasts and is let go however the frame ends - normally, or torn
    down by a killer or by pool->clear(). run() is a member, so its frame carries `this` and not a
    reference of its own; without this, a script whose handle nobody kept could be collected while
    it was still parked. 2026-09-22 09:00 */
    auto keep_alive = to_related<lua_coro_t>();

    luaw_resume_coro(thread, lua_gettop(thread) - 1);
    if (status == LUA_YIELD)
        co_await done->wait();

    if (status != LUA_OK) {
        DBG("The script failed: %s", lua_tostring(thread, -1));
        co_return VC_ERROR_FAILED_CALL;
    }
    co_return VC_ERROR_OK;
}

/* See lua_coro_t::close()'s declaration in virt_composer_coroutines.h for its doc comment. */
err_e lua_coro_t::close() {
    if (!thread)
        return VC_ERROR_OK;

    /* Lua's own rule, from coroutine.close (minilua.h, luaB_close through auxstatus): a coroutine
    may be closed while it is suspended or dead, never while it is running or below one it
    resumed. Both of those are a LUA_OK thread that still has frames - Lua tells them apart only
    so that coroutine.status can name them, and refuses either - so one test covers both and the
    calling state is not needed here. `status` cannot answer this: it is written once lua_resume
    returns, so it says nothing about a script that is running now. 2026-09-22 10:20 */
    lua_Debug ar;
    if (lua_status(thread) == LUA_OK && lua_getstack(thread, 0, &ar)) {
        DBG("This coroutine is running, or sits below one it resumed: it cannot be closed");
        return VC_ERROR_FAILED_CALL;
    }

    /* The wrapper of a wait in flight goes first. It holds this thread and reaches back here
    through the thread's extra space, so it has to stop existing before the thread is reset or the
    object it points at is let go of. Killing it takes its whole call stack with it, the awaited
    work included: abandoning a wait abandons what was being waited for. 2026-09-22 08:20 */
    if (wait_killer) {
        wait_killer();
        wait_killer = nullptr;
    }

    bool was_waiting = is_running();
    auto *vs = luaw_get_virt_state(thread);

    lua_closethread(thread, vs ? luaw_get_lua_state(vs) : nullptr);
    /* A thread closed after an error keeps that error on its stack, since that is what
    lua_closethread answers with (minilua.h:6614). A thread about to take a new call has no use for
    it, and a callee pushed above it would not be the one resumed. 2026-09-21 20:49 */
    lua_settop(thread, 0);
    status   = LUA_OK;
    nres     = 0;
    wait_err = {};

    /* A script killed part-way did not finish, and anyone waiting for its result is owed that
    answer rather than silence. LUA_ERRRUN is written by hand because no resume produced it: the
    thread itself is clean, and this says the call it was on did not come to an end.
    2026-09-22 08:20 */
    if (was_waiting) {
        status = LUA_ERRRUN;
        done->signal_all();
    }
    return VC_ERROR_OK;
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

/* vc.coroutine_create(f, ...) -- makes a coroutine and sets `f` and its arguments as its next
call. The callee and the arguments are moved rather than copied: this function is about to return
and its own stack goes with it. 2026-09-22 04:30 */
static int luaw_coroutine_create(lua_State *L) {
    auto *vs = vc::luaw_get_virt_state(L);
    if (!vs) {
        vc::luaw_push_error(L, "vc.coroutine_create: no virt state behind this lua state");
        return 0;
    }
    if (!lua_isfunction(L, 1)) {
        vc::luaw_push_error(L, "vc.coroutine_create: expects a function");
        return 0;
    }

    auto co = lua_coro_t::create(vs);
    int  n  = lua_gettop(L);                /* the callee and its arguments */

    lua_settop(co->thread, 0);
    lua_xmove(L, co->thread, n);            /* off this stack, onto the coroutine's, in order */

    if (vc::push_vc_object(L, co->to_related<vc::object_t>()) < 0) {
        vc::luaw_push_error(L, "vc.coroutine_create: could not answer the coroutine");
        return 0;
    }
    return 1;
}

/* vc.coroutine_spawn(f, ...) -- the two common ones together. coroutine_create() moves the callee
and the arguments away and leaves the coroutine as the only thing on this stack, so this reads it
back only to get hold of it in C++: it is the object create() just made and pushed, not something
a script handed over, so neither the lookup nor the cast can fail. 2026-09-22 07:50 */
static int luaw_coroutine_spawn(lua_State *L) {
    if (luaw_coroutine_create(L) != 1)
        return 0;
    vc::get_object_from_lua(L, -1)->to_related<lua_coro_t>()->start();
    return 1;
}

/* ------------------------------------------------------------------------------------------- */
/* Registration                                                                                 */
/* ------------------------------------------------------------------------------------------- */

/* See coroutines_register_meta()'s declaration in virt_composer_coroutines.h for its doc
comment. */
err_e coroutines_register_meta(virt_state_t *vs) {
    if (err_e err = add_lua_tab_funcs(vs, {
            {"coroutine_create", luaw_coroutine_create},
            {"coroutine_spawn",  luaw_coroutine_spawn},
        }); err != VC_ERROR_OK)
    {
        DBG("Failed to add the coroutine functions to the vc table");
        return err;
    }

    VC_REGISTER_MEMBER_FUNCTION(vs, lua_coro_t, start);

    /* The typed waits. wait_result is a template, so each is registered by hand rather than
    through VC_REGISTER_MEMBER_FUNCTION, which would name them after the template-id. Each answers
    a coroutine, so luaw_returner_t<co::task<T>> suspends the calling script on it and pushes the
    pair it ends with - a script reads the value at [1] and the code at [2], and the code compares
    against vc.VC_ERROR_OK. A script wanting a type not listed here registers its own; nothing
    about these three is privileged. 2026-09-22 06:50 */
    luaw_register_member_function<lua_coro_t, &lua_coro_t::wait_result<int64_t>>(
            vs, "wait_result_i64");
    luaw_register_member_function<lua_coro_t, &lua_coro_t::wait_result<double>>(
            vs, "wait_result_f64");
    luaw_register_member_function<lua_coro_t, &lua_coro_t::wait_result<std::string>>(
            vs, "wait_result_str");

    /* co:close() -- ends whatever is on this coroutine, and answers vc.VC_ERROR_FAILED_CALL
    rather than obeying when it is one that must not be reset. 2026-09-22 10:20 */
    VC_REGISTER_MEMBER_FUNCTION(vs, lua_coro_t, close);

    /* Inline, so the registration says what it builds where it registers it. A coroutine built
    from a config has a thread and no callee, which is the state a fresh vc::lua_object_t is in as
    well: something gives it one later. 2026-09-22 04:30 */
    return add_named_builder_callback(vs, "vc::lua_coro_t",
            [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
            {
                (void)node;
                auto obj = lua_coro_t::create(vs);
                vc::mark_dependency_solved(vs, node_name, obj->to_related<vc::object_t>());
                co_return obj->to_related<vc::object_t>();
            });
}

}; /* namespace virt_composer */
