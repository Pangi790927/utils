#ifndef DEMANGLE_H
#define DEMANGLE_H

#include <cstdlib>
#include <memory>
#include <cxxabi.h>

template <int indent = 0>
inline std::string demangle(const char *name) {
    int status = -4;
    std::unique_ptr<char, void (*)(void *)> res {
        abi::__cxa_demangle(name, NULL, NULL, &status),
        std::free
    };

    if (indent <= 0)
        return (status == 0) ? res.get() : name;

    std::string to_indent = (status == 0) ? res.get() : name;
    std::string indented = "";
    int indent_level = 0;
    bool was_nl = false;

    for (auto c : to_indent) {
        if (c == ' ' && was_nl)
            continue;
        was_nl = false;
        if (c == '<') {
            indent_level++;
        }
        if (c == '>') {
            indent_level--;
            indented += "\n" + std::string(indent_level * indent, ' ') + c;
            was_nl = true;
        }
        else {
            indented += c;
        }
        if (c == ',' || c == '<') {
            indented += "\n" + std::string(indent_level * indent, ' ');
            was_nl = true;
        }
    }
    return indented + "\n";
}

template <class T, int indent = 0>
inline std::string demangle() {
    return demangle<indent>(typeid(T).name());
}

template <class T, int indent = 0>
inline std::string demangle(const T& t) {
    return demangle<T, indent>();
}

template <bool B, typename T>
consteval void demangle_static_assert(const char *description) {
    if constexpr (!B)
        throw description; /* This throw forces the termination of compilation */
}


#endif
