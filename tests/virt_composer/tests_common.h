#ifndef TESTS_COMMON_H
#define TESTS_COMMON_H

/* windows.h (pulled in transitively through colib.h/co_utils.h) defines min/max macros that break
any later `std::numeric_limits<T>::max()` call (see yaml.h) unless NOMINMAX is defined before the
first windows.h include anywhere in the translation unit - this must be the very first thing in
every test file, which is why tests_common.h (included first, per this directory's convention) is
also the thing that pulls in virt_composer.h itself. */
#ifdef _MSC_VER
# define NOMINMAX
#endif

#include "../../virt_composer.h"

#include <string.h>
#include <fstream>
#include <iostream>

/* DBG/ASSERT_FN/ASSERT_COFN/CHK_BOOL/CHK_PTR already come transitively from debug.h/co_utils.h
(via virt_composer.h) - no need to redefine them here. */

namespace vc = virt_composer;

/* Test result output with colors, mirrors co-lib/tests/tests_common.h's print_test_result. */
inline void print_test_result(const char* filename, bool passed) {
#if defined(_WIN32)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (passed) {
        SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        std::cout << "[PASSED]: " << filename << std::endl;
    } else {
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
        std::cout << "[FAILED]: " << filename << std::endl;
    }
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    if (passed) {
        std::cout << "\033[32m[PASSED]: " << filename << "\033[0m" << std::endl;
    } else {
        std::cout << "\033[31m[FAILED]: " << filename << "\033[0m" << std::endl;
    }
#endif
}

/*! parse_config() only reads from a file path (fkyaml::node::deserialize(ifstream)), there is no
in-memory-string overload - so every test that wants an inline YAML config has to spill it to a
file first. Each test writes its own '<test-file-basename>.yaml' next to the binary (see the
per-test .gitignore rule for '*.tmp.yaml') so parallel test runs never collide on a shared name. */
inline std::string write_temp_yaml(const std::string& name, const std::string& content) {
    std::string path = name + ".tmp.yaml";
    std::ofstream f(path, std::ios::trunc);
    f << content;
    f.close();
    return path;
}

#endif /* TESTS_COMMON_H */
