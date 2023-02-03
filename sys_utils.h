#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include <vector>
#include <fcntl.h>
#include <sys/select.h>
#include <systemd/sd-bus.h>

#include "debug.h"
#include "misc_utils.h"

struct select_wrap_ret_t {
    int res = -1;
    std::vector<int> in_fds;
    std::vector<int> out_fds;
    std::vector<int> exc_fds;

    bool in() { return in_fds.size() != 0; }
    bool out() { return out_fds.size() != 0; }
    bool exc() { return exc_fds.size() != 0; }
};

template <typename InV, typename OutV, typename ExcV>
inline select_wrap_ret_t select_wrap(InV&& in_fds, OutV&& out_fds, ExcV&& exc_fds,
        uint64_t timeout_us = -1);

inline int read_sz(int fd, void *dst, size_t len);
inline int write_sz(int fd, const void *src, size_t len);

inline int systemctl_start(const char *service_name);
inline int systemctl_stop(const char *service_name);
inline int systemctl_restart(const char *service_name);
inline int systemctl_disable(const char *service_name);
inline int systemctl_enable(const char *service_name);
inline int systemctl_create_service(const char *service_name, const char *target_exec,
        bool replace = false);

inline sockaddr_in create_sa_ipv4(std::string addr, uint16_t port);
inline sockaddr_in create_sa_ipv4(uint32_t addr, uint16_t port);

/* IMPLEMENTATION:
================================================================================================= */

template <typename IterIn, typename IterOut, typename IterExc>
inline select_wrap_ret_t select_wrap(IterIn&& in_fds, IterOut&& out_fds,
        IterExc&& exc_fds, uint64_t timeout_us)
{
    struct timeval timeout = {0};
    fd_set in_set;
    fd_set out_set;
    fd_set exc_set;

    timeout.tv_usec = timeout_us % 1000'000;
    timeout.tv_sec  = timeout_us / 1000'000;

    FD_ZERO(&in_set);
    FD_ZERO(&out_set);
    FD_ZERO(&exc_set);

    int nfds = -2;
    for (auto &&fd : in_fds) {
        FD_SET(fd, &in_set);
        nfds = std::max(nfds, fd);
    }

    for (auto &&fd : out_fds) {
        FD_SET(fd, &out_set);
        nfds = std::max(nfds, fd);
    }

    for (auto &&fd : exc_fds) {
        FD_SET(fd, &exc_set);
        nfds = std::max(nfds, fd);
    }

    nfds++;

    if (nfds > 1024) {
        DBG("Resulting nfds must be smaller than 1024");
        return select_wrap_ret_t{ .res = -1 };
    }

    int res = select(
        nfds,
        in_fds.size() ? &in_set : NULL,
        out_fds.size() ? &out_set : NULL,
        exc_fds.size() ? &exc_set : NULL,
        timeout_us == uint64_t(-1) ? NULL : &timeout
    );

    select_wrap_ret_t ret = { .res = res };
    if (res < 0) {
        return ret;
    }

    for (auto &&fd : in_fds) {
        if (FD_ISSET(fd, &in_set))
            ret.in_fds.push_back(fd);
    }

    for (auto &&fd : out_fds) {
        if (FD_ISSET(fd, &out_set))
            ret.out_fds.push_back(fd);
    }

    for (auto &&fd : exc_fds) {
        if (FD_ISSET(fd, &exc_set))
            ret.exc_fds.push_back(fd);
    }
    return ret;
}


inline int read_sz(int fd, void *dst, size_t len) {
    /* reads the exact len into buffer */
    auto buff = (char *)dst;
    while (true) {
#ifdef WINDOWS_BUILD
        int sent = _read(fd, buff, (int)len);
#else
        int sent = read(fd, buff, (int)len);
#endif
        if (sent < 0) {
            DBGE("Failed to read data: %d", sent);
            return -1;
        }
        if (sent == 0) {
            DBG("Connection was closed");
            return -1;
        }
        len -= sent;
        buff += sent;
        if (len == 0)
            return 0;
    }
}

inline int write_sz(int fd, const void *src, size_t len) {
    /* writes the exact len into buffer */
    auto buff = (const char *)src;
    while (true) {
#ifdef WINDOWS_BUILD
        int sent = _write(fd, buff, (int)len);
#else
        int sent = write(fd, buff, (int)len);
#endif
        if (sent < 0) {
            DBGE("Failed to write data: %d", sent);
            return -1;
        }
        if (sent == 0) {
            DBG("Connection was closed");
            return -1;
        }
        len -= sent;
        buff += sent;
        if (len == 0)
            return 0;
    }
}

template <typename ...Args>
inline int systemctl_call(const char *method_name, const char *method_layout,
        bool en_output, Args ...args)
{
    sd_bus *dbus;
    sd_bus_error err = SD_BUS_ERROR_NULL;
    sd_bus_message* msg = NULL;

    ASSERT_FN(sd_bus_default_system(&dbus));

    FnScope scope([&]{
        sd_bus_error_free(&err);
        sd_bus_message_unref(msg);
    });
    FnScope err_scope([&]{
        DBG("Errored out for: method[%s], err[%s]",
                method_name, err.message);
    });
    ASSERT_FN(sd_bus_call_method(dbus,
        "org.freedesktop.systemd1",
        "/org/freedesktop/systemd1",
        "org.freedesktop.systemd1.Manager",
        method_name,
        &err,
        &msg,
        method_layout,
        args...
    ));

    if (en_output) {
        char *rsp = NULL;
        ASSERT_FN(sd_bus_message_read(msg, "o", &rsp));
        if (rsp) {
            DBG("from systemd:\n%s", rsp);
        }
    }

    err_scope.disable();
    sd_bus_unref(dbus);
    return 0;
}

inline int systemctl_start(const char *service_name) {
    /* ss comes from string-string */
    ASSERT_FN(systemctl_call("StartUnit", "ss", true, service_name, "replace"));
    return 0;
}

inline int systemctl_stop(const char *service_name) {
    ASSERT_FN(systemctl_call("StopUnit", "ss", true, service_name, "replace"));
    return 0;
}

inline int systemctl_restart(const char *service_name) {
    ASSERT_FN(systemctl_call("RestartUnit", "ss", true, service_name, "replace"));
    return 0;
}

inline int systemctl_enable(const char *service_name) {
    /* a-??? s-string b-bool b-bool */
    ASSERT_FN(systemctl_call("EnableUnitFiles", "asbb", false, 1, service_name, false, true));
    return 0;
}

inline int systemctl_disable(const char *service_name) {
    /* a-??? s-string b-bool */
    ASSERT_FN(systemctl_call("DisableUnitFiles", "asb", false, 1, service_name, false));
    return 0;
}

inline int systemctl_create_service(const char *service_name, const char *target_exec,
        bool replace)
{
    std::string service_desc = sformat(
        "[Unit]\n"
        "Description=The description of the service...\n"
        "After=network.target\n"
        "StartLimitIntervalSec=0\n"
        "\n"
        "[Service]\n"
        "Type=simple\n"
        "Restart=always\n"
        "RestartSec=1\n"
        "User=root\n"
        "ExecStart=%s\n"
        "StandardOutput=journal\n"
        "StandardError=journal\n"
        "\n"
        "[Install]\n"
        "WantedBy=multi-user.target\n",
        target_exec
    );

    std::string file_path = sformat("/etc/systemd/system/%s", service_name);

    int fd;
    ASSERT_FN(fd = open(file_path.c_str(), (replace ? 0 : O_EXCL) | O_CREAT | O_WRONLY, 0644));
    FnScope scope([&]{ close(fd); });

    ASSERT_FN(write(fd, service_desc.c_str(), service_desc.size()));
    return 0;
}

inline sockaddr_in create_sa_ipv4(uint32_t addr, uint16_t port) {
    sockaddr_in sa_addr = {};
    sa_addr.sin_family = AF_INET;
    sa_addr.sin_addr.s_addr = INADDR_ANY;
    sa_addr.sin_port = htons(SERVER_PORT);
    return sa_addr;
}

inline sockaddr_in create_sa_ipv4(std::string addr, uint16_t port) {
    return create_sa_ipv4(inet_addr(addr.c_str()), port);
}


#endif
