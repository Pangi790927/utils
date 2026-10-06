#ifndef OS_COMPILE_UTILS_H
#define OS_COMPILE_UTILS_H

/*! @file
 * Says which system a file is compiled for, and brings the system headers every other header of
 * utils would otherwise include in its own order.
 *
 * Core:
 *   - `UTILS_OS_WINDOWS` or `UTILS_OS_LINUX` is defined, the one a file asks with `#ifdef`.
 *   - On Windows, `NOMINMAX` is defined before windows.h is read: its min and max macros break
 *     `std::numeric_limits<T>::max()` in every file after it, yaml.h's among them.
 *   - On Windows, winsock2.h comes before windows.h, which would otherwise pull in the old
 *     winsock.h and clash with it.
 *
 * Detail:
 *   - It includes no other header of utils, so any of them may include it first. path_utils.h
 *     does, and debug.h through it, so a file that includes debug.h has all of this.
 *
 * @date 28-09-2026-10:30 */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# define UTILS_OS_WINDOWS
#elif defined(__linux__)
# define UTILS_OS_LINUX
#endif

#if defined(UTILS_OS_WINDOWS)
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
# include <ws2tcpip.h>
# include <mswsock.h>
# include <windows.h>
# include <psapi.h>
# include <io.h>
#endif

/* MSVC's C runtime has no ssize_t, the type POSIX reads and writes answer in. 28-09-2026-13:00 */
#if defined(_MSC_VER)
# include <cstddef>
using ssize_t = ptrdiff_t;
#endif

#endif /* OS_COMPILE_UTILS_H */
