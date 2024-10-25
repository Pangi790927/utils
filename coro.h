#ifndef CORO_H
#define CORO_H

/* TODO:
    - write the fd stuff
    - add debug modifs, things are already breaking and I don't know why
    - consider implementing co_yield
    - consider implementing exception handling
    - write the tutorial at the start of this file
    - speed optimizations
    - add the licence
    - check the comments
    - check the review again
*/

/* TODO:
    valgrind --tool=callgrind ./a.out
    kcachegrind callgrind.out.1955178

    - take into consideration keeping a cache of allocations because it seems that this hurts
    the library, the idea is to have some objects, like shared pointers, allocated from the start,
    such that the library won't spend so much time on allocating things that are re-allocated
    constantly. one such example is _awake_one in sem_internal_t */

/* rewrite of co_utils.h ->
    - CPP library over the CPP corutines
    - Built around epoll (ie, epoll is the only blocking function)
    - Meant to have one pool/thread or enable mutexes on pool (this will allow callbacks and pool
    sharing)
    - Meant to have modifs (callbacks on corutine execution)
*/

#include <memory>
#include <vector>
#include <chrono>
#include <coroutine>
#include <functional>
#include <variant>
#include <unordered_set>
#include <map>
#include <deque>
#include <set>
#include <source_location>
#include <cinttypes>
#include <list>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>

/* The maximum amount of concurent timers that can be awaited */
#ifndef CORO_MAX_TIMER_POOL_SIZE
# define CORO_MAX_TIMER_POOL_SIZE 64
#endif

/* Set if a pool can be available in more than one thread, necesary if you want to transfer the pool
to a callback that will be called from another thread */
#ifndef CORO_ENABLE_MULTITHREADS
# define CORO_ENABLE_MULTITHREADS false
#endif

/* Enable the corutines to catch and throw exceptions */
#ifndef CORO_ENABLE_EXCEPTIONS
# define CORO_ENABLE_EXCEPTIONS false
#endif

/* If set to true, will enable the debug callbacks */
#ifndef CORO_ENABLE_CALLBACKS
# define CORO_ENABLE_CALLBACKS false
#endif

/* If set to true, will enable the logging callback */
#ifndef CORO_ENABLE_LOGGING
# define CORO_ENABLE_LOGGING true
#endif

/* If set to true, makes names for corutines */
#ifndef CORO_ENABLE_DEBUG_NAMES
# define CORO_ENABLE_DEBUG_NAMES false
#endif

/* If set, will create a debug tracer and it will add it to all coroutines */
#ifndef CORO_ENABLE_DEBUG_TRACE_ALL
# define CORO_ENABLE_DEBUG_TRACE_ALL false
#endif

/* TODO: make this a thing */
/* If set, will throw exceptions wherever only a warning would be logged. This would throw if the
pool no longer exists, if a semaphore is signaled after it was uninitialized, if a semaphore was
unitialized but others are still waiting on it, if an awaitable was created but never awaited, etc. */
#ifndef CORO_ENABLE_WARNING_EXCEPTIONS
# define CORO_ENABLE_WARNING_EXCEPTIONS false
#endif

/* TODO: on default construct, make sure the return value can be default constructed */
#if !defined(CORO_KILL_DEFAULT_CONSTRUCT) && !defined(CORO_KILL_RAISE_EXCEPTION)
# define CORO_KILL_DEFAULT_CONSTRUCT true
# define CORO_KILL_RAISE_EXCEPTION false
#endif
#if defined(CORO_KILL_RAISE_EXCEPTION) && !defined(CORO_KILL_DEFAULT_CONSTRUCT)
#define CORO_KILL_DEFAULT_CONSTRUCT false
#endif
#if defined(CORO_KILL_DEFAULT_CONSTRUCT) && !defined(CORO_KILL_RAISE_EXCEPTION)
#define CORO_KILL_RAISE_EXCEPTION false
#endif

#if CORO_KILL_DEFAULT_CONSTRUCT && CORO_KILL_RAISE_EXCEPTION
# error "Don't choose both"
#endif

#if CO_KILL_RAISE_EXCEPTION && !CORO_ENABLE_EXCEPTIONS
# error "Can't have CO_KILL_RAISE_EXCEPTION without CORO_ENABLE_EXCEPTIONS"
#endif

#if CORO_ENABLE_WARNING_EXCEPTIONS && !CORO_ENABLE_EXCEPTIONS
# error "Can't have CORO_ENABLE_WARNING_EXCEPTIONS without CORO_ENABLE_EXCEPTIONS"
#endif

/* If CORO_ENABLE_DEBUG_NAMES you can also define CORO_REGNAME and use it to register a
corutine's name (a coro::task<T>, std::coroutine_handle or void *) */
#if CORO_ENABLE_DEBUG_NAMES
# ifndef CORO_REGNAME
#  define CORO_REGNAME(thv)   coro::dbg_register_name((thv), "%20s:%5d:%s", __FILE__, __LINE__, #thv)
# endif
#else
# define CORO_REGNAME(thv)    thv
#endif /*CORO_ENABLE_DEBUG_NAMES*/

namespace coro {

constexpr int MAX_TIMER_POOL_SIZE = CORO_MAX_TIMER_POOL_SIZE;

/* corutine return type, this is the result of creating a corutine, you can await it and also pass
it around (practically holds a std::coroutine_handle and can be awaited call the coro and to get the
return value) */
template<typename T> struct task;

/* A pool is the shared state between all corutines. This object holds the epoll handler, timers,
queues, etc. Each corutine has a pointer to this object. You can see this object as an instance or
proxy of epoll. */
struct pool_t;

/* Modifs, corutine modifications. Those modifications controll the way a corutine behaves when
awaited or when spawning a new corutine from it. Those modifications can be inherited, making all
the corutines in the call sub-tree behave in a similar way. For example: adding a timeouts, for this
example, all the corutines that are on the same call-path(not spawn-path) would have a timer, such
that the original call would not exist for more than X time units. (+/- the code between awaits) */
struct modif_t;

/* This is a private table that holds the modifications inside the corutine state */
struct modif_table_t;

/* TODO: think this better this time, including better lifetime */
/* This is a semaphore working on a pool. It can be awaited to decrement it's count and .rel()
increments it's count from wherever. More bellow. */
struct sem_t;

/* Internal state of corutines that is independent of the return value of the corutine. */
struct state_t;

/* used pointers, so _p marks a shared_pointer and a _wp marks a weak pointer. */
using sem_p = std::shared_ptr<sem_t>;
using sem_wp = std::weak_ptr<sem_t>;
using pool_p = std::shared_ptr<pool_t>;
using pool_wp = std::weak_ptr<pool_t>;
using modif_p = std::shared_ptr<modif_t>;
using modif_wp = std::weak_ptr<modif_t>;
using modif_table_p = std::shared_ptr<modif_table_t>;
using modif_table_wp = std::weak_ptr<modif_table_t>;

/* Tasks errors */
enum error_e : int32_t {
    ERROR_OK      =  0,
    ERROR_GENERIC = -1, /* generic error, can use log_str to find the error, or sometimes errno */
    ERROR_TIMEOUT = -2, /* the error comes from a modif, namely a timeout */
    ERROR_WAKEUP  = -3, /* the error comes from force awaking the awaiter */
    ERROR_USER    = -4, /* the error comes from a modif, namely an user defined modif, users can
                        use this if they wish to return from modif cbks */
    ERROR_DEPEND  = -5, /* the error comes from a depend modif, i.e. depended function failed */
};

/* Event loop running errors */
enum run_e : int32_t {
    RUN_OK,      /* when the pool stopped because it ran out of things to do */
    RUN_ERRORED, /* comes from epoll errors */
    RUN_ABORTED, /* if a corutine is resumed without a pool */
    RUN_STOPPED, /* can be re-run (comes from force_stop) */
};

enum modif_e : int32_t {
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

/* all the internal tasks return this, namely error_e but casted to int (a lot of my old code depends
on this and I also find it in theme, as all the linux functions that I use tend to return ints) */
using task_t = task<int>;

/* some internal structures */
struct yield_awaiter_t;
struct sem_awaiter_t;
struct pool_internal_t;
struct sem_internal_t;

template <typename T>
struct sched_awaiter_t;

/* exception error if the lib is configured to throw exception on modif termination */
#if CO_EXCEPT_ON_KILL
/* TODO: the exception that will be thrown by the killing of a courutine */
#endif

/* This is the state struct that each corutine has */
struct state_t {
    error_e err = ERROR_OK;             /* holds the error return in diverse cases */
    pool_wp _pool;                      /* the pool of this coro */
    modif_table_p modif_table;          /* we allocate a table only if there are mods */

    state_t *caller_state = nullptr;    /* this holds the caller's state, and with it the return path */ 
    std::coroutine_handle<void> self;   /* the coro's self handle */

    std::shared_ptr<void> user_ptr;     /* this is a pointer that the user can use for whatever
                                        he feels like. This library will not touch this pointer */
};

struct pool_t {
    /* you use the pointer made by create_pool */
    pool_t(pool_t& sem) = delete;
    pool_t(pool_t&& sem) = delete;
    pool_t &operator = (pool_t& sem) = delete;
    pool_t &operator = (pool_t&& sem) = delete;

    /* Same as coro::sched, but used outside of a corutine, based on the pool */
    template <typename T>
    void sched(task<T> task, const std::vector<modif_p>& v = {}); /* ignores return type */

    /* runs the event loop */
    run_e run();

    /* Not private, but don't touch it, not yours, leave it alone, I don't want to befriend the
    whole lib for it */
    std::unique_ptr<pool_internal_t> internal;

protected:
    friend inline pool_p create_pool();
    pool_t();
};

/* TODO: if the semaphore dies, the things waiting on it will still wait, that doesn't seem ok,
maybe wake them up? give a warning? */
struct sem_t {
    struct unlocker_t{ /* compatibility with guard objects ex: std::lock_guard guard(co_await s); */
        sem_wp _sem;
        unlocker_t(sem_wp _sem) : _sem(_sem) {}
        void lock() {}
        void unlock() { if (auto sem = _sem.lock()) sem->signal(); }
    };

    /* you use the pointer made by create_sem */
    sem_t(sem_t& sem) = delete;
    sem_t(sem_t&& sem) = delete;
    sem_t &operator = (sem_t& sem) = delete;
    sem_t &operator = (sem_t&& sem) = delete;

    sem_awaiter_t wait(); /* if awaited returns unlocker_t{} */
    error_e signal(); /* returns error if the pool disapeared */
    bool try_dec();

    /* again, don't touch, same as pool */
    std::shared_ptr<sem_internal_t> internal;

protected:
    friend inline sem_p create_sem(pool_wp, int64_t);
    friend inline task<sem_p> create_sem(int64_t);
    sem_t(pool_wp pool, int64_t val = 0);
};

/* This is mostly internal, but you can also use it to be able to check if someone is still waiting
on a semaphore */
using sem_waiter_handle_wp =
        std::weak_ptr<                          /* if this pointer is not available, the waiter was
                                                   evicted from the waiters list */
            std::list<                          /* List with the semaphore waiters */
                std::pair<
                    std::coroutine_handle<void>,/* The corutine handle */
                    std::shared_ptr<void>       /* Where the shared_ptr is actually stored */
                >
            >::iterator                         /* iterator in the respective list */
        >;

/* A modif is a callback for a specifi stage in the corutine's flow */
struct modif_t {
    std::variant<
        std::function<error_e(state_t *)>,                        /* call_cbk */
        std::function<error_e(state_t *)>,                        /* sched_cbk */
        std::function<error_e(state_t *)>,                        /* exit_cbk */
        std::function<error_e(state_t *)>,                        /* leave_cbk */
        std::function<error_e(state_t *)>,                        /* enter_cbk */
        std::function<error_e(state_t *, int fd, int wait_cond)>, /* wait_fd_cbk */
        std::function<error_e(state_t *, int fd)>,                /* unwait_fd_cbk */

         /* wait_sem_cbk - OBS: the std::shared_ptr<void> part can be ignored, it's internal */
        std::function<error_e(state_t *, sem_wp, sem_waiter_handle_wp)>,

         /* unwait_sem_cbk */
        std::function<error_e(state_t *, sem_wp, sem_waiter_handle_wp)>
    > cbk;

    modif_e type = CO_MODIF_COUNT;
    modif_flags_e flags = CO_MODIF_INHERIT_ON_CALL;
};

/* Pool & Sched functions:
------------------------------------------------------------------------------------------------  */

inline pool_p create_pool();

/* gets the pool of the current corutine, necesary if you want to sched corutines from callbacks.
Don't forget you need multithreading enabled if you want to schedule from another thread. */
inline task<pool_wp> get_pool();

/* This does not stop the curent corutine, it only schedules the task, but does not yet run it.
Modifs duplicates are eliminated. */
template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const std::vector<modif_p>& v = {});

/* This stops the current coroutine from running and places it at the end of the ready queue. */
inline yield_awaiter_t yield();

/* Modifications
------------------------------------------------------------------------------------------------  */

template <typename Cbk>
inline modif_p create_modif(modif_e type, Cbk&& cbk, modif_flags_e flags);

/* This returns the internal vec that holds the different modifications of the task. Changing them
here is ill-defined */
template <typename T>
inline std::vector<modif_p> task_modifs(task<T> t);

/* This adds a modifier to the task and returns the task. Duplicates will be ignored */
template <typename T>
inline task<T> add_modifs(task<T> t, std::set<modif_p> mods); /* not awaitable */

/* Removes modifier from the vector */
template <typename T>
inline task<T> rm_modifs(task<T> t, std::set<modif_p> mods); /* not awaitable */

/* same as above, but adds them for the current task */
inline task<std::vector<modif_p>> task_modifs();
inline task_t add_modifs(std::set<modif_p> mods); 
inline task_t rm_modifs(std::set<modif_p> mods);

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
inline task_t sleep(const std::chrono::microseconds& us);

/* Flow Controll:
------------------------------------------------------------------------------------------------  */

/* Observation: killed corutines, timed, result of functions that depend on others, futures, all
of those will not be able to construct their return type if they are killed so those
need to be handled, so you must choose that by defining one of the two: CORO_KILL_DEFAULT_CONSTRUCT,
CORO_KILL_RAISE_EXCEPTION ?TODO:CHECK?

Those that return error_e will return their apropiate error in this case, the rest will either
have the error returned in the exception or set in the task/awaiter object.

If CORO_KILL_DEFAULT_CONSTRUCT, then the return value will be the default constructed type.(except if
type is error_e).
*/

/* Creates semaphores, those need the pool to exist */
inline sem_p create_sem(pool_wp pool, int64_t val = 0);
inline task<sem_p> create_sem(int64_t val = 0);

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
    auto fut = coro::create_future(t)
    co_await coro::sched(t);
    ...
    co_await fut; // returns the value of co_task once it has finished executing 
*/
template <typename T>
inline task<T> creat_future(task<T> t); /* not awaitable */

/* wait for all the tasks to finish, the return value can be found in the respective task, killing
one kills all (sig_killer installed in all). The inheritance is the same as with 'call'. */
template <typename ...ret_v>
inline task_t wait_all(task<ret_v>... tasks);

/* causes the running pool::run to stop, the function will stop at this point, can be resumed with
another run */
inline task_t force_stop(int ret);

/* EPOLL & Linux Specific:
------------------------------------------------------------------------------------------------  */

/* TODO: describe their usage */
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

    co_await coro::stopfd(fd);
    close(fd);
*/
inline task_t stopfd(int fd);

/* Debug Interfaces:
------------------------------------------------------------------------------------------------  */

struct dbg_format_helper_t;

template <typename... Args>
inline void dbg(dbg_format_helper_t dfmt, Args&&... args);

/* Registers a name for a given task */
template <typename T, typename ...Args>
inline task<T> dbg_register_name(task<T> t, const char *fmt, Args&&...);

template <typename P, typename ...Args>
inline std::coroutine_handle<P> dbg_register_name(std::coroutine_handle<P> h,
        const char *fmt, Args&&...);

template <typename ...Args>
inline void * dbg_register_name(void *addr, const char *fmt, Args&&...);

/* creates a modifier that traces things from corutines, mostly a convenience function, it also
uses the log_str function, or does nothing else if it isn't defined. If you don't like the
verbosity, be free to null any callback you don't care about. */
inline modif_p creat_dbg_tracer();

/* Obtains the name given to the respective task, handle or address */
template <typename T>
inline std::string dbg_name(task<T> t);

template <typename P>
inline std::string dbg_name(std::coroutine_handle<P> h);

inline std::string dbg_name(void *v);


/* formats a string using the C snprintf, similar in functionality to snprintf+std::format, in the
version of g++ that I'm using std::format is not available  */
template <typename... Args>
inline std::string dbg_format(const char *fmt, Args&& ...args);

/* corutine callbacks for diverse points in the code, if in a multithreaded environment, you must
take all the locks before rewriting those. The pool locks are held when those callbacks are called
*/
#if CO_ENABLE_CALLBACKS
/* add callbacks here */
inline std::function<void(void)> on_call;
#endif /* CO_ENABLE_CALLBACKS */

/* calls log_str to save the log string */
#if CORO_ENABLE_LOGGING
inline std::function<int(const std::string&)> log_str =
        [](const std::string& msg){ return printf("%s", msg.c_str()); };
#endif

/* IMPLEMENTATION 
=================================================================================================
=================================================================================================
================================================================================================= */

/* Helpers
------------------------------------------------------------------------------------------------- */

template <typename P>
using handle = std::coroutine_handle<P>;

struct dbg_format_helper_t {
    std::string fmt;
    std::source_location loc;

    template <typename Arg>
    dbg_format_helper_t(Arg&& arg,
            std::source_location const& loc = std::source_location::current())
    : fmt{std::forward<Arg>(arg)}, loc{loc} {}
};

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key)) != std::forward<T>(data_struct).end();
}

using sem_wait_list_t = std::list<std::pair<handle<void>, std::shared_ptr<void>>>;
using sem_wait_list_it = sem_wait_list_t::iterator;

/* Modifs Part
------------------------------------------------------------------------------------------------- */

struct modif_table_t {
    std::array<std::vector<modif_p>, CO_MODIF_COUNT> table;
};

inline size_t get_modif_table_sz(modif_table_p ptable) {
    if (!ptable)
        return 0;
    size_t ret = 0;
    for (auto &t : ptable->table)
        ret += t.size();
    return ret;
}

inline modif_table_p create_modif_table(const std::vector<modif_p>& to_add) {
    modif_table_p ret = std::make_shared<modif_table_t>();
    std::unordered_set<modif_p> existing;
    for (auto &m : to_add) {
        if (m && !has(existing, m)) {
            existing.insert(m); /* we silently eliminate duplicates, as per description */
            ret->table[m->type].push_back(m);
        }
    }
    if (get_modif_table_sz(ret) == 0)
        return nullptr;
    return ret;
}

inline void inherit_modifs(state_t *state, modif_table_p parent_table, modif_flags_e inherit_place) {
    if (!parent_table)
        return ;
    modif_table_p new_table = state->modif_table;
    std::unordered_set<modif_p> existing;
    if (new_table) {
        /* should be rare */
        for (auto &cbk_table : new_table->table)
            for (auto &modif : cbk_table)
                existing.insert(modif);
    }
    if (!new_table)
        new_table = std::make_shared<modif_table_t>();
    for (auto& cbk_table : parent_table->table)
        for (auto& modif : cbk_table)
            if (modif && !has(existing, modif) && (modif->flags & inherit_place))
                new_table->table[modif->type].push_back(modif);
    if (get_modif_table_sz(new_table) != 0)
        state->modif_table = new_table;   
}

template <modif_e cbk_id, typename ...Args>
inline error_e do_generic_modifs(state_t *state, Args&& ...args) {
    if (auto modif_table = state->modif_table) {
        for (auto &modif : modif_table->table[cbk_id]) {
            error_e ret = std::get<cbk_id>(modif->cbk)(state, args...);
            if (ret != ERROR_OK) {
                /* If any callback returned an error, we stop their execution and return the error.
                We also set in the task that the respective error happened. */
                state->err = ret;
                return ret;
            }
        }
    }
    return ERROR_OK;
}

inline error_e do_sched_modifs(state_t *state, modif_table_p parent_table) {
    inherit_modifs(state, parent_table, CO_MODIF_INHERIT_ON_SCHED);
    return do_generic_modifs<CO_MODIF_SCHED_CBK>(state);
}

inline error_e do_call_modifs(state_t *state, modif_table_p parent_table) {
    inherit_modifs(state, parent_table, CO_MODIF_INHERIT_ON_CALL);
    return do_generic_modifs<CO_MODIF_CALL_CBK>(state);
}

inline error_e do_leave_modifs(state_t *state) {
    return do_generic_modifs<CO_MODIF_LEAVE_CBK>(state);
}

inline error_e do_entry_modifs(state_t *state) {
    return do_generic_modifs<CO_MODIF_ENTER_CBK>(state);
}

inline error_e do_exit_modifs(state_t *state) {
    return do_generic_modifs<CO_MODIF_EXIT_CBK>(state);
}

inline error_e do_wait_fd_modifs(state_t *state, int fd, int wait_cond) {
    return do_generic_modifs<CO_MODIF_WAIT_FD_CBK>(state, fd, wait_cond);
}

inline error_e do_unwait_fd_modifs(state_t *state, int fd) {
    return do_generic_modifs<CO_MODIF_UNWAIT_FD_CBK>(state, fd);
}

inline error_e do_wait_sem_modifs(state_t *state, sem_wp _sem, sem_waiter_handle_wp _it) {
    return do_generic_modifs<CO_MODIF_WAIT_SEM_CBK>(state, _sem, _it);
}

inline error_e do_unwait_sem_modifs(state_t *state, sem_wp _sem, sem_waiter_handle_wp _it) {
    return do_generic_modifs<CO_MODIF_UNWAIT_SEM_CBK>(state, _sem, _it);
}

/* considering you may want to create a modif at runtime this seems to be the best way */
template <typename Cbk>
inline modif_p create_modif(modif_e type, Cbk&& cbk, modif_flags_e flags) {
    modif_p ret = modif_p(new modif_t{
        .type = type,
        .flags = flags,
    });

    switch (type) {
        case CO_MODIF_CALL_CBK:
            ret->cbk.emplace<CO_MODIF_CALL_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_SCHED_CBK:
            ret->cbk.emplace<CO_MODIF_SCHED_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_EXIT_CBK:
            ret->cbk.emplace<CO_MODIF_EXIT_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_LEAVE_CBK:
            ret->cbk.emplace<CO_MODIF_LEAVE_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_ENTER_CBK:
            ret->cbk.emplace<CO_MODIF_ENTER_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_WAIT_FD_CBK:
            ret->cbk.emplace<CO_MODIF_WAIT_FD_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_UNWAIT_FD_CBK:
            ret->cbk.emplace<CO_MODIF_UNWAIT_FD_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_WAIT_SEM_CBK:
            ret->cbk.emplace<CO_MODIF_WAIT_SEM_CBK>(std::forward<Cbk>(cbk));
            break;

        case CO_MODIF_UNWAIT_SEM_CBK:
            ret->cbk.emplace<CO_MODIF_UNWAIT_SEM_CBK>(std::forward<Cbk>(cbk));
            break;

        default:
            return nullptr;
            break;
    }
    return ret;
}

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

    /* Those need to be implemented bellow the task_state */
    template <typename P>
    handle<void> await_suspend(handle<P> caller) noexcept;
    T            await_resume() noexcept;
    pool_wp      get_pool();

    error_e get_err() { return ERROR_OK; };

    bool ever_called = false;
    handle_t h;
};

/* has to know about pool, will be implemented bellow the pool, but it is required here and to be
accesible to others that need to clean the corutine (for example in modifs, if the tasks need
killing) */
inline handle<void> final_awaiter_cleanup(pool_wp _pool, state_t *ending_task_state,
        handle<void> ending_task_handle);

template <typename T>
struct task_state_t {
    struct final_awaiter_t {
        bool         await_ready() noexcept                   { return false; }
        void         await_resume() noexcept                  { /* TODO: here some exceptions? */ }
        
        handle<void> await_suspend(handle<task_state_t<T>> ending_task) noexcept {
            return final_awaiter_cleanup(_pool, &ending_task.promise().state, ending_task);
        }

        pool_wp _pool;
    };

    task<T> get_return_object() {
        typename task<T>::handle_t h = task<T>::handle_t::from_promise(*this);
        state.self = h;
        return task<T>{h};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    final_awaiter_t     final_suspend() noexcept   { return final_awaiter_t{ ._pool = state._pool }; }
    void                unhandled_exception()      { /* TODO: something with exceptions */ }

    template <typename R>
    void return_value(R&& ret) {
        /* TODO: maybe if this coro is of our type it should also take into consideration the
        return value of the state.err? (or maybe better -> think more about that error thing) */
        _ret.template emplace<T>(std::forward<R>(ret));
    }

    state_t state;
    std::variant<std::monostate, T> _ret;
};

template <typename T>
template <typename P>
inline handle<void> task<T>::await_suspend(handle<P> caller) noexcept {
    ever_called = true;

    h.promise().state.caller_state = &caller.promise().state;
    h.promise().state._pool = caller.promise().state._pool;

    if (do_call_modifs(&this->h.promise().state, caller.promise().state.modif_table) != ERROR_OK) {
        return caller;
    }

    return h;
}

template <typename T>
inline T task<T>::await_resume() noexcept {
    ever_called = true;
    auto ret = std::get<T>(h.promise()._ret);
    h.destroy();
    return ret;
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
    void sched(task<T> task, modif_table_p parent_table) {
        /* first we give our new task the pool */
        task.h.promise().state._pool = _pool;

        /* second we call our callbacks on it because it is now scheduled */
        if (do_sched_modifs(&task.h.promise().state, parent_table) != ERROR_OK) {
            return ;
        }

        /* third, we add the task to the pool */
        ready_tasks.push_back(task.h);
    }

    run_e run() {
        dbg_register_name(task_t{std::noop_coroutine()}, "std::noop_coroutine");

        auto h = next_task();
        if (h == std::noop_coroutine())
            return RUN_OK;

        ret_val = RUN_ABORTED;
        h.resume();
        return ret_val;
    }

    void push_ready(handle<void> h) {
        ready_tasks.push_back(h);
    }

    handle<void> next_task() {
        if (!ready_tasks.empty()) {
            auto ret = ready_tasks.front();
            ready_tasks.pop_front();
            return ret;
        }
        /*TODO: else, try to ask the fd engine for a new coro */

        /* we have nothing else to do, we resume the pool_t::run */
        ret_val = RUN_OK;
        return std::noop_coroutine();
    }

    void wait_fd(handle<void> h, int fd, int wait_cond) {
        /* add the file descriptor inside epoll and schedule the corutine for waiting on it */
    }

    run_e ret_val;
    pool_wp _pool; /* I know this is uggly, but I didn't find another solution to hide redundant
                      functions from pool_t without this strange indirection; */
private:
    std::deque<handle<void>> ready_tasks;
};

inline handle<void> final_awaiter_cleanup(pool_wp _pool, state_t *ending_task_state,
        handle<void> ending_task_handle)
{
    /* not sure if I should do something with the return value ... */
    state_t *caller_state = ending_task_state->caller_state;
    do_exit_modifs(ending_task_state);

    if (caller_state) {
        do_entry_modifs(caller_state);
        return caller_state->self;
    }
    else if (auto pool = ending_task_state->_pool.lock()) {
        /* If the task that we are final_awaiting has no caller, then it is the moment to destroy it,
        no one needs it's return value(futures?). Else it will be destroyed by the caller. */
        /* TODO: maybe the future can just simply create a task and set it's coro as the continuation
        here? Such that the future continues in the caller_state branch? */
        ending_task_handle.destroy();
        return pool->internal->next_task();
    }
    else {
        dbg("Failed to aquire the pool");
        return std::noop_coroutine();
    }
}

inline pool_t::pool_t() : internal(std::make_unique<pool_internal_t>()) {}

template <typename T>
inline void pool_t::sched(task<T> task, const std::vector<modif_p>& v) {
    internal->sched(task, create_modif_table(v));
}

inline run_e pool_t::run() {
    return internal->run();
}

inline pool_p create_pool() {
    auto ret = pool_p(new pool_t);
    ret->internal->_pool = ret;
    return ret;
}

/* Awaiters
------------------------------------------------------------------------------------------------- */

struct yield_awaiter_t {
    yield_awaiter_t() {}
    yield_awaiter_t(const yield_awaiter_t &oth) = delete;
    yield_awaiter_t &operator = (const yield_awaiter_t &oth) = delete;
    yield_awaiter_t(yield_awaiter_t &&oth) = delete;
    yield_awaiter_t &operator = (yield_awaiter_t &&oth) = delete;

    bool await_ready() { return false; }

    template <typename P>
    inline handle<void> await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            pool->internal->push_ready(h);
            state = &h.promise().state;
            do_leave_modifs(state);
            return pool->internal->next_task();
        }
        /* we don't have a pool anymore... */
        dbg("yield failed because we don't have a pool anymore");
        return std::noop_coroutine();
    }

    void await_resume() {
        do_entry_modifs(state);
    }

    state_t *state;
};

template <typename T>
struct sched_awaiter_t {
    sched_awaiter_t(task<T> t, modif_table_p table) : t(t), table(table) {}
    sched_awaiter_t(const sched_awaiter_t &oth) = delete;
    sched_awaiter_t &operator = (const sched_awaiter_t &oth) = delete;
    sched_awaiter_t(sched_awaiter_t &&oth) = delete;
    sched_awaiter_t &operator = (sched_awaiter_t &&oth) = delete;

    bool await_ready() { return false;}
    void await_resume() {}

    template <typename P>
    bool await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            pool->internal->sched(t, table);
        }
        else {
            /* TODO: huh? */
        }
        return false;
    }

    task<T> t;
    modif_table_p table;
};

struct get_pool_awaiter_t {
    get_pool_awaiter_t() {}
    get_pool_awaiter_t(const get_pool_awaiter_t &oth) = delete;
    get_pool_awaiter_t &operator = (const get_pool_awaiter_t &oth) = delete;
    get_pool_awaiter_t(get_pool_awaiter_t &&oth) = delete;
    get_pool_awaiter_t &operator = (get_pool_awaiter_t &&oth) = delete;

    template <typename P>
    bool await_suspend(handle<P> h) {
        _pool = h.promise().state._pool;
        return false;
    }

    bool    await_ready()              { return false; };
    pool_wp await_resume()             { return _pool; }

    pool_wp _pool;
};

/* This is the object with which you wait for a file descriptor, you create it with a valid
wait_cond and fd and co_await on it. The resulting integer must be positive to not be an error */
struct fd_awaiter_t {
    int wait_cond;  /* set with epoll wait condition */
    int fd;         /* set with waited file descriptor */

    fd_awaiter_t(int fd, int wait_cond) : fd(fd), wait_cond(wait_cond) {}
    fd_awaiter_t(const fd_awaiter_t &oth) = delete;
    fd_awaiter_t &operator = (const fd_awaiter_t &oth) = delete;
    fd_awaiter_t(fd_awaiter_t &&oth) = delete;
    fd_awaiter_t &operator = (fd_awaiter_t &&oth) = delete;

    bool await_ready() { return false; };

    template <typename P>
    handle<void> await_suspend(handle<P> h) {
        if (auto pool = h.promise().state._pool.lock()) {
            state = &h.promise().state;
            /* in case we can't schedule the fd we log the failure and return the same coro */
            if (do_wait_fd_modifs(state, fd, wait_cond) != ERROR_OK) {
                return h;
            }
            error_e err;
            if ((err = pool->internal->wait_fd(h, fd, wait_cond)) != ERROR_OK) {
                /* TODO: log the fail */
                state->err = err;
                return h;
            }
            return pool->internal->next_task();
        }
        return std::noop_coroutine();
    }

    error_e await_resume() {
        do_unwait_fd_modifs(state, fd);
        return state->err;
    }

private:
    /* The state of the corutine that called us. We know it will exist at least as much as the
    suspension */
    state_t *state;
};

/* Semaphore
------------------------------------------------------------------------------------------------- */

struct sem_internal_t {
    sem_internal_t(pool_wp pool, int64_t val) : _pool(pool), val(val) {}

    bool await_ready() {
        if (val > 0) {
            val--;
            return true;
        }
        return false;
    }

    bool try_dec() {
        return await_ready();
    }

    error_e signal() {
        if (val == 0 && waiting_on_sem.size())
            return _awake_one();
        
        val++;
        if (val == 0)
            return _awake_one();

        /* we awake when val is 0, do nothing on negative, only increment and on positive we
        don't have awaiters */
        return ERROR_OK;
    }

protected:
    friend sem_awaiter_t;
    friend sem_t;
    friend inline sem_p create_sem(pool_wp pool, int64_t val);

    sem_wp _sem;

    sem_waiter_handle_wp push_waiter(handle<void> to_suspend) {
        /* Don't know another way to fix this mess, so, yeah... The problem is that the awaiter
        bellow needs a way to register the waiter on this list so it may potentialy awake it (via the
        modif callbacks). Now the problem is that this semaphore may also want to awake the waiter,
        wich would invalidate the iterator inside the callback. So we give both the callback and
        the awaiter bellow a weak pointer to a pointer held at the same place with the corutine,
        so that if the corutine awakes, the pointer dies and the weak_pointer locks to null. */
        auto p = std::make_shared<sem_wait_list_it>();
        waiting_on_sem.push_front({to_suspend, p});
        *p = waiting_on_sem.begin();
        return p;
    }

    void erase_waiter(sem_wait_list_it it) {
        waiting_on_sem.erase(it);
    }

    pool_wp get_pool() {
        return _pool;
    }

private:
    error_e _awake_one() {
        auto to_awake = waiting_on_sem.back();
        if (auto pool = _pool.lock()) {
            pool->internal->push_ready(to_awake.first);
            waiting_on_sem.pop_back();
            return ERROR_OK;
        }
        else {
            dbg("could't get the pool");
            return ERROR_GENERIC;
        }
    }

    sem_wait_list_t waiting_on_sem;
    int64_t val;
    pool_wp _pool;
};

struct sem_awaiter_t {
    sem_awaiter_t(sem_wp _sem) : _sem(_sem) {}
    sem_awaiter_t(const sem_awaiter_t &oth) = delete;
    sem_awaiter_t &operator = (const sem_awaiter_t &oth) = delete;
    sem_awaiter_t(sem_awaiter_t &&oth) = delete;
    sem_awaiter_t &operator = (sem_awaiter_t &&oth) = delete;

    template <typename P>
    handle<void> await_suspend(handle<P> to_suspend) {
        state = &to_suspend.promise().state;

        auto sem = _sem.lock();
        if (!sem) {
            /* TODO: stop the pool, or return to caller? or except? */
            dbg("Invalid sem");
            return std::noop_coroutine();
        }
        auto pool = sem->internal->get_pool().lock();
        if (!pool) {
            /* TODO: as above? error handling, what to do? */
            dbg("Invalid pool");
            return std::noop_coroutine();
        }
        _sem_it = sem->internal->push_waiter(to_suspend);
        if (do_wait_sem_modifs(state, _sem, _sem_it) != ERROR_OK) {
            sem->internal->erase_waiter(*(_sem_it.lock()));
            return to_suspend;
        }

        triggered = true;
        return pool->internal->next_task();
    }

    sem_t::unlocker_t await_resume() {
        if (triggered)
            do_unwait_sem_modifs(state, _sem, _sem_it);
        triggered = false;
        return sem_t::unlocker_t(_sem);
    }

    bool await_ready() {
        if (auto sem = _sem.lock())
            return sem->internal->await_ready();
        return false;
    }

    state_t *state = nullptr;
    sem_wp _sem;
    sem_waiter_handle_wp _sem_it;
    bool triggered = false;
};

inline sem_t::sem_t(pool_wp pool, int64_t val)
: internal(std::make_shared<sem_internal_t>(pool, val)) {}

inline sem_awaiter_t sem_t::wait() { return sem_awaiter_t(internal->_sem); }
inline error_e       sem_t::signal() { return internal->signal(); }
inline bool          sem_t::try_dec() { return internal->try_dec(); }

inline sem_p create_sem(pool_wp pool, int64_t val) {
    auto ret = sem_p(new sem_t{pool, val}); 
    ret->internal->_sem = ret;
    return ret;
}

inline task<sem_p> create_sem(int64_t val) {
    co_return create_sem(co_await get_pool(), val);
}

/* Functions
------------------------------------------------------------------------------------------------  */

template <typename ...Args>
inline task_t wait_all(task<Args>... tasks) {
    /* TODO: this should return once all the tasks have been executed (use futures here?) */
    co_return ERROR_OK;
}

template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const std::vector<modif_p>& v) {
    return sched_awaiter_t(to_sched, create_modif_table(v));
}

template <typename Awaiter>
inline task_t await(Awaiter&& awaiter) {
    co_await std::forward<Awaiter>(awaiter);
    co_return ERROR_OK;
}

inline yield_awaiter_t yield() {
    return yield_awaiter_t{};
}

inline task<pool_wp> get_pool() {
    get_pool_awaiter_t awaiter;
    co_return co_await awaiter;
}

inline task<modif_p> creat_depend() {
    /* TODO: create a killer, that will kill this corutine and the ones spawned from it? if the
    depended task fails, this is kinda stupid, I may ignore it, (or maybe it is necesary to be able
    to do something like wait_any?) */
    co_return nullptr;
}

inline task_t stopfd(int fd) {
    /* TOSO: get the fd out of the poll, awakening all it's waiters */
    co_return ERROR_OK;
}

inline task_t force_stop(run_e ret) {
    /* TODO: simply interrupt the pool, if you continue it, it will continue from this place */
    co_return ERROR_OK;
}

inline task_t sleep(const std::chrono::microseconds& timeo) {
    /* TODO: create sleeper and sleep */
    co_return ERROR_OK;
}

inline task_t sleep_us(uint64_t timeo_us) {
    co_return co_await sleep(std::chrono::microseconds(timeo_us));
}

inline task_t sleep_ms(uint64_t timeo_ms) {
    co_return co_await sleep(std::chrono::milliseconds(timeo_ms));
}

inline task_t sleep_s(uint64_t timeo_s) {
    co_return co_await sleep(std::chrono::seconds(timeo_s));
}

inline task_t wait_event(int fd, int event) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t connect(int fd, sockaddr *sa, socklen_t *len) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t accept(int fd, sockaddr *sa, socklen_t *len) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t read(int fd, void *buff, size_t len) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t write(int fd, const void *buff, size_t len) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t read_sz(int fd, void *buff, size_t len) {
    /* TODO */
    co_return ERROR_OK;
}

inline task_t write_sz(int fd, const void *buff, size_t len) {
    /* TODO */
    co_return ERROR_OK;
}

struct task_modifs_getter_t {
    modif_table_p table;

    bool          await_ready() noexcept { return false; }
    modif_table_p await_resume() noexcept { return table; }

    template <typename P>
    bool await_suspend(handle<P> h) noexcept {
        table = h.promise().state.modif_table;
        return false;
    }
};

inline std::vector<modif_p> get_task_modifs(modif_table_p table) {
    std::vector<modif_p> ret;
    if (!table)
        return ret;
    for (auto &cbk_table : table->table) {
        for (auto &modif : cbk_table)
            ret.push_back(modif);
    }
    return ret;
}

template <typename T>
inline std::vector<modif_p> task_modifs(task<T> t) {
    auto mt = t.h.promise().state.modif_table;
    return get_task_modifs(mt);
}

inline task<std::vector<modif_p>> task_modifs() {
    auto table = co_await task_modifs_getter_t{};
    co_return get_task_modifs(table);
}

inline void add_modifs_to_table(modif_table_p table, std::set<modif_p> mods) {
    if (!table)
        return;
    for (auto &cbk_table : table->table)
        for (auto &modif : cbk_table)
            if (has(mods, modif))
                mods.erase(modif);
    for (auto &mod : mods)
        if (mod)
            table->table[mod->type].push_back(mod);
}

inline void rm_modifs_from_table(modif_table_p table, const std::set<modif_p>& mods) {
    if (!table)
        return;
    for (auto &cbk_table : table->table) {
        cbk_table.erase(std::remove_if(cbk_table.begin(), cbk_table.end(),
                [&mods](modif_p m) { return has(mods, m); }), cbk_table.end());
    }
}

template <typename T>
inline task<T> add_modifs(task<T> t, const std::set<modif_p>& mods) {
    add_modifs_to_table(t.h.promise().state.modif_table, mods);
    return t;
}

template <typename T>
inline task<T> rm_modifs(task<T> t, const std::set<modif_p>& mods) {
    rm_modifs_from_table(t.h.promise().state.modif_table, mods);
    return t;
}

inline task_t add_modifs(const std::set<modif_p>& mods) {
    auto table = co_await task_modifs_getter_t{};
    add_modifs_to_table(table, mods);
}

inline task_t rm_modifs(const std::set<modif_p>& mods) {
    auto table = co_await task_modifs_getter_t{};
    rm_modifs_from_table(table, mods);
}

template <typename T>
inline task<T> creat_future(task<T> t) {
    /* TODO: create a function that will return the return value of t once awaited. */
    return t;
}

/* TODO: depends on the pool_t to create the timer */
inline modif_p creat_timeo(const std::chrono::microseconds& timeo) {
    /* TODO: create the 'modif packet' that will kill the corutines in the call-path when the
    timer expires */
    return nullptr;
}

inline modif_p creat_killer() {
    /* TODO: create the 'modif packet' that can stop the corutines in the call-path when triggered */
    return nullptr;
}
inline error_e sig_killer(modif_p p) {
    /* TODO: this signals the coro_killer that it should kill the coro */
    /* TODO: change the return type, make it such that it makes sense ('modif packet') */
    return ERROR_OK;
}

/* Debug stuff
------------------------------------------------------------------------------------------------- */

template <typename T, typename ...Args>
inline task<T> dbg_register_name(task<T> t, const char *fmt, Args&&... args) {
    dbg_register_name(t.h.address(), fmt, std::forward<Args>(args)...);
    return t;
}

template <typename P, typename ...Args>
inline handle<P> dbg_register_name(handle<P> h, const char *fmt, Args&&... args) {
    dbg_register_name(h.address(), fmt, std::forward<Args>(args)...);
    return h;
}

template <typename T>
inline std::string dbg_name(task<T> t) {
    return dbg_name(t.h.address());
}

template <typename P>
inline std::string dbg_name(handle<P> h) {
    return dbg_name(h.address());
}

inline modif_p creat_dbg_tracer() {
    /* TODO: create a 'modif packet' that can be used to trace all the corutine's actions */
    return nullptr;
}

template <typename... Args>
inline std::string dbg_format(const char *fmt, Args&& ...args) {
    std::vector<char> buff;
    int cnt = snprintf(NULL, 0, fmt, std::forward<Args>(args)...);
    if (cnt <= 0)
        return "[failed snprintf1]";
    buff.resize(cnt + 1);
    if (snprintf(buff.data(), cnt + 1, fmt, std::forward<Args>(args)...) < 0)
        return "[failed snprintf2]";
    return buff.data();
}

inline uint64_t dbg_get_time() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

#if CORO_ENABLE_DEBUG_NAMES

/* TODO: mutex it if needed by multi-threads */
inline std::map<void *, std::pair<std::string, int>> dbg_names;

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) {
    if (!has(dbg_names, addr))
        dbg_names[addr] = {dbg_format(fmt, std::forward<Args>(args)...), 1};
    else
        dbg_names[addr].second++;
    return addr;
}

inline std::string dbg_name(void *v) {
    if (!has(dbg_names, v))
        dbg_register_name(v, "%p", v);
    return dbg_format("%s[%" PRIu64 "]", dbg_names[v].first.c_str(), dbg_names[v].second);
}

#else /*CORO_ENABLE_DEBUG_NAMES*/

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) { return addr; }

inline std::string dbg_name(void *v) { return ""; };

#endif /*CORO_ENABLE_DEBUG_NAMES*/

#if CORO_ENABLE_LOGGING

inline void dbg_raw(const std::string& msg,
        std::source_location const& sloc = std::source_location::current())
{
    if (log_str) {
        /* TODO: function_name: this is bullshit, replace it with something else */
        log_str(dbg_format("[%" PRIu64 "] %s:%4d %s() :> %s\n", dbg_get_time(),
                sloc.file_name(), (int)sloc.line(), "sloc.function_name()", msg.c_str()));
    }
}

template <typename... Args>
inline void dbg(dbg_format_helper_t dfmt, Args&&... args) {
    dbg_raw(dbg_format(dfmt.fmt.c_str(), std::forward<Args>(args)...), dfmt.loc);
}

#else /*CORO_ENABLE_LOGGING*/

template <typename... Args> /* no logging -> do nothing */
inline void dbg(dbg_format_helper_t, Args&&...) {}

#endif /*CORO_ENABLE_LOGGING*/

/* The end
------------------------------------------------------------------------------------------------- */

}

#endif
