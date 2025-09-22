#ifndef LOGGER_H
#define LOGGER_H

#include <sstream>
#include <iomanip>
#include <mutex>
#include <errno.h>
#include <string.h>
#include <atomic>

#include "path_utils.h"

#ifdef UTILS_OS_WINDOWS
#elif defined(UTILS_OS_LINUX)
# include <fcntl.h>
# include <unistd.h>
# if __GLIBC__ == 2 && __GLIBC_MINOR__ <= 28
#  include <sys/syscall.h>
#  include <linux/fs.h>
# endif
#endif

#define LOGGER_DEFAULT_MAXSZ	(16*1024*1024)

#ifndef LOGGER_DEFAULT_NAME
#define LOGGER_DEFAULT_NAME		"./logfile"
#endif /* LOGGER_DEFAULT_NAME */

#define LOGGER_DEFAULT_PERM		0666

struct logger_spinlock_t {
	std::atomic_flag locked = ATOMIC_FLAG_INIT;
public:
	void lock() { while (locked.test_and_set(std::memory_order_acquire)) { ; } }
	void unlock() { locked.clear(std::memory_order_release); }
};

struct logger_data_t {
	logger_spinlock_t logger_sl;

	std::string active_file;
	std::string old_file;
	uint64_t maxsz;
	uint64_t curr_sz;

	int active_fd;
	int old_fd;

	bool is_init = false;
};

inline int logger_init(const char *logfile_path = LOGGER_DEFAULT_NAME,
		uint64_t maxsz = LOGGER_DEFAULT_MAXSZ, int perm = LOGGER_DEFAULT_PERM);
inline bool logger_is_init();
inline void logger_uninit();

inline int logger_log(const char *log_line);

inline logger_data_t _logger_data;

/* IMPLEMENTATION:
================================================================================================= */

inline std::string logger_get_date() {
	auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    auto str = oss.str();

    return str;
}

inline int logger_init(const char *logfile_path, uint64_t maxsz, int perm) {
#ifdef UTILS_OS_WINDOWS
	/* TODO: */
	return 0;
#elif defined(UTILS_OS_LINUX)
	{
		std::lock_guard guard(_logger_data.logger_sl);

		if (_logger_data.is_init) {
			printf("Logger was initialized before, can't do that twice\n");
			return -1;
		}

		std::string logfile_relpath = path_get_relative(logfile_path);

		_logger_data.active_file = std::string(logfile_relpath) + ".log";
		_logger_data.old_file = std::string(logfile_relpath) + ".old.log";
		_logger_data.maxsz = maxsz / 2; /* half for active and half for old */

		int flags = O_CREAT | O_RDWR | O_CLOEXEC | O_TRUNC;
		_logger_data.active_fd = open(_logger_data.active_file.c_str(), flags, perm);
		if (_logger_data.active_fd < 0) {
			printf("Couldn't open logfile[%s], strerror[errno]: %s[%d]\n",
					_logger_data.active_file.c_str(), strerror(errno), errno);
			return -1;
		}

		_logger_data.old_fd = open(_logger_data.old_file.c_str(), flags, perm);
		if (_logger_data.old_fd < 0) {
			printf("Couldn't open old logfile[%s], strerror[errno]: %s[%d]\n",
					_logger_data.old_file.c_str(), strerror(errno), errno);
			return -1;
		}

		_logger_data.curr_sz = lseek(_logger_data.active_fd, 0, SEEK_END);
		_logger_data.is_init = true;
	}

	std::string init_msg = "<<<< LOGGER INIT [" + logger_get_date() + "] >>>>\n";
	return logger_log(init_msg.c_str());
#endif
}

inline bool logger_is_init() {
	return _logger_data.is_init;
}

/* does this function have any use? */
inline void logger_uninit() {
#ifdef UTILS_OS_WINDOWS
	/* TODO: */
#elif defined(UTILS_OS_LINUX)
	if (!_logger_data.is_init)
		return ;
	close(_logger_data.old_fd);
	close(_logger_data.active_fd);
	_logger_data.is_init = false;
#endif
}

#ifdef UTILS_OS_LINUX
# if __GLIBC__ == 2 && __GLIBC_MINOR__ <= 28

inline int renameat2(int olddirfd, const char *oldpath,
             int newdirfd, const char *newpath, unsigned int flags)
{
	return syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}

# endif
#endif

inline int logger_swap_files() {
#ifdef UTILS_OS_WINDOWS
	/* TODO: */
	return 0;
#elif defined(UTILS_OS_LINUX)
	/* atomically moves active file to old file */
	int ret = renameat2(AT_FDCWD, _logger_data.active_file.c_str(),
			AT_FDCWD, _logger_data.old_file.c_str(), RENAME_EXCHANGE);

	if (ret < 0) {
		printf("Couldn't exchange old and active files, strerror[errno]: %s[%d]\n",
				strerror(errno), errno);
		return -1;
	}

	std::swap(_logger_data.old_fd, _logger_data.active_fd);
	if (ftruncate(_logger_data.active_fd, 0) < 0) {
		printf("Couldn't truncate active file to 0, strerror[errno]: %s[%d]",
				strerror(errno), errno);
		return -1;
	}
	lseek(_logger_data.active_fd, 0, SEEK_SET);

	_logger_data.curr_sz = 0;

	return 0;
#endif
}

inline int logger_log(const char *msg) {
#ifdef UTILS_OS_WINDOWS
	/* TODO: */
	return 0;
#elif defined(UTILS_OS_LINUX)
	/* if file would grow longer than maxsz the write will be done in  */
	uint32_t len = strlen(msg);
	if (len > _logger_data.maxsz) {
		/* TODO: print maxsz with platform agnostic printf */
		printf("Can't log more than allowed size: len[%u] maxsz[%ld]", len, _logger_data.maxsz);
		return -1;
	}

	int active_fd = 0;
	{
		std::lock_guard guard(_logger_data.logger_sl);
		if (len + _logger_data.curr_sz > _logger_data.maxsz)
			logger_swap_files();
		else
			_logger_data.curr_sz += len;
		active_fd = _logger_data.active_fd;
	}

	if (write(active_fd, msg, len) != len) {
		printf("Couldn't write msg in active file[%s] strerror[errno]: %s[%d]", msg,
				strerror(errno), errno);
		return -1;
	}
	return 0;
#endif
}

inline void logger_log_autoinit(const char *msg) {
	if (!logger_is_init())
		logger_init();
	logger_log(msg);
}

#endif
