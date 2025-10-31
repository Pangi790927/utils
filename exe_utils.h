#ifndef EXE_UTILS_H
#define EXE_UTILS_H

/* TODO: (WIP PROJECT)
check #include <elfutils/libdwfl.h>
check libdwarf
check DbgHelp.dll: API: SymFromAddr + SymGetLineFromAddr64

 */

/*! @file
 * This header file will help with programs formats and the sort, ie: finding an address line,
 * program path, module path, etc.
 * The objective is to have cpp_backtrace and path_utils both dependent on this one header file
 * and to get rid of them in the future. */

#if defined(__linux__)
# define EXE_UTILS_OS_LINUX
#endif /* CHK_IF_LINUX */

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN   // Exclude rarely-used Windows headers
#  define EXE_UTILS_UNDEF_WIN32_LEAN_AND_MEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX              // Prevent Windows from defining min/max macros
#  define EXE_UTILS_UNDEF_WIN32_NOMINMAX
# endif
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0501   // Windows XP
#  define EXE_UTILS_UNDEF_WIN32_WINNT
# endif
# include <windows.h>
# define EXE_UTILS_OS_WINDOWS
# ifdef EXE_UTILS_UNDEF_WIN32_LEAN_AND_MEAN
#  undef WIN32_LEAN_AND_MEAN
#  undef EXE_UTILS_UNDEF_WIN32_LEAN_AND_MEAN
# endif
# ifdef EXE_UTILS_UNDEF_WIN32_NOMINMAX
#  undef NOMINMAX
#  undef EXE_UTILS_UNDEF_WIN32_NOMINMAX
# endif
# ifdef EXE_UTILS_UNDEF_WIN32_WINNT
#  undef _WIN32_WINNT
#  undef EXE_UTILS_UNDEF_WIN32_WINNT
# endif
#endif /* CHK_IF_WINDOWS */

namespace exe_utils
{

/*! Get the current path */
inline std::string get_program_path();

/*! Get the specific pid path */
inline std::string get_pid_path(pid_t pid);

/*! Get the path of the current module */
inline std::string get_module_path();

/*! Get the line of code of the specific address from within this program */
inline uint32_t get_address_line(intptr_t addr);

/* IMPLEMENTATION
=================================================================================================
=================================================================================================
================================================================================================= */

} /* exe_utils */

#endif
