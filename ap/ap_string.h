#ifndef AP_STRING_H
#define AP_STRING_H

#include "ap_vector.h"

#ifdef AP_ENABLE_AUTOINIT
extern ap_ctx_t *ap_static_ctx;
#endif

struct ap_string_t {
    /* Short string optimizations are a bit of a pain because I would need to implement both the
    sso and the lso or to hack the vector class and put the sso inside it, avoiding it's id_ctx and
    I don't feel like doing any of that */
    ap_vector_t<char> vec;

    friend ap_vector_t<char>;
    using iterator_t = ap_vector_t<char>::iterator_t;

#ifdef AP_ENABLE_AUTOINIT
    ap_string_t() {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
    }
    ap_string_t(const std::string& str) {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
        append(str);
    }
    ap_string_t(const char *str) {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
        append(str);
    }
#endif

#ifdef AP_ENABLE_AUTOINIT
private:
#endif
    int init(ap_ctx_t *ctx) {
#ifndef AP_ENABLE_AUTOINIT
        ASSERT_FN(vec.init(ctx));
#endif
        vec.push_back('\0');
        return 0;
    }

    void uninit() {
        vec.uninit();
    }
#ifdef AP_ENABLE_AUTOINIT
public:
#endif

    ap_string_t &append(const std::string& s) {
        vec.insert(end(), s.cbegin(), s.cend());
        return *this;
    }

    ap_string_t &append(const ap_string_t& s) {
        vec.insert(end(), s.cbegin(), s.cend());
        return *this;
    }

    ap_string_t &append(const char *cstr) {
        auto orig = cstr;
        while (*cstr)
            cstr++;
        vec.insert(end(), orig, cstr);
        return *this;
    }

    char *c_str() const {
        return vec.data();
    }

    void clear() {
        vec.clear();
        vec.push_back('\0');
    }

    uint64_t size() const {
        return vec.size() - 1;
    }

    iterator_t begin() {
        return vec.begin();
    }

    iterator_t end() {
        return vec.end() - 1;
    }

    const iterator_t cbegin() const {
        return vec.cbegin();
    }

    const iterator_t cend() const {
        return vec.cend() - 1;
    }

    template <typename T>
    ap_string_t &operator= (const T& str) {
        clear();
        return append(str);
    }

    template <typename T>
    ap_string_t &operator+= (const T& str) {
        return append(str);
    }

    template <typename T>
    ap_string_t operator + (const T& str) {
        ap_string_t ret = *this;
        ret.append(str);
        return ret;
    }

    char &operator [] (int64_t i) {
        return vec[i];
    }

    const char &operator[] (int64_t i) const {
        return vec[i];
    }

    bool operator < (const ap_string_t& str) const {
        int i = 0, j = 0;
        while (i < size() && j < str.size()) {
            if (vec[i] < str[i])
                return true;
            if (vec[i] > str[i])
                return false;
            i++;
            j++;
        }
        if (i == size() && j != str.size())
            return true;
        return false;
    }

};

#endif