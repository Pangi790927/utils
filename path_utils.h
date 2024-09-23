#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stdio.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <dirent.h>

inline std::string path_get_module_path() {
    // Dl_info info;
    // if (!dladdr((void *)&path_get_module_path, &info)) {
    //     return "";
    // }
    // return info.dli_fname;
    char path_buff[PATH_MAX] = {0};
    if (!realpath("/proc/self/exe", path_buff))
        return "[path_error]";
    return path_buff;
}

inline std::string path_get_module_dir() {
    std::string str = path_get_module_path();
    std::size_t found = str.find_last_of("/\\");
    return str.substr(0, found + 1);
}

inline std::string path_get_abs(std::string path) {
    char path_buff[PATH_MAX] = {0};
    if (!realpath(path.c_str(), path_buff))
        return "[path_error]";
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
        DBGE("can't open %s", dirname.c_str());
        return {};
    }

    std::vector<std::string> ret;
    while((ent = readdir(dir)) != NULL) {
        ret.push_back(ent->d_name);
    }

    closedir(dir);
    return ret;
}

#endif
