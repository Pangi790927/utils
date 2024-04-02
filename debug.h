#ifndef DEBUG_H
#define DEBUG_H

#include "errnoname.h"
#include "logger.h"

#include <errno.h>
#include <string.h>
#include <chrono>
#include <string>
#include <vector>
#include <inttypes.h>

#define LOGGER_BUFF_SIZE 1024

/* or-ed with the values bellow */
#ifndef LOGGER_VERBOSE_LVL
#define LOGGER_VERBOSE_LVL 0
#endif

/* Additional info, but kept short, can still be on in product (example: initialization stages) */
#ifndef LOGGER_VERBOSE_1_VAL
#define LOGGER_VERBOSE_1_VAL false
#endif

/* A lot of additional info (example: at initialization, the contents of tables, pointers, etc.)*/
#ifndef LOGGER_VERBOSE_2_VAL
#define LOGGER_VERBOSE_2_VAL false
#endif

/* Logs even at runtime, so it will print lots of logs (example: inside draw loops) */
#ifndef LOGGER_VERBOSE_3_VAL
#define LOGGER_VERBOSE_3_VAL false
#endif

inline void logger_log_message(const char *msg) {
    printf("%s", msg);
    logger_log_autoinit(msg);
}

inline uint64_t logger_get_time() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

#define DBG_RAW(fmt, dbg_filename_, dbg_line_, dbg_funcname_, ...)                                 \
([&](const char *dbg_filename, int dbg_line, const char *dbg_funcname) {                           \
    std::vector<char> logger_buff;                                                                 \
    uint64_t time_ms = logger_get_time();                                                          \
    logger_buff.resize(snprintf(NULL, 0, "[%" PRIu64 "] %s:%d %s() :> " fmt "\n",                  \
            time_ms, dbg_filename, dbg_line, dbg_funcname, ##__VA_ARGS__) + 1);                    \
    snprintf(logger_buff.data(), logger_buff.size(), "[%" PRIu64 "] %s:%d %s() :> " fmt "\n",      \
            time_ms, dbg_filename, dbg_line, dbg_funcname, ##__VA_ARGS__);                         \
    logger_log_message(logger_buff.data());                                                        \
}(dbg_filename_, dbg_line_, dbg_funcname_));

#define DBG(fmt, ...) DBG_RAW(fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#define DBGV(fmt, ...)   \
do { \
    if ((LOGGER_VERBOSE_1_VAL) || ((LOGGER_VERBOSE_LVL) >= 1)) { \
        DBG("[V  ] " fmt, ##__VA_ARGS__); \
    } \
} while (0)

#define DBGVV(fmt, ...)  \
do { \
    if ((LOGGER_VERBOSE_2_VAL) || ((LOGGER_VERBOSE_LVL) >= 2)) { \
        DBG("[VV ] " fmt, ##__VA_ARGS__); \
    } \
} while (0)

#define DBGVVV(fmt, ...) \
do { \
    if ((LOGGER_VERBOSE_3_VAL) || ((LOGGER_VERBOSE_LVL) >= 3)) { \
        DBG("[VVV] " fmt, ##__VA_ARGS__); \
    } \
} while (0)


#define DBGE(fmt, ...)\
    DBG("[SYS] " fmt "[err: %s [%s], code: %d]", ##__VA_ARGS__,\
            strerror(errno), errnoname(errno), errno)

#define ASSERT_FN(fn_call)\
if (intptr_t(fn_call) < 0) {\
    DBGE("FAILED: " #fn_call);\
    return -1;\
}

// #define ASSERT_PTR(fn_call)                                                                        \
// if (!(fn_call)) {                                                                                  \
//     DBGE("FAILED: " #fn_call);                                                                     \
//     return -1;                                                                                     \
// }

#define CHK_MMAP(x) ((x) == MAP_FAILED ? -1 : 0)
#define CHK_BOOL(x) ((x) ? 0 : -1)
#define CHK_PTR(x)  ((x) == NULL ? -1 : 0)

struct DbgScope {
    std::string file;
    int line;
    std::string func;
    DbgScope(std::string file, int line, std::string func) : file(file), line(line), func(func) {
        DBG_RAW(" --> ", file.c_str(), line, func.c_str());
    }
    ~DbgScope() {
        DBG_RAW(" <-- ", file.c_str(), line, func.c_str());
    }
};

#define DBG_SCOPE() DbgScope debug_scope_var(__FILE__, __LINE__, __func__);

#endif
