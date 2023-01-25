#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <cstdlib>
#include <memory>
#include <cxxabi.h>

inline std::string demangle(const char *name) {
    int status = -4;
    std::unique_ptr<char, void (*)(void *)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    return (status == 0) ? res.get() : name;
}

template <class T>
inline std::string demangle() {
    return demangle(typeid(T).name());
}

template <class T>
inline std::string demangle(const T& t) {
    return demangle<T>();
}


#endif