#ifndef CORO_H
#define CORO_H

/* TODO:
    - write the fd stuff
    - add debug modifs, things are already breaking and I don't know why
    - consider implementing co_yield
    - consider implementing exception handling
    - write the tutorial at the start of this file
    + speed optimizations [added allocator]
    - add the licence
    - check own comments
    - check the review again
*/

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
#include <utility>

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


// // no inline, required by [replacement.functions]/3
// void* operator new(std::size_t sz)
// {
//     std::printf("1) new(size_t), size = %zu\n", sz);
//     if (sz == 0)
//         ++sz; // avoid std::malloc(0) which may return nullptr on success
 
//     if (void *ptr = std::malloc(sz))
//         return ptr;
 
//     throw std::bad_alloc{}; // required by [new.delete.single]/3
// }
 
// // no inline, required by [replacement.functions]/3
// void* operator new[](std::size_t sz)
// {
//     std::printf("2) new[](size_t), size = %zu\n", sz);
//     if (sz == 0)
//         ++sz; // avoid std::malloc(0) which may return nullptr on success
 
//     if (void *ptr = std::malloc(sz))
//         return ptr;
 
//     throw std::bad_alloc{}; // required by [new.delete.single]/3
// }
 
// void operator delete(void* ptr) noexcept
// {
//     std::puts("3) delete(void*)");
//     std::free(ptr);
// }
 
// void operator delete(void* ptr, std::size_t size) noexcept
// {
//     std::printf("4) delete(void*, size_t), size = %zu\n", size);
//     std::free(ptr);
// }
 
// void operator delete[](void* ptr) noexcept
// {
//     std::puts("5) delete[](void* ptr)");
//     std::free(ptr);
// }
 
// void operator delete[](void* ptr, std::size_t size) noexcept
// {
//     std::printf("6) delete[](void*, size_t), size = %zu\n", size);
//     std::free(ptr);
// }

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
using pool_p = std::shared_ptr<pool_t>;
using modif_p = std::shared_ptr<modif_t>;
using modif_table_p = std::shared_ptr<modif_table_t>;

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

/* some forward declarations of internal structures */
struct yield_awaiter_t;
struct sem_awaiter_t;
struct pool_internal_t;
struct sem_internal_t;
struct allocator_memory_t;

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

protected:
    template <typename U>
    friend struct allocator_t; /* lonely class */

    pool_t *pool = nullptr; /* this allocator is allways called on a valid pool */
};
template <typename T> 
struct deallocator_t { /* This class is part of the allocator implementation and is here only
                          because a definition needs it as a full type */
    pool_t *pool;
    void operator ()(T *p) { p->~T(); allocator_t<T>{pool}.deallocate(p, 1); }
};

/* exception error if the lib is configured to throw exception on modif termination */
#if CO_EXCEPT_ON_KILL
/* TODO: the exception that will be thrown by the killing of a courutine */
#endif

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

    /* Better to not touch this function */
    pool_internal_t *get_internal();

protected:
    template <typename T>
    friend struct allocator_t;

    std::unique_ptr<allocator_memory_t> allocator_memory;

    friend inline std::shared_ptr<pool_t> create_pool();
    pool_t();

private:
    std::unique_ptr<pool_internal_t> internal;
};

/* TODO: if the semaphore dies, the things waiting on it will still wait, that doesn't seem ok,
maybe wake them up? give a warning? */
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

/* This is the state struct that each corutine has */
struct state_t {
    error_e err = ERROR_OK;             /* holds the error return in diverse cases */
    pool_t *pool;                       /* the pool of this coro */
    modif_table_p modif_table;          /* we allocate a table only if there are mods */

    state_t *caller_state = nullptr;    /* this holds the caller's state, and with it the return path */ 
    std::coroutine_handle<void> self;   /* the coro's self handle */

    std::shared_ptr<void> user_ptr;     /* this is a pointer that the user can use for whatever
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
                    std::coroutine_handle<void>,/* The corutine handle */
                    std::shared_ptr<void>       /* Where the shared_ptr is actually stored */
                >,
                allocator_t<std::
                    pair<
                        std::coroutine_handle<void>,
                        std::shared_ptr<void>
                    >
                >                               /* profiling shows the default is slow */
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
        std::function<error_e(state_t *, sem_t *, sem_waiter_handle_p)>,

         /* unwait_sem_cbk */
        std::function<error_e(state_t *, sem_t *, sem_waiter_handle_p)>
    > cbk;

    modif_e type = CO_MODIF_COUNT;
    modif_flags_e flags = CO_MODIF_INHERIT_ON_CALL;
};

/* Pool & Sched functions:
------------------------------------------------------------------------------------------------  */

inline std::shared_ptr<pool_t> create_pool();

/* gets the pool of the current corutine, necesary if you want to sched corutines from callbacks.
Don't forget you need multithreading enabled if you want to schedule from another thread. */
inline task<pool_t *> get_pool();

/* This does not stop the curent corutine, it only schedules the task, but does not yet run it.
Modifs duplicates are eliminated. */
template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const std::vector<modif_p>& v = {});

/* This stops the current coroutine from running and places it at the end of the ready queue. */
inline yield_awaiter_t yield();

/* Modifications
------------------------------------------------------------------------------------------------  */

template <typename Cbk>
inline modif_p create_modif(pool_t *pool, modif_e type, Cbk&& cbk, modif_flags_e flags);

template <typename Cbk>
inline modif_p create_modif(pool_p pool, modif_e type, Cbk&& cbk, modif_flags_e flags);

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
inline sem_p create_sem(pool_t *pool, int64_t val = 0);
inline sem_p create_sem(pool_p  pool, int64_t val = 0);
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

/* why would I replace the old string with the new one based on the allocator? because I need to
know that the library allocates only through the allocator, so debugging would interfere with
that. */
using dbg_string_t = std::basic_string<char, std::char_traits<char>, allocator_t<char>>;

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
inline dbg_string_t dbg_name(task<T> t);

template <typename P>
inline dbg_string_t dbg_name(std::coroutine_handle<P> h);

inline dbg_string_t dbg_name(void *v);


/* formats a string using the C snprintf, similar in functionality to snprintf+std::format, in the
version of g++ that I'm using std::format is not available  */
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

struct dbg_format_helper_t {
    dbg_string_t fmt;
    std::source_location loc;

    template <typename Arg>
    dbg_format_helper_t(Arg&& arg,
            std::source_location const& loc = std::source_location::current())
    : fmt{std::forward<Arg>(arg), allocator_t<char>{nullptr}}, loc{loc} {}
};

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key)) != std::forward<T>(data_struct).end();
}

using sem_wait_list_t = std::list<std::pair<handle<void>, std::shared_ptr<void>>,
        allocator_t<std::pair<std::coroutine_handle<void>,std::shared_ptr<void>>>>;
using sem_wait_list_it = sem_wait_list_t::iterator;

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
        int stack_head;

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

    /* There are some objectives for this buckets array:
        1. The buckets must all stay togheter (localized in memory)
        2. The buckets must all be constructed automatically from allocator_bucket_sizes
    */
    buckets_type<buckets_cnt> buckets;

    allocator_memory_t() {
        std::apply([](auto&&... args) { ((args.init()), ...); }, buckets);
    }

    void *alloc(size_t bytes) {
        void *ret = NULL;
        bool invalidated = false;
        std::apply([&](auto&&... args) {
            ((ret = (ret || invalidated) ? ret : args.alloc(bytes, invalidated)), ...);
        }, buckets);
        return ret;
    }

    void free(void *ptr) {
        std::apply([&](auto&&... args) { ((args.free(ptr)), ...); }, buckets);
    }
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

inline error_e do_wait_fd_modifs(state_t *state, int fd, int wait_cond) {
    return do_generic_modifs<CO_MODIF_WAIT_FD_CBK>(state, fd, wait_cond);
}

inline error_e do_unwait_fd_modifs(state_t *state, int fd) {
    return do_generic_modifs<CO_MODIF_UNWAIT_FD_CBK>(state, fd);
}

inline error_e do_wait_sem_modifs(state_t *state, sem_t *sem, sem_waiter_handle_p _it) {
    return do_generic_modifs<CO_MODIF_WAIT_SEM_CBK>(state, sem, _it);
}

inline error_e do_unwait_sem_modifs(state_t *state, sem_t *sem, sem_waiter_handle_p _it) {
    return do_generic_modifs<CO_MODIF_UNWAIT_SEM_CBK>(state, sem, _it);
}

/* considering you may want to create a modif at runtime this seems to be the best way */
template <typename Cbk>
inline modif_p create_modif(pool_t *pool, modif_e type, Cbk&& cbk, modif_flags_e flags) {
    modif_p ret = modif_p(alloc<modif_t>(pool, modif_t{
        .type = type,
        .flags = flags,
    }), dealloc_create<modif_t>(pool), allocator_t<int>{pool});

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

template <typename Cbk>
inline modif_p create_modif(pool_p pool, modif_e type, Cbk&& cbk, modif_flags_e flags) {
    return create_modif(pool.get(), type, std::forward<Cbk>(cbk), flags);
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
    pool_t       *get_pool();

    error_e get_err() { return ERROR_OK; };

    bool ever_called = false;
    handle_t h;
};

/* has to know about pool, will be implemented bellow the pool, but it is required here and to be
accesible to others that need to clean the corutine (for example in modifs, if the tasks need
killing) */
inline handle<void> final_awaiter_cleanup(state_t *ending_task_state,
        handle<void> ending_task_handle);

template <typename T>
struct task_state_t {
    struct final_awaiter_t {
        bool await_ready() noexcept  { return false; }
        void await_resume() noexcept { /* TODO: here some exceptions? */ }
        
        handle<void> await_suspend(handle<task_state_t<T>> ending_task) noexcept {
            return final_awaiter_cleanup(&ending_task.promise().state, ending_task);
        }

        pool_t *pool;
    };

    task<T> get_return_object() {
        typename task<T>::handle_t h = task<T>::handle_t::from_promise(*this);
        state.self = h;
        return task<T>{h};
    }

    std::suspend_always initial_suspend() noexcept { return {}; }
    final_awaiter_t     final_suspend() noexcept   { return final_awaiter_t{ .pool = state.pool }; }
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
    h.promise().state.pool = caller.promise().state.pool;

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
inline pool_t *task<T>::get_pool() {
    return h.promise().state.pool;
}

/* The Pool & Epoll engine
------------------------------------------------------------------------------------------------- */

/* If this is not really needed here we move it bellow */
struct pool_internal_t {
    pool_internal_t(pool_t *_pool) : pool(_pool), ready_tasks{allocator_t<handle<void>>(_pool)} {}

    template <typename T>
    void sched(task<T> task, modif_table_p parent_table) {
        /* first we give our new task the pool */
        task.h.promise().state.pool = pool;

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

private:
    pool_t *pool;
    std::deque<handle<void>, allocator_t<handle<void>>> ready_tasks;
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


inline handle<void> final_awaiter_cleanup(state_t *ending_task_state,
        handle<void> ending_task_handle)
{
    /* not sure if I should do something with the return value ... */
    state_t *caller_state = ending_task_state->caller_state;
    do_exit_modifs(ending_task_state);

    if (caller_state) {
        do_entry_modifs(caller_state);
        return caller_state->self;
    }
    auto pool = ending_task_state->pool;
    /* If the task that we are final_awaiting has no caller, then it is the moment to destroy it,
    no one needs it's return value(futures?). Else it will be destroyed by the caller. */
    /* TODO: maybe the future can just simply create a task and set it's coro as the continuation
    here? Such that the future continues in the caller_state branch? */
    ending_task_handle.destroy();
    return pool->get_internal()->next_task();
}

inline pool_t::pool_t() {
    allocator_memory = std::make_unique<allocator_memory_t>();
    internal = std::make_unique<pool_internal_t>(this);
}

template <typename T>
inline void pool_t::sched(task<T> task, const std::vector<modif_p>& v) {
    internal->sched(task, create_modif_table(this, v));
}

inline run_e pool_t::run() {
    return internal->run();
}

inline pool_internal_t *pool_t::get_internal() {
    return internal.get();
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
        pool->get_internal()->push_ready(h);
        state = &h.promise().state;
        do_leave_modifs(state);
        return pool->get_internal()->next_task();
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
        auto pool = h.promise().state.pool;
        pool->get_internal()->sched(t, table);
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
        pool = h.promise().state.pool;
        return false;
    }

    bool    await_ready()              { return false; };
    pool_t *await_resume()             { return pool; }

    pool_t *pool;
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
        auto pool = h.promise().state.pool;
        state = &h.promise().state;
        /* in case we can't schedule the fd we log the failure and return the same coro */
        if (do_wait_fd_modifs(state, fd, wait_cond) != ERROR_OK) {
            return h;
        }
        error_e err;
        if ((err = pool->get_internal()->wait_fd(h, fd, wait_cond)) != ERROR_OK) {
            /* TODO: log the fail */
            state->err = err;
            return h;
        }
        return pool->get_internal()->next_task();
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
        if (val == 0)
            return _awake_one();

        /* we awake when val is 0, do nothing on negative, only increment and on positive we
        don't have awaiters */
        return ERROR_OK;
    }

protected:
    friend sem_awaiter_t;
    friend sem_t;
    friend inline sem_p create_sem(pool_t *pool, int64_t val);

    sem_waiter_handle_p push_waiter(handle<void> to_suspend) {
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
        waiting_on_sem.push_front({to_suspend, p});
        *p = waiting_on_sem.begin();
        return p;
    }

    void erase_waiter(sem_wait_list_it it) {
        waiting_on_sem.erase(it);
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
        _sem_it = sem->get_internal()->push_waiter(to_suspend);
        if (do_wait_sem_modifs(state, sem, _sem_it) != ERROR_OK) {
            sem->get_internal()->erase_waiter(*_sem_it);
            return to_suspend;
        }

        triggered = true;
        return pool->get_internal()->next_task();
    }

    sem_t::unlocker_t await_resume() {
        if (triggered)
            do_unwait_sem_modifs(state, sem, _sem_it);
        triggered = false;
        return sem_t::unlocker_t(sem);
    }

    bool await_ready() {
        return sem->get_internal()->await_ready();
    }

    state_t *state = nullptr;
    sem_t *sem;
    sem_waiter_handle_p _sem_it;
    bool triggered = false;
};

inline sem_t::sem_t(pool_t *pool, int64_t val)
: internal(std::unique_ptr<sem_internal_t, deallocator_t<sem_internal_t>>(
        alloc<sem_internal_t>(pool, pool, val), dealloc_create<sem_internal_t>(pool))) {}

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

template <typename ...Args>
inline task_t wait_all(task<Args>... tasks) {
    /* TODO: this should return once all the tasks have been executed (use futures here?) */
    co_return ERROR_OK;
}

template <typename T>
inline sched_awaiter_t<T> sched(task<T> to_sched, const std::vector<modif_p>& v) {
    return sched_awaiter_t(to_sched, create_modif_table(to_sched.h.promise().state.pool, v));
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
inline dbg_string_t dbg_name(task<T> t) {
    return dbg_name(t.h.address());
}

template <typename P>
inline dbg_string_t dbg_name(handle<P> h) {
    return dbg_name(h.address());
}

inline modif_p creat_dbg_tracer() {
    /* TODO: create a 'modif packet' that can be used to trace all the corutine's actions */
    return nullptr;
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
inline std::map<void *, std::pair<dbg_string_t, int>, std::less<void *>,
        allocator_t<std::pair<const void *, std::pair<dbg_string_t, int>>>> dbg_names(
        allocator_t<std::pair<const void *, std::pair<dbg_string_t, int>>>> {nullptr});

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) {
    if (!has(dbg_names, addr))
        dbg_names[addr] = {dbg_format(fmt, std::forward<Args>(args)...), 1};
    else
        dbg_names[addr].second++;
    return addr;
}

inline dbg_string_t dbg_name(void *v) {
    if (!has(dbg_names, v))
        dbg_register_name(v, "%p", v);
    return dbg_format("%s[%" PRIu64 "]", dbg_names[v].first.c_str(), dbg_names[v].second);
}

#else /*CORO_ENABLE_DEBUG_NAMES*/

template <typename ...Args>
inline void *dbg_register_name(void *addr, const char *fmt, Args&&... args) { return addr; }

inline dbg_string_t dbg_name(void *v) { return dbg_string_t{"", allocator_t<char>{nullptr}}; };

#endif /*CORO_ENABLE_DEBUG_NAMES*/

#if CORO_ENABLE_LOGGING

inline void dbg_raw(const dbg_string_t& msg,
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
