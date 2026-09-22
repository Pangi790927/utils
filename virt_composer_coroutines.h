#ifndef VIRT_COMPOSER_COROUTINES_H
#define VIRT_COMPOSER_COROUTINES_H

/*!
 * @file
 * @brief Joins a Lua script's suspension to a colib task, so a script can wait for work the actor
 * is doing while the actor goes on doing it.
 *
 * Core:
 *   - @ref lua_coro_t is a Lua thread held as a composer object. A call is set on it, it is run,
 *     and what it left behind is read directly or waited for.
 *   - @ref lua_await is what a `lua_CFunction` returns when it cannot answer yet. It suspends the
 *     calling script, lets the pool carry on, and answers the script once the task it was handed
 *     completes.
 *   - A script gains `vc.coro_create`, `vc.coro_adopt` and `vc.spawn`, plus `start` and `wait` on
 *     a coroutine object.
 *   - `VC_TYPE_LUA_CORO` is registered here, so a coroutine can also be named in a config under
 *     `m_type: vc::lua_coro_t`.
 *
 * Detail:
 *   - This file is a part of virt_composer.h and is included by it. It is not includable on its
 *     own: the type id it registers is counted in the order the declarations of a translation
 *     unit are read, so a file that could include it at a place of its own choosing would give
 *     `VC_TYPE_LUA_CORO` a different value in every such unit.
 *   - Everything whose body reads a member of `virt_state_t` lives in
 *     virt_composer_coroutines.cpp, and reaches the state only through the accessors
 *     virt_composer.h publishes. Nothing here needs `virt_state_t` to be a complete type.
 *
 * @date 2026-09-21 20:49
 */

#ifndef VIRT_COMPOSER_H
# error "virt_composer_coroutines.h is included by virt_composer.h and not on its own - see the \
file comment for why the include place is not the includer's to choose."
#endif

namespace virt_composer {

/* The seventh type of the library. It is registered after the six in virt_composer.h and before
any user header, which is what keeps every id but its own where it already was.
2026-09-21 20:49 */
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_LUA_CORO);

/*!
 * @brief A Lua coroutine the actor drives: a thread of its own, a call set on it, and a result
 * whoever wants it can wait for.
 *
 * Core:
 *   - A script that waits has to run on a thread of its own, since the main state cannot yield.
 *     This is that thread, held as an object so that `ref_t` decides when it dies and so that a
 *     config or a script can name one.
 *   - The order of use is: set the call, run it, read the result. `set_call()` puts the callee and
 *     its arguments on the thread, `run()` is a coroutine that ends when the script ends, and
 *     `result()` converts what it left behind.
 *   - `run()` may be called again once the script has ended: the thread is reused for the next
 *     call. Calling it while the script is still suspended is refused.
 *   - A call set after a failed one is taken: `set_call()` closes an errored thread itself, so a
 *     caller never has to know that it must.
 *   - While a script is in flight the object holds a reference to itself, so scheduling a run and
 *     forgetting the handle is safe.
 *   - `close()` kills a suspended script, runs its to-be-closed variables and makes the thread fit
 *     to be called again. It is also what the destructor does.
 *   - A wait that cannot be answered raises in the script at the point of the wait, the same way a
 *     failed call does, and leaves the thread errored until it is closed.
 *
 * Detail:
 *   - The thread is kept from the collector by an ordinary @ref lua_object_t holding it, which is
 *     the same mechanism any other Lua value here is held by.
 *   - `get_L()` is the way out for anything the templates do not cover: arguments built by hand, a
 *     callee pushed from somewhere unusual.
 *
 * @warning Nothing here is safe to use past the death of the `virt_state_t` it was made on, the
 *       thread included, for the reason every other object of this library carries.
 *
 * @see lua_await, lua_object_t
 *
 * @date 2026-09-21 20:49
 */
struct lua_coro_t : public vc::object_t {
    lua_State              *thread = nullptr;  /* the coroutine itself */
    vc::ref_t<lua_object_t> held;              /* holds `thread` against the collector */
    vc::ref_t<lua_coro_t>   self;              /* held while a script is in flight */
    co::sem_p               done;              /* signal_all'd when a script ends */
    int                     status = LUA_OK;   /* what the last resume answered */
    int                     nres   = 0;        /* results the script left on the thread */
    std::string             wait_err;          /* set by a wrapper that cannot answer a wait */

    lua_coro_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~lua_coro_t() { close(); }

    /*! Makes a thread on `vs`, holds it against the collector and answers it with no call set on
     * it yet. @date 2026-09-21 20:49 */
    static vc::ref_t<lua_coro_t> create(virt_state_t *vs);

    /*! Takes a thread Lua already made and drives it from here on: holds it, marks it and gives it
     * a completion semaphore. `idx` is where the thread sits on `L`. Sound only while nothing else
     * resumes that thread, since two resumers would each believe they own it.
     * @date 2026-09-21 20:49 */
    static vc::ref_t<lua_coro_t> adopt(virt_state_t *vs, lua_State *L, int idx);

    virtual vc::object_type_e type_id() const override { return VC_TYPE_LUA_CORO; }
    static vc::object_type_e type_id_static() { return VC_TYPE_LUA_CORO; }

    inline std::string to_string() const override {
        return std::format("vc::lua_coro_t[{}]: thread={} status={} nres={}",
                (const void *)this, (void *)thread, status, nres);
    }

    /*! Answers the thread itself, for stack work the templates do not cover.
     * @date 2026-09-21 20:49 */
    lua_State *get_L() { return thread; }

    /*! Answers whether a script is on the thread right now, suspended or running.
     * @date 2026-09-21 20:49 */
    bool is_running() const { return status == LUA_YIELD; }

    /*! Clears the thread and sets a global function and its arguments as the next call. Refused
     * while a script is in flight. @date 2026-09-21 20:49 */
    template <typename ...Args>
    err_e set_call(const char *fn_name, Args ...args);

    /*! The same, for a callee held as a @ref lua_object_t rather than named.
     * @date 2026-09-21 20:49 */
    template <typename ...Args>
    err_e set_call(vc::ref_t<lua_object_t> fn, Args ...args);

    /*! Clears the thread and puts only the callee on it, the arguments being the caller's own
     * business through `get_L()`. @date 2026-09-21 20:49 */
    err_e push_call(vc::ref_t<lua_object_t> fn);

    /*! **Coroutine** that runs what `set_call()` left and ends when the script ends, however many
     * times it waited in between. @date 2026-09-21 20:49 */
    co::task<err_e> run();

    /*! Converts the first result the script left. Fails if a script is still in flight, if the
     * last one errored, if it left nothing, or if what it left will not become an `R`.
     * @date 2026-09-21 20:49 */
    template <typename R>
    std::pair<R, err_e> result();

    /*! **Coroutine** that waits for the script to end and then does what `result()` does. Every
     * waiter is woken and every waiter converts its own copy. @date 2026-09-21 20:49 */
    template <typename R>
    co::task<std::pair<R, err_e>> wait_result();

    /*! **Coroutine** that waits for the script to end and answers only whether it succeeded. What
     * it left is read afterwards, off the thread, which is what the Lua-visible `wait` does since
     * it has no C++ type to convert to. @date 2026-09-21 20:49 */
    co::task<err_e> wait_done();

    /*! Kills whatever is on the thread, runs its to-be-closed variables and leaves the thread fit
     * to be called again. @date 2026-09-21 20:49 */
    void close();

    /*! [INTERNAL] Clears what the last call left and answers whether the thread may take a new
     * one. The three call setters share it. @date 2026-09-21 20:49 */
    err_e ready_thread();
};

/*!
 * @brief Registers everything this file brings onto a fresh virt-state.
 *
 * Core:
 *   - Adds `vc.coro_create`, `vc.coro_adopt` and `vc.spawn`, the `start` and `wait` members of a
 *     coroutine object, and the `vc::lua_coro_t` builder a config names.
 *   - Must be called before any Lua code runs on the state, because it also marks the main state
 *     as driven by nobody, and a thread made before that mark would inherit a value that means
 *     nothing.
 *   - The caller registers it, right after create_state(). Nothing inside the library can:
 *     virt_composer.h does not include this file yet, so create_state() cannot name this
 *     function. Once the component is collected there, this becomes one line of virt_composer's
 *     own register_meta, alongside every other internal component's.
 *   - It is not a composer's `register_meta()`. That name belongs to a unit of types registered
 *     into a state from outside the library; this is inside it.
 *
 * @param vs Virtual state context.
 *
 * @return `VC_ERROR_OK`, or what the registration that failed answered.
 *
 * @date 2026-09-22 03:32
 */
err_e coroutines_register_meta(virt_state_t *vs);

/*!
 * @brief Waits for `task` to finish without stopping the actor, and answers its result to the Lua
 * script that called.
 *
 * Core:
 *   - Call it from a `lua_CFunction` and return its result directly. It does not return to its
 *     caller: the script is suspended at this point and the function is left through `push` when
 *     `task` completes, so anything written after the call never runs.
 *   - While the script waits, every other task on the state's pool keeps running.
 *   - `push` is called once `task` has completed, with the state of the calling script and the
 *     task's result. It pushes whatever the function answers to Lua and returns how many values it
 *     pushed. Left out, the function answers nothing.
 *   - `push` decides what a failure of the awaited work looks like, since only the author knows
 *     how the task reports one. The convention the actors speak is a table `{errid, errstr}`.
 *   - A failure of the wait itself - `push` throwing, or the wrapper being killed - raises in the
 *     script at the point of the wait, so `pcall` catches it like any other error.
 *   - Only a script the actor drives may wait. The main state, a config being parsed and a
 *     coroutine Lua made for itself each raise their own Lua error instead.
 *
 * @tparam T    The result type of the awaited task, deduced from `task`. A task that answers
 *              nothing has no result to hand over and is not what this takes; colib's own
 *              `task_t` answers an `int`.
 * @param L     The state of the calling script, as the `lua_CFunction` received it.
 * @param task  The work to wait for.
 * @param push  Turns the result into Lua values and answers how many it pushed.
 *
 * @return What `lua_yieldk` answers; the caller returns it unexamined.
 *
 * @see lua_coro_t
 *
 * @date 2026-09-21 20:49
 */
template <typename T>
int lua_await(lua_State *L, co::task<T> task, std::function<int(lua_State *, T &)> push = {});


/*! IMPLEMENTATION
 * 
 * 
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * (This block is so large so I easily see it in sublime's right window)
 * 
 * 
 */

/*! [INTERNAL] Resumes the script on `thread`, with `n` values already pushed onto it, records what
 * the resume answered and wakes every waiter once the script has ended. A thread nobody drives is
 * left alone. @date 2026-09-21 20:49 */
void luaw_resume_coro(lua_State *thread, int n);

/* [INTERNAL] Answers the coroutine object driving `L`, or null when `L` is the main state or a
thread Lua made for itself. 2026-09-21 20:49 */
inline lua_coro_t *luaw_get_coro(lua_State *L) {
    return *(lua_coro_t **)lua_getextraspace(L);
}

/* [INTERNAL] Pushes each argument onto the thread, clearing it on the first failure so that a
half-built call is never left behind. 2026-09-21 20:49 */
template <typename ...Args>
err_e luaw_coro_push_args(lua_State *thread, Args &...args) {
    bool ok = true;
    ([&](auto &arg){
        if (ok && luaw_push_cpp_object(thread, arg) < 0) {
            DBG("Failed to push an argument");
            ok = false;
        }
    }(args), ...);

    if (!ok) {
        lua_settop(thread, 0);
        return VC_ERROR_FAILED_CALL;
    }
    return VC_ERROR_OK;
}

template <typename ...Args>
err_e lua_coro_t::set_call(const char *fn_name, Args ...args) {
    if (err_e err = ready_thread(); err != VC_ERROR_OK)
        return err;
    lua_getglobal(thread, fn_name);
    if (!lua_isfunction(thread, -1)) {
        DBG("Not a callable global: %s", fn_name);
        lua_settop(thread, 0);
        return VC_ERROR_FAILED_CALL;
    }
    return luaw_coro_push_args(thread, args...);
}

template <typename ...Args>
err_e lua_coro_t::set_call(vc::ref_t<lua_object_t> fn, Args ...args) {
    if (err_e err = push_call(fn); err != VC_ERROR_OK)
        return err;
    return luaw_coro_push_args(thread, args...);
}

inline err_e lua_coro_t::push_call(vc::ref_t<lua_object_t> fn) {
    if (!fn) {
        DBG("No callee given");
        return VC_ERROR_FAILED_CALL;
    }
    if (err_e err = ready_thread(); err != VC_ERROR_OK)
        return err;
    fn->push(thread);
    if (!lua_isfunction(thread, -1)) {
        DBG("The given object does not hold a callable");
        lua_settop(thread, 0);
        return VC_ERROR_FAILED_CALL;
    }
    return VC_ERROR_OK;
}

/* [INTERNAL] Converts the value on top of `L` into the `R` that the light userdata upvalue points
at. It exists to be pcall'd: luaw_lua_to_cpp_object() raises a Lua error on a shape it cannot take,
and a raise outside a protected call ends the process rather than the conversion.
2026-09-21 20:49 */
template <typename R>
int luaw_coro_convert_result(lua_State *L) {
    try {
        R *out = (R *)lua_touserdata(L, lua_upvalueindex(1));
        luaw_lua_to_cpp_object(L, -1, *out);
        return 0;
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename R>
std::pair<R, err_e> lua_coro_t::result() {
    if (is_running()) {
        DBG("This coroutine is still in the middle of a call");
        return {R{}, VC_ERROR_FAILED_CALL};
    }
    if (status != LUA_OK) {
        DBG("The last call errored, there is no result to read");
        return {R{}, VC_ERROR_FAILED_CALL};
    }
    if (nres < 1) {
        DBG("The last call returned nothing");
        return {R{}, VC_ERROR_FAILED_CALL};
    }

    R ret{};
    /* The conversion reads the top of the stack whatever index it is given, so the first result is
    copied up there rather than pointed at. 2026-09-21 20:49 */
    int first = lua_gettop(thread) - nres + 1;
    lua_pushlightuserdata(thread, &ret);
    lua_pushcclosure(thread, luaw_coro_convert_result<R>, 1);
    lua_pushvalue(thread, first);
    if (lua_pcall(thread, 1, 0, 0) != LUA_OK) {
        DBG("Could not convert the result: %s", lua_tostring(thread, -1));
        lua_pop(thread, 1);
        return {R{}, VC_ERROR_FAILED_CALL};
    }
    return {ret, VC_ERROR_OK};
}

template <typename R>
co::task<std::pair<R, err_e>> lua_coro_t::wait_result() {
    if (is_running())
        co_await done->wait();
    co_return result<R>();
}

/* [INTERNAL] The continuation every waiting function returns through. It exists for the one thing
a bare resume cannot do: raise inside the script. `ctx` carries the thread's stack top as it stood
at the yield, so the difference is how many values the wrapper pushed, and that is what the waiting
function answers to Lua. 2026-09-21 20:49 */
inline int luaw_await_k(lua_State *L, int status, lua_KContext ctx) {
    (void)status;

    lua_coro_t *co = luaw_get_coro(L);
    if (co && !co->wait_err.empty()) {
        std::string err = std::move(co->wait_err);
        co->wait_err.clear();
        luaw_push_error(L, err);        /* raises, exactly as a failed call does */
    }
    return lua_gettop(L) - (int)ctx;
}

/* [INTERNAL] Awaits the author's task, turns its result into Lua values through `push` and resumes
the script with them. Everything it needs is a parameter and nothing is captured: a lambda dies at
the end of the statement that made it, while the frame outlives it. 2026-09-21 20:49 */
template <typename T>
co::task_t luaw_await_wrapper(lua_State *thread, co::task<T> task,
        std::function<int(lua_State *, T &)> push)
{
    T res = co_await task;
    int n = 0;

    try {
        if (push)
            n = push(thread, res);
    }
    catch (std::exception &e) {
        DBG("The push callback threw: %s", e.what());
        if (lua_coro_t *co = luaw_get_coro(thread))
            co->wait_err = std::format("the push callback threw: {}", e.what());
        n = 0;
    }
    luaw_resume_coro(thread, n);
    co_return VC_ERROR_OK;
}

/* See lua_await()'s declaration above for its doc comment. 2026-09-22 03:00 */
template <typename T>
int lua_await(lua_State *L, co::task<T> task, std::function<int(lua_State *, T &)> push) {
    /* luaw_push_error() raises, so none of these three return and the `return 0` is only for the
    compiler. The rest of virt_composer reports a C++ error this way. 2026-09-21 20:49 */
    auto *vs = luaw_get_virt_state(L);
    if (!vs) {
        luaw_push_error(L, "vc::lua_await: no virt state behind this lua state");
        return 0;
    }
    if (!lua_isyieldable(L)) {
        luaw_push_error(L, "vc::lua_await: nothing to suspend here, this is the main state or "
                "a config being parsed");
        return 0;
    }
    if (!luaw_get_coro(L)) {
        luaw_push_error(L, "vc::lua_await: this coroutine is not driven by the actor, use a vc "
                "coroutine to wait");
        return 0;
    }

    lua_KContext top = (lua_KContext)lua_gettop(L);

    /* Scheduling only queues the wrapper; the pool gets its turn once this resume has answered, so
    the yield below always happens before the wrapper's first step. 2026-09-21 20:49 */
    luaw_get_pool(vs)->sched(luaw_await_wrapper<T>(L, std::move(task), std::move(push)));
    return lua_yieldk(L, 0, top, luaw_await_k);
}

}; /* namespace virt_composer */

#endif /* VIRT_COMPOSER_COROUTINES_H */
