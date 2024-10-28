#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stdio.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <dirent.h>

/* path utils is a pre-debug header */

inline std::string path_pid_path(pid_t pid) {
    char path_buff[PATH_MAX] = {0};
    char proc_pid_path[64];
    snprintf(proc_pid_path, sizeof(proc_pid_path), "/proc/%d/exe", pid);
    if (!realpath(proc_pid_path, path_buff)) {
        return ""; /* it's clear that a path can't be empty, hence error */
    }
    return path_buff;
}

inline std::string path_pid_dir(pid_t pid) {
    std::string ppath_dir = path_pid_path(pid);
    std::size_t found = ppath_dir.find_last_of("/\\");
    ppath_dir = ppath_dir.substr(0, found + 1);
    return ppath_dir;
}

inline std::string path_get_module_path() {
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
}

inline std::string path_get_module_dir() {
    std::string str = path_get_module_path();
    std::size_t found = str.find_last_of("/\\");
    return str.substr(0, found + 1);
}

inline std::string path_get_abs(std::string path) {
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
}

inline std::string path_get_module_name() {
    std::string str = path_get_module_path();
    std::size_t found = str.find_last_of("/\\");
    return str.substr(found + 1);
}

inline std::string path_get_relative(std::string filename) {
    if (filename.size() == 0)
        return path_get_module_dir();
    if (filename[0] == '/')
        return filename;
    if (toupper(filename[0]) == 'C' && filename.size() > 1 && filename[1] == ':')
        return filename;
    return path_get_module_dir() + filename;
}

inline std::vector<std::string> list_dir(std::string dirname) {
    DIR* dir;
    struct dirent* ent;
    char* endptr;
    char buf[512];

    if (!(dir = opendir(dirname.c_str()))) {
        return {}; /* the directory has to have at least .. and . */
    }

    std::vector<std::string> ret;
    while((ent = readdir(dir)) != NULL) {
        ret.push_back(ent->d_name);
    }

    closedir(dir);
    return ret;
}

#endif
