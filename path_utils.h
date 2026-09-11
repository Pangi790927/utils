#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stdio.h>
#include <string>
#include <vector>

/* move this in the root-most file, as needed */
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# define UTILS_OS_WINDOWS
#elif defined(__linux__)
# define UTILS_OS_LINUX
#endif

#if defined(UTILS_OS_WINDOWS)
# include <winsock2.h>
# include <mswsock.h>
# include <windows.h>
# include <psapi.h>
# include <filesystem>
#elif defined(UTILS_OS_LINUX)
# include <dirent.h>
# include <dlfcn.h>
#endif

#include "misc_utils.h"

/* path utils is a pre-debug header */

inline std::string path_pid_path(int pid) {
#ifdef UTILS_OS_WINDOWS
    char path_buff[1024] = {0};
    HANDLE proc = OpenProcess(READ_CONTROL, FALSE, pid);
    if (!proc)
        return "";
    int ret = 0;
    if (!(ret = GetModuleFileNameExA(proc, NULL, path_buff, sizeof(path_buff)))) {
        CloseHandle(proc);
        return "";
    }
    CloseHandle(proc);
    return path_buff;
#elif defined(UTILS_OS_LINUX)
    char path_buff[PATH_MAX] = {0};
    char proc_pid_path[64];
    snprintf(proc_pid_path, sizeof(proc_pid_path), "/proc/%d/exe", pid);
    if (!realpath(proc_pid_path, path_buff)) {
        return ""; /* it's clear that a path can't be empty, hence error */
    }
    return path_buff;
#endif
}

inline std::string path_pid_dir(int pid) {
    std::string ppath_dir = path_pid_path(pid);
    std::size_t found = ppath_dir.find_last_of("/\\");
    ppath_dir = ppath_dir.substr(0, found + 1);
    return ppath_dir;
}

inline std::string path_get_module_path() {
#ifdef UTILS_OS_WINDOWS
    HMODULE mod = NULL;
    DWORD ret = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&path_get_module_path, &mod);
    if (ret == 0)
        mod = NULL;
    /* NOTHING IS CLOSED HERE, and both of the CloseHandle(mod) calls that used to be are gone.

    Two independent reasons, either one sufficient. An HMODULE is a module BASE ADDRESS, not a
    kernel handle - the call that releases one is FreeLibrary, and handing a base address to
    CloseHandle passes it something that was never a handle. And GET_MODULE_HANDLE_EX_FLAG_
    UNCHANGED_REFCOUNT, above, means no reference was taken in the first place: that flag exists
    precisely so the caller has nothing to release, by any call.

    WHY IT LOOKED HARMLESS FOR SO LONG: Windows only raises STATUS_INVALID_HANDLE for a bad close
    when a debugger is attached. Without one it is swallowed, so this fired on every run and was
    never seen. With one it stops the process before main() finishes - found 2026-09-11 while
    trying to attach cdb to math_writer, which could not get past startup until the exception was
    explicitly ignored. That is the real cost: it made the program undebuggable.
    @date 2026-09-11 15:10 */
    char path_buff[1024] = {0};
    if (!(ret = GetModuleFileNameExA(GetCurrentProcess(), mod, path_buff, sizeof(path_buff))))
        return "";
    return path_buff;
#elif defined(UTILS_OS_LINUX)
    std::string mod_path;
    Dl_info info;
    if (!dladdr((void *)&path_get_module_path, &info)) {
        mod_path = "";
    }
    mod_path = info.dli_fname;
    if (mod_path.size() && mod_path.ends_with(".so")) {
        return mod_path;
    }
    else {
        return path_pid_dir(getpid());
    }
#endif
}

inline std::string path_get_module_dir() {
    std::string str = path_get_module_path();
    std::size_t found = str.find_last_of("/\\");
    return str.substr(0, found + 1);
}

inline std::string path_get_abs(std::string path) {
#ifdef UTILS_OS_WINDOWS
    char path_buff[1024] = {0};
    char *path_ptr;
    DWORD len = GetFullPathNameA(path.c_str(), path.size() + 1, path_buff, &path_ptr);
    return path_ptr;
#elif defined(UTILS_OS_LINUX)
    char path_buff[PATH_MAX] = {0};
    if (path.size() && path[0] == '/')
        return path;
    if (!realpath(path.c_str(), path_buff)) {
        /* path probably doesn't yet exist, so we imagine it is relative to cwd */
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            return cwd + ("/" + path);
        return ""; /* it's clear that a path can't be empty, hence "" is for error */
    }
    return path_buff;
#endif
}

inline std::string path_get_name(const std::string& name) {
    std::string str = name;
    std::size_t found = str.find_last_of("/\\");
    return str.substr(found + 1);
}


inline std::string path_get_module_name() {
    return path_get_name(path_get_module_path());
}

inline std::string path_get_relative(std::string filename) {
    if (filename.size() == 0)
        return path_get_module_dir();
    if (filename[0] == '/')
        return filename;
    if (filename.size() > 1 && filename[1] == ':')
        return filename;
    return path_get_module_dir() + filename;
}

inline std::vector<std::string> list_dir(std::string dirname) {
#ifdef UTILS_OS_WINDOWS
    using namespace std::filesystem;
    std::vector<std::string> ret;
    for (const auto & entry : std::filesystem::directory_iterator(dirname)) {
        std::string path_string = entry.path().string();
        ret.push_back(path_string);
    }
    return ret;
#elif defined(UTILS_OS_LINUX)
    DIR* dir;
    struct dirent* ent;

    if (!(dir = opendir(dirname.c_str()))) {
        return {}; /* the directory has to have at least .. and . */
    }

    std::vector<std::string> ret;
    while((ent = readdir(dir)) != NULL) {
        ret.push_back(ent->d_name);
    }

    closedir(dir);
    return ret;
#endif
}

#endif
