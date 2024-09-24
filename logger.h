#ifndef LOGGER_H
#define LOGGER_H

#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <errno.h>
#include <string.h>
#include <atomic>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ <= 28
#include <sys/syscall.h>
#include <linux/fs.h>
#endif

#define LOGGER_DEFAULT_MAXSZ	(16*1024*1024)
#define LOGGER_DEFAULT_NAME		"./logfile"
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
	{
		std::lock_guard guard(_logger_data.logger_sl);

		if (_logger_data.is_init) {
			printf("Logger was initialized before, can't do that twice\n");
			return -1;
		}

		/* quick and dirty relative-path for linux (relative to the binary) */
		std::string logfile_relpath;
		if (logfile_path[0] != '/') {
			char exe_path[PATH_MAX] = {0};
		    if (!realpath("/proc/self/exe", exe_path)) {
		    	printf("Can't get proc path\n");
		    	return -1;
		    }
		    logfile_relpath = exe_path;
		    std::size_t found = logfile_relpath.find_last_of("/\\");
		    logfile_relpath = logfile_relpath.substr(0, found + 1) + "/" + logfile_path;
		}
		else {
			logfile_relpath = logfile_path;
		}

		_logger_data.active_file = std::string(logfile_relpath) + ".log";
		_logger_data.old_file = std::string(logfile_relpath) + ".old.log";
		_logger_data.maxsz = maxsz / 2; /* half for active and half for old */

		int flags = O_CREAT | O_RDWR | O_CLOEXEC;
		_logger_data.active_fd = open(_logger_data.active_file.c_str(), flags, perm);
		if (_logger_data.active_fd < 0) {
			printf("Couldn't open logfile, strerror[errno]: %s[%d]\n", strerror(errno), errno);
			return -1;
		}

		_logger_data.old_fd = open(_logger_data.old_file.c_str(), flags, perm);
		if (_logger_data.old_fd < 0) {
			printf("Couldn't open old logfile, strerror[errno]: %s[%d]\n", strerror(errno), errno);
			return -1;
		}

		_logger_data.curr_sz = lseek(_logger_data.active_fd, 0, SEEK_END);
		_logger_data.is_init = true;
	}

	std::string init_msg = "<<<< LOGGER INIT [" + logger_get_date() + "] >>>>\n";
	return logger_log(init_msg.c_str());
}

inline bool logger_is_init() {
	return _logger_data.is_init;
}

/* does this function have any use? */
inline void logger_uninit() {
	if (!_logger_data.is_init)
		return ;
	close(_logger_data.old_fd);
	close(_logger_data.active_fd);
	_logger_data.is_init = false;
}

#if __GLIBC__ == 2 && __GLIBC_MINOR__ <= 28

inline int renameat2(int olddirfd, const char *oldpath,
             int newdirfd, const char *newpath, unsigned int flags)
{
	return syscall(SYS_renameat2, olddirfd, oldpath, newdirfd, newpath, flags);
}

#endif

inline int logger_swap_files() {
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
}

inline int logger_log(const char *msg) {
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
}

inline void logger_log_autoinit(const char *msg) {
	if (!logger_is_init())
		logger_init();
	logger_log(msg);
}

#endif
