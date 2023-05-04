#ifndef MISC_UTILS_H
#define MISC_UTILS_H

#include <ctype.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <condition_variable>

#define PACKED_STRUCT           __attribute__((packed))
#define HAS(data_struct, key)   (data_struct.find(key) != data_struct.end())

#define DIV_UP(x, d)            ({decltype(d) ds = (d); (x + d - 1) / d;})
#define ALIGN(x, a)             ({decltype(a) as = (a); DIV_UP(x, as) * as;})

#define UP_ALIGN(nr, align)     ((nr + align - 1) / align * align)
#define DOWN_ALIGN(nr, align)   (nr / align * align)

#define ARR_SZ(a)               (sizeof((a)) / sizeof((a)[0]))

#define KNRM                "\x1B[0m"
#define KRED                "\x1B[31m"
#define KGRN                "\x1B[32m"
#define KYEL                "\x1B[33m"
#define KBLU                "\x1B[34m"
#define KMAG                "\x1B[35m"
#define KCYN                "\x1B[36m"
#define KWHT                "\x1B[37m"

struct c_free_deleter {
    template <typename T>
    void operator()(T *p) const {
        std::free(const_cast<std::remove_const_t<T>*>(p));
    }
};

template <typename T>
using unique_c_ptr = std::unique_ptr<T, c_free_deleter>;

struct Semaphore {
    int counter;
    std::mutex mu;
    std::condition_variable cv;

    Semaphore(int counter = 0) : counter(counter) {}

    void acquire() {
        std::unique_lock <std::mutex>lock(mu);
        cv.wait(lock, [&]{
            return counter > 0;
        });
        counter--;
    }

    void release() {
        std::lock_guard <std::mutex>guard(mu);
        counter++;
        cv.notify_all();
    }

    void sig() { release(); }
    void wait() { acquire(); }
};

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

template <typename Arg>
inline auto sformat_arg(const Arg& arg) {
    return arg;
}

inline const char *sformat_arg(const std::string& arg) {
    return arg.c_str();
}

template <typename ...Args>
inline std::string sformat(const char *fmt, Args&&...args) {
    auto wformat_err_bypass = &snprintf;
    int cnt = wformat_err_bypass(NULL, 0, fmt, sformat_arg(args)...);
    if (cnt <= 0)
        return "";
    std::vector<char> buff(cnt + 1);
    wformat_err_bypass(buff.data(), buff.size(), fmt, sformat_arg(args)...);
    return buff.data();
}

inline std::string str_replace(std::string src, const std::string& what,
        const std::string& with)
{
    std::string::size_type pos = 0;
    while ((pos = src.find(what, pos)) != std::string::npos) {
        src.replace(pos, what.size(), with);
        pos += with.size();
    }
    return src;
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

inline std::vector<std::string> ssplit(const std::string& src, const std::string& delim) {
    std::vector<std::string> ret;
    std::string copystr = src;
    while (true) {
        size_t pos = copystr.find(delim);
        std::string token = copystr.substr(0, pos);
        if (token != "")
            ret.push_back(token);
        if (pos == std::string::npos)
            break;
        copystr.erase(0, pos + delim.size());
    }
    return ret;
}


#endif