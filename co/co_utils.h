// #include <coro/coro.hpp>
#include <iostream>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <set>
#include <stack>
#include <map>
#include <queue>
#include <coroutine>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#include "demangle.h"

// check this?
// https://stackoverflow.com/questions/67446478/symmetric-transfer-does-not-prevent-stack-overflow-for-c20-coroutines

#define ASSERT_COFN(fn_call)\
if (intptr_t(fn_call) < 0) {\
    DBGE("FAILED: " #fn_call);\
    co_return -1;\
}

namespace co
{

struct pool_t;
struct promise_t;
struct task_t;

using handle_t = std::coroutine_handle<promise_t>;
using handle_vt = std::coroutine_handle<void>;

/* main awaiter and main promise handle. */
struct task_t {
    using promise_type = promise_t;

    task_t(handle_t handle) : handle(handle) {}

    bool await_ready() noexcept { return false; }
    handle_t await_suspend(handle_t caller) noexcept;
    int await_resume() noexcept;

    handle_t handle;
};

static const char *co_str(std::coroutine_handle<> handle) {
    static std::map<void *, std::string> id_str;
    static uint64_t id = 0;
    if (!HAS(id_str, handle.address()))
        id_str[handle.address()] = sformat("co[%d]", id++);
    return id_str[handle.address()].c_str();
}

/* this holds the promise handle, this is it's main usage */

struct final_awaiter_t {
    final_awaiter_t(pool_t *pool) : pool(pool) {}

    bool await_ready() noexcept { return false; }

    handle_vt await_suspend(handle_t oth) noexcept;

    void await_resume() noexcept {
        DBG("Shouldn't reach await_resume in caller_info_t?");
        DBG("Maybe for yields?")
        std::terminate();
    }

    pool_t *pool;
};

static int64_t primise_cnt;

/* this holds the task metadata */
struct promise_t {

    promise_t() { primise_cnt++; }
    ~promise_t() { primise_cnt--; }

    task_t get_return_object() {
        return task_t{handle_t::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
        // DBG("Initial suspend <%s>", co_str(handle_t::from_promise(*this)));
        return {};
    }

    final_awaiter_t final_suspend() noexcept {
        return final_awaiter_t(pool);
    }

    void return_value(int ret_val) {
        this->ret_val = ret_val;
    }

    void unhandled_exception() {
        /* std::current_exception() to get exc */
        DBG("Exceptions are not my coup of tea, if it goes it goes");
        std::terminate();
    }

    int ret_val;
    int call_fd;
    int call_res;
    pool_t *pool;

    handle_t caller;
};

struct pool_data_t {
    handle_t next_handle;
};

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

struct fd_sched_t {
    std::queue<handle_t> ready_tasks;
    std::vector<struct epoll_event> ret_evs;
    int epoll_fd;

    std::map<int, handle_t> fds;

    struct fd_data_t {
        int fd;
        handle_t handl;
    };

    fd_sched_t() {
        epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd < 0) {
            DBG("Failed to create epoll");
        }
    }

    int check_events_noblock() {
        int num_evs;
        ASSERT_FN(num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), 0));
        int ret = handle_events(num_evs);
        return ret;
    }

    bool pending() {
        return ready_tasks.size();
    }

    handle_t get_next() {
        auto ret = ready_tasks.front();
        ready_tasks.pop();
        return ret;
    }

    int wait_events() {
        int num_evs;
        ASSERT_FN(num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), -1));
        return handle_events(num_evs);
    }

    int insert_wait(handle_t to_wait, int fd, int wait_cond) {
        struct epoll_event ev = {};
        ev.events = wait_cond;
        ev.data.ptr = to_wait.address();
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev));
        fds.insert({fd, to_wait});
        ret_evs.push_back(epoll_event{});
        return 0;
    }

    int remove_wait(int fd) {
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL));
        fds.erase(fd);
        ret_evs.pop_back();
        return 0;
    }

    int handle_events(int num_evs) {
        for (int i = 0; i < num_evs; i++) {
            auto handle = handle_t::from_address(ret_evs[i].data.ptr);
            if (handle) {
                handle.promise().call_res = 0;
                ready_tasks.push(handle);
            }
        }
        return 0;
    }
};

#define MAX_TIMER_POOL_SIZE 64

/* this gets tasks as input and runs them continuosly */
struct pool_t {
    using task_it_t = std::set<handle_t>::iterator;

    /* those are the tasks that wait to be rescheduled */
    std::queue<handle_t> waiting_tasks;
    fd_sched_t fd_sched;
    std::stack<int> timer_fd_pool;

    void sched(handle_t handle) {
        // DBG("scheduling: %s", co_str(handle));
        handle.promise().pool = this;
        waiting_tasks.push(handle);
    }

    void sched(task_t task) {
        sched(task.handle);
    }

    int run() {
        /* TODO: add logic to make it run only once, such that current_task will not be replaced
        by running 'run' twice */
        if (waiting_tasks.size()) {
            handle_t first_task = waiting_tasks.front();
            waiting_tasks.pop();
            first_task.resume();
            /* here we can know that  */
        }
        return 0;
    }

    handle_vt next_task() {
        /* TODO: refactor this code, if epoll errors out simply tell all coroutines to close */
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
                if (fd_sched.fds.size() == 0) {
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

    handle_vt resched_wait_fd(handle_t to_wait, int fd, int wait_cond) {
        // DBG("Scheduling %s to start on fd: %d mask: %x", co_str(to_wait), fd, wait_cond);
        if (fd_sched.insert_wait(to_wait, fd, wait_cond) < 0) {
            DBG("Can't insert a file descriptor for waiting");
            return std::noop_coroutine();
        }
        return next_task();
    }

    handle_vt resched_wait_sem(handle_t to_wait) {
        return next_task();
    }

    ~pool_t() {
        while (timer_fd_pool.size()) {
            close(timer_fd_pool.top());
            timer_fd_pool.pop();
        }
    }

private:

};

handle_vt final_awaiter_t::await_suspend(handle_t ending_task) noexcept {
    auto caller = ending_task.promise().caller;
    ending_task.destroy();
    if (caller) {
        return caller;
    }
    else {
        return pool->next_task();
    }
}

handle_t task_t::await_suspend(handle_t caller) noexcept {
    handle.promise().caller = caller;
    handle.promise().pool = caller.promise().pool;
    return handle;
}

int task_t::await_resume() noexcept {
    return handle.promise().ret_val;
}

/* this is a generic awaiter, it will know how to reschedule fd-stuff, sleep-stuff and mu-stuff */
/* this is only for internal use */
struct fd_awaiter_t {
    bool await_ready() {
        return false;
    }

    handle_vt await_suspend(handle_t caller_handle) {
        /* We get the pool object, we mark in the pool that we wait for events on this fd and we
        let the next coroutine to continue on this thread */
        pool = caller_handle.promise().pool;
        this->caller_handle = caller_handle;
        return pool->resched_wait_fd(caller_handle, fd, wait_cond);
    }

    int await_resume() {
        /* this coroutine can only suspend on call, so we know that the scheduler was asked to
        take controll of this coroutine and we know that 'sched' and 'handle' are set, as such
        we can request the return value for the fd we just queried */

        return caller_handle.promise().call_res;
    }

    ~fd_awaiter_t() {
        pool->fd_sched.remove_wait(fd);
    }

    int wait_cond;
    int fd;

    handle_t caller_handle;
    pool_t *pool;
};

/* This awaiter does only one thing and that is to schedule a coroutine */
struct sched_awaiter_t {
    handle_t to_sched;

    bool await_ready() { return false; }
    bool await_suspend(handle_t handle) {
        handle.promise().pool->sched(to_sched);
        return false;
    }
    void await_resume() {}
};

auto sched(task_t to_sched) {
    DBG("Will sched: %s", co_str(to_sched.handle));
    return sched_awaiter_t{to_sched.handle};
}

task_t accept(int fd, sockaddr *sa, socklen_t *len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLIN,
        .fd = fd,
    };
    ASSERT_COFN(co_await awaiter);
    co_return ::accept(fd, sa, len);
}

task_t read_sz(int fd, void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLIN,
        .fd = fd,
    };
    size_t original_len = len;
    while (true) {
        if (!len)
            break ;
        ASSERT_COFN(co_await awaiter);
        int ret = read(fd, buff, len);
        if (ret == 0) {
            DBG("Read failed, closed peer");
            co_return 0;
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

task_t write_sz(int fd, const void *buff, size_t len) {
    fd_awaiter_t awaiter {
        .wait_cond = EPOLLOUT,
        .fd = fd,
    };
    size_t original_len = len;
    while (true) {
        if (!len)
            break ;
        ASSERT_COFN(co_await awaiter);
        int ret = write(fd, buff, len);
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

struct sleep_awaiter_t {
    bool await_ready() { return false; }
    handle_vt await_suspend(handle_t caller_handle) {
        pool = caller_handle.promise().pool;

        FnScope err_scope;
        if (pool->timer_fd_pool.size() > 0) {
            timer_fd = pool->timer_fd_pool.top();
            pool->timer_fd_pool.pop();
            err_scope([&]{ close(timer_fd); });
        }
        else {
            timer_fd = timerfd_create(CLOCK_REALTIME, 0);
            if (timer_fd < 0) {
                DBGE("Failed to allocate new timer");
                return std::noop_coroutine();
            }
        }
        itimerspec its = {};
        its.it_value.tv_nsec = (sleep_us % 1000'000) * 1000ULL;
        its.it_value.tv_sec = sleep_us / 1000'000ULL;
        if (timerfd_settime(timer_fd, 0, &its, NULL) < 0) {
            DBGE("Failed to set expiration date");
            return std::noop_coroutine();
        }

        err_scope.disable();
        this->caller_handle = caller_handle;
        return pool->resched_wait_fd(caller_handle, timer_fd, EPOLLIN);
    }

    int await_resume() {
        /* this coroutine can only suspend on call, so we know that the scheduler was asked to
        take controll of this coroutine and we know that 'sched' and 'handle' are set, as such
        we can request the return value for the fd we just queried */

        pool->fd_sched.remove_wait(timer_fd);
        if (pool->timer_fd_pool.size() >= MAX_TIMER_POOL_SIZE) {
            close(timer_fd);
        }
        else {
            pool->timer_fd_pool.push(timer_fd);
        }

        return caller_handle.promise().call_res;
    }

    uint64_t sleep_us;
    int timer_fd;
    handle_t caller_handle;
    pool_t *pool;
};

task_t sleep_us(uint64_t timeo_us) {
    ASSERT_COFN(co_await sleep_awaiter_t{ .sleep_us = timeo_us });
    co_return 0;
}

task_t sleep_ms(uint64_t timeo_ms) {
    ASSERT_COFN(co_await sleep_us(timeo_ms * 1000));
    co_return 0;
}

task_t sleep_s(uint64_t timeo_s) {
    ASSERT_COFN(co_await sleep_us(timeo_s * 1000'000));
    co_return 0;
}

struct yield_awaiter_t {
    bool await_ready() { return false; }
    handle_vt await_suspend(handle_t handle) {
        handle.promise().pool->waiting_tasks.push(handle);
        return handle.promise().pool->next_task();
    }
    void await_resume() {}
};

auto yield() {
    DBG("Will yield");
    return yield_awaiter_t{};
}

struct sem_t {
    int64_t counter;
    std::queue<handle_t> waiting_on_sem;

    sem_t(int64_t counter = 0) : counter(counter) {}

    bool await_ready() {
        if (counter > 0) {
            counter--;
            return true;
        }
        return false;
    }

    handle_vt await_suspend(handle_t to_suspend) {
        /* if we are here it must mean that the counter was zero and as such we must yield */
        waiting_on_sem.push(to_suspend);
        return to_suspend.promise().pool->next_task();
    }

    int await_resume() {
        return 0;
    }

    void rel() {
        if (counter == 0 && waiting_on_sem.size())
            _awake_one();
        else {
            counter++;
            if (counter == 0)
                _awake_one();
        }
    }

    void rel_all() {
        if (counter > 0)
            return ;
        while (waiting_on_sem.size())
            _awake_one();
    }

private:
    void _awake_one() {
        auto to_awake = waiting_on_sem.front();
        waiting_on_sem.pop();
        to_awake.promise().pool->waiting_tasks.push(to_awake);
    }
};

}
