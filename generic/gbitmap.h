#ifndef GBITMAP_H
#define GBITMAP_H

#include <cstdint>
#include "misc_utils.h"
#include "debug.h"

namespace ap
{
    template <typename T>
    concept ap_bmap_ctx_req = requires(T) {
        typename T::W;
        typename T::I;
        typename T::SZ;
        { T{}.get_word_fn(typename T::I{})                  } -> std::same_as<typename T::W>;
        { T{}.set_word_fn(typename T::I{}, typename T::W{}) } -> std::same_as<void>;
        { T{}.get_sz_fn()                                   } -> std::same_as<typename T::SZ>;
        { T{}.resize_fn(typename T::SZ{})                   } -> std::same_as<void> ;
    };

    template <typename T> requires ap_bmap_ctx_req<T>
    struct ap_bmap_ctx_wrap_t { T o; };

    inline void FIX_THE_SYNTAX_IN_ST4__LISTS (){ /* TODO: remove when fixed */ }

    struct bmap_ctx_example_t {
        using W = uint64_t;
        using I = size_t;
        using SZ = size_t;

        W    get_word_fn(I i) const { return arr[i]; }
        void set_word_fn(I i, W w)  { arr[i] = w; }
        SZ   get_sz_fn()      const { return sizeof(arr) / sizeof(arr[0]); }
        void resize_fn()            { ; }

    private:
        uint64_t arr[128];
    };
}

template <typename bmap_ctx_t>
struct generic_bitmap_t : public ap::ap_bmap_ctx_wrap_t<bmap_ctx_t> {
    using ctx_t = ap::ap_bmap_ctx_wrap_t<bmap_ctx_t>;
    using W = bmap_ctx_t::W;
    using I = bmap_ctx_t::I;
    using SZ = bmap_ctx_t::SZ;

    static constexpr uint64_t bpb = 8;
    static constexpr uint64_t bpw = sizeof(W) * bpb;

    struct bit_view_t {
        const generic_bitmap_t *bm;
        I i;

        bit_view_t(const generic_bitmap_t *bm, I i) : bm(bm), i(i) {}

        operator bool () {
            return bm->get(i);
        }

        bool operator = (bool val) {
            bm->set(i, val);
            return bm->get(i);
        }
    };

    struct iter_t {
        bit_view_t bv;

        iter_t(const generic_bitmap_t *bm, I i) : bv(bm, i) {}

        iter_t &operator ++(   ) {                      bv.i++;           return *this; }
        iter_t &operator --(   ) {                      bv.i--;           return *this; }
        iter_t  operator ++(int) { auto iter = (*this); bv.i++;           return iter;  }
        iter_t  operator --(int) { auto iter = (*this); bv.i--;           return iter;  }
        iter_t  operator + (I n) { auto iter = (*this); iter.bv.i += n;   return iter;  }
        iter_t  operator - (I n) { auto iter = (*this); iter.bv.i -= n;   return iter;  }
        iter_t &operator +=(I n) {                      bv.i += n;        return *this; }
        iter_t &operator -=(I n) {                      bv.i -= n;        return *this; }

        bool    operator == (iter_t oth) {
            return bv.bm == oth.bv.bm && bv.i == oth.bv.i;
        }

        bool    operator != (iter_t oth) {
            return !((*this) == oth);
        }

        bit_view_t &operator *() { return bv; };
    };

    bool get(I i) const {
        auto sz = get_sz();
        auto word_idx = i / bpw;
        if (word_idx >= sz)
            return 0;
        auto w = get_word(word_idx);
        bool res = !!(w & (W(1) << (i % bpw)));
        return res;
    }

    void set(I i, bool val) {
        auto sz = get_sz();
        auto word_idx = i / bpw;
        if (word_idx >= sz) {
            resize(word_idx + 1);
            if (word_idx >= get_sz())
                return;
        }
        auto w = get_word(word_idx);
        if (val)
            w |= (W(1) << (i % bpw));
        else
            w &= ~(W(1) << (i % bpw));
        set_word(word_idx, w);
    }

    I next_one(I i) {
        auto sz = get_sz();
        auto word_idx = i / bpw;
        while (word_idx < sz) {
            auto w = get_word(word_idx);
            if (w) {
                do {
                    if (!!(w & (W(1) << (i % bpw))))
                        return i;
                    i++;
                } while (i % bpw);
            }
            else
                i = (i / bpw + 1) * bpw;
            word_idx = i / bpw;
        }
        return I(-1);
    }

    I next_zero(I i) {
        auto sz = get_sz();
        auto word_idx = i / bpw;
        while (word_idx < sz) {
            auto w = get_word(word_idx);
            if (w != W(-1)) {
                do {
                    if (!!((~w) & (W(1) << (i % bpw))))
                        return i;
                    i++;
                } while (i % bpw);
            }
            else
                i = (i / bpw + 1) * bpw;
            word_idx = i / bpw;
        }
        return I(-1);
    }

    iter_t begin() const {
        return iter_t(this, 0);
    }

    iter_t end() const {
        return iter_t(this, get_sz() * bpw);
    }

    W       get_word(I i)       const   { return ctx_t::o.get_word_fn(i); }
    void    set_word(I i, W w)          { ctx_t::o.set_word_fn(i, w); }
    SZ      get_sz()            const   { return ctx_t::o.get_sz_fn(); }
    void    resize(SZ sz)               { ctx_t::o.resize_fn(sz); }
};

#endif
