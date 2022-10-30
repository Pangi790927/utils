#ifndef GHEAP_H
#define GHEAP_H

#include <cstdint>
#include "misc_utils.h"
#include "debug.h"

namespace ap
{
    template <typename T>
    concept ap_heap_ctx_req = requires(T) {
        { sizeof(typename T::U)                         } -> std::same_as<size_t>;
        { sizeof(typename T::I)                         } -> std::same_as<size_t>;
        { T{}.objref(typename T::I{})                   } -> std::same_as<typename T::U&>;
        { T{}.cmp_fn(typename T::U{}, typename T::U{})  } -> std::same_as<bool>;
        { T{}.get_sz_fn()                               } -> std::same_as<typename T::I>;
        { T{}.resize_fn(typename T::I{})                } -> std::same_as<void> ;
    };

    template <typename T> requires ap_heap_ctx_req<T>
    struct ap_heap_ctx_wrap_t { T o; };

    inline void FIX_THE_SYNTAX_IN_ST4__HEAP (){ /* TODO: remove when fixed */ }

    template <typename T>
    struct heap_ctx_example_t {
        using I = size_t;
        using U = T;

        T&   objref(I i)                            { return arr[i]; }
        bool cmp_fn(const T& a, const T& b) const   { return a < b; }
        I    get_sz_fn()                    const   { return sizeof(arr) / sizeof(arr[0]); }
        void resize_fn(I sz)                        { ; }

    private:
        I cnt = 0;
        uint64_t arr[128];
    };
}

template <typename bmap_ctx_t>
struct generic_heap_t : public ap::ap_heap_ctx_wrap_t<bmap_ctx_t> {
    using ctx_t = ap::ap_heap_ctx_wrap_t<bmap_ctx_t>;
    using I = bmap_ctx_t::I;
    using T = bmap_ctx_t::U;

    using iter_ctx_t      = void *;
    using iter_cbk_t      = void        (*)(T &elem, I i, int lvl, iter_ctx_t c);

    void insert(const T& b) {
        auto cnt = get_sz();
        resize(cnt + 1);
        objref(cnt) = b;
        I curr = cnt;
        while (curr != I(0)) {
            if (!cmp(objref(curr), ref_parr(curr)))
                break;
            std::swap(objref(curr), ref_parr(curr));
            curr = parr(curr);
        }
    }

    T& top() {
        return objref(I(0));
    }

    void pop() {
        auto cnt = get_sz();
        if (!cnt)
            return ;
        std::swap(top(), objref(cnt - 1));
        resize(cnt - 1);
        I curr = I(0);
        while (inside(curr)) {
            I largest = curr;
            if (inside(left(curr)) && cmp(ref_left(curr), objref(largest)))
                largest = left(curr);
            if (inside(right(curr)) && cmp(ref_right(curr), objref(largest)))
                largest = right(curr);
            if (largest != curr) {
                std::swap(objref(curr), objref(largest));
                curr = largest;
            }
            else
                break;
        }
    }

    void iter(iter_cbk_t cbk, iter_ctx_t c) {
        rec_iter(cbk, c, 0, 0);
    }

private:
    bool inside(I i) { return i < get_sz(); }

    I left(I i)  { return i * 2 + 1; }
    I right(I i) { return i * 2 + 2; }
    I parr(I i)  { return (i - 1) / 2; }

    T &ref_left(I i)  { return objref(left(i)); }
    T &ref_right(I i) { return objref(right(i)); }
    T &ref_parr(I i)  { return objref(parr(i)); }

    void rec_iter(iter_cbk_t cbk, iter_ctx_t c, I i, int lvl) {
        if (!inside(i))
            return;
        cbk(objref(i), i, lvl, c);
        rec_iter(cbk, c, left(i), lvl + 1);
        rec_iter(cbk, c, right(i), lvl + 1);
    }

    T       &objref(I i)                { return ctx_t::o.objref(i); }
    bool    cmp(const T& a, const T& b) { return ctx_t::o.cmp_fn(a, b); }
    I       get_sz()                    { return ctx_t::o.get_sz_fn(); }
    void    resize(I sz)                { ctx_t::o.resize_fn(sz); }
};

#endif
