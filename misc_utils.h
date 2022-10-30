#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <functional>

#define PACKED_STRUCT           __attribute__((packed))
#define HAS(data_struct, key)   (data_struct.find(key) != data_struct.end())
#define DIV_UP(x, d)            ({decltype(d) ds = (d); (x + d - 1) / d;})
#define ALIGN(x, a)             ({decltype(a) as = (a); DIV_UP(x, as) * as;})
#define ARR_SZ(a)               (sizeof((a)) / sizeof((a)[0]))

struct FnScope {
    using fn_t = std::function<void(void)>;
    std::vector<fn_t> fns;
    bool done = false;

    FnScope(fn_t fn) {
        add(fn);
    }

    FnScope() {}

    ~FnScope() {
        call();
    }

    void operator() (fn_t fn) {
        add(fn);
    }

    void add(fn_t fn) {
        fns.push_back(fn);
    }

    void disable() {
        fns.clear();
    }

    void call() {
        for (auto &f : fns)
            f();
        fns.clear();
    }
};

template <typename ...Args>
inline std::string sformat(const char *fmt, Args&&...args) {
    int cnt = snprintf(NULL, 0, fmt, args...);
    if (cnt <= 0)
        return "";
    std::vector<char> buff(cnt + 1);
    snprintf(buff.data(), buff.size(), fmt, args...);
    return buff.data();
}

inline std::string hexdump_str(void *ptr, int buflen) {
    std::string ret;
    std::string last_line;
    bool repeated = false;
    unsigned char *buf = (unsigned char*)ptr;
    int i;
    for (i = 0; i < buflen; i += 16) {
        std::string curr;
        for (int j = 0; j < 16; j++) 
            if (i + j < buflen)
                curr += sformat("%02x ", buf[i + j]);
            else
                curr += sformat("   ");
            curr += sformat(" ");
        for (int j = 0; j < 16; j++) 
            if (i + j < buflen)
                curr += sformat("%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        if (curr == last_line) {
            if (!repeated) {
                ret += sformat("%06x: ...\n", i);
                repeated = true;
            }
        }
        else {
            if (repeated)
                ret += sformat("%06x: ", i - 16) + last_line + "\n";
            ret += sformat("%06x: ", i) + curr;
            repeated = false;
            last_line = curr;
            ret += "\n";
        }
    }
    if (repeated)
        ret += sformat("%06x: ", i - 16) + last_line + "\n";
    return ret;
}

inline void hexdump(void *ptr, int buflen) {
    std::string to_print = hexdump_str(ptr, buflen);
    printf("%s", to_print.c_str());
}

inline uint32_t round_up(uint32_t to_div, uint32_t div) {
    return ((to_div + (div - 1)) / div) * div;
}

#endif