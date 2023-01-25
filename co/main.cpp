// #include <coro/coro.hpp>
#include <iostream>
#include <unordered_map>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <set>
#include <map>
#include <queue>
#include <coroutine>
#include <sys/epoll.h>
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#include "demangle.h"

https://stackoverflow.com/questions/67446478/symmetric-transfer-does-not-prevent-stack-overflow-for-c20-coroutines

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

    handle_t await_suspend(handle_t oth) noexcept;

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

enum {
    WAIT_FD_READ = 1,
    WAIT_FD_WRITE = 2,
    WAIT_FD_NOBLOCK = 4,
};

struct fd_sched_t {
    std::queue<handle_t> ready_tasks;
    std::vector<struct epoll_event> ret_evs;
    int epoll_fd;

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
        if (wait_cond & WAIT_FD_READ)
            ev.events |= EPOLLIN;
        if (wait_cond & WAIT_FD_WRITE)
            ev.events |= EPOLLOUT;
        // to_wait.promise().call_fd = fd;
        ev.data.ptr = to_wait.address();
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev));
        ret_evs.push_back(epoll_event{});
        return 0;
    }

    int remove_wait(int fd) {
        ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL));
        ret_evs.pop_back();
        return 0;
    }

    int handle_events(int num_evs) {
        for (int i = 0; i < num_evs; i++) {
            auto handle = handle_t::from_address(ret_evs[i].data.ptr);
            handle.promise().call_res = 0;
            ready_tasks.push(handle);
        }
        return 0;
    }
};

/* this gets tasks as input and runs them continuosly */
struct pool_t {
    using task_it_t = std::set<handle_t>::iterator;

    /* those are the tasks that wait to be rescheduled */
    std::queue<handle_t> waiting_tasks;
    handle_t current_task;

    fd_sched_t fd_sched;

    void sched(handle_t handle) {
        // DBG("scheduling: %s", co_str(handle));
        handle.promise().pool = this;
        waiting_tasks.push(handle);
    }

    void sched(task_t task) {
        sched(task.handle);
    }

    void run() {
        /* TODO: add logic to make it run only once, such that current_task will not be replaced
        by running 'run' twice */
        if (waiting_tasks.size()) {
            current_task = waiting_tasks.front();
            waiting_tasks.pop();
            current_task.resume();
        }
    }

    handle_t next_task() {
        /* TODO: refactor this code, if epoll errors out simply tell all coroutines to close */
        if (waiting_tasks.size()) {
            /* We run an already scheduled task */
            current_task = waiting_tasks.front();
            waiting_tasks.pop();
            return current_task;
        }
        else {
            handle_t ret;

            if (fd_sched.pending()) {
                ret = fd_sched.get_next();
            }
            else {
                int err = fd_sched.check_events_noblock();
                if (err < 0) {
                    DBG("The epoll loop failed, will stop the scheduler");
                    // return std::noop_coroutine();
                }
                if (fd_sched.pending()) {
                    ret = fd_sched.get_next();
                }
                else {
                    err = fd_sched.wait_events();
                    if (err < 0) {
                        DBG("The epoll loop failed to wait, stopping the scheduler");
                        // return std::noop_coroutine();
                    }
                    if (!fd_sched.pending()) {
                        DBG("We have no more pending waits, this is an invalid state");
                        // return std::noop_coroutine();
                    }
                    ret = fd_sched.get_next();
                }
            }
            // DBG("Done, returning: %s", co_str(ret));
            return ret;
        }
    }

    handle_t resched_wait_fd(handle_t to_wait, int fd, int wait_cond) {
        // DBG("Scheduling %s to start on fd: %d mask: %x", co_str(to_wait), fd, wait_cond);
        if (fd_sched.insert_wait(to_wait, fd, wait_cond) < 0) {
            DBG("Can't insert a file descriptor for waiting");
            // return std::noop_coroutine();
        }
        return next_task();
    }

    handle_t resched_task_done(handle_t old_task) {
        DBG("Replacing task %s", co_str(old_task));
        old_task.destroy();
        DBG("Will get next task");
        auto ret = next_task();
        DBG("Next task: %s", co_str(ret));
        return ret;
    }

    bool is_ready(int task_id) {
        /* TODO: this should tell us if the task is ready to be executed */
        return false;
    }

private:


};

handle_t final_awaiter_t::await_suspend(handle_t ending_task) noexcept {
    auto caller = ending_task.promise().caller;
    // DBG("<%s> returning", co_str(ending_task));
    if (caller) {
        // DBG("Returned ctrl to %s", co_str(ending_task.promise().caller));
        ending_task.destroy();
        return caller;
    }
    else {
        return pool->resched_task_done(ending_task);
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
struct awaiter_t {
    bool await_ready() {
        return false;
    }

    handle_t await_suspend(handle_t caller_handle) {
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

    ~awaiter_t() {
        pool->fd_sched.remove_wait(fd);
    }

    int wait_cond;
    int fd;
    handle_t caller_handle;
    pool_t *pool;
};

/* TODO: sched - this function will  */

auto sched(task_t to_sched) {
    struct sched_awaiter_t {
        handle_t to_sched;

        bool await_ready() { return false; }
        bool await_suspend(handle_t handle) {
            handle.promise().pool->sched(to_sched);
            handle.promise().caller = handle_t{};
            return false;
        }
        void await_resume() {}
    };
    DBG("Will sched: %s", co_str(to_sched.handle));
    return sched_awaiter_t{to_sched.handle};
}

task_t accept(int fd, sockaddr *sa, socklen_t *len) {
    awaiter_t awaiter {
        .wait_cond = WAIT_FD_READ,
        .fd = fd,
    };
    ASSERT_COFN(co_await awaiter);
    co_return ::accept(fd, sa, len);
}

task_t read_sz(int fd, void *buff, size_t len) {
    awaiter_t awaiter {
        .wait_cond = WAIT_FD_READ,
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
    awaiter_t awaiter {
        .wait_cond = WAIT_FD_WRITE,
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

}

#define SERVER_PORT 5123

co::task_t client_handle(int sock_fd) {
    DBG_SCOPE();
    FnScope scope([&] {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    });

    while (true) {
        char msg[] = "Ini iro mara";
        char rcv[] = "Ini iro mara";
        int ret;

        ASSERT_COFN(co_await co::read_sz(sock_fd, rcv, sizeof(rcv)));
        DBG("received: %s", rcv);
        ASSERT_COFN(co_await co::write_sz(sock_fd, msg, sizeof(msg)));
    }
}

co::task_t server() {
    DBG_SCOPE();
    int sock_fd;
    int enable = 1;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SERVER_PORT);
    socklen_t len = sizeof(addr);

    ASSERT_COFN(sock_fd = socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_COFN(setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)));
    ASSERT_COFN(bind(sock_fd, (sockaddr *)&addr, len));
    ASSERT_COFN(listen(sock_fd, 5));

    FnScope scope([&] {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    });

    while (true) {
        int client_fd;
        ASSERT_COFN(client_fd = co_await co::accept(sock_fd, (sockaddr *)&addr, &len));
        co_await co::sched(client_handle(client_fd));
    }
}

co::task_t client() {
    DBG_SCOPE();
    int sock_fd;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(SERVER_PORT);
    socklen_t len = sizeof(addr);

    ASSERT_COFN(sock_fd = socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_COFN(connect(sock_fd, (sockaddr *)&addr, len));

    FnScope scope([&] {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    });

    while (true) {
        char msg[] = "Ana are mere";
        int ret;
        ASSERT_COFN(co_await co::write_sz(sock_fd, msg, sizeof(msg)));
        ASSERT_COFN(ret = co_await co::read_sz(sock_fd, msg, sizeof(msg)));
        if (ret == 0) {
            DBG("Connection closed");
            co_return 0;
        }
        DBG("received: %s, %ld", msg, co::primise_cnt);
    }
}

int main(int argc, char const *argv[])
{
    co::pool_t pool;

    DBG("Will schedule server");
    pool.sched(server());
    
    DBG("Will schedule client");
    pool.sched(client());

    DBG("Will run the pool");

    pool.run();
    return 0;
}