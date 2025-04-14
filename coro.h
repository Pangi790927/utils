#ifndef CORO_H
#define CORO_H

/* TODO:
    - consider porting for windows
    - fix error propagation (error_e) (is there a problem?)
    - tests
    - write the tutorial at the start of this file
    - check own comments
    - check the review again
    - add the licence
    - fix noexcept if there are problems
    + consider implementing exception handling
    + consider implementing co_yield
    + write the fd stuff
    + add debug modifs, things are already breaking and I don't know why
    + speed optimizations [added allocator]
*/

/* rewrite of co_utils.h ->
    - CPP library over the CPP corutines
    - Built around epoll (ie, epoll is the only blocking function)
    - Meant to have one pool/thread or enable mutexes on pool (this will allow callbacks and pool
    sharing)
    - Meant to have modifs (callbacks on corutine execution)
*/

/*  Documentation Tutorial
====================================================================================================


====================================================================================================
*/

#include <chrono>
#include <cinttypes>
#include <coroutine>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <set>
#include <stack>

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

/* The maximum amount of concurent timers that can be awaited */
#ifndef CORO_MAX_TIMER_POOL_SIZE
# define CORO_MAX_TIMER_POOL_SIZE 64
#endif

/* if file descriptor number is lower than this number it's associated awaiter data will be
remembered in a faster structure */
#ifndef CORO_MAX_FAST_FD_CACHE
# define CORO_MAX_FAST_FD_CACHE 1024
#endif

/* Set if a pool can be available in more than one thread, necesary if you want to transfer the pool
to a callback that will be called from another thread, WARNING: this slows down a lot of things */
#ifndef CORO_ENABLE_MULTITHREAD_SCHED
# define CORO_ENABLE_MULTITHREAD_SCHED false
#endif

/* Enable the corutines to catch and throw exceptions */
#ifndef CORO_ENABLE_EXCEPTIONS
# define CORO_ENABLE_EXCEPTIONS false
#endif

/* If set to true, will enable the logging callback */
#ifndef CORO_ENABLE_LOGGING
# define CORO_ENABLE_LOGGING true
#endif
#if CORO_ENABLE_LOGGING
# define CORO_DEBUG(fmt, ...) dbg(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)
#else
# define CORO_DEBUG(fmt, ...) do {} while (0)
#endif
/* If set to true, makes names for corutines */
#ifndef CORO_ENABLE_DEBUG_NAMES
# define CORO_ENABLE_DEBUG_NAMES false
#endif

/* If set, will create a debug tracer and it will add it to all coroutines */
#ifndef CORO_ENABLE_DEBUG_TRACE_ALL
# define CORO_ENABLE_DEBUG_TRACE_ALL false
#endif

/* TODO: this */
/* If you don't like the allocator and you would rather not have it use up memory */
#ifndef CORO_DISABLE_ALLOCATOR
# define CORO_DISABLE_ALLOCATOR false
#endif

/* used inside the allocator to scale the ammount of memory it uses per pool */
#ifndef CORO_ALLOCATOR_SCALE
# define CORO_ALLOCATOR_SCALE 16
#endif

/* TODO: */
/* if defined, the library will expect you to implement an allocator and the definitions for this
one will be removed */
#ifndef CORO_ALLOCATOR_REPLACE
#define CORO_ALLOCATOR_REPLACE false
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
constexpr int MAX_FAST_FD_CACHE = CORO_MAX_FAST_FD_CACHE;
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
using pool_p = std::shared_ptr<pool_t>;
using modif_p = std::shared_ptr<modif_t>;
using modif_table_p = std::shared_ptr<modif_table_t>;

using modif_pack_t = std::vector<modif_p>;

/* Tasks errors */
enum error_e : int32_t {
    ERROR_YIELDED =  1, /* not really an error, but used to signal that the coro yielded */
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
    RUN_OK = 0,       /* when the pool stopped because it ran out of things to do */
    RUN_ERRORED = -1, /* comes from epoll errors */
    RUN_ABORTED = -2, /* if a corutine had a stroke (some sort of internal error) */
    RUN_STOPPED = -3, /* can be re-run (comes from force_stop) */
};

enum modif_e : int32_t {
    /* This is called when a task is called (on the task), via 'co_await task' */
    CO_MODIF_CALL_CBK = 0,

    /* This is called on the corutine that is scheduled. Other mods are inherited before this is
    called. The return value of the callback is ignored. */
    CO_MODIF_SCHED_CBK,

    /* This is called on a corutine right before it is destroyed. The return value of the callback
    is ignored */
    CO_MODIF_EXIT_CBK,

    /* This is called on each suspended corutine. The return value of the callback is ignored*/
    CO_MODIF_LEAVE_CBK,

    /* This is called on a resume. The return value of the callback is ignored */
    CO_MODIF_ENTER_CBK,

    /* This is called when a corutine is waiting for an IO (after the leave cbk). If the return
    value is not ERROR_OK, then the wait is aborted. */
    CO_MODIF_WAIT_IO_CBK,

    /* This is called when the io is done and the corutine that awaited it is resumed */
    CO_MODIF_UNWAIT_IO_CBK,

    /* This is similar to wait_io, but on a semaphore */
    CO_MODIF_WAIT_SEM_CBK,

    /* This is similar to unwait_io, but on a semaphore */
    CO_MODIF_UNWAIT_SEM_CBK,

    CO_MODIF_COUNT,
};

enum modif_flags_e : int32_t {
    CO_MODIF_INHERIT_NONE = 0x0,
    CO_MODIF_INHERIT_ON_CALL = 0x1,
    CO_MODIF_INHERIT_ON_SCHED = 0x2,
};

/* all the internal tasks return this, namely error_e but casted to int (a lot of my old code depends
on this and I also find it in theme, as all the linux functions that I use tend to return ints) */
using task_t = task<int>;

/* some forward declarations of internal structures */
struct yield_awaiter_t;
struct sem_awaiter_t;
struct pool_internal_t;
struct sem_internal_t;
struct allocator_memory_t;
struct io_desc_t;

template <typename T>
struct sched_awaiter_t;

/*
There are two things that don't need custom allocating: lowspeed stuff and the corutine promise.
It makes no sense to allocate the corutine promise because:
    1. It is a user defined type so the promise is not going to fit well in our allocator
    2. I expect it be allocated rarelly
    3. It would be a pain to allocate them
    4. a malloc now and then is not such a big deal

For the rest of the code those 4 are not generaly true.

The idea of this allocator is that allocated objects are actually small in size and get allocated/
deallocated fast. The assumption is that there is a direct corellation with the number
num_fds+call_depth, and most of the time there aren't that many file descriptors or depth to
calls (at least from my experience).

So what it does is this: it has 5 bucket levels: 32, 64, 128, 512, 2048 (bytes), with a number of
maximum allocations 16384, 8192, 4096, 1024 and 256 of each and a stack coresponding to each of
them, that is initiated at the start to contain every index from 0 to the max amount of slots in
each bucket. When an allocation occours, an index is poped from the lowest fitting bucket, and
that slot is returned. On a free, if the memory is from a bucket, the index is calculated and pushed
back in that bucket's stack else the normal free is used.

Those bucket levels can be configured, by changing the array bellow (Obs: The buckets must be in
ascending size order).

The improvement from malloc, as a guess, (I used a profiler to see if it does something) is that
    a. the memory is already there, no need to fetch it if not available and no need to check if it
    is available
    b. there are no locks. since we already assume that the code that does the allocations is
    protected either by the absence of multithreading or by the pool's lock
    c. there is no fragmentation, at least until the memory runs out, the pool's memory is localized
*/
/* No shared pointers if not needed: around 80% time spent decrease */
/* Own allocator: around 10%-30% further time decrease (not sure if it was worth the time, but at
least my test go smoother now) */
/* array of {element size, bucket size} */
constexpr std::pair<int, int> allocator_bucket_sizes[] = {
    {32,    CORO_ALLOCATOR_SCALE * 1024},
    {64,    CORO_ALLOCATOR_SCALE * 512},
    {128,   CORO_ALLOCATOR_SCALE * 256},
    {512,   CORO_ALLOCATOR_SCALE * 64},
    {2048,  CORO_ALLOCATOR_SCALE * 16}
};
template <typename T>
struct allocator_t {
    constexpr static int common_alignment = 16;
    static_assert(common_alignment % alignof(T) == 0,
            "ERROR: The allocator assumption is that internal data is aligned to at most 16 bytes");

    using value_type = T;
    allocator_t(pool_t *pool) : pool(pool) {}

    template<typename U>
    allocator_t(const allocator_t <U>& o) noexcept : pool(o.pool) {}

    template<typename U>
    allocator_t &operator = (const allocator_t <U>& o) noexcept { this->pool = o.pool; return *this; }

    T* allocate(size_t n);
    void deallocate(T* _p, std::size_t n) noexcept;

    template <typename U>
    bool operator != (const allocator_t<U>& o) noexcept { return this->pool != o.pool; }

    template <typename U>
    bool operator == (const allocator_t<U>& o) noexcept { return this->pool == o.pool; }

protected:
    template <typename U>
    friend struct allocator_t; /* lonely class */

    pool_t *pool = nullptr; /* this allocator should allways be called on a valid pool */
};
template <typename T> 
struct deallocator_t { /* This class is part of the allocator implementation and is here only
                          because a definition needs it as a full type */
    pool_t *pool = nullptr;
    void operator ()(T *p) { p->~T(); allocator_t<T>{pool}.deallocate(p, 1); }
};

/* exception error if the lib is configured to throw exception on modif termination */
#if CO_EXCEPT_ON_KILL
/* TODO: the exception that will be thrown by the killing of a courutine */
#endif

/* having a task be called on two pools or two threads is UB */
struct pool_t {
    /* you use the pointer made by create_pool */
    pool_t(pool_t& sem) = delete;
    pool_t(pool_t&& sem) = delete;
    pool_t &operator = (pool_t& sem) = delete;
    pool_t &operator = (pool_t&& sem) = delete;

    ~pool_t() { clear(); }

    /* Same as coro::sched, but used outside of a corutine, based on the pool */
    template <typename T>
    void sched(task<T> task, const modif_pack_t& v = {}); /* ignores return type */

    /* runs the event loop */
    run_e run();

    /* clears all the corutines that are linked to this pool, so all waiters, all ready tasks, etc.
    This will happen automatically inside the destructor */
    error_e clear();

    /* stops awaiters from waiting on the descriptor (this is the non-awaiting variant) */
    error_e stop_io(const io_desc_t& io_desc);
    error_e stop_fd(int fd);

    /* Better to not touch this function, you need to understand the internals of pool_t to use it */
    pool_internal_t *get_internal();

    int64_t stopval = 0;            /* a value that is set by force_stop(stopval) */
    std::shared_ptr<void> user_ptr; /* the library won't touch this, do whatever with it */

protected:
    template <typename T>
    friend struct allocator_t;

    std::unique_ptr<allocator_memory_t> allocator_memory;

    friend inline std::shared_ptr<pool_t> create_pool();
    pool_t();

private:
    std::unique_ptr<pool_internal_t> internal;
};

struct sem_t {
    struct unlocker_t{ /* compatibility with guard objects ex: std::lock_guard guard(co_await s); */
        sem_t *sem;
        unlocker_t(sem_t *sem) : sem(sem) {}
        void lock() {}
        void unlock() { sem->signal(); }
    };

    /* you use the pointer made by create_sem */
    sem_t(sem_t& sem) = delete;
    sem_t(sem_t&& sem) = delete;
    sem_t &operator = (sem_t& sem) = delete;
    sem_t &operator = (sem_t&& sem) = delete;

    /* If the semaphore dies while waiters wait, they will all be forcefully destroyed (their entire
    call stack) */
    ~sem_t();

    sem_awaiter_t wait(); /* if awaited returns unlocker_t{} */
    error_e signal(); /* returns error if the pool disapeared */
    bool try_dec();

    /* again, beeter don't touch, same as pool */
    sem_internal_t *get_internal();

protected:
    template <typename T, typename ...Args>
    friend inline T *alloc(pool_t *, Args&&...);

    sem_t(pool_t *pool, int64_t val = 0);

private:
    std::unique_ptr<sem_internal_t, deallocator_t<sem_internal_t>> internal;
};

/* This describes the async io op. It may be redefined if this lib will be ported to windows or
other systems. */
struct io_desc_t {
    int fd = -1;                        /* file descriptor */
    uint32_t events = 0xffff'ffff;      /* epoll events to be waited on the file descriptor */
};

/* This is the state struct that each corutine has */
struct state_t {
    error_e err = ERROR_OK;                 /* holds the error return in diverse cases */
    pool_t *pool = nullptr;                 /* the pool of this coro */
    modif_table_p modif_table;              /* we allocate a table only if there are mods */

    state_t *caller_state = nullptr;        /* this holds the caller's state, and with it the
                                            return path */ 

    std::coroutine_handle<void> self;       /* the coro's self handle */

    std::exception_ptr exception = nullptr; /* the exception that must be propagated */

    std::shared_ptr<void> user_ptr;         /* this is a pointer that the user can use for whatever
                                            he feels like. This library will not touch this pointer */
};

/* This is mostly internal, but you can also use it to be able to check if someone is still waiting
on a semaphore (TODO) */
/* TODO: will no longer use weak_ptr's on the important path, I think it allways breaks performance */
using sem_waiter_handle_p =
        std::shared_ptr<                        /* if this pointer is not available, the waiter was
                                                   evicted from the waiters list */
            std::list<                          /* List with the semaphore waiters */
                std::pair<
                    state_t *,                  /* The waiting corutine state */
                    std::shared_ptr<void>       /* Where the shared_ptr is actually stored */
                >,
                allocator_t<std::
                    pair<
                        state_t *,
                        std::shared_ptr<void>
                    >
                >                               /* profiling shows the default is slow */
            >::iterator                         /* iterator in the respective list */
        >;

/* A modif is a callback for a specifi stage in the corutine's flow */
struct modif_t {
    using variant_t = std::variant<
        std::function<error_e(state_t *)>,              /* call_cbk */
        std::function<error_e(state_t *)>,              /* sched_cbk */
        std::function<error_e(state_t *)>,              /* exit_cbk */
        std::function<error_e(state_t *)>,              /* leave_cbk */
        std::function<error_e(state_t *)>,              /* enter_cbk */
        std::function<error_e(state_t *, io_desc_t&)>,  /* wait_io_cbk */
        std::function<error_e(state_t *, io_desc_t&)>,  /* unwait_io_cbk */

         /* wait_sem_cbk - OBS: the std::shared_ptr<void> part can be ignored, it's internal */
        std::function<error_e(state_t *, sem_t *, sem_waiter_handle_p)>,

         /* unwait_sem_cbk */
        std::function<error_e(state_t *, sem_t *, sem_waiter_handle_p)>
    >;

    variant_t cbk;
    modif_e type = CO_MODIF_COUNT;
    modif_flags_e flags = CO_MODIF_INHERIT_ON_CALL;
};

/* Pool & Sched functions:
------------------------------------------------------------------------------------------------  */

inline std::shared_ptr<pool_t> create_pool();

/* gets the pool of the current corutine, necesary if you want to sched corutines from callbacks.
Don't forget you need multithreading enabled if you want to schedule from another thread. */
inline task<pool_t *> get_pool();
inline task<state_t *> get_state();

/* This does not stop the curent corutine, it only schedules the task, but does not yet run it.
Modifs duplicates are eliminated. */
template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const modif_pack_t& v = {});

/* This stops the current coroutine from running and places it at the end of the ready queue. */
inline yield_awaiter_t yield();

/* Modifications
------------------------------------------------------------------------------------------------  */

template <modif_e type, typename Cbk>
inline modif_p create_modif(pool_t *pool, modif_flags_e flags, Cbk&& cbk);

template <modif_e type, typename Cbk>
inline modif_p create_modif(pool_p  pool, modif_flags_e flags, Cbk&& cbk);

/* This returns the internal vec that holds the different modifications of the task. Changing them
here is ill-defined */
template <typename T>
inline std::vector<modif_p> task_modifs(task<T> t);

/* This adds a modifier to the task and returns the task. Duplicates will be ignored */
template <typename T>
inline task<T> add_modifs(pool_t *pool, task<T> t, const std::set<modif_p>& mods); /* not awaitable */

/* Removes modifier from the vector */
template <typename T>
inline task<T> rm_modifs(task<T> t, const std::set<modif_p>& mods); /* not awaitable */

/* same as above, but adds them for the current task */
inline task<std::vector<modif_p>> task_modifs();
inline task_t add_modifs(const std::set<modif_p>& mods); 
inline task_t rm_modifs(const std::set<modif_p>& mods);

/* calls an awaitable inside a task_t, this is done to be able to use modifs on awaitables */
template <typename Awaiter>
inline task_t await(Awaiter&& awaiter);

/* Timing
------------------------------------------------------------------------------------------------  */

/* adds a timeout modification to the task, i.e. the corutine will be stopped from executing if the
given time passes (uses create_killer) */
template <typename T>
inline task<std::pair<T, error_e>> create_timeo(
        task<T> t, pool_t *pool, const std::chrono::microseconds& timeo);

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
inline sem_p create_sem(pool_t *pool, int64_t val = 0);
inline sem_p create_sem(pool_p  pool, int64_t val = 0);
inline task<sem_p> create_sem(int64_t val = 0);

/* modifier that, once signaled, will awake the corutine and make it return ERROR_WAKEUP. If the
corutine is not waiting it will return as soon as it reaches any co_await ?or co_yield? */
/* TODO: don't forget to awake the fds nicely(something like stop_fd) */
inline std::pair<modif_pack_t, std::function<error_e(void)>> create_killer(pool_t *pool, error_e e);

/* takes a task and adds the requred modifications to it such that the returned object will be
returned once the return value of the task is available so:
    auto t = co_task();
    auto fut = coro::create_future(t)
    co_await coro::sched(t);
    ...
    co_await fut; // returns the value of co_task once it has finished executing 
*/
template <typename T>
inline task<T> create_future(pool_t *pool, task<T> t); /* not awaitable */

/* wait for all the tasks to finish, the return value can be found in the respective task, killing
one kills all (sig_killer installed in all). The inheritance is the same as with 'call'. */
template <typename ...ret_v>
inline task<std::tuple<ret_v>...> wait_all(task<ret_v>... tasks);

/* causes the running pool::run to stop, the function will stop at this point, can be resumed with
another run */
inline task_t force_stop(int64_t stopval = 0);

/* EPOLL:
------------------------------------------------------------------------------------------------  */

/* event can be EPOLLIN or EPOLLOUT (maybe some others work too?) This can be used to be notified
when those are ready without doing a read or a write. */
inline task_t wait_event(const io_desc_t& io_desc);

/* closing a file descriptor while it is managed by the coroutine pool will break the entire system
so you must use stop_fd on the file descriptor before you close it. This makes sure the fd is
awakened and ejected from the system before closing it. For example:

    co_await coro::stop_fd(fd);
    close(fd);
*/
inline task_t stop_io(const io_desc_t& io_desc);

/* Linux Specific:
------------------------------------------------------------------------------------------------  */

inline task_t stop_fd(int fd);

/* TODO: describe their usage */
inline task_t        connect(int fd, sockaddr *sa, socklen_t *len);
inline task_t        accept(int fd, sockaddr *sa, socklen_t *len);
inline task<ssize_t> read(int fd, void *buff, size_t len);
inline task<ssize_t> write(int fd, const void *buff, size_t len);
inline task<ssize_t> read_sz(int fd, void *buff, size_t len);
inline task<ssize_t> write_sz(int fd, const void *buff, size_t len);

/* Debug Interfaces:
------------------------------------------------------------------------------------------------  */

/* why would I replace the old string with the new one based on the allocator? because I need to
know that the library allocates only through the allocator, so debugging would interfere with
that. */
using dbg_string_t = std::basic_string<char, std::char_traits<char>, allocator_t<char>>;

template <typename... Args>
inline void dbg(const char *file, const char *func, int line, const char *fmt, Args&&... args);

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
inline modif_pack_t dbg_create_tracer(pool_t *pool);

/* Obtains the name given to the respective task, handle or address */
template <typename T>
inline dbg_string_t dbg_name(task<T> t);

template <typename P>
inline dbg_string_t dbg_name(std::coroutine_handle<P> h);

inline dbg_string_t dbg_name(void *v);

/* Obtains a string from the given enum */
inline dbg_string_t dbg_enum(error_e code);
inline dbg_string_t dbg_enum(run_e code);
inline dbg_string_t dbg_epoll_events(uint32_t events);

/* formats a string using the C snprintf, similar in functionality to a combination of
snprintf+std::format, in the version of g++ that I'm using std::format is not available  */
template <typename... Args>
inline dbg_string_t dbg_format(const char *fmt, Args&& ...args);

/* corutine callbacks for diverse points in the code, if in a multithreaded environment, you must
take all the locks before rewriting those. The pool locks are held when those callbacks are called
*/
#if CO_ENABLE_CALLBACKS
/* add callbacks here */
inline std::function<void(void)> on_call;
#endif /* CO_ENABLE_CALLBACKS */

/* calls log_str to save the log string */
#if CORO_ENABLE_LOGGING
inline std::function<int(const dbg_string_t&)> log_str =
        [](const dbg_string_t& msg){ return printf("%s", msg.c_str()); };
#endif

/* IMPLEMENTATION 
=================================================================================================
=================================================================================================
================================================================================================= */

/* Helpers
------------------------------------------------------------------------------------------------- */

template <typename P>
using handle = std::coroutine_handle<P>;

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

using sem_wait_list_t = std::list<std::pair<state_t *, std::shared_ptr<void>>,
        allocator_t<std::pair<state_t *,std::shared_ptr<void>>>>;
using sem_wait_list_it = sem_wait_list_t::iterator;

inline void destroy_state(state_t *curr) {
    while (curr) {
        state_t *next = curr->caller_state;
        curr->self.destroy();
        curr = next;
    }
}

struct FnScope {
    std::function<void(void)> fn;
    FnScope(std::function<void(void)> fn) : fn(fn) {}
    ~FnScope() {
        if (fn)
            fn();
    }
    void precall() {
        fn();
        fn = {};
    }
};

/* Allocator
------------------------------------------------------------------------------------------------- */

struct allocator_memory_t {
    constexpr static int buckets_cnt =
            sizeof(allocator_bucket_sizes)/sizeof(allocator_bucket_sizes[0]);

    template <size_t obj_sz, size_t cnt>
    struct bucket_t {
        static_assert(obj_sz);
        using bucket_object_t = char[obj_sz];

        alignas(allocator_t<int>::common_alignment) bucket_object_t objects[cnt];
        int free_stack[cnt];
        int stack_head = 0;

        static constexpr int floorlog2(int x) {
            return x == 1 ? 0 : 1 + floorlog2(x >> 1);
        }

        void init() {
            stack_head = cnt - 1;
            for (int i = 0; i < cnt; i++)
                free_stack[i] = cnt - i - 1;
        }

        void *alloc(size_t bytes, bool &invalidated) {
            if (bytes > obj_sz)
                return NULL;
            else
                invalidated = true; /* shouldn't check the other buckets if the size was good, but
                                       the allocation failed for other reasons */
            if (!stack_head)
                return NULL;

            // if (stack_head > cnt) {
                // CORO_DEBUG("alloc bytes[%ld] stack_head[%d] obj_sz[%ld], cnt[%ld]",
                //         bytes, stack_head, obj_sz, cnt);
            // }
            void *ret = &objects[free_stack[stack_head]];
            stack_head--;
            return ret;
        }

        void free(void *ptr) {
            auto p = (char *)ptr;
            auto start = (char *)&objects;
            auto stop = start + sizeof(objects);

            if (p < start || p >= stop)
                return ;

            int index = ((p - start) >> floorlog2(obj_sz));
            stack_head++;
            free_stack[stack_head] = index;
        }
    };

    template <size_t ...I>
    struct buckets_type_helper {
        using type = std::tuple<bucket_t<
                allocator_bucket_sizes[I].first, allocator_bucket_sizes[I].second>...>;

        buckets_type_helper(std::index_sequence<I...>) {}
    };

    template <size_t N>
    using buckets_type = decltype(buckets_type_helper(std::make_index_sequence<N>{}))::type;

    allocator_memory_t() {
        std::apply([](auto&&... args) { ((args.init()), ...); }, buckets);
    }

    void *alloc(size_t bytes) {
#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        void *ret = NULL;
        bool invalidated = false;
        std::apply([&](auto&&... args) {
            ((ret = (ret || invalidated) ? ret : args.alloc(bytes, invalidated)), ...);
        }, buckets);
        return ret;
    }

    void free(void *ptr) {
#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        std::apply([&](auto&&... args) { ((args.free(ptr)), ...); }, buckets);
    }

    /* There are some objectives for this buckets array:
        1. The buckets must all stay togheter (localized in memory)
        2. The buckets must all be constructed automatically from allocator_bucket_sizes
    */
    buckets_type<buckets_cnt> buckets;

#if CORO_ENABLE_MULTITHREAD_SCHED
    std::mutex lock;
#endif
};

/* allocator_t<T>::allocate/deallocate are declared under pool_internal_t */

template <typename T, typename ...Args>
inline T *alloc(pool_t *pool, Args&&... args) {
    return new(allocator_t<T>{pool}.allocate(1)) T(std::forward<Args>(args)...);
}

template <typename T>
inline auto dealloc_create(pool_t *pool) {
    return deallocator_t<T>{ .pool = pool };
}

/* Modifs Part
------------------------------------------------------------------------------------------------- */

struct modif_table_t {
    modif_table_t(pool_t *pool)
    : table{
            /* :( */
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}},
            std::vector<modif_p, allocator_t<modif_p>>{allocator_t<modif_p>{pool}}
    } {}

    std::array<std::vector<modif_p, allocator_t<modif_p>>, CO_MODIF_COUNT> table;
};

inline size_t get_modif_table_sz(modif_table_p ptable) {
    if (!ptable)
        return 0;
    size_t ret = 0;
    for (auto &t : ptable->table)
        ret += t.size();
    return ret;
}

inline modif_table_p create_modif_table(pool_t *pool, const std::vector<modif_p>& to_add) {
    modif_table_p ret = std::shared_ptr<modif_table_t>(alloc<modif_table_t>(pool, pool),
            dealloc_create<modif_table_t>(pool), allocator_t<int>{pool});

    std::unordered_set<modif_p, std::hash<modif_p>, std::equal_to<modif_p>,
            allocator_t<modif_p>> existing(allocator_t<modif_p>{pool});

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
    auto pool = state->pool;
    if (!parent_table)
        return ;
    modif_table_p new_table = state->modif_table;

    std::unordered_set<modif_p, std::hash<modif_p>, std::equal_to<modif_p>,
            allocator_t<modif_p>> existing(allocator_t<modif_p>{pool});

    if (new_table) {
        /* should be rare */
        for (auto &cbk_table : new_table->table)
            for (auto &modif : cbk_table)
                existing.insert(modif);
    }
    if (!new_table) {
        new_table = std::shared_ptr<modif_table_t>(alloc<modif_table_t>(pool, pool),
                dealloc_create<modif_table_t>(pool), allocator_t<int>{pool});
    }
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

inline error_e do_wait_io_modifs(state_t *state, io_desc_t &io_desc) {
    return do_generic_modifs<CO_MODIF_WAIT_IO_CBK>(state, io_desc);
}

inline error_e do_unwait_io_modifs(state_t *state, io_desc_t &io_desc) {
    return do_generic_modifs<CO_MODIF_UNWAIT_IO_CBK>(state, io_desc);
}

inline error_e do_wait_sem_modifs(state_t *state, sem_t *sem, sem_waiter_handle_p _it) {
    return do_generic_modifs<CO_MODIF_WAIT_SEM_CBK>(state, sem, _it);
}

inline error_e do_unwait_sem_modifs(state_t *state, sem_t *sem, sem_waiter_handle_p _it) {
    return do_generic_modifs<CO_MODIF_UNWAIT_SEM_CBK>(state, sem, _it);
}

/* considering you may want to create a modif at runtime this seems to be the best way */
template <modif_e type, typename Cbk>
inline modif_p create_modif(pool_t *pool, modif_flags_e flags, Cbk&& cbk) {
    modif_p ret = modif_p(alloc<modif_t>(pool, modif_t{
        .type = type,
        .flags = flags,
    }), dealloc_create<modif_t>(pool), allocator_t<int>{pool});
    
    ret->cbk = modif_t::variant_t(std::in_place_index_t<type>{}, std::forward<Cbk>(cbk));
    return ret;
}

template <modif_e type, typename Cbk>
inline modif_p create_modif(pool_p pool, modif_flags_e flags, Cbk&& cbk) {
    return create_modif<type>(pool.get(), flags, std::forward<Cbk>(cbk));
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
    T            await_resume();
    pool_t       *get_pool();
    state_t      *get_state();

    error_e get_err() { return ERROR_OK; };

    bool ever_called = false;
    handle_t h;
};

/* has to know about pool, will be implemented bellow the pool, but it is required here and to be
accesible to others that need to clean the corutine (for example in modifs, if the tasks need
killing) */
inline handle<void> final_awaiter_cleanup(state_t *ending_task_state);
inline handle<void> cpp_yield_awaiter(state_t *yielding_task_state);

template <typename T>
struct task_state_t {
    struct final_awaiter_t {
        bool await_ready() noexcept  { return false; }
        void await_resume() noexcept {}
        
        handle<void> await_suspend(handle<task_state_t<T>> ending_task) noexcept {
            return final_awaiter_cleanup(&ending_task.promise().state);
        }
    };

    struct cpp_yield_awaiter_t {
        bool await_ready() noexcept  { return false; }
        void await_resume() noexcept {}
        
        handle<void> await_suspend(handle<task_state_t<T>> yielding_task) noexcept {
            return cpp_yield_awaiter(&yielding_task.promise().state);
        }
    };

    task<T> get_return_object() {
        typename task<T>::handle_t h = task<T>::handle_t::from_promise(*this);
        state.self = h;
        return task<T>{h};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    final_awaiter_t     final_suspend() noexcept   { return final_awaiter_t{}; }
    void                unhandled_exception()      { state.exception = std::current_exception(); }

    /* probably slower than most generators, but if you need it it's here */
    template <typename R>
    cpp_yield_awaiter_t yield_value(R&& ret) {
        this->ret.template emplace<T>(std::forward<R>(ret));
        return {};
    }

    template <typename R>
    void return_value(R&& ret) {
        /* TODO: maybe if this coro is of our type it should also take into consideration the
        return value of the state.err? (or maybe better -> think more about that error thing) */
        this->ret.template emplace<T>(std::forward<R>(ret));
    }

    state_t state;
    std::exception_ptr exception;
    std::variant<std::monostate, T> ret;
};

template <typename T>
template <typename P>
inline handle<void> task<T>::await_suspend(handle<P> caller) noexcept {
    do_leave_modifs(&caller.promise().state);
    ever_called = true;

    h.promise().state.caller_state = &caller.promise().state;
    h.promise().state.pool = caller.promise().state.pool;

    if (do_call_modifs(&this->h.promise().state, caller.promise().state.modif_table) != ERROR_OK) {
        do_entry_modifs(&caller.promise().state);
        return caller;
    }

    return h;
}

template <typename T>
inline T task<T>::await_resume() {
    ever_called = true;
    do_entry_modifs(h.promise().state.caller_state);

    // propagate coroutine exception in called context
    std::exception_ptr exc_ptr = h.promise().state.exception;
    if (exc_ptr) {
        h.destroy();
        std::rethrow_exception(exc_ptr);
    }

    auto ret = std::get<T>(h.promise().ret);
    if (h.promise().state.err != ERROR_YIELDED)
        h.destroy();

    return ret;
}

template <typename T>
inline pool_t *task<T>::get_pool() {
    return h.promise().state.pool;
}

template <typename T>
inline state_t *task<T>::get_state() {
    return &h.promise().state;
}

/* The Pool & Epoll engine
------------------------------------------------------------------------------------------------- */

/* This is a component of the pool_internal that is somehow prepared to be replaced in case this lib
will be ported to other systems. In theory, to change the waiting mechanism you want to change this
struct and io_desc_t and otherwise this whole library should be ignorant to the system async
mechanisms until the endpoint functions like accept, connect, write, etc. */
struct io_pool_t {
    struct fd_data_t {
        struct waiter_t {
            uint32_t mask = 0; /* individual waiter mask */
            state_t *state;
        };
        int fd;
        uint32_t mask = 0; /* all the masks in one */
        std::vector<waiter_t, allocator_t<waiter_t>> waiters;
    };

    io_pool_t(pool_t *pool, std::deque<state_t *, allocator_t<state_t *>> &ready_tasks)
    :       pool{pool},
            fd_data_slow(allocator_t<int>{pool}),
            ret_evs(allocator_t<int>{pool}),
            ready_tasks{ready_tasks}
    {
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            CORO_DEBUG("FAILED epoll_create1 err:%s[%d] -> ret:%d",
                    strerror(errno), errno, epoll_fd);
            /* TODO: we can except on this warning if enabled */
        }
    }

    bool is_ok() {
        return epoll_fd >= 0;
    }

    error_e handle_ready() {
        if (ready_tasks.size() > 0) {
            /* we don't do anything if there are tasks ready, we only start interogating the system
            about ready io events if we don't have corutines to serve */
            return ERROR_OK;
        }

        if (ret_evs.size() == 0) {
            /* we don't have what to wait on, there are no registered awaiters */
            return ERROR_OK;
        }

        /* Now that we know we have no corutines ready we know we have to wait for some sort of io
        event to happen(timers, file io, network io, etc.). */
        int num_evs;
        do {
            num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), -1);
        }
        while (num_evs < 0 && errno == EINTR);
        if (num_evs < 0) {
            CORO_DEBUG("FAILED epoll_wait: epoll_fd[%d] err:%s[%d] -> ret:%d",
                    epoll_fd, strerror(errno), errno, num_evs);
            return ERROR_GENERIC;
        }

        /* New events arrived, it means that we can take those and push them into the ready_tasks */
        for (int i = 0; i < num_evs; i++) {
            int fd = ret_evs[i].data.fd;
            fd_data_t *data = get_data(fd);
            if (!data) {
                CORO_DEBUG("FAILED: fd[%d] doesn't have associated data", fd);
                return ERROR_GENERIC;
            }

            uint32_t events = ret_evs[i].events;
            if (~data->mask & events) {
                CORO_DEBUG("WARNING: unexpected events[%s] on fd[%d] with mask[%s]",
                        dbg_epoll_events(events).c_str(), fd, dbg_epoll_events(data->mask).c_str());
                /* TODO: figure it out (probably throw) */
            }

            uint32_t remove_mask = 0;
            for (auto &w : data->waiters) {
                if (w.mask & events) {
                    w.state->err = ERROR_OK;
                    ready_tasks.push_back(w.state);
                    remove_mask |= w.mask;
                }
            }
            error_e ret;
            if ((ret = remove_waiter(io_desc_t{ .fd = fd, .events = remove_mask })) != ERROR_OK) {
                CORO_DEBUG("FAILED to remove awaiter fd[%d] events%s",
                        fd, dbg_epoll_events(remove_mask).c_str());
                return ret;
            }
        }

        return ERROR_OK;
    }

    error_e add_waiter(state_t *state, const io_desc_t& io_desc) {
        if (!io_desc.events) {
            CORO_DEBUG("FAILED Can't await zero events fd[%d]", io_desc.fd);
            return ERROR_GENERIC;
        }
        fd_data_t *data = nullptr;
        if (data = get_data(io_desc.fd)) {
            if (data->mask & io_desc.events) {
                CORO_DEBUG("FAILED Can't wait on same events twice: fd[%d] existing%s attempt%s",
                        io_desc.fd,
                        dbg_epoll_events(data->mask).c_str(),
                        dbg_epoll_events(io_desc.events).c_str());
                return ERROR_GENERIC;
            }
            struct epoll_event ev = {};
            ev.events = io_desc.events | data->mask;
            ev.data.fd = io_desc.fd;
            int ret = 0;
            if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, io_desc.fd, &ev)) < 0) {
                CORO_DEBUG("FAILED epoll_ctl EPOLL_CTL_MOD fd[%d] events%s err:%s[%d] -> ret:%d",
                        io_desc.fd, dbg_epoll_events(ev.events).c_str(), strerror(errno), ret);
                return ERROR_GENERIC;
            }
            data->mask = ev.events;
            data->waiters.push_back(fd_data_t::waiter_t{
                .mask = io_desc.events,
                .state = state,
            });
            state->err = ERROR_GENERIC;
            return ERROR_OK;
        }
        else {
            struct epoll_event ev = {};
            ev.events = io_desc.events;
            ev.data.fd = io_desc.fd;
            int ret;
            if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, io_desc.fd, &ev)) < 0) {
                CORO_DEBUG("FAILED epoll_ctl EPOLL_CTL_ADD fd[%d] events%s err:%s[%d] -> ret:%d",
                        io_desc.fd, dbg_epoll_events(ev.events).c_str(), strerror(errno), ret);
                return ERROR_GENERIC;
            }
            data = alloc<fd_data_t>(pool, fd_data_t{
                .fd = io_desc.fd,
                .mask = ev.events,
                .waiters = std::vector<fd_data_t::waiter_t, allocator_t<fd_data_t::waiter_t>>{pool},
            });
            set_data(io_desc.fd, data);
            data->waiters.push_back(fd_data_t::waiter_t{
                .mask = io_desc.events,
                .state = state,
            });
            ret_evs.push_back(epoll_event{});
            state->err = ERROR_GENERIC;
            return ERROR_OK;
        }
    }

    error_e force_awake(const io_desc_t& io_desc, error_e retcode) {
        auto data = get_data(io_desc.fd);
        if (!data || !(data->mask & io_desc.events)) {
            return ERROR_OK;
        }

        uint32_t remove_mask = 0;
        for (auto &w : data->waiters) {
            if ((w.mask & io_desc.events)) {
                remove_mask |= w.mask;
                w.state->err = retcode;
                ready_tasks.push_back(w.state);
            }
        }
        error_e ret;
        if ((ret = remove_waiter(io_desc_t{ .fd = io_desc.fd, .events = remove_mask })) != ERROR_OK) {
            CORO_DEBUG("FAILED remove_waiter in force_awake fd[%d] events%s",
                    io_desc.fd, dbg_epoll_events(remove_mask).c_str());
            return ret;
        }
        return ERROR_OK;
    }

    error_e clear() {
        for (int i = 0; i < MAX_FAST_FD_CACHE; i++) {
            if (auto *data = fd_data_fast[i]) {
                for (auto &w : data->waiters) {
                    destroy_state(w.state);
                }
                if (remove_waiter(io_desc_t{ .fd = i, .events = 0xffff'ffff }) != ERROR_OK) {
                    CORO_DEBUG("FAILED to remove awaiter: fd:%d", i);
                    return ERROR_GENERIC;
                }
            }
            fd_data_fast[i] = nullptr;
        }
        for (auto &[fd, data] : fd_data_slow) {
            if (data) {
                for (auto &w : data->waiters) {
                    destroy_state(w.state);
                }
                if (remove_waiter(io_desc_t{ .fd = fd, .events = 0xffff'ffff }) != ERROR_OK) {
                    CORO_DEBUG("FAILED to remove awaiter: fd:%d", fd);
                    return ERROR_GENERIC;
                }
            }
        }
        fd_data_slow.clear();
        return ERROR_OK;
    }

private:
    error_e remove_waiter(const io_desc_t& io_desc) {
        auto data = get_data(io_desc.fd);
        if (!data) {
            CORO_DEBUG("FAILED attempted to remove an inexisting waiter fd[%d]", io_desc.fd);
            return ERROR_GENERIC;
        }
        if ((data->mask & ~io_desc.events) != 0) {
            struct epoll_event ev;
            ev.events = data->mask & ~io_desc.events;
            ev.data.fd = io_desc.fd;
            data->mask = ev.events;
            int ret;
            if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, io_desc.fd, &ev)) < 0) {
                CORO_DEBUG("FAILED epoll_ctl EPOLL_CTL_MOD fd[%d] events%s err:%s[%d] -> ret:%d",
                        io_desc.fd, dbg_epoll_events(ev.events).c_str(), strerror(errno), ret);
                return ERROR_GENERIC;
            }
            data->waiters.erase(
                std::remove_if(
                    data->waiters.begin(),
                    data->waiters.end(),
                    [events = io_desc.events](const fd_data_t::waiter_t& m) {
                        return events & m.mask;
                    }
                ),
                data->waiters.end()
            );
        }
        else {
            set_data(io_desc.fd, nullptr);
            int ret;
            if ((ret = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, io_desc.fd, NULL)) < 0) {
                CORO_DEBUG("FAILED epoll_ctl EPOLL_CTL_DEL fd[%d] err:%s[%d] -> ret:%d",
                        io_desc.fd, strerror(errno), ret);
                return ERROR_GENERIC;
            }
            ret_evs.pop_back();
        }
        return ERROR_OK;
    }

    fd_data_t *get_data(int fd) {
        if (fd < 0)
            return nullptr;
        if (fd >= MAX_FAST_FD_CACHE)
            return has(fd_data_slow, fd) ? fd_data_slow[fd] : nullptr;
        else
            return fd_data_fast[fd];
    }

    void set_data(int fd, fd_data_t *data) {
        /* for some reason, that I forgot, but there was a reason, I think, i preffered explicit
        allocation/deallocation, if you find it, the reason, consider sharing it with me. */
        if (auto p = get_data(fd))
            deallocator_t<fd_data_t>{pool}(p);
        if (fd < 0)
            return ;
        if (fd >= MAX_FAST_FD_CACHE) {
            if (!data)
                fd_data_slow.erase(fd);
            else
                fd_data_slow[fd] = data;
        }
        else {
            fd_data_fast[fd] = data;
        }
    }
    /* usually fds are at most 1024, so an array of ints is 4kb and will be way faster to access
    than a map */
    pool_t *pool = nullptr;
    fd_data_t *fd_data_fast[MAX_FAST_FD_CACHE] = {nullptr,};
    std::map<int, fd_data_t *, std::less<int>, allocator_t<std::pair<const int, fd_data_t *>>>
            fd_data_slow;

    std::vector<struct epoll_event, allocator_t<struct epoll_event>> ret_evs;

    std::deque<state_t *, allocator_t<state_t *>> &ready_tasks;
    int epoll_fd = -1;
};

/* same as above, this is a part of pool_internal_t and it stays here for potential later rewriting.
The idea is that timers need to be created by some sort of system provided mechanism and waiting
on it should be compatibile with the io_pool_t and as such, the timer should return a io_desc_t */
struct timer_pool_t {
    error_e get_timer(io_desc_t& new_timer) {
        if (stack_head > 0) {
            new_timer.fd = timer_stack[stack_head];
            new_timer.events = EPOLLIN;
            stack_head--;
            return ERROR_OK;
        }

        int timer_fd = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC);
        if (timer_fd < 0) {
            CORO_DEBUG("FAILED to allocate new timer err:%s[%d] -> ret: %d",
                    strerror(errno), errno, timer_fd);
            return ERROR_GENERIC;
        }

        new_timer.fd = timer_fd;
        new_timer.events = EPOLLIN;

        return ERROR_OK;
    }

    /* This function arms the timer and it will be triggered once the timer expires */
    /* not necesary to be a member function on Linux, but I feel it is better to put it here for
    future use */
    error_e set_timer(const io_desc_t& timer, const std::chrono::microseconds& time_us) {
        int64_t t = time_us.count();
        itimerspec its = {};
        its.it_value.tv_nsec = (t % 1000'000) * 1000ULL;
        its.it_value.tv_sec = t / 1000'000ULL;
        int ret = 0;
        if ((ret = timerfd_settime(timer.fd, 0, &its, NULL)) < 0) {
            CORO_DEBUG("FAILED to set expiration date fd[%d] err:%s[%d] -> ret: %d",
                    timer.fd, strerror(errno), errno, ret);
            return ERROR_GENERIC;
        }
        return ERROR_OK;
    }

    error_e free_timer(const io_desc_t& timer) {
        if (stack_head + 1 < MAX_TIMER_POOL_SIZE) {
            stack_head++;
            timer_stack[stack_head] = timer.fd;
        }
        else {
            close(timer.fd);
        }
        return ERROR_OK;
    }

    ~timer_pool_t() {
        while (stack_head >= 0) {
            close(timer_stack[stack_head]);
            stack_head--;
        }
    }
private:
    int timer_stack[MAX_TIMER_POOL_SIZE];
    int stack_head = -1;
};

/* If this is not really needed here we move it bellow */
struct pool_internal_t {
    pool_internal_t(pool_t *_pool)
    :   pool(_pool),
        ready_tasks{allocator_t<state_t *>{_pool}},
        io_pool{_pool, ready_tasks},
        sem_pool{allocator_t<sem_t *>{_pool}}
    {}

    template <typename T>
    void sched(task<T> task, modif_table_p parent_table) {
        /* first we give our new task the pool */
        task.h.promise().state.pool = pool;

        /* second we call our callbacks on it because it is now scheduled */
        if (do_sched_modifs(&task.h.promise().state, parent_table) != ERROR_OK) {
            return ;
        }

#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif

        /* third, we add the task to the pool */
        ready_tasks.push_back(&task.h.promise().state);
    }

    run_e run() {
        if (!io_pool.is_ok()) {
            CORO_DEBUG("the io pool is not working, check previous logs");
            return RUN_ERRORED;
        }

        dbg_register_name(task_t{std::noop_coroutine()}, "std::noop_coroutine");

        state_t *state = next_task_state();
        if (state == nullptr)
            return RUN_OK;

        ret_val = RUN_ABORTED;
        do_entry_modifs(state);
        state->self.resume();

        if (posted_exception) {
            auto pe = posted_exception;
            posted_exception = nullptr;
            std::rethrow_exception(pe);
        }

        return ret_val;
    }

    void set_exception(std::exception_ptr exc) {
        posted_exception = exc;
    }

    void push_ready(state_t *state) {
#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        ready_tasks.push_back(state);
    }

    void push_ready_front(state_t *state) {
#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        ready_tasks.push_front(state);
    }

    bool remove_ready(state_t *state) {
#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        for (auto it = ready_tasks.begin(); it != ready_tasks.end(); it++) {
            if (*it == state) {
                ready_tasks.erase(it);
                return true;
            }
        }
        return false;
    }

    state_t *next_task_state() {
        if (io_pool.handle_ready() != ERROR_OK) {
            CORO_DEBUG("Failed io pool");
            ret_val = RUN_ERRORED;
            return nullptr;
        }

#if CORO_ENABLE_MULTITHREAD_SCHED
        std::lock_guard guard(lock);
#endif
        if (!ready_tasks.empty()) {
            auto ret = ready_tasks.front();
            ready_tasks.pop_front();
            return ret;
        }

        /* we have nothing else to do, we resume the pool_t::run */
        ret_val = RUN_OK;
        return nullptr;
    }

    handle<void> next_task() {
        state_t *ns = next_task_state();
        return ns ? ns->self : std::noop_coroutine();
    }

    template <typename P>
    error_e wait_io(handle<P> h, const io_desc_t& io_desc) {
        /* add the file descriptor inside epoll and schedule the corutine for waiting on it */
        return io_pool.add_waiter(&h.promise().state, io_desc);
    }

    error_e stop_io(const io_desc_t& io_desc, error_e retcode) {
        /* stop the io and set it's return code as retcode */
        return io_pool.force_awake(io_desc, retcode);
    }

    error_e get_timer(io_desc_t& new_timer) {
        /* returns a timer object, referenced by the io descriptor new_timer, gaining ownership over
        it */
        return timer_pool.get_timer(new_timer);
    }

    error_e set_timer(const io_desc_t& timer, const std::chrono::microseconds& time_us) {
        /* arms the timer referenced by the descriptor timer to be awakened after time_us
        microseconds from now */
        return timer_pool.set_timer(timer, time_us);
    }

    error_e free_timer(const io_desc_t& timer) {
        /* releases the ownership of the timer back to the pool */
        return timer_pool.free_timer(timer);
    }

    void add_sem(sem_t *s) {
        sem_pool.insert(s);
    }

    void rm_sem(sem_t *s) {
        sem_pool.erase(s);
    }

    /* This function needs the semaphore definition so it is implemented bellow the semaphore */
    error_e clear();

    run_e ret_val;

private:
    pool_t *pool;
    std::deque<state_t *, allocator_t<state_t *>> ready_tasks;
    io_pool_t io_pool;
    timer_pool_t timer_pool;

    std::exception_ptr posted_exception = nullptr;

    /* bookkeeping for end of life destruction */
    std::set<sem_t *, std::less<sem_t *>, allocator_t<sem_t *>> sem_pool;

#if CORO_ENABLE_MULTITHREAD_SCHED
    std::mutex lock;
#endif
};

/* Allocate/deallocate need the definition of pool_internal_t */
template <typename T>
inline T* allocator_t<T>::allocate(size_t n) {
    T *ret = nullptr;
    if (!CORO_DISABLE_ALLOCATOR && pool && !std::is_same_v<char, T>)
        ret = static_cast<T*>(pool->allocator_memory->alloc(n * sizeof(T)));
    if (!ret)
        ret = static_cast<T*>(std::malloc(n * sizeof(T)));
    return ret;
}
template <typename T>
inline void allocator_t<T>::deallocate(T* _p, std::size_t) noexcept {
    if (!pool) {
        std::free(_p);
        return ;
    }

    char *p = (char *)_p;
    char *start = (char *)pool->allocator_memory.get();
    char *stop = start + sizeof(*pool->allocator_memory);

    if (p < start || p >= stop) {
        std::free(p);
        return ;
    }

    pool->allocator_memory->free(p);
}

inline handle<void> cpp_yield_awaiter(state_t *yielding_task_state) {
    /* bassicaly the same as bellow, except we don't destroy the corutine */
    do_leave_modifs(yielding_task_state);

    yielding_task_state->err = ERROR_YIELDED;
    state_t *caller_state = yielding_task_state->caller_state;

    /* from the point of view of the corutine modifications we are exiting here, this keeps the
    call stack proper */
    do_exit_modifs(yielding_task_state);

    if (caller_state) {
        return caller_state->self;
    }

    return yielding_task_state->pool->get_internal()->next_task();
}

inline handle<void> final_awaiter_cleanup(state_t *ending_task_state) {
    do_leave_modifs(ending_task_state);
    /* not sure if I should do something with the return value ... */
    state_t *caller_state = ending_task_state->caller_state;
    do_exit_modifs(ending_task_state);

    if (caller_state) {
        return caller_state->self;
    }
    auto pool = ending_task_state->pool;
    /* If the task that we are final_awaiting has no caller, then it is the moment to destroy it,
    no one needs it's return value. Else it will be destroyed by the caller. */
    /* TODO: maybe the future can just simply create a task and set it's coro as the continuation
    here? Such that the future continues in the caller_state branch? */
    if (ending_task_state->exception) {
        pool->get_internal()->set_exception(ending_task_state->exception);
        ending_task_state->self.destroy();
        return std::noop_coroutine();
    }
    ending_task_state->self.destroy();
    return pool->get_internal()->next_task();
}

inline pool_t::pool_t() {
    allocator_memory = std::make_unique<allocator_memory_t>();
    internal = std::make_unique<pool_internal_t>(this);
}

template <typename T>
inline void pool_t::sched(task<T> task, const modif_pack_t& v) {
    internal->sched(task, create_modif_table(this, v));
}

inline run_e pool_t::run() {
    return internal->run();
}

/*  OBS: if clear is called, it is called from outside of the pool, else this is UB
    There are 4 places where tasks can reside:
        1. the io_pool - waiting on some sort of event
        2. a semaphore queue - waiting on a semaphore to signal
        3. the ready queue - waiting to be rescheduled
        4. call stacks - waiting for a call to finish

    Tasks can be created by either:
        a. sched
        b. call

    So on clear we will destroy the corutines in the following order:
        *. whenever a corutine is terminated, also terminate the caller (state_t *caller_state)
        1. terminate all corutines waiting on the io_pool
        2. terminate all the corutines waiting on semaphores
        3. terminate all corutines waiting in the ready queue, in the ready order, terminating
 */
inline error_e pool_t::clear() {
    return get_internal()->clear();
}

inline pool_internal_t *pool_t::get_internal() {
    return internal.get();
}

inline error_e pool_t::stop_io(const io_desc_t& io_desc) {
    return get_internal()->stop_io(io_desc, ERROR_WAKEUP);
}

inline error_e pool_t::stop_fd(int fd) {
    return stop_io(io_desc_t{.fd = fd});
}

inline std::shared_ptr<pool_t> create_pool() {
    return std::shared_ptr<pool_t>(new pool_t{});
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
        auto pool = h.promise().state.pool;
        state = &h.promise().state;
        do_leave_modifs(&h.promise().state);
        pool->get_internal()->push_ready(&h.promise().state);
        return pool->get_internal()->next_task();
    }

    void await_resume() {
        do_entry_modifs(state);
    }

    state_t *state;
};

template <typename T>
struct sched_awaiter_t {
    sched_awaiter_t(task<T> t, std::vector<modif_p> v) : t(t), v(v) {}
    sched_awaiter_t(const sched_awaiter_t &oth) = delete;
    sched_awaiter_t &operator = (const sched_awaiter_t &oth) = delete;
    sched_awaiter_t(sched_awaiter_t &&oth) = delete;
    sched_awaiter_t &operator = (sched_awaiter_t &&oth) = delete;

    bool await_ready() { return false;}
    void await_resume() {}

    template <typename P>
    bool await_suspend(handle<P> h) {
        auto pool = h.promise().state.pool;
        pool->get_internal()->sched(t, create_modif_table(pool, v));
        return false;
    }

    task<T> t;
    std::vector<modif_p> v;
};

struct get_pool_awaiter_t {
    get_pool_awaiter_t() {}
    get_pool_awaiter_t(const get_pool_awaiter_t &oth) = delete;
    get_pool_awaiter_t &operator = (const get_pool_awaiter_t &oth) = delete;
    get_pool_awaiter_t(get_pool_awaiter_t &&oth) = delete;
    get_pool_awaiter_t &operator = (get_pool_awaiter_t &&oth) = delete;

    template <typename P>
    bool await_suspend(handle<P> h) {
        pool = h.promise().state.pool;
        return false;
    }

    bool    await_ready()              { return false; };
    pool_t *await_resume()             { return pool; }

    pool_t *pool;
};


struct get_state_awaiter_t {
    get_state_awaiter_t() {}
    get_state_awaiter_t(const get_state_awaiter_t &oth) = delete;
    get_state_awaiter_t &operator = (const get_state_awaiter_t &oth) = delete;
    get_state_awaiter_t(get_state_awaiter_t &&oth) = delete;
    get_state_awaiter_t &operator = (get_state_awaiter_t &&oth) = delete;

    template <typename P>
    bool await_suspend(handle<P> h) {
        state = &h.promise().state;
        return false;
    }

    bool     await_ready()              { return false; };
    state_t *await_resume()             { return state; }

    state_t *state;
};

/* This is the object with which you wait for a file descriptor, you create it with a valid
wait_cond and fd and co_await on it. The resulting integer must be positive to not be an error */
struct io_awaiter_t {
    io_desc_t io_desc;
    error_e err;

    io_awaiter_t(const io_desc_t& io_desc) : io_desc(io_desc) {}
    io_awaiter_t(const io_awaiter_t &oth) = delete;
    io_awaiter_t &operator = (const io_awaiter_t &oth) = delete;
    io_awaiter_t(io_awaiter_t &&oth) = delete;
    io_awaiter_t &operator = (io_awaiter_t &&oth) = delete;

    bool await_ready() { return false; };

    template <typename P>
    handle<void> await_suspend(handle<P> h) {
        auto pool = h.promise().state.pool;
        state = &h.promise().state;
        /* in case we can't schedule the fd we log the failure and return the same coro */
        do_leave_modifs(state);
        if (do_wait_io_modifs(state, io_desc) != ERROR_OK) {
            do_entry_modifs(state);
            return h;
        }
        if ((err = pool->get_internal()->wait_io(h, io_desc)) != ERROR_OK) {
            CORO_DEBUG("Failed to register wait: %s on: %s",
                    dbg_enum(err).c_str(), dbg_name(h).c_str());
            do_entry_modifs(state);
            return h;
        }
        return pool->get_internal()->next_task();
    }

    error_e await_resume() {
        do_unwait_io_modifs(state, io_desc);
        do_entry_modifs(state);
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
    using sem_aloc = allocator_t<std::pair<std::coroutine_handle<void>, std::shared_ptr<void>>>;

    sem_internal_t(pool_t *pool, int64_t val) : pool(pool), val(val),
            waiting_on_sem(sem_aloc{pool}) {}

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
        if (val == 0) {
            /* for negative initialized semaphores: if there is no awaiter we create a slot to be
            taken by the first awaiter by increasing the counter twice. It is, at least for me,
            more intuitive to initialize the semaphore with a negative number amounting to the
            count of signals needed to awake the semaphore, not with that number -1 */
            if (waiting_on_sem.size())
                return _awake_one();
            else
                val++;
        }

        /* we awake when val is 0, do nothing on negative, only increment and on positive we
        don't have awaiters */
        return ERROR_OK;
    }

    error_e clear() {
        while (waiting_on_sem.size()) {
            auto to_awake = waiting_on_sem.back();
            destroy_state(to_awake.first);
            waiting_on_sem.pop_back();
        }
        return ERROR_OK;
    }

    /* be carefull with this one */
    void erase_waiter(sem_wait_list_it it) {
        waiting_on_sem.erase(it);
    }

protected:
    friend sem_awaiter_t;
    friend sem_t;
    friend inline sem_p create_sem(pool_t *pool, int64_t val);

    sem_waiter_handle_p push_waiter(state_t *state) {
        /* TODO: explanation no longer true, search other solution */
        /* Don't know another way to fix this mess, so, yeah... The problem is that the awaiter
        bellow needs a way to register the waiter on this list so it may potentialy awake it (via the
        modif callbacks). Now the problem is that this semaphore may also want to awake the waiter,
        wich would invalidate the iterator inside the callback. So we give both the callback and
        the awaiter bellow a weak pointer to a pointer held at the same place with the corutine,
        so that if the corutine awakes, the pointer dies and the weak_pointer locks to null. */
        auto p = std::shared_ptr<sem_wait_list_it>(
                alloc<sem_wait_list_it>(pool), dealloc_create<sem_wait_list_it>(pool),
                allocator_t<int>{pool});
        waiting_on_sem.push_front({state, p});
        *p = waiting_on_sem.begin();
        return p;
    }


    pool_t *get_pool() {
        return pool;
    }

private:
    error_e _awake_one() {
        auto to_awake = waiting_on_sem.back();
        pool->get_internal()->push_ready(to_awake.first);
        waiting_on_sem.pop_back();
        return ERROR_OK;
    }

    sem_wait_list_t waiting_on_sem;
    int64_t val;
    pool_t *pool;
};

inline error_e pool_internal_t::clear() {
    if (io_pool.clear() != ERROR_OK) {
        CORO_DEBUG("FAILED to clear events waiting for io");
        return ERROR_GENERIC;
    }
    for (auto &s : sem_pool) {
        if (s->get_internal()->clear() != ERROR_OK) {
            CORO_DEBUG("FAILED to clear events waiting on one of the semaphores");
            return ERROR_GENERIC; 
        }
    }
    for (auto &state : ready_tasks) {
        destroy_state(state);
    }
    ready_tasks.clear();
    return ERROR_OK;
}

/* TODO: find a way for sem_waiter_handle_p to make sense */
struct sem_awaiter_t {
    sem_awaiter_t(sem_t *sem) : sem(sem) {}
    sem_awaiter_t(const sem_awaiter_t &oth) = delete;
    sem_awaiter_t &operator = (const sem_awaiter_t &oth) = delete;
    sem_awaiter_t(sem_awaiter_t &&oth) = delete;
    sem_awaiter_t &operator = (sem_awaiter_t &&oth) = delete;

    template <typename P>
    handle<void> await_suspend(handle<P> to_suspend) {
        state = &to_suspend.promise().state;

        auto pool = sem->get_internal()->get_pool();
        psem_it = sem->get_internal()->push_waiter(state);

        do_leave_modifs(state);
        if (do_wait_sem_modifs(state, sem, psem_it) != ERROR_OK) {
            sem->get_internal()->erase_waiter(*psem_it);
            do_entry_modifs(state);
            return to_suspend;
        }

        triggered = true;
        return pool->get_internal()->next_task();
    }

    sem_t::unlocker_t await_resume() {
        if (triggered) {
            do_unwait_sem_modifs(state, sem, psem_it);
            do_entry_modifs(state);
        }
        triggered = false;
        return sem_t::unlocker_t(sem);
    }

    bool await_ready() {
        return sem->get_internal()->await_ready();
    }

    state_t *state = nullptr;
    sem_t *sem = nullptr;
    sem_waiter_handle_p psem_it;
    bool triggered = false;
};

inline sem_t::sem_t(pool_t *pool, int64_t val)
: internal(std::unique_ptr<sem_internal_t, deallocator_t<sem_internal_t>>(
        alloc<sem_internal_t>(pool, pool, val), dealloc_create<sem_internal_t>(pool)))
{
    get_internal()->pool->get_internal()->add_sem(this);
}

inline sem_t::~sem_t() {
    get_internal()->clear();
    get_internal()->pool->get_internal()->rm_sem(this);
}

inline sem_awaiter_t sem_t::wait() { return sem_awaiter_t(this); }
inline error_e       sem_t::signal() { return internal->signal(); }
inline bool          sem_t::try_dec() { return internal->try_dec(); }

inline sem_internal_t *sem_t::get_internal() {
    return internal.get();
}

inline sem_p create_sem(pool_t *pool, int64_t val) {
    auto ret = std::shared_ptr<sem_t>(
            alloc<sem_t>(pool, pool, val), dealloc_create<sem_t>(pool), allocator_t<int>{pool}); 
    return ret;
}
inline sem_p create_sem(pool_p pool, int64_t val) {
    return create_sem(pool.get(), val);
}

inline task<sem_p> create_sem(int64_t val) {
    co_return create_sem(co_await get_pool(), val);
}

/* Functions
------------------------------------------------------------------------------------------------  */

template <typename ...ret_v>
inline task<std::tuple<ret_v...>> wait_all(task<ret_v>... tasks) {
    auto pool = co_await get_pool();

    std::tuple<task<ret_v>...> futures = {create_future(pool, tasks)...};

    (co_await sched(tasks), ...);

    co_return co_await std::apply([](auto&&... futures) -> task<std::tuple<ret_v...>> {
        co_return std::tuple<ret_v...>{co_await futures...};
    }, futures);
}

template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const modif_pack_t& v) {
    return sched_awaiter_t(to_sched, v);
}

template <typename Awaiter>
inline task_t await(Awaiter&& awaiter) {
    co_await std::forward<Awaiter>(awaiter);
    co_return ERROR_OK;
}

inline yield_awaiter_t yield() {
    return yield_awaiter_t{};
}

inline task<pool_t *> get_pool() {
    get_pool_awaiter_t awaiter;
    co_return co_await awaiter;
}

inline task<state_t *> get_state() {
    get_state_awaiter_t awaiter;
    co_return co_await awaiter;
}

inline task_t stop_io(const io_desc_t& io_desc) {
    co_return (co_await get_pool())->get_internal()->stop_io(io_desc, ERROR_WAKEUP);
}

inline task_t stop_fd(int fd) {
    /* gets the fd out of the poll, awakening all it's waiters with error_e ERROR_WAKEUP */
    co_return (co_await stop_io(io_desc_t{.fd = fd}));
}

inline task_t force_stop(int64_t stopval) {
    struct stop_awaiter_t {
        stop_awaiter_t(int64_t stopval) : stopval(stopval) {}

        bool await_ready() { return false; };

        handle<void> await_suspend(handle<task_state_t<int>> h) {
            state = &h.promise().state;
            do_leave_modifs(state);
            state->pool->get_internal()->push_ready_front(state);
            state->pool->get_internal()->ret_val = RUN_STOPPED;
            state->pool->stopval = stopval;
            return std::noop_coroutine();
        }

        error_e await_resume() {
            do_entry_modifs(state);
            return ERROR_OK; /* no errors to be had here */
        }

        int64_t stopval = 0;
        state_t *state = nullptr;
    };
    /* this interrupts the pool, if you continue it, it will continue from this place */
    co_return (co_await stop_awaiter_t{stopval});
}

inline task_t sleep(const std::chrono::microseconds& timeo) {
    auto pool = co_await get_pool();
    io_desc_t timer;
    error_e err;
    if ((err = pool->get_internal()->get_timer(timer)) != ERROR_OK) {
        CORO_DEBUG("FAILED to get a timer %s", dbg_enum(err).c_str());
        co_return err;
    }
    if ((err = pool->get_internal()->set_timer(timer, timeo)) != ERROR_OK) {
        CORO_DEBUG("FAILED to set a timer %s", dbg_enum(err).c_str());
        error_e err_err;
        if ((err_err = pool->get_internal()->free_timer(timer)) != ERROR_OK) {
            CORO_DEBUG("FAILED to free(on error: %s) the timer %s",
                    dbg_enum(err).c_str(), dbg_enum(err_err).c_str());
        }
        co_return err;
    }

    FnScope scope([pool, timer] {
        /* this function can be called on a kill */
        error_e err_err;
        if ((err_err = pool->get_internal()->free_timer(timer)) != ERROR_OK) {
            CORO_DEBUG("FAILED to free(on error: %s) the timer %s",
                    dbg_enum(err_err).c_str());
            /* good place for an exception warning */
        }
    });
    io_awaiter_t awaiter(timer);
    if ((err = co_await awaiter) != ERROR_OK) {
        CORO_DEBUG("FAILED waiting on timer: %s", dbg_enum(err).c_str());
        scope.precall();
        co_return err;
    }

    scope.precall();
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

inline task_t wait_event(const io_desc_t& io_desc) {
    io_awaiter_t awaiter(io_desc);
    co_return (co_await awaiter);
}

inline task_t connect(int fd, sockaddr *sa, socklen_t len) {
    /* connect is special, first we take it and make it non-blocking for the initial connection */
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) {
        CORO_DEBUG("FAILED to get old flags for fd[%d] %s[%d]",
                fd, strerror(errno), errno);
        co_return ERROR_GENERIC;
    }
    bool need_nonblock = (flags & O_NONBLOCK) == 0;
    if (need_nonblock && (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
        CORO_DEBUG("FAILED to toggle on the O_NONBLOCK flag on fd[%d] %s[%d]",
                fd, strerror(errno), errno);
        co_return ERROR_GENERIC;
    }

    int res = ::connect(fd, sa, len);

    if (need_nonblock && (flags = fcntl(fd, F_GETFL, 0) < 0)) {
        CORO_DEBUG("FAILED to get the new flags for the fd[%d] %s[%d]",
                fd, strerror(errno), errno);
        co_return ERROR_GENERIC;
    }
    if (need_nonblock && (fcntl(fd, F_SETFL, flags & ~O_NONBLOCK) < 0)) {
        CORO_DEBUG("FAILED to toggle off the O_NONBLOCK flag on fd[%d] %s [%d]",
                fd, strerror(errno), errno);
        co_return ERROR_GENERIC;
    }

    /* now that our intention to connect was sent to the os we first check if the connection was
    not done */
    if (res < 0 && errno != EINPROGRESS) {
        co_return res;
    }

    if (res == 0)
        co_return 0;

    /* if not, then we need to create an awaiter */
    io_awaiter_t awaiter{io_desc_t{
        .fd = fd,
        .events = EPOLLOUT,
    }};
    error_e err = co_await awaiter;

    if (err != ERROR_OK) {
        CORO_DEBUG("Failed waiting operation on %d co_err: %s errno: %s[%d]",
                fd, dbg_enum(err).c_str(), strerror(errno), errno);
        co_return err;
    }

    /* now that we where signaled back by the os, we can check that we are connected and return: */
    int result;
    socklen_t result_len = sizeof(result);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
        CORO_DEBUG("FAILED getsockopt: fd: %d err: %s[%d]", fd, strerror(errno), errno);
        co_return ERROR_GENERIC;
    }

    if (result != 0) {
        CORO_DEBUG("FAILED connect: fd: %d err: %s[%d]", fd, strerror(errno), errno);
        co_return result;
    }

    co_return ERROR_OK;
}

inline task_t accept(int fd, sockaddr *sa, socklen_t *len) {
    io_awaiter_t awaiter(io_desc_t{
        .fd = fd,
        .events = EPOLLIN | EPOLLHUP,
    });
    error_e err = co_await awaiter;
    if (err != ERROR_OK) {
        // CORO_DEBUG("Failed waiting operation on %d co_err: %s errno: %s[%d]",
        //         fd, dbg_enum(err).c_str(), strerror(errno), errno);
        co_return err;
    }
    /* OBS: those functions are a bit special, they return the result of the operation, not only
    the enums listed in the lib */
    co_return ::accept(fd, sa, len); 
}

inline task<ssize_t> read(int fd, void *buff, size_t len) {
    io_awaiter_t awaiter(io_desc_t{
        .fd = fd,
        .events = EPOLLIN,
    });
    error_e err = co_await awaiter;
    if (err != ERROR_OK) {
        // CORO_DEBUG("Failed waiting operation on %d co_err: %s errno: %s[%d]",
        //         fd, dbg_enum(err).c_str(), strerror(errno), errno);
        co_return err;
    }
    /* OBS: those functions are a bit special, they return the result of the operation, not only
    the enums listed in the lib */
    co_return ::read(fd, buff, len);
}

inline task<ssize_t> write(int fd, const void *buff, size_t len) {
    io_awaiter_t awaiter(io_desc_t{
        .fd = fd,
        .events = EPOLLOUT,
    });
    error_e err = co_await awaiter;
    if (err != ERROR_OK) {
        // CORO_DEBUG("Failed waiting operation on %d co_err: %s errno: %s[%d]",
        //         fd, dbg_enum(err).c_str(), strerror(errno), errno);
        co_return err;
    }
    /* OBS: those functions are a bit special, they return the result of the operation, not only
    the enums listed in the lib */
    co_return ::write(fd, buff, len);
}

inline task<ssize_t> read_sz(int fd, void *buff, size_t len) {
    ssize_t original_len = len;
    while (true) {
        if (!len)
            break ;
        ssize_t ret = co_await read(fd, buff, len);
        if (ret == 0) {
            CORO_DEBUG("Read failed, peer is closed, fd: %d", fd);
            co_return -1;
        }
        else if (ret < 0) {
            CORO_DEBUG("Failed read, fd: %d", fd);
            co_return ret;
        }
        else {
            len -= ret;
            buff = (char *)buff + ret;
        }
    }
    co_return original_len;
}

inline task<ssize_t> write_sz(int fd, const void *buff, size_t len) {
    ssize_t original_len = len;
    while (true) {
        if (!len)
            break ;
        ssize_t ret = co_await write(fd, buff, len);
        if (ret < 0) {
            CORO_DEBUG("Failed write, fd: %d", fd);
            co_return ret;
        }
        else {
            len -= ret;
            buff = (char *)buff + ret;
        }
    }
    co_return original_len;
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
inline task<T> add_modifs(pool_t *pool, task<T> t, const std::set<modif_p>& mods) {
    auto &table = t.h.promise().state.modif_table;
    if (!table)
        table = std::shared_ptr<modif_table_t>(alloc<modif_table_t>(pool, pool),
                dealloc_create<modif_table_t>(pool), allocator_t<int>{pool});
    add_modifs_to_table(table, mods);
    if (get_modif_table_sz(table) == 0)
        table = nullptr;
    return t;
}

template <typename T>
inline task<T> rm_modifs(task<T> t, const std::set<modif_p>& mods) {
    auto &table = t.h.promise().state.modif_table;
    rm_modifs_from_table(table, mods);
    if (get_modif_table_sz(table) == 0)
        table = nullptr;
    return t;
}

inline task_t add_modifs(const std::set<modif_p>& mods) {
    auto &table = (co_await get_state())->modif_table;
    auto pool = co_await get_pool();
    if (!table)
        table = std::shared_ptr<modif_table_t>(alloc<modif_table_t>(pool, pool),
                dealloc_create<modif_table_t>(pool), allocator_t<int>{pool});
    add_modifs_to_table(table, mods);
    if (get_modif_table_sz(table) == 0)
        table = nullptr;
    co_return ERROR_OK;
}

inline task_t rm_modifs(const std::set<modif_p>& mods) {
    auto table = co_await task_modifs_getter_t{};
    rm_modifs_from_table(table, mods);
    if (get_modif_table_sz(table) == 0)
        table = nullptr;
    co_return ERROR_OK;
}

template <typename T>
inline task<T> create_future(pool_t *pool, task<T> t) {
    using data_t = std::pair<T, bool>;

    auto sem = create_sem(pool, 0);
    auto data = std::shared_ptr<data_t>(alloc<data_t>(pool),
            dealloc_create<data_t>(pool), allocator_t<int>{pool});

    data->second = false;

    /* ! this std::function will not be used using our allocator */
    auto exit_func = [sem, data](state_t *state) -> error_e {
        typename task<T>::handle_t h = task<T>::handle_t::from_address(state->self.address());
        data->first = std::get<T>(h.promise().ret);
        data->second = true;
        sem->signal();
        return ERROR_OK;
    };

    add_modifs(pool, t, std::set<modif_p>{
            create_modif<CO_MODIF_EXIT_CBK>(pool, CO_MODIF_INHERIT_NONE, exit_func)});

    return [](sem_p sem, std::shared_ptr<data_t> data) -> task<T> {
        if (!data->second) {
            co_await sem->wait();
        }
        co_return data->first;
    }(sem, data);
}

template <typename T>
inline task<std::pair<T, error_e>> create_timeo(
        task<T> t, pool_t *pool, const std::chrono::microseconds& timeo)
{
    /* TODO: create the 'modif packet' that will kill the corutines in the call-path when the
    timer expires */
    struct timer_state_t {
        std::function<error_e(void)> timer_elapsed_sig;
        std::function<error_e(void)> timer_sig;
        sem_p sem;
        uint64_t duration;
        error_e err;
        task<T> t;
        T ret;
    };

    auto tstate = std::shared_ptr<timer_state_t>(alloc<timer_state_t>(pool),
            dealloc_create<timer_state_t>(pool), allocator_t<int>{pool});

    auto [timer_elapsed_killer, timer_elapsed_sig] = create_killer(pool, ERROR_WAKEUP);
    auto [timer_killer, timer_sig] = create_killer(pool, ERROR_TIMEOUT);

    tstate->sem = create_sem(pool, 0);
    tstate->timer_elapsed_sig = timer_elapsed_sig;
    tstate->timer_sig = timer_sig;
    tstate->duration = timeo.count();
    tstate->err = ERROR_OK;
    tstate->t = t;

    auto exec_coro = [](std::shared_ptr<timer_state_t> tstate) -> task_t {
        tstate->ret = co_await tstate->t;
        tstate->timer_sig();
        tstate->sem->signal();
        tstate->err = ERROR_OK;
        co_return ERROR_OK;
    }(tstate);

    auto timer_coro = [](std::shared_ptr<timer_state_t> tstate) -> task_t {
        error_e err;
        if ((err = co_await sleep_us(tstate->duration)) != ERROR_OK) {
            tstate->err = err;
            tstate->timer_elapsed_sig();
            co_return ERROR_GENERIC;
        }
        tstate->err = ERROR_TIMEOUT;
        tstate->timer_elapsed_sig();
        tstate->sem->signal();
        co_return ERROR_OK;
    }(tstate);

    add_modifs(pool, timer_coro, timer_killer);
    add_modifs(pool, exec_coro, timer_elapsed_killer);

    pool->sched(timer_coro);
    pool->sched(exec_coro);

    return [](std::shared_ptr<timer_state_t> tstate) -> task<std::pair<T, error_e>>{
        co_await tstate->sem->wait();
        co_return {tstate->ret, tstate->err};
    }(tstate);
}

/* TODO: link the kill-fn to the pack on a 1 to 1 basis (you can't use one pack for multiple
call-stacks) */
/* CAUTION: this doesn't kill sched paths (for example 'futures' or 'wait_all') */
/* this is inherited by-call and it must kill all coros in the call path and also stop all waiters
(io and sem) */
/* CAUTION: This will be similar to an exception thrown on the active await and the call stack */
inline std::pair<modif_pack_t, std::function<error_e(void)>> create_killer(pool_t *pool, error_e e) {
    struct kill_state_t {
        std::stack<state_t *> call_stack;
        io_desc_t io_desc;
        sem_t *sem = nullptr;
        sem_waiter_handle_p it;
    };

    auto kstate = std::shared_ptr<kill_state_t>(alloc<kill_state_t>(pool),
            dealloc_create<kill_state_t>(pool), allocator_t<int>{pool});

    modif_flags_e flags = CO_MODIF_INHERIT_ON_CALL;

    /* ! those std::functions will not be used using our allocator */
    auto sig_kill = [kstate, e]() -> error_e {
        /* If there is no call stack we have nothing to awake */
        if (kstate->call_stack.size() == 0)
            return ERROR_GENERIC;

        /* the top of the stack holds a pointer to the pool */
        auto pool = kstate->call_stack.top()->pool;

        /* if we are waiting for something in the top level we will first try to see if the last
        pushed state is ready for resuming, case in witch we remove it from the ready queue. Else
        we remove it from the io or semaphore waiting queue, whichever was holding the awaiter. */
        if ((kstate->sem || kstate->io_desc.fd != -1) &&
                pool->get_internal()->remove_ready(kstate->call_stack.top()))
        {
            kstate->sem = nullptr;
            kstate->io_desc = io_desc_t{};
        }
        else if (kstate->sem) {
            kstate->sem->get_internal()->erase_waiter(*kstate->it);
            kstate->sem = nullptr;
        }
        else if (kstate->io_desc.fd != -1) {
            pool->get_internal()->stop_io(kstate->io_desc, ERROR_WAKEUP);
            kstate->io_desc = io_desc_t{};
        }

        /* Now we unwind the call stack, removing all except the last one. The last one will be
        removed by it's caller */
        while (kstate->call_stack.size() > 1) {
            kstate->call_stack.top()->self.destroy();
            kstate->call_stack.pop();
        }

        /* Finally we prepare the root of the trace and schedule it's caller */
        kstate->call_stack.top()->err = e;
        pool->get_internal()->push_ready(kstate->call_stack.top());
        kstate->call_stack.pop();
        return ERROR_OK;
    };

    modif_pack_t pack;
    pack.push_back(create_modif<CO_MODIF_CALL_CBK>(pool, flags,
        [kstate](state_t *s) -> error_e {
            kstate->call_stack.push(s);
            return ERROR_OK;
        }
    ));
    pack.push_back(create_modif<CO_MODIF_EXIT_CBK>(pool, flags,
        [kstate](state_t *s) -> error_e {
            kstate->call_stack.pop();
            return ERROR_OK;
        }
    ));
    pack.push_back(create_modif<CO_MODIF_WAIT_IO_CBK>(pool, flags,
        [kstate](state_t *s, io_desc_t &io_desc) -> error_e {
            kstate->io_desc = io_desc;
            return ERROR_OK;
        }
    ));
    pack.push_back(create_modif<CO_MODIF_UNWAIT_IO_CBK>(pool, flags,
        [kstate](state_t *s, io_desc_t &io_desc) -> error_e {
            kstate->io_desc = io_desc_t{};
            return ERROR_OK;
        }
    ));
    pack.push_back(create_modif<CO_MODIF_WAIT_SEM_CBK>(pool, flags,
        [kstate](state_t *s, sem_t *sem, sem_waiter_handle_p it) -> error_e {
            /* TODO: save semaphore here */
            kstate->sem = sem;
            kstate->it = it;
            return ERROR_OK;
        }
    ));
    pack.push_back(create_modif<CO_MODIF_UNWAIT_SEM_CBK>(pool, flags,
        [kstate](state_t *s, sem_t *sem, sem_waiter_handle_p it) -> error_e {
            kstate->sem = nullptr;
            return ERROR_OK;
        }
    ));

    return {pack, sig_kill};
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
inline dbg_string_t dbg_name(task<T> t) {
    return dbg_name(t.h.address());
}

template <typename P>
inline dbg_string_t dbg_name(handle<P> h) {
    return dbg_name(h.address());
}

inline dbg_string_t dbg_enum(error_e code) {
    switch (code) {
        case ERROR_OK:      return dbg_string_t{"ERROR_OK",        allocator_t<char>{nullptr}};
        case ERROR_GENERIC: return dbg_string_t{"ERROR_GENERIC",   allocator_t<char>{nullptr}};
        case ERROR_TIMEOUT: return dbg_string_t{"ERROR_TIMEOUT",   allocator_t<char>{nullptr}};
        case ERROR_WAKEUP:  return dbg_string_t{"ERROR_WAKEUP",    allocator_t<char>{nullptr}};
        case ERROR_USER:    return dbg_string_t{"ERROR_USER",      allocator_t<char>{nullptr}};
        case ERROR_DEPEND:  return dbg_string_t{"ERROR_DEPEND",    allocator_t<char>{nullptr}};
        default:            return dbg_string_t{"[ERROR_UNKNOWN]", allocator_t<char>{nullptr}};
    }
}

inline dbg_string_t dbg_enum(run_e code) {
    dbg_string_t ret{"", allocator_t<char>{nullptr}};
    switch (code) {
        case RUN_OK:      return dbg_string_t{"RUN_OK"       , allocator_t<char>{nullptr}};      
        case RUN_ERRORED: return dbg_string_t{"RUN_ERRORED"  , allocator_t<char>{nullptr}}; 
        case RUN_ABORTED: return dbg_string_t{"RUN_ABORTED"  , allocator_t<char>{nullptr}}; 
        case RUN_STOPPED: return dbg_string_t{"RUN_STOPPED"  , allocator_t<char>{nullptr}};
        default:          return dbg_string_t{"[RUN_UNKNOWN]", allocator_t<char>{nullptr}};
    }
}

inline dbg_string_t dbg_epoll_events(uint32_t events) {
    dbg_string_t ret{"[", allocator_t<char>{nullptr}};
    if (events & EPOLLIN)    ret += "EPOLLIN|";
    if (events & EPOLLOUT)   ret += "EPOLLOUT|";
    if (events & EPOLLRDHUP) ret += "EPOLLRDHUP|";
    if (events & EPOLLPRI)   ret += "EPOLLPRI|";
    if (events & EPOLLERR)   ret += "EPOLLERR|";
    if (events & EPOLLHUP)   ret += "EPOLLHUP|";
    if (ret.size() > 1)
        ret[ret.size() - 1] = ']';
    else
        ret += "]";
    return ret;
}

inline modif_pack_t dbg_create_tracer(pool_t *pool) {
    modif_flags_e flags = modif_flags_e(CO_MODIF_INHERIT_ON_CALL | CO_MODIF_INHERIT_ON_SCHED);
    modif_pack_t mods;

    mods.push_back(create_modif<CO_MODIF_CALL_CBK>(pool, flags, [&](state_t *s) -> error_e {
        CORO_DEBUG(">  CALL: %s", dbg_name(s->self).c_str());
        return ERROR_OK;
    }));
    mods.push_back(create_modif<CO_MODIF_SCHED_CBK>(pool, flags, [&] (state_t *s) -> error_e {
        CORO_DEBUG("> SCHED: %s", dbg_name(s->self).c_str());
        return ERROR_OK;
    }));
    mods.push_back(create_modif<CO_MODIF_EXIT_CBK>(pool, flags, [&] (state_t *s) -> error_e {
        CORO_DEBUG(">  EXIT: %s", dbg_name(s->self).c_str());
        return ERROR_OK;
    }));
    mods.push_back(create_modif<CO_MODIF_LEAVE_CBK>(pool, flags, [&] (state_t *s) -> error_e {
        CORO_DEBUG("> LEAVE: %s", dbg_name(s->self).c_str());
        return ERROR_OK;
    }));
    mods.push_back(create_modif<CO_MODIF_ENTER_CBK>(pool, flags, [&] (state_t *s) -> error_e {
        CORO_DEBUG("> ENTRY: %s", dbg_name(s->self).c_str());
        return ERROR_OK;
    }));
    mods.push_back(create_modif<CO_MODIF_WAIT_IO_CBK>(pool, flags,
        [&] (state_t *s, io_desc_t &io_desc) -> error_e {
            CORO_DEBUG(">  WAIT: %s", dbg_name(s->self).c_str());
            return ERROR_OK;
        })
    );
    mods.push_back(create_modif<CO_MODIF_UNWAIT_IO_CBK>(pool, flags,
        [&] (state_t *s, io_desc_t &io_desc) -> error_e {
            CORO_DEBUG(">UNWAIT: %s", dbg_name(s->self).c_str());
            return ERROR_OK;
        }
    ));
    mods.push_back(create_modif<CO_MODIF_WAIT_SEM_CBK>(pool, flags,
        [&] (state_t *s, sem_t *sem, sem_waiter_handle_p _it) -> error_e {
            CORO_DEBUG(">   SEM: %s", dbg_name(s->self).c_str());
            return ERROR_OK;
        }
    ));
    mods.push_back(create_modif<CO_MODIF_UNWAIT_SEM_CBK>(pool, flags,
        [&] (state_t *s, sem_t *sem, sem_waiter_handle_p _it) -> error_e {
            CORO_DEBUG("> UNSEM: %s", dbg_name(s->self).c_str());
            return ERROR_OK;
        }
    ));
    return mods;
}

template <typename... Args>
inline dbg_string_t dbg_format(const char *fmt, Args&& ...args) {
    std::vector<char, allocator_t<char>> buff(allocator_t<char>{nullptr});

    int cnt = snprintf(NULL, 0, fmt, std::forward<Args>(args)...);
    if (cnt <= 0)
        return dbg_string_t{"[failed snprintf1]", allocator_t<char>{nullptr}};

    buff.resize(cnt + 1);
    if (snprintf(buff.data(), cnt + 1, fmt, std::forward<Args>(args)...) < 0)
        return dbg_string_t{"[failed snprintf2]", allocator_t<char>{nullptr}};

    return dbg_string_t{buff.data(), allocator_t<char>{nullptr}};
}

inline uint64_t dbg_get_time() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

#if CORO_ENABLE_DEBUG_NAMES

/* TODO: mutex it if needed by multi-threads */
using dbg_val_type = std::map<void *, std::pair<dbg_string_t, int>>::value_type;
inline std::map<void *,std::pair<dbg_string_t, int>, std::less<void *>, allocator_t<dbg_val_type>>
        dbg_names{allocator_t<dbg_val_type> {nullptr}};

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) {
    if (!has(dbg_names, addr))
        dbg_names.insert({addr, {dbg_format(fmt, std::forward<Args>(args)...), 1}});
    else
        dbg_names.find(addr)->second.second++;
    return addr;
}

inline dbg_string_t dbg_name(void *v) {
    if (!has(dbg_names, v))
        dbg_register_name(v, "%p", v);
    return dbg_format("%s[%" PRIu64 "]", dbg_names.find(v)->second.first.c_str(),
            dbg_names.find(v)->second.second);
}

#else /*CORO_ENABLE_DEBUG_NAMES*/

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) { return addr; }

inline dbg_string_t dbg_name(void *v) { return dbg_string_t{"", allocator_t<char>{nullptr}}; };

#endif /*CORO_ENABLE_DEBUG_NAMES*/

#if CORO_ENABLE_LOGGING

inline void dbg_raw(const dbg_string_t& msg, const char *file, const char *func, int line) {
    if (log_str) {
        log_str(dbg_format("[%" PRIu64 "] %s:%4d %s() :> %s\n", dbg_get_time(),
                file, line, func, msg.c_str()));
    }
}

template <typename... Args>
inline void dbg(const char *file, const char *func, int line, const char *fmt, Args&&... args) {
    dbg_raw(dbg_format(fmt, std::forward<Args>(args)...), file, func, line);
}

#else /*CORO_ENABLE_LOGGING*/

template <typename... Args> /* no logging -> do nothing */
inline void dbg(const char *, const char *, int, const char *fmt, Args&&...) {}

#endif /*CORO_ENABLE_LOGGING*/

/* The end
------------------------------------------------------------------------------------------------- */

}

#endif
