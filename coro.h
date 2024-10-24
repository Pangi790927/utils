#ifndef CORO_H
#define CORO_H
/* rewrite of co_utils.h ->
    - CPP library over the CPP corutines
    - Built around epoll (ie, epoll is the only blocking function)
    - Meant to have one pool/thread or enable mutexes on pool (this will allow callbacks and pool
    sharing)
    - Meant to have modifs (callbacks on corutine execution)
*/

#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <chrono>
#include <coroutine>
#include <functional>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

/* The maximum amount of concurent timers that can be awaited */
#ifndef CO_MAX_TIMER_POOL_SIZE
# define CO_MAX_TIMER_POOL_SIZE 64
#endif

/* Set if a pool can be available in more than one thread, necesary if you want to transfer the pool
to a callback that will be called from another thread */
#ifndef CO_ENABLE_MULTITHREADS
# define CO_ENABLE_MULTITHREADS false
#endif

/* If set to true, will enable the debug callbacks */
#ifndef CO_ENABLE_CALLBACKS
# define CO_ENABLE_CALLBACKS false
#endif

/* If set to true, will enable the logging callback */
#ifndef CO_ENABLE_LOGGING
# define CO_ENABLE_LOGGING true
#endif

/* If set to true, makes names for corutines */
#ifndef CO_ENABLE_DEBUG_NAMES
# define CO_ENABLE_DEBUG_NAMES false
#endif

/* If set, will create a debug tracer and it will add it to all coroutines */
#ifndef CO_ENABLE_DEBUG_TRACE_ALL
# define CO_ENABLE_DEBUG_TRACE_ALL false
#endif

#if !defined(CO_KILL_DEFAULT_CONSTRUCT) && !defined(CO_KILL_DEFAULT_CONSTRUCT)
# define CO_KILL_DEFAULT_CONSTRUCT true
# define CO_KILL_RAISE_EXCEPTION false
#endif

#if defined(CO_KILL_DEFAULT_CONSTRUCT) && defined(CO_KILL_DEFAULT_CONSTRUCT)
# if CO_KILL_DEFAULT_CONSTRUCT && CO_KILL_RAISE_EXCEPTION
#  error "Don't choose both"
# endif
#endif

/* If CO_ENABLE_DEBUG_NAMES you can also define CORO_REG and use it to register a corutine's name */
#if CO_ENABLE_DEBUG_NAMES
# ifndef CORO_REG
#  define CORO_REG(t)   coro::dbg_register_name((t), "%20s:%5d:%s", __FILE__, __LINE__, #t)
# endif
#else
# define CORO_REG(t)    t
#endif

namespace coro {

constexpr int MAX_TIMER_POOL_SIZE = CO_MAX_TIMER_POOL_SIZE;

/* corutine return type, this is the result of creating a corutine, you can await it and also pass
it around (practically holds a std::coroutine_handle and can be awaited to get the return) */
template<typename T> struct task;

/* A pool is the shared state between all corutines. This object holds the epoll handler, timers,
queues, etc. Each corutine has a pointer to this object. You can see this object as an instance of
epoll. */
struct pool_t;

/* Modifs, corutine modifications. Those modifications controll the way a corutine behaves when
awaited or when spawning a new corutine from it. Those modifications can be inherited, making all
the corutines in the call sub-tree behave in a similar way. For example: adding a timeouts, for this
example, all the corutines that are on the same call-path(not spawn-path) would have a timer, such
that the original call would not exist for more than X time units. (+/- the code between awaits) */
struct modif_t;

/* TODO: think this better this time, including better lifetime */
/* This is a semaphore working on a pool. It can be awaited to decrement it's count and .rel()
increments it's count from wherever. More bellow. */
struct sem_t;

/* Internal state of corutines that is independent of the return value of the corutine. */
struct state_t;

/* used pointers, so _p marks a shared_pointer and a _wp marks a weak pointer. If the pool holds
the object you will get a weak pointer and if you are to hold the object you will get a shared
pointer. */
using pool_p = std::shared_ptr<pool_t>;
using pool_wp = std::weak_ptr<pool_t>;
using modif_p = std::shared_ptr<modif_t>;
using modif_wp = std::weak_ptr<modif_t>;

enum error_e : int32_t {
    ERROR_OK      = 0,
    ERROR_GENERIC = -1, /* the source is not specified, can use log_str to find the error,
                        or sometimes errno */
    ERROR_TIMEOUT = -2, /* the error comes from a modif, namely a timeout */
    ERROR_WAKEUP  = -3, /* the error comes from force awaking the awaiter */
    ERROR_USER    = -4, /* the error comes from a modif, namely an user defined modif, users can
                        use this if they wish to return from modif cbks */
    ERROR_DEPEND  = -5, /* the error comes from a depend modif, i.e. depended function failed */
};

enum run_e : int32_t {
    RUN_OK,
    RUN_DONE,
    RUN_ERRORED,
    RUN_STOPPED, /* can be re-run (comes from force_stop) */
};

/* all the internal tasks return this */
using task_t = task<error_e>;

/* some internal structures that are not part of the api, but are needed for the
functions/structures bellow
*/
struct yield_awaiter_t;
struct sched_awaiter_t;
struct pool_internal_t;
struct sem_internal_t;

#if CO_EXCEPT_ON_KILL
/* TODO: the exception that will be thrown by the killing of non default constructible types */
#endif

/* declare it again here, so it can befriend pool_t */

struct pool_t {
    template <typename T>
    void sched(task<T> task, const std::vector<modif_p>& v = {}); /* ignores return type */
    run_e run();

    /* Not private, but don't touch, not yours, leave it alone */
    std::unique_ptr<pool_internal_t> internal;

protected:
    friend pool_p create_pool();
    pool_t();
};

struct sem_t {
    struct unlocker_t{ /* compatibility with guard objects ex: std::lock_guard guard(co_await s); */
        sem_t &sem;
        unlocker_t(sem_t &sem) : sem(sem) {}
        void lock() {}
        void unlock() { sem.inc(); }
    };

    /* can be created and moved, but not copied */
    sem_t(pool_wp pool, int64_t val = 0);
    sem_t(sem_t& sem) = delete;
    sem_t(sem_t&& sem) { internal = std::move(sem.internal); }
    sem_t &operator = (sem_t& sem) = delete;
    sem_t &operator = (sem_t&& sem) { internal = std::move(sem.internal); return *this; }

    void inc();
    bool try_dec();
    /* await instead of dec() */

    template <typename P>
    std::coroutine_handle<void> await_suspend(std::coroutine_handle<P> to_suspend);
    bool                        await_ready();
    unlocker_t                  await_resume();

private:
    std::unique_ptr<sem_internal_t> internal;
};

/* A modif is a callback for a specifi stage in the corutine's flow */
struct modif_t {
    enum modif_e : int32_t {
        /* Call callback is called on  */
        CO_MODIF_CALL_CBK = 0,
        CO_MODIF_SCHED_CBK,
        CO_MODIF_EXIT_CBK,
        CO_MODIF_LEAVE_CBK,
        CO_MODIF_ENTER_CBK,
        CO_MODIF_WAIT_FD_CBK,
        CO_MODIF_UNWAIT_FD_CBK,
        CO_MODIF_WAIT_SEM_CBK,
        CO_MODIF_UNWAIT_SEM_CBK,

        CO_MODIF_COUNT,
    };
    enum modif_flags_e : int32_t {
        CO_MODIF_INHERIT_ON_CALL = 0x1,
        CO_MODIF_INHERIT_ON_SCHED = 0x2,
    };
    std::variant<
        std::function<error_e(state_t *)>,                             /* call_cbk */
        std::function<error_e(state_t *)>,                             /* sched_cbk */
        std::function<error_e(state_t *)>,                             /* exit_cbk */
        std::function<error_e(state_t *)>,                             /* leave_cbk */
        std::function<error_e(state_t *)>,                             /* enter_cbk */
        std::function<error_e(state_t *, int fd, int wait_cond)>,      /* wait_fd_cbk */
        std::function<error_e(state_t *, int fd, int res, int err_no)>,/* unwait_fd_cbk */
        std::function<error_e(state_t *, sem_t *)>,                    /* wait_sem_cbk */
        std::function<error_e(state_t *, sem_t *)>,                    /* unwait_sem_cbk */
    > cbks;

    modif_e type = CO_MODIF_COUNT;
    modif_flags_e flags = CO_MODIF_INHERIT_ON_CALL;

    /* don't touch this */
    std::shared_ptr<void> internal;
};

/* Pool & Sched functions:
------------------------------------------------------------------------------------------------  */

inline pool_p create_pool();

/* gets the pool of the current corutine, necesary if you want to sched corutines from callbacks.
Don't forget you need multithreading enabled if you want to schedule from another thread. */
inline task<pool_wp> get_pool();

/* This does not stop the curent corutine, it only schedules the task, but does not yet run it */
template <typename T>
inline sched_awaiter_t sched(task<T> to_sched, const std::vector<modif_p>& v = {});

/* This stops the current coroutine from running and places it at the end of the ready queue. */
inline yield_awaiter_t yield();

/* Modifications
------------------------------------------------------------------------------------------------  */

inline modif_p create_modif(const modif_t& modif_template);

/* This returns the internal vec that holds the different modifications of the task. Changing them
here is ill-defined */
template <typename T>
inline std::vector<modif_p> &task_modifs(task<T> t);

/* This adds a modifier to the task and returns the task. Duplicates will be ignored */
template <typename T>
inline task<T> add_modif(task<T> t, modif_p mod);

/* Removes modifier from the vector */
template <typename T>
inline task<T> rm_modif(task<T> t, modif_p mod);

/* calls an awaitable inside a task_t, this is done to be able to use modifs on awaitables */
template <typename Awaiter>
inline task_t await(Awaiter&& awaiter);

/* Timing
------------------------------------------------------------------------------------------------  */

/* creates a timeout modification, i.e. the corutine will be stopped from executing if the given
time passes */
inline modif_p creat_timeo(const std::chrono::microseconds& timeo);

/* sleep functions */
inline task_t sleep_us(uint64_t timeo_us);
inline task_t sleep_ms(uint64_t timeo_ms);
inline task_t sleep_s(uint64_t timeo_s);
inline task_t sleep(const std::chrono::milliseconds& us);

/* Flow Controll:
------------------------------------------------------------------------------------------------  */

/* Observation: killed corutines, timed, result of functions that depend on others, futures, all
of those will not be able to construct their return type if they are killed so those
need to be handled, so you must choose that by defining one of the two: CO_KILL_DEFAULT_CONSTRUCT,
CO_KILL_RAISE_EXCEPTION ?TODO:CHECK?

Those that return error_e will return their apropiate error in this case, the rest will either
have the error returned in the exception or set in the task/awaiter object.

If CO_KILL_DEFAULT_CONSTRUCT, then the return value will be the default constructed type.(except if
type is error_e).
*/

/* Creates semaphores, those need the pool to exist */
inline sem_t create_sem(pool_wp pool, int64_t val = 0);
inline task<sem_t> create_sem(int64_t val = 0);

/* a modification that depends on the pool, this is used in scheduled coroutines to end the current
corutine if they die. This is not inherited by either calls or spawns, but it is inherited by other
calls to creat_depend further down. */
inline task<modif_p> creat_depend();

/* modifier that, once signaled, will awake the corutine and make it return ERROR_WAKEUP. If the
corutine is not waiting it will return as soon as it reaches any co_await ?or co_yield? */
/* TODO: don't forget to awake the fds nicely(something like stopfd) */
inline modif_p creat_killer();
inline error_e sig_killer(modif_p p);

/* takes a task and adds the requred modifications to it such that the returned object will be
returned once the return value of the task is available so:
    auto t = co_task();
    auto fut = co::create_future(t)
    co_await co::sched(t);
    ...
    co_await fut; // returns the value of co_task once it has finished executing 
*/
template <typename T>
inline task<T> creat_future(task<T> t);

/* wait for all the tasks to finish, the return value can be found in the respective task, killing
one kills all (sig_killer installed in all). The inheritance is the same as with 'call'. */
template <typename ...ret_v>
inline task_t wait_all(task<ret_v>... tasks);

/* causes the running pool::run to stop, the function will stop at this point, can be resumed with
another run */
inline task_t force_stop(int ret);

/* EPOLL & Linux Specific:
------------------------------------------------------------------------------------------------  */

/* convenience functions provided: */
inline task_t connect(int fd, sockaddr *sa, socklen_t *len);
inline task_t accept(int fd, sockaddr *sa, socklen_t *len);
inline task_t read(int fd, void *buff, size_t len);
inline task_t write(int fd, const void *buff, size_t len);
inline task_t read_sz(int fd, void *buff, size_t len);
inline task_t write_sz(int fd, const void *buff, size_t len);

/* event can be EPOLLIN or EPOLLOUT (maybe some others work too?) This can be used to be notified
when those are ready without doing a read or a write. */
inline task_t wait_event(int fd, int event);

/* closing a file descriptor while it is managed by the coroutine pool will break the entire system
so you must use stopfd on the file descriptor before you close it. This makes sure the fd is
awakened and ejected from the system before closing it. For example:

    co_await co::stopfd(fd);
    close(fd);
*/
inline task_t stopfd(int fd);

/* Debug Interfaces:
------------------------------------------------------------------------------------------------  */

/* Registers a name for a given task */
template <typename T, typename ...Args>
inline task<T> dbg_register_name(task<T> t, const char *fmt, task<T>&&...);

/* creates a modifier that traces things from corutines, mostly a convenience function, it also
uses the log_str function, or does nothing else if it isn't defined. If you don't like the
verbosity, be free to null any callback you don't care about. */
inline modif_p creat_dbg_tracer();

/* Obtains the name given to the respective task */
template <typename T>
inline std::string dbg_name(task<T> t);

/* corutine callbacks for diverse points in the code, if in a multithreaded environment, you must
take all the locks before rewriting those. The pool locks are held when those callbacks are called
*/
#if CO_ENABLE_CALLBACKS
/* add callbacks here */
inline std::function<void(void)> on_call;
#endif /* CO_ENABLE_CALLBACKS */

/* calls log_str to save the log string */
#if CO_ENABLE_LOGGING
inline std::function<int(const std::string&)> log_str;
#endif

/* IMPLEMENTATION 
=================================================================================================
=================================================================================================
================================================================================================= */

template <typename P>
using handle = std::coroutine_handle<P>;

/* Modif Definitions
------------------------------------------------------------------------------------------------- */

struct modif_table_t {
    std::vector<modif_p> call_cbks;
    std::vector<modif_p> sched_cbks;
    std::vector<modif_p> exit_cbks;
    std::vector<modif_p> leav_cbks;
    std::vector<modif_p> enter_cbks;
    std::vector<modif_p> wait_fd_cbks;
    std::vector<modif_p> unwait_fd_cbks;
    std::vector<modif_p> wait_sem_cbks;
    std::vector<modif_p> unwait_sem_cbks;
};

template <typename T>
inline error_e do_sched_modifs(task<T> &task, const std::vector<modif_p>& to_add);

template <typename T>
inline error_e do_call_modifs(task<T> &task, const std::vector<modif_p>& to_add);


/* The Task
------------------------------------------------------------------------------------------------- */

template <typename T>
struct task_state_t;

/* So, a corutine handle points to a corutine, so a task. A task should be able to be awaited,
hence this task object can A. hold the corutine and B. be awaited to get the corutine return value*/
template<typename T>
struct task {
    using promise_type = task_state_t<T>;
    using handle_t = handle<task_state_t<T>>;

    task() : h(std::noop_coroutine()) {}
    task(handle_t h) : h(h) {}
    task(handle<void> h) : h(handle_t::from_address(h.address())) {}

    /* this is here because we would like to know if a corutine died without ever beeing called */
    task(task &oth) : h(oth.h) { oth.ever_called = true; }
    task(task &&oth) : h(std::move(oth.h)) { oth.ever_called = true; }
    task& operator = (task &oth) { oth.ever_called = true; h = oth.h; return *this; }
    task& operator = (task &&oth) { oth.ever_called = true; h = std::move(oth.h); return *this; }

    bool await_ready() noexcept { return false; }

    /* Those two need to be implemented bellow the task_state */
    template <typename P>
    handle<void> await_suspend(handle<P> caller) noexcept;
    T            await_resume() noexcept;
    pool_wp      get_pool();

    error_e get_err() { return ERROR_OK; };

    bool ever_called = false;
    handle_t h;
};

struct state_t {
    error_e err;
    pool_wp _pool;

    handle<void> caller;
    std::unique_ptr<modif_table_t> pmod_table;
};

template <typename T>
struct task_state_t {
    struct initial_awaiter_t {
        bool await_ready() noexcept                    { return false; }
        void await_suspend(handle<void> self) noexcept {}
        void await_resume() noexcept                   {}
    };

    struct final_awaiter_t {
        bool         await_ready() noexcept                   { return false; }
        handle<void> await_suspend(handle<void> oth) noexcept { return return_next(_pool, oth); }
        void         await_resume() noexcept                  { /* TODO: here some exceptions? */ }

        static handle<void> return_next(pool_wp _pool, handle<void> oth) {
            /* TODO: I guess some cleanup, some callbacks and send the next coreo out */
            return oth;
        }

        pool_wp _pool;
    };

    task<T> get_return_object() { return task<T>{task<T>::handle_t::from_promise(*this)}; }

    initial_awaiter_t initial_suspend() noexcept { return {}; }
    final_awaiter_t   final_suspend() noexcept   { return final_awaiter_t{ ._pool = state._pool }; }
    void              return_value(T&& ret)      { ret.emplace(std::move(ret)); }
    void              return_value(T&  ret)      { ret.emplace(ret); }
    void              unhandled_exception()      { /* TODO: something with exceptions */ }

    state_t state;
    std::variant<std::monostate, T> ret;
};

template <typename T>
template <typename P>
inline handle<void> task<T>::await_suspend(handle<P> caller) noexcept {
    ever_called = true;
    h.promise().state.caller = caller;
    h.promise().state._pool = caller.promise().state._pool;
    /* TODO: modifs inheritance here and modifs call */

    return h;
}

template <typename T>
inline T task<T>::await_resume() noexcept {
    ever_called = true;
    auto pret = std::move(h.promise().pret);
    h.destroy();
    return *pret;
}

template <typename T>
inline pool_wp task<T>::get_pool() {
    return h.promise().state._pool;
}

/* The Pool & Epoll engine
------------------------------------------------------------------------------------------------- */

/* If this is not really needed here we move it bellow */
struct pool_internal_t {
    template <typename T>
    void sched(task<T> task, const std::vector<modif_p>& v) {
        /* First we add all the pmods over the task's pmods (if they pass the inheritance) */
        if (do_sched_modifs(task, v) < 0) {
            return ;
        }
        /* TODO: finaly add it to some sort of queue here */
    }

    run_e run() {
        /* TODO */
        return RUN_OK;
    }

    void push_ready(handle<void> h) {
        /*TODO*/
    }

    handle<void> next_task() {
        /*TODO*/
        return std::noop_coroutine();
    }

    void wait_fd(handle<void> h, int fd, int wait_cond) {
        /* add the file descriptor inside epoll and schedule the corutine for waiting on it */
    }
};

inline pool_t::pool_t() : internal(std::make_unique<pool_internal_t>()) {}

template <typename T>
inline void pool_t::sched(task<T> task, const std::vector<modif_p>& v) {
    internal->sched(task, v);
}

inline run_e pool_t::run() {
    return internal->run();
}

/* Awaiters
------------------------------------------------------------------------------------------------- */

struct yield_awaiter_t {
    bool await_ready() { return false; }

    template <typename P>
    inline handle<void> await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            pool->internal->push_ready(h);
            state = &h.promise().state;
            /* TODO: handle modifs */
            return pool->internal->next_task();
        }
        /* we don't have a pool anymore... */
        // TODO: dbg("This should not happen");
        return std::noop_coroutine();
    }

    void await_resume() {
        /* TODO: handle modifs */
    }

    state_t *state;
};

struct sched_awaiter_t {
    bool await_ready() { return false;}
    void await_resume() {}

    template <typename P>
    bool await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            pool->sched(h);
        }
        else {
            /* TODO: huh? */
        }
        return false;
    }
};

struct get_pool_awaiter {
    template <typename P>
    bool    await_suspend(handle<P> h) { _pool = h.promise().state._pool; return false; }

    bool    await_ready()              { return false; };
    pool_wp await_resume()             { return _pool; }

    pool_wp _pool;
};

/* This is the object with which you wait for a file descriptor, you create it with a valid
wait_cond and fd and co_await on it. The resulting integer must be positive to not be an error */
template <typename T>
struct fd_awaiter_t {
    int wait_cond;  /* set with epoll wait condition */
    int fd;         /* set with waited file descriptor */

    bool await_ready() { return false; };

    template <typename P>
    handle<void> await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            state = &h.promise().state;
            /* TODO: handle modifs */
            error_e err;
            /* in case we can't schedule the fd we log the failure and return the same coro */
            if ((err = pool->internal->wait_fd(h, fd, wait_cond)) < 0) {
                /* TODO: log the fail */
                state->err = err;
                return h;
            }
            return pool->internal->next_task();
        }
        return std::noop_coroutine();
    }

    error_e await_resume() {
        return state->err;
    }

private:
    state_t *state; /* The state of the corutine that called us */
};

/* Semaphores
------------------------------------------------------------------------------------------------- */

struct sem_internal_t {
    sem_internal_t(pool_wp pool, int64_t val) : _pool(pool), val(val) {}

    template <typename P>
    handle<void> await_suspend(handle<P> to_suspend) {
        /* TODO; */
        return std::noop_coroutine();
    }

    bool await_ready() {
        if (val > 0) {
            val--;
            return true;
        }
        return false;
    }

    bool try_dec() {
        /* TODO */
        return false;
    }

    void inc() {
        /* TODO */
    }

private:
    uint64_t val;
    pool_wp _pool;
};

inline sem_t::sem_t(pool_wp pool, int64_t val)
: internal(std::make_unique<sem_internal_t>(pool, val)) {}

inline void                 sem_t::inc() { internal->inc(); }
inline bool                 sem_t::try_dec() { return internal->try_dec(); }
inline bool                 sem_t::await_ready() { return internal->await_ready(); }
inline sem_t::unlocker_t    sem_t::await_resume() { return unlocker_t(*this); }

template <typename P>
inline handle<void>         sem_t::await_suspend(handle<P> t) { return internal->await_suspend(t); }

/* Modif Declarations
------------------------------------------------------------------------------------------------- */

template <typename T>
inline error_e do_sched_modifs(task<T> &task, const std::vector<modif_p>& to_add) {
    for (auto &mp : to_add) {
        modif_p m;
        if (mp->inherit_sched_cbk)
            m = mp->inherit_sched_cbk(mp); 
        if (m)
            task.h.promise()._state.modifs.insert(m);
        /* TODO: more on insertion, like move them  */
    }
    for (auto &mp : task.h.promise()._state.modifs) {
        if (mp.sched_cbk) {
            if (mp.sched_cbk(&task.h.promise()._state, mp) != ERROR_OK) {
                /* TODO: Failed to schedule the corutine, awake all futures */
                return ;
            }
        }
    }
    return ERROR_GENERIC;
}

template <typename T>
inline error_e do_call_modifs(task<T> &task, const std::vector<modif_p>& to_add) {
    return ERROR_GENERIC;
}

/* Functions
------------------------------------------------------------------------------------------------  */

template <typename ...ret_v>
inline task_t wait_all(task<ret_v>... tasks) {
    /* TODO; */
    return task_t{};
}

template <typename T>
inline sched_awaiter_t sched(task<T> to_sched, const std::vector<modif_p>& v) {
    /* TODO */
    return sched_awaiter_t{};
}

template <typename Awaiter>
inline task_t await(Awaiter&& awaiter);

inline yield_awaiter_t yield();

inline task<pool_wp> get_pool() {
    get_pool_awaiter awaiter;
    co_return co_await awaiter;
}

inline task<sem_t> create_sem(int64_t val) {
    auto pool = co_await get_pool();
    co_return sem_t(pool, val);
}

inline task<modif_p> creat_depend();

inline task_t stopfd(int fd);
inline task_t force_stop(int ret);

inline task_t sleep(const std::chrono::milliseconds& us);
inline task_t sleep_us(uint64_t timeo_us);
inline task_t sleep_ms(uint64_t timeo_ms);
inline task_t sleep_s(uint64_t timeo_s);

inline task_t wait_event(int fd, int event);
inline task_t connect(int fd, sockaddr *sa, socklen_t *len);
inline task_t accept(int fd, sockaddr *sa, socklen_t *len);
inline task_t read(int fd, void *buff, size_t len);
inline task_t write(int fd, const void *buff, size_t len);
inline task_t read_sz(int fd, void *buff, size_t len);
inline task_t write_sz(int fd, const void *buff, size_t len);

/* non awaitable functions: */

template <typename T>
inline std::vector<modif_p> &task_modifs(task<T> t);

template <typename T>
inline task<T> add_modif(task<T> t, modif_p mod); 

template <typename T>
inline task<T> rm_modif(task<T> t, modif_p mod);

template <typename T>
inline task<T> creat_future(task<T> t);

inline pool_p  create_pool();
inline sem_t   create_sem(pool_wp pool, int64_t val);
inline modif_p create_modif(const modif_t& modif_template);
inline modif_p creat_timeo(const std::chrono::microseconds& timeo);
inline modif_p creat_killer();
inline error_e sig_killer(modif_p p);

/* Debug stuff
------------------------------------------------------------------------------------------------- */

template <typename T, typename ...Args>
inline task<T> dbg_register_name(task<T> t, const char *fmt, task<T>&&...) {
    /* TODO: */
    /* Probably remember names inside the pool */
    return t;
}

inline modif_p creat_dbg_tracer() {
    /* TODO: */
    return nullptr;
}

/* Obtains the name given to the respective task */
template <typename T>
inline std::string dbg_name(task<T> t) {
    /* TODO: */
    return "";
}

/* The end
------------------------------------------------------------------------------------------------- */

}

// comment this if you don't like it
namespace co = coro;

#endif
