#ifndef CO_UTILS_H
#define CO_UTILS_H

#include <iostream>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <set>
#include <stack>
#include <map>
#include <queue>
#include <list>
#include <coroutine>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

/* TODO: define to disable task_t::ever_called */

#define CO_MAX_TIMER_POOL_SIZE  64

/*  - provide CO_NEXT to have all coro traced
    - provide co_REG_INTERN to have internal coroutines named as well
    - add CO_REG when calling a coro to actually register it's name */

#define CO_REG(t)         t
#define CO_REG_INTERN(t)  t
#define CO_NEXT(h)        h

#ifndef CO_REG
# define CO_REG(t)        co::dbg_register(t, sformat("%20s:%5d:%s", __FILE__, __LINE__, #t))
#endif

#ifndef CO_REG_INTERN
# define CO_REG_INTERN(t) co::dbg_register(t, sformat("%26d:__%s", __LINE__, #t))
#endif

#ifndef CO_NEXT
# define CO_NEXT(h)       co::dbg_coro_str_forward(h)
#endif

/* ASSERT COroutine FuNction */
#define ASSERT_COFN(fn_call)\
if (intptr_t(fn_call) < 0) {\
    DBGE("FAILED: " #fn_call);\
    co_return -1;\
}

/* ASSERT End COroutine FuNction (used on the function that you want to use to return the final
result) */
#define ASSERT_ECOFN(fn_call)\
if (intptr_t(fn_call) < 0) {\
    DBGE("FAILED: " #fn_call);\
    co_await co::force_stop(-1);\
    co_return -1;\
}

namespace co
{

struct pool_t;
struct promise_t;
struct co_mod_t;
struct sem_t;

using handle_t = std::coroutine_handle<promise_t>;
using handle_vt = std::coroutine_handle<void>;
using co_mod_ptr_t = std::shared_ptr<co_mod_t>;
using sem_it_t = std::list<handle_t>::iterator;

using trace_ctx_t = void *;
using trace_fn_t = void (*)(int moment, void *coro_addr, trace_ctx_t ctx);

enum {
    CO_MOD_NONE = 0,
    CO_MOD_TIMEOUT,
    CO_MOD_TRACE,

    CO_MOD_TIMEO_STATE_RUNNING,
    CO_MOD_TIMEO_STATE_FD,
    CO_MOD_TIMEO_STATE_SEM,
    CO_MOD_TIMEO_STATE_TIMEO,
    CO_MOD_TIMEO_STATE_STOPPED,

    CO_MOD_TRACE_MOMENT_CALL,       /* When a function called is  */
    CO_MOD_TRACE_MOMENT_LEAVE,
    CO_MOD_TRACE_MOMENT_RETURN,
    CO_MOD_TRACE_MOMENT_REENTRY,
    CO_MOD_TRACE_MOMENT_FD_WAIT,
    CO_MOD_TRACE_MOMENT_FD_UNWAIT,
    CO_MOD_TRACE_MOMENT_SEM_WAIT,
    CO_MOD_TRACE_MOMENT_SEM_UNWAIT,

    CO_MOD_ERR_GENERIC = -1,
    CO_MOD_ERR_TIMEO = -2,
    CO_WAKEUP_ERR = -4,

    CO_WAKEUP_ALL = 0xffff'ffff,
};

struct sleep_handle_t {
    int *fd_ptr = NULL;

    int stop();
};

/* TODO: make internal vars private and mark functions as friend functions */

/* Struct to remember registered actions for child coroutines spawned in the future. Usefull for
registering cancelation functions, for registering traces, etc. New actions are registered by
inserting into the chain and are inherited by copying the chain at it's current root. */
static uint64_t como_num_g = 0;
struct co_mod_t {
    int type;
    co_mod_ptr_t next;

    uint64_t como_num;
    co_mod_t(int type);
    ~co_mod_t();

    co_mod_t(const co_mod_t &) = delete;
    co_mod_t(co_mod_t &&) = delete;
    co_mod_t &operator = (const co_mod_t &) = delete;
    co_mod_t &operator = (co_mod_t &&) = delete;

    union {
        struct { /* used by CO_MOD_TIMEOUT */
            char sem_it_data[sizeof(sem_it_t)];
            char sleep_handle_data[sizeof(sleep_handle_t)];

            uint64_t timeo_us;
            bool timer_started;
            int state;
            sem_it_t *sem_it;
            sleep_handle_t *sleep_handle;
            int wait_fd;
            sem_t *sem;         /* if the blocking is by a semaphore */
            void *leaf_coro;    /* the lowest coro that is currently waiting */
            void *root_coro;    /* the coro that initiated the sleep, on timeo it will be
                                        rescheduled */
        } timeo;
        struct {
            trace_ctx_t ctx;
            trace_fn_t  fn;
        } trace;
    }m;

    static co_mod_ptr_t attach(co_mod_ptr_t origin, co_mod_ptr_t other);

    /* TODO: figure out if those need to return void or int, some of them should return void */
    static int handle_pmods_call(handle_t handle);
    static int handle_pmods_ret(handle_t handle);
    static int handle_pmods_leave(handle_t handle);
    static int handle_pmods_reentry(handle_t handle);

    static int handle_pmods_fd_wait(handle_t handle, int fd);
    static int handle_pmods_fd_unwait(handle_t handle);
    static int handle_pmods_sem_wait(handle_t handle, sem_t *sem, sem_it_t it);
    static int handle_pmods_sem_unwait(handle_t handle);
};

/* main awaiter and main promise handle. */
struct task_t {
    using promise_type = promise_t;

    task_t(handle_t handle);
    ~task_t();

    task_t(task_t& oth);
    task_t(task_t&& oth);
    task_t &operator = (task_t& oth);
    task_t &operator = (task_t&& oth);

    bool      await_ready() noexcept;
    handle_vt await_suspend(handle_t caller) noexcept;
    int       await_resume() noexcept;

    handle_t handle;
    bool ever_called = false;
};

struct initial_awaiter_t {
    bool await_ready() noexcept;
    void await_suspend(handle_t self) noexcept;
    void await_resume() noexcept;
};

struct final_awaiter_t {
    final_awaiter_t(pool_t *pool);

    bool      await_ready() noexcept;
    handle_vt await_suspend(handle_t oth) noexcept;
    void      await_resume() noexcept;

    static handle_vt final_awaiter_cleanup(pool_t *pool, handle_t ending_task);

    pool_t *pool = nullptr;
};

/* this holds the task metadata */
struct promise_t {
    task_t get_return_object();

    initial_awaiter_t initial_suspend() noexcept;
    final_awaiter_t   final_suspend() noexcept;
    void              return_value(int ret_val);
    void              unhandled_exception();

    int ret_val;
    int call_res;
    pool_t *pool = nullptr;

    handle_t caller;
    co_mod_ptr_t pmods;
};

struct fd_sched_t {
    struct fd_data_t {
        struct waiter_t {
            uint32_t mask = 0;
            void *ptr = nullptr;
        };
        uint32_t mask = 0;
        std::vector<waiter_t> waiters;
    };

    fd_sched_t();
    handle_t get_next();

    int  insert_wait(handle_t to_wait, int fd, uint32_t wait_cond);
    int  remove_wait(int fd, uint32_t wait_cond);
    int  wakeup_wait(int fd, uint32_t mask = CO_WAKEUP_ALL);

    bool pending();
    int  check_events_noblock();
    int  wait_events();
    int  handle_events(int num_evs);

    std::map<int, fd_data_t> waiting_tasks;
    iter_queue<handle_t> ready_tasks;
    std::vector<struct epoll_event> ret_evs;
    int epoll_fd;

};

struct pool_t {
    using task_it_t = std::set<handle_t>::iterator;

    void sched(handle_t handle, co_mod_ptr_t pmods = nullptr);
    void sched(task_t task, co_mod_ptr_t pmods = nullptr);

    int  run();

    handle_vt next_task();
    handle_vt resched_wait_fd(handle_t to_wait, int fd, int wait_cond);

    ~pool_t();

    iter_queue<handle_t> waiting_tasks;
    std::stack<int> timer_fd_pool;
    fd_sched_t fd_sched;
    int ret_val = 0;
};

struct fd_awaiter_t {
    bool      await_ready();
    handle_vt await_suspend(handle_t caller_handle);
    int       await_resume();

    ~fd_awaiter_t();

    int wait_cond;
    int fd;

    handle_t caller_handle;
    pool_t *pool;
};

/* This awaiter does only one thing and that is to schedule a coroutine */
struct sched_awaiter_t {
    handle_t to_sched;

    bool await_ready();
    bool await_suspend(handle_t handle);
    void await_resume();
};

struct yield_awaiter_t {
    bool      await_ready();
    handle_vt await_suspend(handle_t handle);
    void      await_resume();
};

struct sleep_awaiter_t {
    bool      await_ready();
    handle_vt await_suspend(handle_t caller_handle);
    int       await_resume();

    int **fd_ref = NULL;
    uint64_t sleep_us;
    int timer_fd;

    handle_t caller_handle;
    pool_t *pool;
};

/* The semaphore is an object with an internal counter and is usefull in signaling and mutual
exclusion:
    1. co_await - suspends the current coroutine if the counter is zero, else decrements the counter
                  returns an unlocker_t object that has a member function .unlock() which calls
                  rel(). unlocker_t also has a function lock() that does nothing, this is to allow
                  unlocker_t to be used inside std::lock_guard.
    2. rel      - increments the counter.
    3. rel_all  - increments the counter with the amount of waiting coroutines on this semaphore

    At a suspension point coroutines that suspended on a wait are candidates for rescheduling. Upon
    rescheduling the internal counter will be decremented.

    You must manually use (co_await co::yield()) to suspend the current coroutine if you want the
    notified coroutine to have a chance to be rescheduled. You can also call a suspending coro to
    reschedule.
*/

struct sem_t {
    sem_t(int64_t counter = 0);

    /* TODO: make it move safe and delete-copy because in the semaphore it is very important that
    internal pointers don't break and for now on copy you loose the pointer */
    // sem_t(const sem_t &) = delete;
    // sem_t(sem_t &&) = delete;
    // sem_t &operator = (const sem_t &) = delete;
    // sem_t &operator = (sem_t &&) = delete;

    struct unlocker_t {
        unlocker_t(sem_t *sem);

        void lock();
        void unlock();

        sem_t *sem;
    };

    struct sem_awaiter_t {
        sem_awaiter_t(sem_t *sem);

        bool       await_ready();
        handle_vt  await_suspend(handle_t to_suspend);
        unlocker_t await_resume();

        sem_t *sem;
    };

    sem_awaiter_t operator co_await () noexcept;

    void rel();
    void rel_all();
    
    void _dec();                      /* this is not private but don't use it */
    void _erase_waiting(sem_it_t it); /* this is not private but don't use it */

private:
    void _awake_one();

    int64_t counter;
    std::list<handle_t> waiting_on_sem;
};

/* helper functions to convert co:: variables to string */
inline const char *co_str(void *addr);
inline const char *co_str(handle_vt handle);
inline const char *co_str_intern(void *addr, std::string name = "");            // just don't touch
inline const char *co_str_intern(handle_vt handle, std::string co_name = "");   // just don't touch
inline const char *enum_str(int e);
inline std::string epoll_ev2str(uint32_t code);
inline void dbg_trace_fn(int moment, void *addr, void *);
template <typename TASK_T>
inline TASK_T dbg_register(TASK_T &&task, std::string task_name);
template <typename CORO_T>
inline CORO_T dbg_coro_str_forward(CORO_T &&coro);
inline task_t dbg_print_internal_info();

/* This function schedules a coroutine on the current pool, a pointer to a modifier list can be
supplied to attach it to the newly scheduled coroutine.  */
inline sched_awaiter_t sched(task_t to_sched, co_mod_ptr_t pmods = nullptr);

/* This stops the current coroutine from running and places it in the ready queue. */
inline yield_awaiter_t yield();

/* tasks call other tasks, creating call-chains. A modifier can be attached to a task to be
propagated to it and called tasks. The modifiers do not propagate to tasks that are scheduled by
using co::sched or pool->sched, but you can modify the task to contain a modifier. */
inline task_t mod_task(task_t task, co_mod_ptr_t pmods);

/* add a timer for an entire call-chain, on suspension points the whole chain will be destroied if
the timer reached zero and the root task will return CO_MOD_ERR_TIMEO */
inline task_t timed(task_t task, uint64_t timeo_us);

/* add a callback to be called when a call-chain reaches one of the stages CO_MOD_TRACE_MOMENT_* */
inline task_t trace(task_t task, trace_fn_t fn = dbg_trace_fn, trace_ctx_t ctx = NULL);

/* calls an awaitable inside a task_t, this is done to be able to use pmods on awaitables */
template <typename Awaiter>
inline task_t await(Awaiter& awaiter);

/* causes the running pool::run to return */
inline task_t force_stop(int ret);

/* Functions for rw from file descriptors if needed you can use them as an example */
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

/* Normal sleeps */
inline task_t sleep_us(uint64_t timeo_us);
inline task_t sleep_ms(uint64_t timeo_ms);
inline task_t sleep_s(uint64_t timeo_s);

/* Those fns are meant to create interuptible sleeps */
inline task_t var_sleep_us(uint64_t timeo_us, sleep_handle_t *sleep_handle);
inline task_t var_sleep_ms(uint64_t timeo_ms, sleep_handle_t *sleep_handle);
inline task_t var_sleep_s(uint64_t timeo_s, sleep_handle_t *sleep_handle);
inline int stop_sleep(sleep_handle_t *sleep_handle);

/* OBS: Broken, breaks gcc if I call it */
inline task_t when_all(std::vector<task_t> tasks);

/* IMPLEMENTATION:
================================================================================================= */
/* ============================================================================================== */
/* ============================================================================================== */

inline int sleep_handle_t::stop() {
    ASSERT_FN(stop_sleep(this));
    return 0;
}

inline co_mod_t::co_mod_t(int type) : type(type), como_num(como_num_g++) {
    memset(&m, 0, sizeof(m));
    if (type == CO_MOD_TIMEOUT) {
        m.timeo.sem_it = new (m.timeo.sem_it_data) sem_it_t();
        m.timeo.sleep_handle = new (m.timeo.sleep_handle_data) sleep_handle_t();
    }
}

inline co_mod_t::~co_mod_t() {
    if (type == CO_MOD_TIMEOUT) {
        m.timeo.sem_it->~sem_it_t();
        m.timeo.sleep_handle->~sleep_handle_t();
    }
}

inline co_mod_ptr_t co_mod_t::attach(co_mod_ptr_t origin, co_mod_ptr_t other) {
    if (!origin)
        return other;
    auto curr = origin;
    while (curr) {
        if (!curr->next) {
            curr->next = other;
            break;
        }
        curr = curr->next;
    }
    return origin;
}

/* The handle is the handle to the pmod  */
inline int co_mod_t::handle_pmods_call(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                if (!curr->m.timeo.timer_started) {
                    auto timeo_task = [](co_mod_ptr_t pmod) -> task_t
                    {
                        ASSERT_COFN(co_await CO_REG_INTERN(var_sleep_us(pmod->m.timeo.timeo_us,
                                pmod->m.timeo.sleep_handle)));
                        if (pmod->m.timeo.state == CO_MOD_TIMEO_STATE_STOPPED) {
                            /* If we are in this case it means that the action concluded before the
                            timeout so there is nothing more to do */
                            co_return 0;
                        }
                        int state = pmod->m.timeo.state;
                        pmod->m.timeo.state = CO_MOD_TIMEO_STATE_TIMEO;

                        auto root_coro = handle_t::from_address(pmod->m.timeo.root_coro);
                        auto pool = root_coro.promise().pool;
                        /*  At this point we know that the timer elapsed while the coroutine was
                        waiting for some event to complete. We must now stop that event and we must
                        unwrap the call such that the coroutine that made the 'timed' call will be
                        rescheduled and the return value in the respective task will be nagative. */

                        if (state == CO_MOD_TIMEO_STATE_RUNNING) {
                            /* nothing to do here? check in yield maybe? */
                        }
                        if (state == CO_MOD_TIMEO_STATE_FD) {
                            int ret = pool->fd_sched.remove_wait(pmod->m.timeo.wait_fd, EPOLLIN);
                            if (ret < 0) {
                                DBG("Failed to remove fd from wating list, will ignore");
                            }
                        }
                        if (state == CO_MOD_TIMEO_STATE_SEM) {
                            pmod->m.timeo.sem->_erase_waiting(*pmod->m.timeo.sem_it);
                        }

                        /* unwind all the coroutines from leaf to root */
                        auto leaf = handle_t::from_address(pmod->m.timeo.leaf_coro);
                        while (leaf && leaf.address() != root_coro.address()) {
                            leaf.promise().call_res = CO_MOD_ERR_TIMEO;
                            auto caller = leaf.promise().caller;
                            leaf.destroy();
                            leaf = caller;
                        }
                        if (!leaf) {
                            DBG("This makes no sense");
                            co_return CO_MOD_ERR_GENERIC; 
                        }

                        /* schedule the caller of co::timed to be awakened with timeo */
                        root_coro.promise().call_res = CO_MOD_ERR_TIMEO;
                        root_coro.promise().ret_val = CO_MOD_ERR_TIMEO;
                        pool->waiting_tasks.push(handle_t::from_address(
                                final_awaiter_t::final_awaiter_cleanup(pool, root_coro).address()));
                        co_return 0;
                    }(curr);
                    handle.promise().pool->sched(CO_REG_INTERN(timeo_task));
                    curr->m.timeo.timer_started = true;
                    curr->m.timeo.root_coro = handle.address();
                    curr->m.timeo.state = CO_MOD_TIMEO_STATE_RUNNING;
                }
                /* The root_coro is set only once, but the leaf_coro is set every time a new coro
                is spawned. That's because the coro chain must be unwinded on an abort from the
                last called coroutine to the original coroutine. */
                curr->m.timeo.leaf_coro = handle.address();
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_CALL, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_ret(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.leaf_coro = handle.address();
                if (curr->m.timeo.leaf_coro == curr->m.timeo.root_coro) {
                    ASSERT_FN(stop_sleep(curr->m.timeo.sleep_handle));
                    curr->m.timeo.state = CO_MOD_TIMEO_STATE_STOPPED;
                }
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_RETURN, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_leave(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_LEAVE, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_reentry(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.leaf_coro = handle.address();
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_REENTRY, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_fd_wait(handle_t handle, int fd) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.state = CO_MOD_TIMEO_STATE_FD;
                curr->m.timeo.wait_fd = fd;
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_FD_WAIT, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_fd_unwait(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.state = CO_MOD_TIMEO_STATE_RUNNING;
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_FD_UNWAIT, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_sem_wait(handle_t handle, co::sem_t *sem, sem_it_t it) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.state = CO_MOD_TIMEO_STATE_SEM;
                curr->m.timeo.sem = sem;
                *curr->m.timeo.sem_it = it;
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_SEM_WAIT, handle.address(), curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}

inline int co_mod_t::handle_pmods_sem_unwait(handle_t handle) {
    co_mod_ptr_t curr = handle.promise().pmods;
    while (curr) {
        switch (curr->type) {
            case CO_MOD_TIMEOUT: {
                curr->m.timeo.state = CO_MOD_TIMEO_STATE_RUNNING;
            } break;
            case CO_MOD_TRACE: {
                curr->m.trace.fn(CO_MOD_TRACE_MOMENT_SEM_UNWAIT, handle.address(),
                        curr->m.trace.ctx);
            } break;
        }
        curr = curr->next;
    }
    return 0;
}


/* task_t: -------------------------------------------------------------------------------------- */

inline task_t::task_t(handle_t handle) : handle(handle) {}

inline task_t::~task_t() {
    if (!ever_called) {
        DBG("This coroutine was never called, that's not ok");
    }
}

inline task_t::task_t(task_t& oth) {
    handle = oth.handle;
    oth.ever_called = true;
}

inline task_t::task_t(task_t&& oth) {
    handle = std::move(oth.handle);
    oth.ever_called = true;
}

inline task_t &task_t::operator = (task_t& oth) {
    handle = oth.handle;
    oth.ever_called = true;
    return *this;
}

inline task_t &task_t::operator = (task_t&& oth) {
    handle = std::move(oth.handle);
    oth.ever_called = true;
    return *this;
}


inline bool task_t::await_ready() noexcept {
    return false;
}

inline handle_vt task_t::await_suspend(handle_t caller) noexcept {
    ever_called = true;
    handle.promise().caller = caller;
    handle.promise().pmods = co_mod_t::attach(handle.promise().pmods, caller.promise().pmods);
    handle.promise().pool = caller.promise().pool;

    co_mod_t::handle_pmods_leave(caller);
    int ret = co_mod_t::handle_pmods_call(handle);
    if (ret < 0) {
        DBG("Failed to handle moded corutine");
        return CO_NEXT(std::noop_coroutine());
    }

    return CO_NEXT(handle);
}

inline int task_t::await_resume() noexcept {
    ever_called = true;
    int ret = handle.promise().ret_val;
    handle.destroy();
    return ret;
}

/* initial_awaiter_t: --------------------------------------------------------------------------- */

inline bool initial_awaiter_t::await_ready() noexcept {
    return false;
}

inline void initial_awaiter_t::await_suspend(handle_t self) noexcept {
}

inline void initial_awaiter_t::await_resume() noexcept {
}

/* final_awaiter_t: ----------------------------------------------------------------------------- */

inline final_awaiter_t::final_awaiter_t(pool_t *pool) : pool(pool) {
}

inline bool final_awaiter_t::await_ready() noexcept {
    return false;
}

inline handle_vt final_awaiter_t::await_suspend(handle_t ending_task) noexcept {
    return CO_NEXT(final_awaiter_cleanup(pool, ending_task));
}

inline void final_awaiter_t::await_resume() noexcept {
    DBG("Exceptions are not my coup of tea, if it goes it goes");
    std::terminate();
}

inline handle_vt final_awaiter_t::final_awaiter_cleanup(pool_t *pool, handle_t ending_task) {
    auto caller = ending_task.promise().caller;
    if (co_mod_t::handle_pmods_ret(ending_task) < 0) {
        DBG("Pmods ret failed for some reason");
        return std::noop_coroutine();
    }
    if (caller) {
        co_mod_t::handle_pmods_reentry(caller);
        return caller;
    }
    else {
        auto ret = pool->next_task();
        ending_task.destroy();
        return ret;
    }
}

/* promise_t: ----------------------------------------------------------------------------------- */

inline task_t promise_t::get_return_object() {
    return task_t{handle_t::from_promise(*this)};
}

inline initial_awaiter_t promise_t::initial_suspend() noexcept {
    return {};
}

inline final_awaiter_t promise_t::final_suspend() noexcept {
    return final_awaiter_t(pool);
}

inline void promise_t::return_value(int ret_val) {
    this->ret_val = ret_val;
}

inline void promise_t::unhandled_exception() {
    DBG("Exceptions are not my coup of tea, if it goes it goes");
    std::terminate();
}

/* fd_sched_t: ---------------------------------------------------------------------------------- */

inline fd_sched_t::fd_sched_t() {
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd < 0) {
        DBG("Failed to create epoll");
    }
}

inline int fd_sched_t::check_events_noblock() {
    int num_evs;
    ASSERT_FN(num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), 0));
    int ret = handle_events(num_evs);
    return ret;
}

inline bool fd_sched_t::pending() {
    return ready_tasks.size();
}

inline handle_t fd_sched_t::get_next() {
    auto ret = ready_tasks.front();
    ready_tasks.pop();
    return ret;
}

inline int fd_sched_t::wait_events() {
    int num_evs;
    do {
        num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), -1);
    } while (num_evs < 0 && errno == EINTR);
    ASSERT_FN(num_evs);
    return handle_events(num_evs);
}

inline int fd_sched_t::insert_wait(handle_t to_wait, int fd, uint32_t wait_cond) {
    if (!wait_cond) {
        DBG("Wait cond can't be zero");
        return -1;
    }
    if (HAS(waiting_tasks, fd)) {
        auto &fd_data = waiting_tasks[fd];
        if (fd_data.mask & wait_cond) {
            DBG("Can't have two coroutines waiting on the same fd and same events %x vs %x",
                    fd_data.mask, wait_cond);
            return -1;
        }
        struct epoll_event ev = {};
        ev.events = wait_cond | fd_data.mask;
        ev.data.fd = fd;
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev));
        fd_data.mask = ev.events;
        fd_data.waiters.push_back(fd_data_t::waiter_t{ .mask = wait_cond, .ptr = to_wait.address() });
        to_wait.promise().call_res = CO_MOD_ERR_GENERIC;
        return 0;
    }
    else {
        auto &fd_data = waiting_tasks[fd];
        struct epoll_event ev = {};
        ev.events = wait_cond;
        ev.data.fd = fd;
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev));
        fd_data.mask = ev.events;
        fd_data.waiters.push_back(fd_data_t::waiter_t{ .mask = wait_cond, .ptr = to_wait.address() });
        to_wait.promise().call_res = CO_MOD_ERR_GENERIC;
        ret_evs.push_back(epoll_event{});
        return 0;
    }
    return 0;
}

inline int fd_sched_t::remove_wait(int fd, uint32_t wait_cond) {
    if (!HAS(waiting_tasks, fd)) {
        DBG("Couldn't remove fd: %d", fd);
        return -1;
    }
    auto &fd_data = waiting_tasks[fd];
    if ((fd_data.mask & ~wait_cond) != 0) {
        struct epoll_event ev = {};
        ev.events = fd_data.mask & ~wait_cond;
        ev.data.fd = fd;
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev));
        fd_data.mask = ev.events;
       
        fd_data.waiters.erase(std::remove_if(
            fd_data.waiters.begin(),
            fd_data.waiters.end(),
            [wait_cond](const fd_data_t::waiter_t& m) { 
                return (wait_cond & m.mask) == m.mask;
            }),
            fd_data.waiters.end()
        );
    }
    else {
        waiting_tasks.erase(fd);
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL));
        ret_evs.pop_back();
    }

    return 0;
}

inline int fd_sched_t::wakeup_wait(int fd, uint32_t mask) {
    if (!HAS(waiting_tasks, fd)) {
        return 0;
    }
    auto &fd_data = waiting_tasks[fd];
    uint32_t final_mask = 0;
    for (auto &w : fd_data.waiters) {
        if ((w.mask & mask) == w.mask) {
            final_mask |= w.mask;
            auto handle = handle_t::from_address(w.ptr);
            if (handle) {
                handle.promise().call_res = CO_WAKEUP_ERR;
                ready_tasks.push(handle);
            }   
        }
    }
    if (final_mask) {
        ASSERT_FN(remove_wait(fd, final_mask));
    }
    return 0;
}

inline int fd_sched_t::handle_events(int num_evs) {
    for (int i = 0; i < num_evs; i++) {
        if (!HAS(waiting_tasks, ret_evs[i].data.fd)) {
            DBG("Something is vey rowng!");
            return -1;
        }
        auto &fd_data = waiting_tasks[ret_evs[i].data.fd];
        for (auto &w : fd_data.waiters) {
            if (w.mask & ret_evs[i].events) {
                auto handle = handle_t::from_address(w.ptr);
                if (handle) {
                    handle.promise().call_res = 0;
                    ready_tasks.push(handle);
                }
            }
        }
    }
    return 0;
}

/* pool_t: -------------------------------------------------------------------------------------- */

inline void pool_t::sched(handle_t handle, co_mod_ptr_t pmods) {
    // DBG("scheduling: %s", co_str(handle));
    handle.promise().pool = this;
    if (pmods)
        handle.promise().pmods = pmods; /* sched is async, so non blocking in regards to the caller */
    waiting_tasks.push(handle);
}

inline void pool_t::sched(task_t task, co_mod_ptr_t pmods) {
    task.ever_called = true;
    sched(task.handle, pmods);
}

inline int pool_t::run() {
    co_str_intern(std::noop_coroutine().address(), "std::noop_coroutine");

    if (!waiting_tasks.size())
        return 0;
    handle_t first_task = waiting_tasks.front();
    waiting_tasks.pop();
    first_task.resume();
    return ret_val;
}

inline handle_vt pool_t::next_task() {
    if (waiting_tasks.size()) {
        /* We run an already scheduled task */
        handle_t to_resume = waiting_tasks.front();
        waiting_tasks.pop();
        return to_resume;
    }
    else {
        handle_t ret;

        if (fd_sched.pending()) {
            ret = fd_sched.get_next();
        }
        else {
            if (fd_sched.ret_evs.size() == 0) {
                DBG("Scheduler doesn't have any more coroutines to handle, will exit");
                return std::noop_coroutine();
            }
            int err = fd_sched.check_events_noblock();
            if (err < 0) {
                DBG("The epoll loop failed, will stop the scheduler");
                return std::noop_coroutine();
            }
            if (fd_sched.pending()) {
                ret = fd_sched.get_next();
            }
            else {
                err = fd_sched.wait_events();
                if (err < 0) {
                    DBG("The epoll loop failed to wait, stopping the scheduler");
                    return std::noop_coroutine();
                }
                if (!fd_sched.pending()) {
                    DBG("We have no more pending waits, this is an invalid state");
                    return std::noop_coroutine();
                }
                ret = fd_sched.get_next();
            }
        }
        return ret;
    }
}

inline handle_vt pool_t::resched_wait_fd(handle_t to_wait, int fd, int wait_cond) {
    if (fd_sched.insert_wait(to_wait, fd, wait_cond) < 0) {
        DBG("Can't insert a file descriptor for waiting");
        return std::noop_coroutine();
    }
    return next_task();
}

inline pool_t::~pool_t() {
    while (timer_fd_pool.size()) {
        close(timer_fd_pool.top());
        timer_fd_pool.pop();
    }
}

/* fd_awaiter_t --------------------------------------------------------------------------------- */

inline bool fd_awaiter_t::await_ready() {
    return false;
}

inline handle_vt fd_awaiter_t::await_suspend(handle_t caller_handle) {
    /* We get the pool object, we mark in the pool that we wait for events on this fd and we
    let the next coroutine to continue on this thread */
    pool = caller_handle.promise().pool;
    this->caller_handle = caller_handle;
    co_mod_t::handle_pmods_fd_wait(caller_handle, fd);
    return CO_NEXT(pool->resched_wait_fd(caller_handle, fd, wait_cond));
}

inline int fd_awaiter_t::await_resume() {
    /* this coroutine can only suspend on call, so we know that the scheduler was asked to
    take controll of this coroutine and we know that 'sched' and 'handle' are set, as such
    we can request the return value for the fd we just queried */

    co_mod_t::handle_pmods_fd_unwait(caller_handle);
    return caller_handle.promise().call_res;
}

inline fd_awaiter_t::~fd_awaiter_t() {
    int res = caller_handle.promise().call_res;
    if (res != CO_MOD_ERR_TIMEO && res != CO_WAKEUP_ERR)
        pool->fd_sched.remove_wait(fd, wait_cond);
}

/* sched_awaiter_t ------------------------------------------------------------------------------ */

inline bool sched_awaiter_t::await_ready() {
    return false;
}

inline bool sched_awaiter_t::await_suspend(handle_t handle) {
    handle.promise().pool->sched(to_sched);
    return false;
}

inline void sched_awaiter_t::await_resume() {}

/* yield_awaiter_t ------------------------------------------------------------------------------ */

inline bool yield_awaiter_t::await_ready() {
    return false;
}

inline handle_vt yield_awaiter_t::await_suspend(handle_t handle) {
    handle.promise().pool->waiting_tasks.push(handle);
    return CO_NEXT(handle.promise().pool->next_task());
}

inline void yield_awaiter_t::await_resume() {}

/* sleep_awaiter_t ------------------------------------------------------------------------------ */

inline bool sleep_awaiter_t::await_ready() {
    return false;
}

inline handle_vt sleep_awaiter_t::await_suspend(handle_t caller_handle) {
    pool = caller_handle.promise().pool;

    FnScope err_scope;
    if (pool->timer_fd_pool.size() > 0) {
        timer_fd = pool->timer_fd_pool.top();
        pool->timer_fd_pool.pop();
        err_scope([&]{ close(timer_fd); });
    }
    else {
        timer_fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
        if (timer_fd < 0) {
            DBGE("Failed to allocate new timer");
            return CO_NEXT(std::noop_coroutine());
        }
    }
    itimerspec its = {};
    its.it_value.tv_nsec = (sleep_us % 1000'000) * 1000ULL;
    its.it_value.tv_sec = sleep_us / 1000'000ULL;
    if (fd_ref) {
        *fd_ref = &timer_fd;
    }
    if (timerfd_settime(timer_fd, 0, &its, NULL) < 0) {
        DBGE("Failed to set expiration date");
        return CO_NEXT(std::noop_coroutine());
    }

    err_scope.disable();
    this->caller_handle = caller_handle;
    co_mod_t::handle_pmods_fd_wait(caller_handle, timer_fd);
    return CO_NEXT(pool->resched_wait_fd(caller_handle, timer_fd, EPOLLIN));
}

inline int sleep_awaiter_t::await_resume() {
    /* this coroutine can only suspend on call, so we know that the scheduler was asked to
    take controll of this coroutine and we know that 'sched' and 'handle' are set, as such
    we can request the return value for the fd we just queried */

    pool->fd_sched.remove_wait(timer_fd, EPOLLIN);
    if (pool->timer_fd_pool.size() >= CO_MAX_TIMER_POOL_SIZE) {
        close(timer_fd);
    }
    else {
        pool->timer_fd_pool.push(timer_fd);
    }

    co_mod_t::handle_pmods_fd_unwait(caller_handle);
    return caller_handle.promise().call_res;
}

/* sem_t ---------------------------------------------------------------------------------------- */

inline sem_t::unlocker_t::unlocker_t(sem_t *sem) : sem(sem) {}

inline void sem_t::unlocker_t::lock() {}

inline void sem_t::unlocker_t::unlock() {
    sem->rel();
}

inline sem_t::sem_t(int64_t counter) : counter(counter) {

}

inline sem_t::sem_awaiter_t::sem_awaiter_t(sem_t *sem) : sem(sem) {

}

inline bool sem_t::sem_awaiter_t::await_ready() {
    if (sem->counter > 0) {
        sem->counter--;
        return true;
    }
    return false;
}

inline handle_vt sem_t::sem_awaiter_t::await_suspend(handle_t to_suspend) {
    /* if we are here it must mean that the counter was zero and as such we must yield */
    sem->waiting_on_sem.push_front(to_suspend);
    sem_it_t it = sem->waiting_on_sem.begin();
    co_mod_t::handle_pmods_sem_wait(to_suspend, sem, it);

    return CO_NEXT(to_suspend.promise().pool->next_task());
}

inline sem_t::unlocker_t sem_t::sem_awaiter_t::await_resume() {
    return sem_t::unlocker_t(sem);
}

inline sem_t::sem_awaiter_t sem_t::operator co_await () noexcept {
    return sem_t::sem_awaiter_t(this);
}


inline void sem_t::rel() {
    if (counter == 0 && waiting_on_sem.size())
        _awake_one();
    else {
        counter++;
        if (counter == 0)
            _awake_one();
    }
}

inline void sem_t::_dec() {
    counter--;
}

inline void sem_t::rel_all() {
    if (counter > 0)
        return ;
    while (waiting_on_sem.size())
        _awake_one();
}

inline void sem_t::_erase_waiting(sem_it_t it) {
    waiting_on_sem.erase(it);
}

inline void sem_t::_awake_one() {
    auto to_awake = waiting_on_sem.back();
    waiting_on_sem.pop_back();
    co_mod_t::handle_pmods_sem_unwait(to_awake);
    to_awake.promise().pool->waiting_tasks.push(to_awake);
}

/* usefull functions ---------------------------------------------------------------------------- */

inline const char *enum_str(int e) {
    switch (e) {
        case CO_MOD_NONE:                    return "CO_MOD_NONE";
        case CO_MOD_TIMEOUT:                 return "CO_MOD_TIMEOUT";
        case CO_MOD_TRACE:                   return "CO_MOD_TRACE";

        case CO_MOD_TIMEO_STATE_RUNNING:     return "CO_MOD_TIMEO_STATE_RUNNING";
        case CO_MOD_TIMEO_STATE_FD:          return "CO_MOD_TIMEO_STATE_FD";
        case CO_MOD_TIMEO_STATE_SEM:         return "CO_MOD_TIMEO_STATE_SEM";
        case CO_MOD_TIMEO_STATE_TIMEO:       return "CO_MOD_TIMEO_STATE_TIMEO";
        case CO_MOD_TIMEO_STATE_STOPPED:     return "CO_MOD_TIMEO_STATE_STOPPED";

        case CO_MOD_TRACE_MOMENT_CALL:       return "CO_MOD_TRACE_MOMENT_CALL";
        case CO_MOD_TRACE_MOMENT_LEAVE:      return "CO_MOD_TRACE_MOMENT_LEAVE";
        case CO_MOD_TRACE_MOMENT_RETURN:     return "CO_MOD_TRACE_MOMENT_RETURN";
        case CO_MOD_TRACE_MOMENT_REENTRY:    return "CO_MOD_TRACE_MOMENT_REENTRY";
        case CO_MOD_TRACE_MOMENT_FD_WAIT:    return "CO_MOD_TRACE_MOMENT_FD_WAIT";
        case CO_MOD_TRACE_MOMENT_FD_UNWAIT:  return "CO_MOD_TRACE_MOMENT_FD_UNWAIT";
        case CO_MOD_TRACE_MOMENT_SEM_WAIT:   return "CO_MOD_TRACE_MOMENT_SEM_WAIT";
        case CO_MOD_TRACE_MOMENT_SEM_UNWAIT: return "CO_MOD_TRACE_MOMENT_SEM_UNWAIT";

        case CO_MOD_ERR_GENERIC:             return "CO_MOD_ERR_GENERIC";
        case CO_MOD_ERR_TIMEO:               return "CO_MOD_ERR_TIMEO";
        default: return "CO_UNKNOWN";
    }
}

/* TODO: make a different co_str for usual usage and for internal usage */
inline const char *co_str(void *addr) {
    return co_str(handle_vt::from_address(addr));
}

inline const char *co_str(handle_vt handle) {
    static std::map<void *, std::string> id_str;
    static uint64_t id = 0;

    if (!HAS(id_str, handle.address()))
        id_str[handle.address()] = sformat("co[%3d]", id++);
    return id_str[handle.address()].c_str();
}

inline const char *co_str_intern(void *addr, std::string name) {
    return co_str_intern(handle_vt::from_address(addr), name);
}

inline const char *co_str_intern(handle_vt handle, std::string co_name) {
    static std::map<void *, std::string> name_str;
    if (!(HAS(name_str, handle.address()) && co_name == ""))
        name_str[handle.address()] = sformat("%s[%s]", co_str(handle), co_name.c_str());
    return name_str[handle.address()].c_str();
}


inline std::string epoll_ev2str(uint32_t code) {
    std::map<int, std::string> ev_str = {
        { EPOLLIN        ,"EPOLLIN"        },
        { EPOLLOUT       ,"EPOLLOUT"       },
        { EPOLLRDHUP     ,"EPOLLRDHUP"     },
        { EPOLLPRI       ,"EPOLLPRI"       },
        { EPOLLERR       ,"EPOLLERR"       },
        { EPOLLHUP       ,"EPOLLHUP"       },
        { EPOLLET        ,"EPOLLET"        },
        { EPOLLONESHOT   ,"EPOLLONESHOT"   },
        { EPOLLWAKEUP    ,"EPOLLWAKEUP"    },
        { EPOLLEXCLUSIVE ,"EPOLLEXCLUSIVE" },
    };

    bool add_or = false;
    std::string ret = "[";
    for (auto &[ev, str] : ev_str) {
        if (code & ev) {
            ret += (add_or ? "|" : "") + str;
            add_or = true;
        }
    }
    ret += "]";
    return ret;
}

inline task_t dbg_print_internal_info() {
    /* TODO: print everything from poll */
    struct dbg_awaiter_t {
        bool await_ready() { return false; }
        bool await_suspend(handle_t curr) {
            auto pool = curr.promise().pool;
            auto sch = &(pool->fd_sched);

            DBG("====== CO::POOL/INTERNALS ======")
            for (auto &handle : pool->waiting_tasks) {
                DBG("pool:waiting: %s", co_str(handle))
            }
            DBG("Timers in pool: %ld", pool->timer_fd_pool.size());
            DBG("Return value: %d", pool->ret_val);

            for (auto &[fd, fdw] : sch->waiting_tasks) {
                DBG("fd: %d mask: %x", fd, fdw.mask);
                for (auto &w : fdw.waiters) {
                    DBG("->\twaiter: %x coro: %s", w.mask, co_str(w.ptr));
                }
            }
            for (auto &handle : sch->ready_tasks) {
                DBG("sch:ready: %s", co_str(handle));
            }
            DBG("------ CO::POOL/INTERNALS ------")

            return false;
        }
        void await_resume() {}
    };
    co_await dbg_awaiter_t{};
    co_return 0;
}

inline sched_awaiter_t sched(task_t to_sched, co_mod_ptr_t pmods) {
    if (pmods)
        to_sched.handle.promise().pmods = pmods;
    to_sched.ever_called = true;
    return sched_awaiter_t{to_sched.handle};
}

inline yield_awaiter_t yield() {
    return yield_awaiter_t{};
}

inline task_t mod_task(task_t task, co_mod_ptr_t pmods) {
    task.handle.promise().pmods = pmods;
    return task;
}

inline task_t timed(task_t task, uint64_t timeo_us) {
    /* here most often then not the promise is allocated but it doesn't yet know what is the pool
    that it is running on. Still, we have access to some pmods. */
    /* What we will do is the following: add the CO_MOD_TIMEOUT to this task and return it to the
    caller. When co_awaited by another task this coroutine will have the timeo flag set with the
    max time it can elapse from that point. The task responsable for the timeout will schedule a
    sleep in paralel to the scheduled coroutine pointing to this co_mod_t.   */

    auto timeo_pmod = std::make_shared<co_mod_t>(CO_MOD_TIMEOUT);
    /* TODO: maybe check if alloc failed? */

    timeo_pmod->m.timeo.timeo_us = timeo_us;

    task.handle.promise().pmods = co_mod_t::attach(timeo_pmod, task.handle.promise().pmods);
    return task;
}

inline task_t trace(task_t task, trace_fn_t fn, trace_ctx_t ctx) {
    auto trace_pmod = std::make_shared<co_mod_t>(CO_MOD_TRACE);

    trace_pmod->m.trace.fn = fn;
    trace_pmod->m.trace.ctx = ctx;

    task.handle.promise().pmods = co_mod_t::attach(trace_pmod, task.handle.promise().pmods);
    return task;
}

template <typename Awaiter>
inline task_t await(Awaiter& awaiter) {
    co_await awaiter;
    co_return 0; 
}

inline task_t force_stop(int ret) {
    struct stop_awaiter_t {
        bool        await_ready() { return false; }
        handle_vt   await_suspend(handle_t curr) {
            curr.promise().pool->ret_val = ret;
            return std::noop_coroutine();
        }
        void        await_resume() {}

        int ret;
    };
    co_await stop_awaiter_t{ret};
    co_return 0;
}

#define CO_INTERNAL_AWAIT(awaiter) \
{ \
    int ret = co_await awaiter; \
    if (ret == CO_WAKEUP_ERR) { \
        co_return CO_WAKEUP_ERR; \
    }\
    if (ret < 0) \
        co_return ret; \
}

/* TODO: I don't really trust this transform from blocking to non-blocking of the socket,
should investigate it further */
inline task_t connect(int fd, sockaddr *sa, socklen_t len) {
    int flags;
    ASSERT_COFN(flags = fcntl(fd, F_GETFL, 0));
    ASSERT_COFN(fcntl(fd, F_SETFL, flags | O_NONBLOCK));

    int res = ::connect(fd, sa, len);

    ASSERT_COFN(flags = fcntl(fd, F_GETFL, 0));
    ASSERT_COFN(fcntl(fd, F_SETFL, flags & ~O_NONBLOCK));

    if (res < 0 && errno != EINPROGRESS) {
        co_return res;
    }

    if (res == 0)
        co_return 0;

    fd_awaiter_t awaiter {
        .wait_cond = EPOLLOUT,
        .fd = fd,
    };
    CO_INTERNAL_AWAIT(awaiter);

    int result;
    socklen_t result_len = sizeof(result);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) < 0) {
        // error, fail somehow, close socket
        co_return -1;
    }

    if (result != 0) {
        // connection failed; error code is in 'result'
        co_return result;
    }

    co_return 0;
}

inline task_t accept(int fd, sockaddr *sa, socklen_t *len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLIN,
        .fd = fd,
    };
    CO_INTERNAL_AWAIT(awaiter);
    co_return ::accept(fd, sa, len);
}

inline task_t read(int fd, void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLIN,
        .fd = fd,
    };
    CO_INTERNAL_AWAIT(awaiter);
    co_return ::read(fd, buff, len);
}

inline task_t write(int fd, const void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLOUT,
        .fd = fd,
    };
    CO_INTERNAL_AWAIT(awaiter);
    co_return ::write(fd, buff, len);
}

inline task_t read_sz(int fd, void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLIN,
        .fd = fd,
    };
    size_t original_len = len;
    while (true) {
        if (!len)
            break ;
        CO_INTERNAL_AWAIT(awaiter);
        int ret = ::read(fd, buff, len);
        if (ret == 0) {
            DBG("Read failed, closed peer");
            co_return -1;
        }
        else if (ret < 0) {
            DBGE("Failed read");
            co_return -1;
        }
        else {
            len -= ret;
            buff = (char *)buff + ret;
        }
    }
    co_return original_len;
}

inline task_t write_sz(int fd, const void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLOUT,
        .fd = fd,
    };
    size_t original_len = len;
    while (true) {
        if (!len)
            break ;
        CO_INTERNAL_AWAIT(awaiter);
        int ret = ::write(fd, buff, len);
        if (ret < 0) {
            DBGE("Failed write");
            co_return -1;
        }
        else {
            len -= ret;
            buff = (char *)buff + ret;
        }
    }
    co_return original_len;
}

inline task_t wait_event(int fd, int event) {
    fd_awaiter_t awaiter {
        .wait_cond = event,
        .fd = fd,
    };
    CO_INTERNAL_AWAIT(awaiter);
    co_return 0;
}

inline task_t stopfd(int fd) {
    struct stopfd_awaiter_t {
        bool await_ready() { return false; }
        bool await_suspend(handle_t curr) {
            res = 0;
            if (curr.promise().pool->fd_sched.wakeup_wait(fd) < 0) {
                DBG("Couldn't awake fd");
                res = -1;
            }
            return false;
        }
        int await_resume() {
            return res;
        }

        int fd;
        int res;
    };
    co_return co_await stopfd_awaiter_t{.fd = fd, .res = 0};
}

inline task_t sleep_us(uint64_t timeo_us) {
    sleep_awaiter_t sleep_awaiter{ .sleep_us = timeo_us };
    ASSERT_COFN(co_await sleep_awaiter);
    co_return 0;
}

inline task_t sleep_ms(uint64_t timeo_ms) {
    ASSERT_COFN(co_await CO_REG_INTERN(sleep_us(timeo_ms * 1000)));
    co_return 0;
}

inline task_t sleep_s(uint64_t timeo_s) {
    ASSERT_COFN(co_await CO_REG_INTERN(sleep_us(timeo_s * 1000'000)));
    co_return 0;
}

inline task_t var_sleep_us(uint64_t timeo_us, sleep_handle_t *sleep_handle) {
    if (timeo_us == 0)
        co_return 0;
    if (sleep_handle->fd_ptr != (int *)(intptr_t)(-1) && sleep_handle->fd_ptr != NULL) {
        DBG("sleep_handle can't be used for multiple var_sleeps at once");
        co_return -1; /* TODO: who receives this return value? */
    }
    if (sleep_handle->fd_ptr == (int *)(intptr_t)(-1)) {
        co_return 0;
    }
    sleep_awaiter_t sleep_awaiter{ .fd_ref = &sleep_handle->fd_ptr, .sleep_us = timeo_us };
    ASSERT_COFN(co_await sleep_awaiter);
    sleep_handle->fd_ptr = (int *)(intptr_t)(-1);
    co_return 0;
}

inline task_t var_sleep_ms(uint64_t timeo_ms, sleep_handle_t *sleep_handle) {
    ASSERT_COFN(co_await CO_REG_INTERN(var_sleep_us(timeo_ms * 1000, sleep_handle)));
    co_return 0;
}

inline task_t var_sleep_s(uint64_t timeo_s, sleep_handle_t *sleep_handle) {
    ASSERT_COFN(co_await CO_REG_INTERN(var_sleep_us(timeo_s * 1000'000, sleep_handle)));
    co_return 0;
}

inline int stop_sleep(sleep_handle_t *sleep_handle) {
    if (sleep_handle->fd_ptr == NULL) {
        /* Timer is stopped apriori */
        sleep_handle->fd_ptr = (int *)(intptr_t)(-1);
        return 0;
    }
    if (sleep_handle->fd_ptr == (int *)(intptr_t)(-1)) {
        /* Timer was stopped, either here or in var_sleep after the timer elapsed */
        return 0;
    }
    itimerspec its = {};
    its.it_value.tv_nsec = 1;
    its.it_value.tv_sec = 0;
    ASSERT_FN(timerfd_settime(*sleep_handle->fd_ptr, 0, &its, NULL));
    sleep_handle->fd_ptr = (int *)(intptr_t)(-1);
    return 0;
}

inline task_t when_all(std::vector<task_t> tasks) {
    sem_t sem(0);
    int ret = 0;
    for (auto &t : tasks) {
        sem._dec();
        auto one_cond_waiter_task = [&sem, &ret](task_t h) -> task_t {
            ret |= co_await h;
            sem.rel();
            co_return ret;
        }(t);
        co_await sched(CO_REG_INTERN(one_cond_waiter_task));
    }
    co_await sem;
    co_return ret;
}

inline void dbg_trace_fn(int e, void *coaddr, void *) {
    DBG("[TRACE] %40s at %s", co::enum_str(e), co::co_str(coaddr));
}


template <typename TASK_T>
inline TASK_T dbg_register(TASK_T &&task, std::string task_name) {
    co_str_intern(task.handle, task_name);
    return std::forward<TASK_T>(task);
}

template <typename CORO_T>
inline CORO_T dbg_coro_str_forward(CORO_T &&coro) {
    DBG("co::> %s", co::co_str_intern(coro));
    return std::forward<CORO_T>(coro);
}

}

#endif
