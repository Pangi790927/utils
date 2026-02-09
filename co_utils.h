#ifndef CO_UTILS_H
#define CO_UTILS_H

#define COLIB_LOG_FUNCTION \
    [](const colib::dbg_string_t& msg){ logger_log_message(msg.c_str()); return 0; }

#include "debug.h"
#include "colib.h"

#define CO_REG(x) COLIB_REGNAME(x)

/* Assert Corutine Function */
#define ASSERT_COFN(fn_call) do { \
    int res = (int)(fn_call); \
    if (res < 0) { \
        DBG("Assert[%s] FAILED[ret: %d, errno: %d[%s]]", #fn_call, res, errno, strerror(errno)); \
        co_return res; \
    } \
} while (0)

#define ASSERT_ECOFN(fn_call) do { \
    if (intptr_t(fn_call) < 0) { \
        DBGE("FAILED: " #fn_call); \
        co_await co::force_stop(-1); \
        co_return -1; \
    } \
} while (0)

namespace co = colib; 

#endif
