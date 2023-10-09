#ifndef AP_PTR_H
#define AP_PTR_H

#ifndef AP_ENABLE_AUTOINIT
# error "ap_ptr can't be used without the autoinit functionality"
#endif

template <typename T>
struct ap_ptr_internal_t {
    ap_off_t off = 0;

    T *operator->() const {
        T *obj = (T *)ap_malloc_ptr(ap_static_ctx, off);
        return obj;
    }

    const T& operator * () const {
        T *obj = (T *)ap_malloc_ptr(ap_static_ctx, off);
        return *obj;
    }

    T& operator * () {
        T *obj = (T *)ap_malloc_ptr(ap_static_ctx, off);
        return *obj;
    }

    template <typename ...Args>
    static ap_ptr_internal_t<T> construct(Args&& ...args) {
        auto [off, ptr] = ap_storage_construct<T>(std::forward<Args>(args)...);
        if (off == 0) {
            DBG("Failed to allocate memory");
            return {};
        }
        return { .off = off };
    }

    static void destruct(ap_ptr_internal_t<T> &ptr) {
        ap_storage_destruct<T>(ptr.off);
        ptr.off = 0;
    }
};

template <typename T>
struct ap_ptr_t {
    struct smptr_storage_t {
        ap_ptr_internal_t<T> ptr;
        uint64_t ref = 0;
    };
    ap_ptr_internal_t<smptr_storage_t> base;

    ap_ptr_t() {}

    T *operator->() const {
        if (!base.off) {
            DBG("INVALID DEREFERENCE 1");
        }
        return &(*base->ptr);
    }

    const T& operator * () const {
        if (!base.off) {
            DBG("INVALID DEREFERENCE 2");
        }
        return *base->ptr;
    }

    T& operator * () {
        if (!base.off) {
            DBG("INVALID DEREFERENCE 3");
        }
        return *base->ptr;
    }

    operator bool() {
        return base.off != 0;
    }

    ap_ptr_t(const ap_ptr_t& oth) {
        base = oth.base;
        inc_ref();
    }
    ap_ptr_t(ap_ptr_t&& oth) {
        base = oth.base;
        inc_ref();
    }
    ap_ptr_t &operator = (const ap_ptr_t& oth) {
        dec_ref();
        base = oth.base;
        inc_ref();
        return *this;
    }
    ap_ptr_t &operator = (ap_ptr_t&& oth) {
        dec_ref();
        base = oth.base;
        inc_ref();
        return *this;
    }

    ~ap_ptr_t() {
        dec_ref();
    }

    void reset() {
        dec_ref();
        base.off = 0;
    }

    void inc_ref() {
        if (base.off)
            base->ref++;
    }

    void dec_ref() {
        if (base.off) {
            base->ref--;
            if (base->ref == 0) {
                ap_ptr_internal_t<T>::destruct(base->ptr);
                ap_ptr_internal_t<smptr_storage_t>::destruct(base);
            }
        }
    }

    template <typename ...Args>
    static ap_ptr_t<T> mkptr(Args&& ...args) {
        ap_ptr_t<T> ret;
        ret.base = ap_ptr_internal_t<smptr_storage_t>::construct();
        ret.base->ptr = ap_ptr_internal_t<T>::construct(std::forward<Args>(args)...);
        ret.base->ref = 1;
        return ret;
    }
};

#endif
