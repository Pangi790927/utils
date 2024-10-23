#ifndef CORO_H
#define CORO_H
/* rewrite of co_utils.h ->
    - CPP library over the CPP corutines
    - Built around epoll (ie, epoll is the only blocking function)
    - Meant to have one pool/thread or enable mutexes on pool (this will allow callbacks and pool
    sharing)
    - Meant to have modifs (callbacks on corutine execution)
*/

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
# error "Don't choose both"
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
it around (practically std::coroutine_handle, but can cast itself into things) */
template<typename RetType> struct task;

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

struct pool_t {
    pool_t();

    void sched(task_t task, const std::vector<modif_p>& v = {}); /* ignores return type */
    run_e run();

    /* Not private, but don't touch, not yours, leave it alone */
    std::unique_ptr<pool_internal_t> internal;
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

    task_t      await_suspend(task_t to_suspend);
    bool        await_ready();
    unlocker_t  await_resume();
private:
    std::unique_ptr<sem_internal_t> internal;
};

struct modif_t {
    /* called when a new corutine is called (co_await on it) */
    std::fucntion<error_e(task_t, modif_t *)> call_cbk;

    /* called when a new corutine that is newly scheduled (co::sched on it) */
    std::fucntion<error_e(task_t, modif_t *)> sched_cbk;

    /* called when the return is valid (co_return) */
    std::fucntion<error_e(task_t, modif_t *)> exit_cbk;

    /* called on a coroutine before it starts/stops execution (co_await, ?co_yield? something else)*/
    std::fucntion<error_e(task_t, modif_t *)> leave_cbk;
    std::fucntion<error_e(task_t, modif_t *)> enter_cbk;

    /* awaiting a file descriptor */
    std::fucntion<error_e(task_t, int fd, int wait_cond, modif_t *)> waitfd_cbk;
    std::fucntion<error_e(task_t, int fd, int res, int err_no, modif_t *)> unwaitfd_cbk;

    /* awaiting on a semaphore */
    std::function<error_e(task_t, sem_t *, modif_t *)> waitsem_cbk;
    std::function<error_e(task_t, sem_t *, modif_t *)> unwaitsem_cbk;

    /* the following callbacks specify how this modifier is inherited on diverse calls, if the
    function does not exist, the mod is not inherited, if it exists, it is called with itself and
    the result is used further on. You can at those points decide to create a new modifier for the
    call or keep the old one.
    When called, all the caller's modifs are inherited and it's modifs(if any) are kept.  */
    std::fucntion<modif_p(modif_p)> inherit_call_cbk;
    std::fucntion<modif_p(modif_p)> inherit_sched_cbk;

    /* those are called by the contructor/deconstructor. */
    std::fucntion<void(modif_t *)> init_cbk;
    std::fucntion<void(modif_t *)> uninit_cbk;

    /* If a modif is added twice(same address) then this callback is called. */
    std::fucntion<void(modif_t *)> same_cbk;

    modif_t() { init_cbk(this); };
    ~modif_t() { uninit_cbk(this); };

    /* do whatever you want with this */
    void *ctx = NULL;

    /* don't ever touch this */
    void *internal = NULL;
};

/* Pool & Sched functions:
------------------------------------------------------------------------------------------------  */

inline pool_p create_pool();

/* gets the pool of the current corutine, necesary if you want to sched corutines from callbacks.
Don't forget you need multithreading enabled if you want to schedule from another thread. */
inline task<pool_wp> get_pool();

/* This does not stop the curent corutine, it only schedules the task, but does not yet run it */
template <typename ret_t>
inline sched_awaiter_t sched(task<ret_t> to_sched, const std::vector<modif_p>& v = {});

/* This stops the current coroutine from running and places it at the end of the ready queue. */
inline yield_awaiter_t yield();

/* Modifications
------------------------------------------------------------------------------------------------  */

inline modif_p create_modif(const modif_t& modif_template);

/* This returns the internal set that holds the different modifications of the task. */
template <typename ret_t>
inline std::set<client_wp, std::owner_less<client_wp>>> &task_modifs(task<ret_t> t);

/* This adds a modifier to the task and returns the task */
template <typename ret_t>
inline task<ret_t> add_modif(task<ret_t> t, modif_p mod);

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
template <typename ret_t>
inline task<ret_t> creat_future(task<ret_t> t);

/* wait for all the tasks to finish, the return value can be found in the respective task, killing
one kills all (sig_killer installed in all). The inheritance is the same as with 'call'. */
template <typename ...ret_v>
inline task_t wait_all(task_t<ret_v>... tasks);

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
template <typename ret_t, typename ...Args>
inline task<ret_t> dbg_register_name(task<ret_t> t, const char *fmt, task<ret_t>&&...);

/* creates a modifier that traces things from corutines, mostly a convenience function, it also
uses the log_str function, or does nothing else if it isn't defined. If you don't like the
verbosity, be free to null any callback you don't care about. */
inline modif_p creat_dbg_tracer();

/* Obtains the name given to the respective task */
template <typename ret_t>
inline std::string dbg_name(task<ret_t> t);

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

/* The Pool
------------------------------------------------------------------------------------------------- */

struct pool_internal_t {
    void sched(task_t task, const std::vector<modif_p>& v) {

    }

    run_e run() {

    }
};

inline pool_t pool() : internal(std::make_unique<pool_internal_t>()) {}

inline void pool_t::sched(task_t task, const std::vector<modif_p>& v = {}) {
    internal->sched(task, v);
}

inline run_e run() {
    return internal->run();
}

/* The Task & Awaiters
------------------------------------------------------------------------------------------------- */

/* The promise */
struct task_state_t {

};

template<typename RetType> struct task {
    
};

struct yield_awaiter_t {

};

struct sched_awaiter_t {

};

/* File descriptors
------------------------------------------------------------------------------------------------- */

/* This is the object with which you wait for a file descriptor, you create it with a valid
wait_cond and fd and co_await on it. The resulting integer must be positive to not be an error */
template <typename ret_t>
struct fd_awaiter_t {
    int wait_cond;  /* set with epoll wait condition */
    int fd;         /* set with waited file descriptor */

    task_t await_suspend(task_t t) {
        /* TODO: wait the thing */
    }

    bool await_ready() {
        return false;
    };

    error_e await_resume() {
        return caller->err;
    }

private:
    task_t caller; /* who to wake up */
    pool_wp _pool; /* the pool that holds this coroutine */
};

/* Semaphore
------------------------------------------------------------------------------------------------- */

struct sem_internal_t {
    sem_internal_t(pool_wp pool, int64_t val) : _pool(pool), val(val) {}

    task_t await_suspend(task_t to_suspend) {
        /* TODO; */
    }

    bool await_ready() {
        if (val > 0) {
            val--;
            return true;
        }
        return false;
    }

private:
    uint64_t val;
    pool_wp _pool;
};

inline sem_t::sem_t(pool_wp pool, int64_t val = 0)
: internal(std::make_unique<sem_internal_t>(pool, val)) {}

inline void         sem_t::inc() { internal->inc(); }
inline bool         sem_t::try_dec() { return internal->try_dec(); }
inline task_t       sem_t::await_suspend(task_t to_suspend) { return internal->await_suspend(); }
inline bool         sem_t::await_ready() { return internal->await_ready(); }
inline unlocker_t   sem_t::await_resume() { return unlocker_t(*this); }

/* Functions
------------------------------------------------------------------------------------------------  */

template <typename ...ret_v>
inline task_t wait_all(task_t<ret_v>... tasks);

template <typename ret_t>
inline sched_awaiter_t sched(task<ret_t> to_sched, const std::vector<modif_p>& v = {});

template <typename Awaiter>
inline task_t await(Awaiter&& awaiter);

inline yield_awaiter_t yield();
inline task<pool_wp> get_pool();

inline task<sem_t> create_sem(int64_t val = 0) {
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

template <typename ret_t>
inline std::set<client_wp, std::owner_less<client_wp>>> &task_modifs(task<ret_t> t);

template <typename ret_t>
inline task<ret_t> add_modif(task<ret_t> t, modif_p mod); 

template <typename ret_t>
inline task<ret_t> creat_future(task<ret_t> t);

inline pool_p  create_pool();
inline sem_t   create_sem(pool_wp pool, int64_t val = 0);
inline modif_p create_modif(const modif_t& modif_template);
inline modif_p creat_timeo(const std::chrono::microseconds& timeo);
inline modif_p creat_killer();
inline error_e sig_killer(modif_p p);

/* Debug stuff
------------------------------------------------------------------------------------------------- */

template <typename ret_t, typename ...Args>
inline task<ret_t> dbg_register_name(task<ret_t> t, const char *fmt, task<ret_t>&&...) {
    /* TODO: */
    /* Probably remember names inside the pool */
}

inline modif_p creat_dbg_tracer() {
    /* TODO: */
}

/* Obtains the name given to the respective task */
template <typename ret_t>
inline std::string dbg_name(task<ret_t> t) {
    /* TODO: */
}

/* The end
------------------------------------------------------------------------------------------------- */

}

// comment this if you don't like it
using co = coro;

#endif
