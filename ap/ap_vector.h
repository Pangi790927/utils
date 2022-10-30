#ifndef AP_VECTOR_H
#define AP_VECTOR_H

#include <utility>
#include <iterator>
#include "ap_malloc.h"
#include "misc_utils.h"
#include "bit_utils.h"
#include "debug.h"
#include "ap_except.h"

#ifdef AP_ENABLE_AUTOINIT
extern ap_ctx_t *ap_static_ctx;
#endif

template <typename T>
struct ap_vector_cpp_fns_t {
    static void new_obj(void *loc) {
        new (loc) T{};
    }
    static void new_obj_move(void *loc, void *oth) {
        T &oth_obj = *(T *)oth;
        new (loc) T{std::move(oth_obj)};
    }
    static void new_obj_copy(void *loc, const void *oth) {
        const T &oth_obj = *(T *)oth;
        new (loc) T{oth_obj};
        const T &obj = *(T *)loc;
    }
    static void delete_obj(void *loc) {
        T &obj = *(T *)loc;
        obj.~T();
    }
};

/* This vector should be able to stay inside shared memory space */
template <typename T, typename FNS_T = ap_vector_cpp_fns_t<T>>
struct ap_vector_t {
    ap_off_t    datap;
    uint64_t    cap;
    uint64_t    cnt;
    ap_ctx_id_t ctx_id;

    using iterator_t = T*;

#ifdef AP_ENABLE_AUTOINIT
    ap_vector_t() {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
    }
#endif

    /* this init function should only be called when this structure is placed inside the shared
    memory space */

#ifdef AP_ENABLE_AUTOINIT
private:
#endif
    int init(ap_ctx_t *ctx) {
        ctx_id = ctx->ctx_id;
        cnt = 0;
        cap = 0;
        datap = 0;
        return 0;
    }
#ifdef AP_ENABLE_AUTOINIT
public:
#endif

    void uninit() {
        clear();
        ctx_id = 0;
    }

    bool is_init() {
        return ctx_id != 0;
    }

    void clear() {
        if (!datap)
            return ;

        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx)
            return ;
        auto p_data = ap_malloc_ptr(ctx, datap);

        for (uint64_t i = 0; i < cnt; i++) {
            void *loc = (void *)((uint8_t *)p_data + cnt * sizeof(T));
            FNS_T::delete_obj(loc);
        }

        ap_malloc_free(ctx, datap);
        datap = ap_off_t{};
        cnt = 0;
        cap = 0;
    }

    int reserve(uint64_t new_cap) {
        if (new_cap <= cap)
            return 0;
        new_cap = next_pow2(new_cap);
        if (new_cap == cap)
            return 0;

        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx) {
            AP_EXCEPT("invalid ctx ptr");
            return -1;
        }

        uint64_t new_sz = new_cap * sizeof(T);

        ap_off_t new_data = ap_malloc_alloc(ctx, new_sz);
        if (!new_data) {
            AP_EXCEPT("Failed to alloc new mem");
            return -1;
        }

        auto p_old_data = ap_malloc_ptr(ctx, datap);
        auto p_new_data = ap_malloc_ptr(ctx, new_data);

        for (int64_t i = 0; i < cnt; i++) {
            void *new_loc = (void *)((uint8_t *)p_new_data + i * sizeof(T));
            void *old_loc = (void *)((uint8_t *)p_old_data + i * sizeof(T));
            FNS_T::new_obj_move(new_loc, old_loc);
            FNS_T::delete_obj(old_loc);
        }

        if (datap)
            ap_malloc_free(ctx, datap);
        datap = new_data;
        cap = new_cap;
        return 0;
    }

    int resize(uint64_t new_cnt, const T& val = T{}) {
        if (new_cnt == cnt)
            return 0;

        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx) {
            AP_EXCEPT("invalid ctx ptr");
            return -1;
        }

        if (new_cnt < cnt) {
            auto p_data = ap_malloc_ptr(ctx, datap);
            for (uint64_t i = new_cnt; i < cnt; i++) {
                void *loc = (void *)((uint8_t *)p_data + i * sizeof(T));
                FNS_T::delete_obj(loc);
            }

            cnt = new_cnt;
        }
        else if (new_cnt > cnt) {
            uint64_t new_cap = next_pow2(new_cnt);
            if (reserve(new_cap) < 0) {
                AP_EXCEPT("failed to reserve memory");
                return -1;
            }
            auto p_data = ap_malloc_ptr(ctx, datap);

            for (uint64_t i = cnt; i < new_cnt; i++) {
                void *loc = (void *)((uint8_t *)p_data + i * sizeof(T));
                FNS_T::new_obj_copy(loc, &val);
            }
            cnt = new_cnt;
        }
        return 0;
    }

    iterator_t begin() {
        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx) {
            AP_EXCEPT("invalid ctx ptr");
            return (T *)NULL;
        }
        return (T *)ap_malloc_ptr(ctx, datap);
    }

    iterator_t end() {
        auto b = begin();
        if (!b)
            return b;
        return b + cnt;
    }

    const iterator_t cbegin() const {
        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx) {
            AP_EXCEPT("invalid ctx ptr");
            return (T *)NULL;
        }
        return (T *)ap_malloc_ptr(ctx, datap);
    }

    const iterator_t cend() const {
        auto b = cbegin();
        if (!b)
            return b;
        return b + cnt;
    }

    iterator_t erase(iterator_t a, iterator_t b) {
        if (a >= b)
            return b;
        auto e = end();
        auto new_cnt = cnt;
        for (auto it = a; it != b; it++) {
            FNS_T::delete_obj(it);
            new_cnt--;
        }
        uint64_t i = 0;
        for (auto it = b; it != e; it++, i++) {
            FNS_T::new_obj_move(a + i, it);
            FNS_T::delete_obj(it);
        }
        cnt = new_cnt;
        return b;
    }

    template <typename IT>
    iterator_t insert(iterator_t pos, IT a, IT b) {
        auto ins_sz = std::distance(a, b);
        int64_t posi = pos - begin();
        reserve(ins_sz + cnt);

        for (int64_t i = ins_sz + cnt - 1, j = cnt - 1; j >= posi; i--, j--) {
            FNS_T::new_obj_move(&(*this)[i], &(*this)[j]);
            FNS_T::delete_obj(&(*this)[j]);
        }

        uint64_t start = posi;
        for (auto it = a; it != b; it++) {
            (*this)[posi] = (*it);
            posi++;
        }
        cnt += ins_sz;
        return begin() + start;
    }

    iterator_t insert(iterator_t pos, const T& val) {
        return insert(pos, &val, &val + 1);
    }

    iterator_t insert(iterator_t pos, std::initializer_list<T> il) {
        for (auto e : il) {
            pos = insert(pos, e) + 1;
        }
        return pos;
    }

    iterator_t erase(iterator_t a) {
        return erase(a, a + 1);
    }

    int push_back(const T& val) {
        if (resize(cnt + 1, val) < 0) {
            AP_EXCEPT("Failed to resize");
            return -1;
        }
        return 0;
    }

    T &back() {
        return *(end() - 1);
    }

    T &front() {
        return *(begin());
    }

    T *data() {
        return begin();
    }

    uint64_t size() {
        return cnt;
    }

    void pop_back() {
        erase(end() - 1, end());
    }

    T &operator[] (int64_t n) {
        if (n < 0)
            return *(end() + n);
        return *(begin() + n);
    }

    const T &operator[] (int64_t n) const {
        if (n < 0)
            return *(end() + n);
        return *(begin() + n);
    }

    /* TODO: implement others */
};

#endif
