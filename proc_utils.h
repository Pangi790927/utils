#ifndef PROC_UTILS_H
#define PROC_UTILS_H

/* Child processes and signals on a colib pool. */

#include <spawn.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <vector>
#include "co_utils.h"
#include "misc_utils.h"

#ifndef SYS_pidfd_open
# define SYS_pidfd_open 434     /* same number on all architectures, Linux >= 5.3 */
#endif

extern char **environ;

/* Starts `args[0]` (searched in PATH if it has no '/') with the arguments `args`. The child starts
with an empty signal mask, because the signals handled with sig_open are blocked here and the mask
is inherited: without this, the child would ignore SIGTERM. SIGPIPE is set back to its default
for the same reason (a TCP server here usually ignores it). Returns the pid of the child. */
inline pid_t proc_spawn(const std::vector<std::string>& args);

/* Awaits the end of the child `pid` and reaps it, `status` is as in waitpid */
inline co::task_t proc_wait(pid_t pid, int *status);

/* Blocks `signals` and returns a signalfd on which they can be awaited with sig_wait */
inline int sig_open(const std::vector<int>& signals);

/* Awaits one of the signals of sig_open, `signo` is the received signal */
inline co::task_t sig_wait(int sig_fd, int *signo);

/* IMPLEMENTATION
=================================================================================================
=================================================================================================
================================================================================================= */

inline pid_t proc_spawn(const std::vector<std::string>& args) {
    ASSERT_FN(CHK_BOOL(args.size() > 0));

    std::vector<char *> argv;
    for (auto &arg : args)
        argv.push_back((char *)arg.c_str());
    argv.push_back(nullptr);

    sigset_t empty_mask;
    sigemptyset(&empty_mask);

    sigset_t default_sigs;
    sigemptyset(&default_sigs);
    sigaddset(&default_sigs, SIGPIPE);

    /* the posix_spawn functions return the error number instead of setting errno */
    posix_spawnattr_t attr;
    ASSERT_FN(CHK_BOOL(posix_spawnattr_init(&attr) == 0));
    FnScope scope([&]{ posix_spawnattr_destroy(&attr); });

    ASSERT_FN(CHK_BOOL(posix_spawnattr_setsigmask(&attr, &empty_mask) == 0));
    ASSERT_FN(CHK_BOOL(posix_spawnattr_setsigdefault(&attr, &default_sigs) == 0));
    ASSERT_FN(CHK_BOOL(posix_spawnattr_setflags(&attr,
            POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF) == 0));

    pid_t pid;
    int err = posix_spawnp(&pid, argv[0], nullptr, &attr, argv.data(), environ);
    if (err != 0) {
        DBG("Failed to start %s: %s", argv[0], strerror(err));
        return -1;
    }
    return pid;
}

inline co::task_t proc_wait(pid_t pid, int *status) {
    /* the pidfd becomes readable when the child ends, it doesn't reap it */
    int pid_fd;
    ASSERT_COFN(pid_fd = syscall(SYS_pidfd_open, pid, 0));
    FnScope scope([&]{ close(pid_fd); });

    ASSERT_COFN(co_await co::wait_event(co::io_desc_t{ .fd = pid_fd, .events = EPOLLIN }));
    ASSERT_COFN(waitpid(pid, status, 0));
    co_return 0;
}

inline int sig_open(const std::vector<int>& signals) {
    sigset_t mask;
    sigemptyset(&mask);
    for (int signo : signals)
        sigaddset(&mask, signo);

    /* blocked, so that they are only delivered through the signalfd */
    ASSERT_FN(sigprocmask(SIG_BLOCK, &mask, nullptr));

    int sig_fd;
    ASSERT_FN(sig_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC));
    return sig_fd;
}

inline co::task_t sig_wait(int sig_fd, int *signo) {
    signalfd_siginfo info;
    ssize_t n;
    ASSERT_COFN(n = co_await co::read(sig_fd, &info, sizeof(info)));
    ASSERT_COFN(CHK_BOOL(n == sizeof(info)));

    *signo = info.ssi_signo;
    co_return 0;
}

#endif
