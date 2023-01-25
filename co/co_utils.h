#ifndef CO_UTILS_H
#define CO_UTILS_H

namespace co
{

/* The main objective of this lib is to use linux-based event handling with the newer c++20
coroutines. This means that I want the rest of the code to remain mostly the same. The async co::*
functions will be named the same as the posix functions of the form fn(fd, args...) and will
hopefully behave the same. Converting between the two, int fd and fd_t will be one to one.

There is one important difference that would make things faster: the file descriptor would be best
created in non-blocking mode and epoll should be used in a the edge-triggered mode not in the
level-triggered mode (see man page of epoll). Meaning that the file descriptor should be set in
non-blocking mode and the scheduler will remember that it has more data on a respective file
descriptor and unlock the read/write functions even after a wait was called, until EAGAIN is
returned. For this the fd_t struct will check if the descriptor is blocking and try to force it
in the non-blocking state unless the co::keep_original is used as an argument.

If the non-blocking mode is not used and co::keep_original is set then the file descriptor will be
used as is and the scheduler will always "forget" the descriptor, i.e. will epoll_wait on it each
time

W/O non-blocking file descriptors on each read the coroutine may be swapped out and another one
scheduled on the respective thread of execution, depending on the selected policy
(TODO: make those swap policies) */

/*
OBS: (TODO: check how it works) thread_local variables will break as different threads will be used
as continuations. */

/* This struct remembers the coroutine handle and the required data to reschedule the task on
co_await */
struct task_t {

};

/* This struct will be an async file descriptor, meaning that it will have the same props as the
usual integer file descriptor but it will be used with co::* functions instead to enable async
ops. The rest of the posix behavior should remain the same. Also if you know that a function will
not block or if you want to bottleneck the scheduling for some reason you can use the normal
functions on the fd_t::val() result. */
struct fd_t {

    int val();
};

/* This is the main scheduler class, it is taken as a parameter, directly or not, by all the objects
that can suspend execution and they give back execution to the scheduler. It(the scheduler) decides
who to call next. The scheduler exits when there is no task waiting and can be forcefully closed
given the right parameters. If configured it will schedule tasks on different threads. */
struct sched_t {

};

}

#endif
