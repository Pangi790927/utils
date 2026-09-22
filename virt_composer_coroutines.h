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
 *   - A script gains `vc.coroutine_create`, `vc.coroutine_adopt` and `vc.coroutine_spawn`, plus
 *     `start` and `wait` on a coroutine object.
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

    /*! Schedules `run()` on the state's pool and answers at once, leaving the script to get on
     * with it. @date 2026-09-22 04:30 */
    void start() { luaw_get_pool(luaw_get_virt_state(thread))->sched(run()); }

    /*! Converts the result the script left. A coroutine answers one value, so a script that
     * returned more has only its first taken here and the rest are reached through `get_L()`.
     * Fails if a script is still in flight, if the last one errored, if it left nothing, or if
     * what it left will not become an `R`. @date 2026-09-22 04:30 */
    template <typename R>
    std::pair<R, err_e> result();

    /*! **Coroutine** that waits for the script to end and then does what `result()` does. Every
     * waiter is woken and every waiter converts its own copy. @date 2026-09-21 20:49 */
    template <typename R>
    co::task<std::pair<R, err_e>> wait_result();

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
 *   - Adds `vc.coroutine_create`, `vc.coroutine_adopt` and `vc.coroutine_spawn`, the `start` and
 *     `wait` members of a coroutine object, and the `vc::lua_coro_t` builder a config names.
 *   - Must be called before any script runs on the state, since a script cannot name what has
 *     not been registered yet.
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
    /* The conversion reads the top of the stack whatever index it is given, so the result is
    copied up there rather than pointed at. It answers rather than raising, which is what lets
    this run here at all: nothing protected is active on this thread once the script has ended.
    2026-09-22 06:50 */
    lua_pushvalue(thread, lua_gettop(thread) - nres + 1);
    int conv = luaw_lua_to_cpp_object(thread, -1, ret);
    lua_pop(thread, 1);
    if (conv < 0) {
        DBG("The result does not convert to the type asked for");
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
    int n = 0;

    /* The await is inside the guard as well as the push. A task that throws is a call that failed,
    and whoever waited on it hears about it the way they hear about any other failed call: raised
    at the point of the wait. Left outside, the throw would leave this frame for the pool and the
    script would stay suspended for good. 2026-09-22 06:50 */
    try {
        T res = co_await task;
        if (push)
            n = push(thread, res);
    }
    catch (std::exception &e) {
        DBG("The awaited work or its push threw: %s", e.what());
        if (lua_coro_t *co = luaw_get_coro(thread))
            co->wait_err = e.what();
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

/*!
 * [INTERNAL]
 * @brief Makes a function that answers a coroutine into a Lua function that suspends.
 *
 * Core:
 *   - A registered function whose return type is a `co::task<T>` does not return to its caller.
 *     The task is awaited, the calling script waits, and `T`'s own returner pushes the result
 *     when it completes.
 *   - It needs nothing of the wrappers: they already end by handing a return value to a returner,
 *     and this is a return value like another. Members and free functions get it alike.
 *   - A task that throws raises at the point of the call, the way a failed call does.
 *
 * Detail:
 *   - Pushing nothing here is not an oversight. `lua_await` leaves by throwing out of
 *     `lua_yieldk`, so nothing after it runs and the wrapper's own `return 1` is never reached -
 *     the result count comes from the continuation when the script is resumed.
 *   - A translation unit that has not read this file answers the primary template instead, whose
 *     static assert says the return type is not a valid one. That is the truth: a waiting
 *     function cannot be registered without this component.
 *
 * @date 2026-09-22 06:50
 */
template <typename T>
struct luaw_returner_t<co::task<T>> {
    void luaw_ret_push(lua_State *L, co::task<T> task) {
        lua_await(L, std::move(task), std::function<int(lua_State *, T &)>(
                [](lua_State *L, T &val) -> int {
                    luaw_returner_t<T>{}.luaw_ret_push(L, val);
                    return 1;
                }));
    }
};

}; /* namespace virt_composer */

#endif /* VIRT_COMPOSER_COROUTINES_H */
