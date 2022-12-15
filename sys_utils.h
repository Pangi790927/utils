#ifndef SYS_UTILS_H
#define SYS_UTILS_H

#include <vector>
#include <sys/select.h>

#include "debug.h"

struct select_wrap_ret_t {
	int res = -1;
	std::vector<int> in_fds;
	std::vector<int> out_fds;
	std::vector<int> exc_fds;

	bool in() { return in_fds.size() != 0; }
	bool out() { return out_fds.size() != 0; }
	bool exc() { return exc_fds.size() != 0; }
};

template <typename IterIn, typename IterOut, typename IterExc>
inline select_wrap_ret_t select_wrap(IterIn&& in_fds, IterOut&& out_fds,
		IterExc&& exc_fds, uint64_t timeout_us = -1)
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
#elif defined(LINUX_BUILD)
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
#elif defined(LINUX_BUILD)
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


#endif