#ifndef PATH_UTILS_H
#define PATH_UTILS_H

#include <stdio.h>
#include <dlfcn.h>
#include <string>

inline std::string path_get_module_path() {
    Dl_info info;
    if (!dladdr((void *)&path_get_module_path, &info)) {
        return "";
    }
    return info.dli_fname;
}

inline std::string path_get_module_dir() {
    std::string str = path_get_module_path();
    std::size_t found = str.find_last_of("/\\");
    return str.substr(0, found + 1);
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

#endif
