#ifndef AP_EXCEPT_H
#define AP_EXCEPT_H

#include "backtrace.h"

/* TODO: add more info to message exception, for example backtrace and other stuff like that */
struct ap_except_info_t {
	std::string bt;
};

inline ap_except_info_t *ap_except_info() {
	auto exc_inf = new ap_except_info_t{};
	exc_inf->bt = cpp_backtrace();
	return exc_inf;
}

inline const char *ap_except_info_str(ap_except_info_t *exc_inf) {
	return exc_inf->bt.c_str();
}

#define AP_EXC_HDR "[EXCEPTION:AP] "

/* in this case you must fill the ap_except_ptr before any code that can cause an exception is
called */
#ifdef AP_EXCEPT_CBK_PTR
#include "misc_utils.h"

using ap_except_cbk_t = void (*)(const char *errmsg, ap_except_info_t *);
inline ap_except_cbk_t ap_except_cbk;

#define AP_EXCEPT(fmt, ...) ap_except_cbk(sformat(AP_EXC_HDR fmt, ##__VA_ARGS__).c_str(), \
		ap_except_info());
#endif

/* in this case a cpp exception will be called on error */
#ifdef AP_EXCEPT_THROW
#include "misc_utils.h"
#include <stdexcept>

#define AP_EXCEPT(fmt, ...) throw std::runtime_error(\
		sformat(AP_EXC_HDR fmt " [einfo: %s]", ##__VA_ARGS__, ap_except_info_str(\
		ap_except_info())).c_str());

#endif

/* in this case the function ap_except_cbk must be implemented and it will be called */
#ifdef AP_EXCEPT_STATIC_CBK
#include "misc_utils.h"

extern void ap_except_cbk(const char *str, ap_except_info_t *exc_inf);
#define AP_EXCEPT(fmt, ...) ap_except_cbk(sformat(AP_EXC_HDR fmt, ##__VA_ARGS__).c_str(), \
		ap_except_info());

#endif

/* in this case the program will segfault intentionally after printing a message */
#ifdef AP_EXCEPT_SEGFAULT
#define AP_EXCEPT(fmt, ...) do {
	DBG(AP_EXC_HDR fmt, ##__VA_ARGS__);
	std::raise(SIGSEGV);
} while (0)
#endif


/* in this case no exception will be called(must be explicit about it) */
#ifdef AP_EXCEPT_NONE
#define AP_EXCEPT(fmt, ...)
#endif


#endif
