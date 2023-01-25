#include <coro/coro.hpp>
#include <iostream>
#include <unordered_map>
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#include "demangle.h"

// namespace co
// {

// struct sched_t {
//     using ctx_t = void *;
//     using cbk_t = int (*) (int fd, uint32_t events, int *errno_value, ctx_t ctx);

//     /* TODO: get rid of useless connections afterwards(pointers and indices) */
//     struct sched_cbk_t {
//         cbk_t cbk;
//         ctx_t ctx;
//     };

//     struct sched_epoll_t {
//         uint32_t en_events;
//         sched_cbk_t cbk;
//     };

//     int epoll_fd;
//     std::unordered_map<int, sched_epoll_t> sched_events;
//     std::vector<struct epoll_event> ret_evs;

//     int  init();
//     void uninit();
//     int  run_sched();
//     int  add_event(int fd, uint32_t en_events, cbk_t cbk, ctx_t ctx);
//     int  rm_event(int fd);
// };

// inline std::string epoll_ev2str(uint32_t code);

// /* IMPLEMENTATION
// ================================================================================================= */

// int sched_t::init() {
//     DBG_SCOPE();
//     ASSERT_FN(epoll_fd = epoll_create1(EPOLL_CLOEXEC));
//     return 0;
// }

// void sched_t::uninit() {
//     close(epoll_fd);
// }

// int sched_t::run_sched() {
//     DBG_SCOPE();
//     while (true) {
//         /* TODO: create a timer fd and use it for delays: use it whenever a timer is called and
//         remember a priority queue such that all the callers are awaken at their desired time.
//         remember the time of awakening in the prio queue, such that we don't add up incremental
//         errors */
//         DBG("ret_evs sz: %ld", ret_evs.size());
//         int num_evs;
//         ASSERT_FN(num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), -1));

//         std::map<int, uint32_t> active_fds;
//         for (int i = 0; i < num_evs; i++)
//             active_fds[ret_evs[i].data.fd] = ret_evs[i].events;

//         while (true) {
//             if (active_fds.empty())
//                 break ;
//             std::vector<int> inactive_fds;
//             for (auto [fd, events] : active_fds) {
//                 std::vector<sched_t::cbk_t> err_cbks;
//                 auto &cbk = sched_events[fd].cbk;
//                 int errno_value = 0;
//                 int ret = cbk.cbk(fd, events, &errno_value, cbk.ctx);
//                 if (ret < 0) {
//                     if (errno_value == EAGAIN) {
//                         inactive_fds.push_back(fd);
//                     }
//                     else {
//                         DBG("cbk failed with: %d", ret);
//                         return -1;
//                     }
//                 }
//             }
//             for (auto &inactive_fd : inactive_fds)
//                 active_fds.erase(inactive_fd);

//             /* In case we have more events that are awakened during handling never ending events
//             we also want to handle them */
//             ASSERT_FN(num_evs = epoll_wait(epoll_fd, ret_evs.data(), ret_evs.size(), 0));
//             for (int i = 0; i < num_evs; i++)
//                 active_fds[ret_evs[i].data.fd] = ret_evs[i].events;
//         }

//         /* TODO: do all events in a while loop to give access to new fd's to have time on cpu and
//         epoll_wait(_, _, _, 0) to add new fd's to the 'active' fds loop */
//         DBG("num_evs: %d", num_evs);
//     }
// }

// int sched_t::add_event(int fd, uint32_t en_events, cbk_t cbk, ctx_t ctx) {
//     /* TODO: add mutex here for multithreading, if needed */
//     if (HAS(sched_events, fd)) {
//         /* TODO: check if if is needed */
//         DBG("ERROR: Double add of fd: %d", fd);
//         return -1;
//     }
//     /* TODO: make the code compatible with fds that can't be non-blocking */
//     int edge_trig = EPOLLET;
//     int flags;
//     ASSERT_FN(flags = fcntl(fd, F_GETFL, 0));
//     ASSERT_FN(fcntl(fd, F_SETFL, flags | O_NONBLOCK));

//     sched_events[fd] = sched_epoll_t {
//         .en_events = en_events,
//         .cbk = sched_cbk_t {
//             .cbk = cbk,
//             .ctx = ctx,
//         },
//     };
//     struct epoll_event ev; 
//     ev.events = en_events | edge_trig;
//     ev.data.fd = fd;
//     FnScope err_scope([this, fd]{ sched_events.erase(fd); });

//     /* TODO: try to make the fd non-blocking before entering this section and if succeded
//     remember this information inside the space in sched_events */
//     ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev));
//     DBG("New fd in epoll[%d]: %d", epoll_fd, fd);
//     err_scope.disable();
//     ret_evs.push_back(epoll_event{});
//     return 0;
// }

// int sched_t::rm_event(int fd) {
//     if (!HAS(sched_events, fd)) {
//         /* TODO: check if if is needed */
//         DBG("ERROR: rm inexisting fd: %d", fd);
//         return -1;
//     }
//     sched_events.erase(fd);
//     ASSERT_FN(epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL));
//     DBG("Rm fd from epoll[%d]: %d", epoll_fd, fd);
//     ret_evs.pop_back();
//     return 0;
// }

// inline std::string epoll_ev2str(uint32_t code) {
//     std::map<int, std::string> ev_str = {
//         { EPOLLIN        ,"EPOLLIN"        },
//         { EPOLLOUT       ,"EPOLLOUT"       },
//         { EPOLLRDHUP     ,"EPOLLRDHUP"     },
//         { EPOLLPRI       ,"EPOLLPRI"       },
//         { EPOLLERR       ,"EPOLLERR"       },
//         { EPOLLHUP       ,"EPOLLHUP"       },
//         { EPOLLET        ,"EPOLLET"        },
//         { EPOLLONESHOT   ,"EPOLLONESHOT"   },
//         { EPOLLWAKEUP    ,"EPOLLWAKEUP"    },
//         { EPOLLEXCLUSIVE ,"EPOLLEXCLUSIVE" },
//     };

//     bool add_or = false;
//     std::string ret = "[";
//     for (auto &[ev, str] : ev_str) {
//         if (code & ev) {
//             ret += (add_or ? "|" : "") + str;
//             add_or = true;
//         }
//     }
//     ret += "]";
//     return ret;
// }

// inline task_t<int> write(int fd, const void *buff, size_t len) {
//     /* TODO: do magic to suspend the execution untill the scheduler leaves us to write */
//     co_return -1;
// }

// inline task_t<int> read(int fd, void *buff, size_t len) {
//     /* TODO: do magic to suspend the execution untill the scheduler leaves us to read */
//     co_return -1;
// }

// inline task_t<void> sleep_s(uint64_t time_s) {
//     /* TODO: */
//     co_return ;
// }

// inline task_t<void> sleep_ms(uint64_t time_ms) {
//     /* TODO: */
//     co_return ;
// }

// inline task_t<void> sleep_us(uint64_t time_us) {
//     /* TODO: */
//     co_return ;
// }


// } /* namespace co */

// /* MAIN:
// ================================================================================================= */

// static int test_producer(int fd, uint32_t events, int *errno_value, void *ctx) {
//     static int num = 0;
//     int ret;
//     num++;
//     ASSERT_FN(ret = write(fd, &num, sizeof(num)));
//     *errno_value = errno;
//     DBG("events: %8x num: %8d errno: %4d ret: %4d", events, num, errno, ret);
//     // sleep_ms(500);
//     return 0;
// }

// static int test_consumer(int fd, uint32_t events, int *errno_value, void *ctx) {
//     int num, ret;
//     ASSERT_FN(ret = read(fd, &num, sizeof(num)));
//     *errno_value = errno;
//     DBG("events: %8x num: %8d errno: %4d ret: %4d", events, num, errno, ret);
//     return 0;
// }

// static co::task_t test_co_producer(int fd) {
//     while (true) {
//         int num, ret;
//         ASSERT_FN(ret = co_await co::write(fd, &num, sizeof(num)))
//         DBG("events: %8x num: %8d errno: %4d ret: %4d", events, num, errno, ret);
//         co_await co::sleep_ms(500);
//     }
// }

// static co::task_t test_co_consumer(int fd) {
//     while (true) {
//         int num, ret;
//         ASSERT_FN(ret = co_await co::read(fd, &num, sizeof(num)))
//         DBG("events: %8x num: %8d errno: %4d ret: %4d", events, num, errno, ret);
//     }
// }

// int main() {
//     DBG_SCOPE();
//     FnScope scope;
//     co::sched_t sched;
//     int fds[2];
//     ASSERT_FN(pipe2(fds, O_DIRECT | O_CLOEXEC | O_NONBLOCK));
//     scope([&]{ close(fds[1]); close(fds[0]); });

//     ASSERT_FN(sched.init());
//     scope([&]{ sched.uninit(); });

//     ASSERT_FN(sched.add_tasks(
//         test_co_consumer(fd[0]),
//         test_co_producer(fd[1])
//     ));

//     ASSERT_FN(sched.add_event(fds[0], EPOLLIN, test_consumer, NULL));
//     ASSERT_FN(sched.add_event(fds[1], EPOLLOUT, test_producer, NULL));

//     ASSERT_FN(sched.run_sched());
// }

#define ASSERT_COFN(fn_call)\
if (intptr_t(fn_call) < 0) {\
    DBGE("FAILED: " #fn_call);\
    co_return -1;\
}

namespace co
{

static const char *co_str(std::coroutine_handle<> handle) {
    static std::map<void *, std::string> id_str;
    static uint64_t id = 0;
    if (!HAS(id_str, handle.address()))
        id_str[handle.address()] = sformat("co[%d]", id++);
    return id_str[handle.address()].c_str();
}

/* This is the interface that the expresion in co_await <expr> must adhere to */
struct awaiter_t {
    bool await_ready() noexcept;
    bool await_suspend(/* gets the co-handle of this co-fn */) noexcept;
    bool await_resume() noexcept;

private:
    /* awaiter stuff */
};


struct sched_base_t {
    /* all scheds need to be able to do this and if a task is apended to a scheduler, then it means
    it has a sched_base_t */

    virtual void add_virtual_fns() {}
};

template <typename T> struct task_t;

/* every coroutine that must switch to the caller will return this thing */
/* TODO: maybe this works only on co_return, test on co_yield? somehow? */
struct caller_info_t {
    using handle_v_t = std::coroutine_handle<>;

    bool await_ready() noexcept {
        DBG("Asked ready");
        return false;
    }
    handle_v_t await_suspend(handle_v_t oth) noexcept {
        DBG("<%s -> %s> returning", co_str(oth), co_str(caller_handle));
        if (!caller_handle)
             return std::noop_coroutine();
        return caller_handle;
    }
    void await_resume() noexcept {
        DBG("Shouldn't reach await_resume in caller_info_t?");
        DBG("Maybe for yields?")
        std::terminate();
    }

    handle_v_t caller_handle;
};

/* general promise template for tasks */
template <typename T>
struct promise_t {
    using handle_t = std::coroutine_handle<promise_t>;
    using handle_v_t = std::coroutine_handle<>;

    promise_t() : t(0) {}

    task_t<T> get_return_object() {
        return task_t<T>{handle_t::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
        DBG("Initial suspend <%s>", co_str(handle_t::from_promise(*this)));
        return {};
    }

    caller_info_t final_suspend() noexcept {
        DBG("Final suspend <%s>", co_str(handle_t::from_promise(*this)));
        if (caller_info.caller_handle && caller_info.caller_handle.done()) {
            DBG("This has no sense... So no one called us? What is this?");
            std::terminate();
        }
        return caller_info;
    }

    // void return_void();
    void return_value(const T& t) {
        DBG("return set");
        this->t = t;
    }

    void unhandled_exception() {
        DBG("Exceptions are not my coup of tea, if it goes it goes");
        std::terminate();
    } /* std::current_exception() to get exc */

    T t = 0;
    caller_info_t caller_info;
    sched_base_t *sched;
};

/* This is the container task for all our tasks, every function that is a coroutine will be
a task */
template <typename T>
struct task_t {
    using promise_type = promise_t<T>;
    using handle_t = std::coroutine_handle<promise_type>;
    using handle_v_t = std::coroutine_handle<>;

    task_t(handle_t handle) : handle(handle) {}
    ~task_t() {
        if (handle)
            handle.destroy();
    }

    T &val() {
        return handle.promise().t;
    }

    void resume() {
        DBG("<%s>", co_str(handle));
        handle.resume();
    }

    bool await_ready() {
        DBG("<%s>", co_str(handle));
        return false; /* will suspend the co-fn */
    }

    handle_t await_suspend(handle_v_t oth_handle) {
        DBG("<%s -> %s>", co_str(oth_handle), co_str(handle));
        DBG("set return of %s to %s", co_str(handle), co_str(oth_handle));
        /* return the handle that needs to replace this handle on this process */
        // this->oth_handle = oth_handle;
        handle.promise().caller_info.caller_handle = oth_handle;
        return handle;
    }

    T await_resume() {
        DBG("<%s> val: %d", co_str(handle), val());
        return val();
    }

private:
    // handle_t oth_handle;
    handle_t handle;
};

struct sched_epoll_t : public sched_base_t {
    using handle_t = std::coroutine_handle<promise_t<int>>;
    using handle_v_t = std::coroutine_handle<>;

    handle_v_t io_sched(int fd, handle_v_t handle) {
        /* sched the handle to be awakened when epoll event appears */
        /* return the next best coroutine to execute */

        /* TODO: if no events are active for any reason, just wait here till something happens */
        /* TODO: if the scheduler is constantly workloaded then ask epoll from time to time
        manually, in this way we will obtain the advantages of an busy loop while consuming only
        one thread. To achieve this give a epoll-retry-policy(default epoll is called after one
        run of all others, or more preciselly, epoll will be another coroutine that will yield
        once it is it's turn. If only that coroutine remains it will get out of the scheduler and
        busy wait, meaning that there is no cpu task to be computed for now)*/
    }

    int get_ret(int fd, handle_v_t handle) {
        /* given that the epoll had been awakened, the epoll result will be returned to the caller */
        /* TODO: maybe add 'events'? */
    }

    void update_fd(int fd, int ret, int _errno) {
        /* this is meant to store in the structure that holds fd the information that this fd
        returned X and has errno set on Y, such that subsequent continuations of this coroutine
        on other threads won't have an effect on the return value and eventual error */
    }
};

struct epoll_awaiter_t {
    /* This is the only promise type we accept inside the epoll_awaiter */
    using handle_t = std::coroutine_handle<promise_t<int>>;
    using handle_v_t = std::coroutine_handle<>;

    bool await_ready() {
        return false;
    }

    handle_v_t await_suspend(handle_t curr) {
        sched = dynamic_cast<sched_epoll_t *>(curr.promise().sched);
        handle = curr;
        auto next = sched->io_sched(fd, handle);
        return next;
    }

    int await_resume() {
        /* this coroutine can only suspend on call, so we know that the scheduler was asked to
        take controll of this coroutine and we know that 'sched' and 'handle' are set, as such
        we can request the return value for the fd we just queried */
        return sched->get_ret(fd, handle);
    }

    /* TODO: make sched pair as (fd, events) because we can listen to different events at once */
    int fd = -10;
    uint32_t events;
    int ret = -10;
    sched_epoll_t *sched;
    handle_v_t handle;
};

task_t<int> read(int fd, void *buff, uint64_t len) {
    /* we need the awaiter result further down the line, as such we are going to save the awaiter
    object.  */
    epoll_awaiter_t eawait{ .fd = fd, .events = EPOLLIN };

    /* In case epoll fails we will notify the function further of the failure via ASSERT_COFN */
    /* This will pop the coroutine from running and wait for an event in epoll */
    ASSERT_COFN(co_await eawait);

    /* Now we have the scheduler so we can give it the results of the read function, because the
    read may give an important error regarding that file descriptor or it can say EAGAIN, until
    EAGAIN is received there is no need for the scheduler to receive a signal from the epoll system,
    as such we will update it's state such that the scheduler can awaken us faster if there is
    no need to wait */
    int ret = ::read(fd, buff, len);
    eawait.sched->update_fd(fd, ret, errno);
    co_return ret;
}

task_t<int> write(int fd, const void *buff, uint64_t len) {
    epoll_awaiter_t eawait{ .fd = fd, .events = EPOLLIN };
    ASSERT_COFN(co_await eawait);

    int ret = ::write(fd, buff, len);
    eawait.sched->update_fd(fd, ret, errno);
    co_return ret;
}


task_t<int> return_an_int(int val) {
    int ret = val * 3;
    DBG("return_an_int: val: %d, ret: %d", val, ret);
    co_return ret;
}

task_t<int> return_an_int2(int val) {
    int ret = (co_await return_an_int(val)) * 3;
    DBG("return_an_int2: val: %d ret: %d", val, ret);
    co_return ret;
}

task_t<int> return_an_int3(int val) {
    int ret = (co_await return_an_int2(val)) * 3;
    DBG("return_an_int3: val: %d ret: %d", val, ret);
    co_return ret;
}

}


int main(int argc, char const *argv[]) {
    /* code */
    auto a = co::return_an_int3(1);
    auto b = co::return_an_int3(2);
    a.resume();
    b.resume();
    DBG("a.val(): %d", a.val());
    DBG("b.val(): %d", b.val());

    return 0;
}
