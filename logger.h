#ifndef LOGGER_H
#define LOGGER_H

#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>

#define LOGGER_DEFAULT_MAXSZ	(16*1024*1024)
#define LOGGER_DEFAULT_NAME		"./logfile"
#define LOGGER_DEFAULT_PERM		0644

struct logger_data_t {
	std::string active_file;
	std::string old_file;
	uint64_t maxsz;
	uint64_t curr_sz;

	int active_fd;
	int old_fd;
};

inline int logger_init(const char *logfile_path = LOGGER_DEFAULT_NAME,
		uint64_t maxsz = LOGGER_DEFAULT_MAXSZ, int perm = LOGGER_DEFAULT_PERM);
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
	_logger_data.active_file = std::string(logfile_path) + ".log";
	_logger_data.old_file = std::string(logfile_path) + ".old.log";
	_logger_data.maxsz = maxsz / 2; /* half for active and half for old */

	_logger_data.active_fd = open(_logger_data.active_file.c_str(), O_CREAT | O_RDWR, perm);
	if (_logger_data.active_fd < 0) {
		printf("Couldn't open logfile, strerror[errno]: %s[%d]\n", strerror(errno), errno);
		return -1;
	}

	_logger_data.old_fd = open(_logger_data.old_file.c_str(), O_CREAT | O_RDWR, perm);
	if (_logger_data.old_fd < 0) {
		printf("Couldn't open old logfile, strerror[errno]: %s[%d]\n", strerror(errno), errno);
		return -1;
	}

	_logger_data.curr_sz = lseek(_logger_data.active_fd, 0, SEEK_END);

	std::string init_msg = "<<<< LOGGER INIT [" + logger_get_date() + "] >>>>\n";
	return logger_log(init_msg.c_str());
}

/* does this function have any use? */
inline void logger_uninit() {
	close(_logger_data.old_fd);
	close(_logger_data.active_fd);
}

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

	_logger_data.curr_sz = 0;

	return 0;
}

inline int logger_log(const char *msg) {
	/* if file would grow longer than maxsz the write will be done in  */
	uint32_t len = strlen(msg) + 1;
	if (len > _logger_data.maxsz) {
		/* TODO: print maxsz with platform agnostic printf */
		printf("Can't log more than allowed size: len[%u] maxsz[%ld]", len, _logger_data.maxsz);
		return -1;
	}

	if (len + _logger_data.curr_sz > _logger_data.maxsz)
		logger_swap_files();

	if (write(_logger_data.active_fd, msg, len) != len) {
		printf("Couldn't write msg in active file[%s] strerror[errno]: %s[%d]", msg,
				strerror(errno), errno);
		return -1;
	}
	return 0;
}

#endif
