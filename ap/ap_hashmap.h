#ifndef AP_HASHMAP_H
#define AP_HASHMAP_H

#include "ap_vector.h"
#include "glist.h"

#ifdef AP_ENABLE_AUTOINIT
extern ap_ctx_t *ap_static_ctx;
#endif

namespace ap
{
    inline uint32_t ap_hash(const void *data, uint32_t len) {
        uint8_t *bytes = (uint8_t *)data;
        uint32_t hash = 5381;
        for (uint32_t i = 0; i < len; i++) {
            hash = (hash * 31) ^ bytes[i];
        }
        return hash;
    }

    inline uint32_t ap_hash(const std::string& str) { return ap_hash(str.c_str(), str.size()); }
    inline uint32_t ap_hash(const char *cstr) { return ap_hash(cstr, strlen(cstr)); }
    inline uint32_t ap_hash(uint32_t a) { return ap_hash(&a, sizeof(a)); }
    inline uint32_t ap_hash(uint64_t a) { return ap_hash(&a, sizeof(a)); }
    inline uint32_t ap_hash(int32_t a)  { return ap_hash(&a, sizeof(a)); }
    inline uint32_t ap_hash(int64_t a)  { return ap_hash(&a, sizeof(a)); }

    struct ap_hmap_node_t {
        ap_off_t next;
        ap_off_t prev;
        ap_off_t gnext;
        ap_off_t gprev;
    };

    inline ap_hmap_node_t *hmap_get_node(ap_ctx_id_t ctx_id, ap_off_t n) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
        if (!ctx)
            return NULL;
        return (ap_hmap_node_t *)ap_malloc_ptr(ctx, n);
    }

    template <bool IS_GLIST>
    struct ap_hmap_list_ctx_t {
        using ptr_t = ap_off_t;
        ap_ctx_id_t ctx_id;

        ptr_t get_next_fn(ptr_t n)          {
            auto node = hmap_get_node(ctx_id, n);
            if constexpr (IS_GLIST)
                return node ? node->gnext : ap_off_t{};
            else
                return node ? node->next : ap_off_t{};
        }

        void  set_next_fn(ptr_t n, ptr_t r) {
            if (auto node = hmap_get_node(ctx_id, n); node) {
                if constexpr (IS_GLIST)
                    node->gnext = r;
                else
                    node->next = r;
            }
        }

        ptr_t get_prev_fn(ptr_t n) { 
            auto node = hmap_get_node(ctx_id, n);
            if constexpr (IS_GLIST)
                return node ? node->gprev : ap_off_t{};
            else
                return node ? node->prev : ap_off_t{};
        }

        void  set_prev_fn(ptr_t n, ptr_t l) {
            if (auto node = hmap_get_node(ctx_id, n); node)
                if constexpr (IS_GLIST)
                    node->gprev = l;
                else
                    node->prev = l;
        }

        ptr_t get_first()                   { return first; }
        ptr_t get_last()                    { return last; }
        void  set_first(ptr_t n)            { first = n; }
        void  set_last(ptr_t n)             { last = n; }

    private:
        ptr_t first = 0;
        ptr_t last = 0;
    };

    using hmap_list_t  = generic_list_t<GLIST_FLAG_DLIST | GLIST_FLAG_LAST, ap_hmap_list_ctx_t<0>>;
    using hmap_glist_t = generic_list_t<GLIST_FLAG_DLIST | GLIST_FLAG_LAST, ap_hmap_list_ctx_t<1>>;
}

template <typename Key, typename Val>
struct ap_hashmap_t {
    static constexpr uint32_t INITIAL_BUCKET_CNT = 64;
    static constexpr uint32_t MAX_BUCKET_CNT = 8;

    /* TODO: add alloc/free callback */

    struct hmap_node_t : public ap::ap_hmap_node_t {
        Key key;
        Val val;
    };

    using iter_fn_t = void (*)(hmap_node_t *node, void *ctx);

    struct bucket_t {
        ap::hmap_list_t nodes;
        uint32_t cnt = 0;
    };

    /* TODO: alloc and free nodes with ap_malloc */
    /* TODO: move ops out of struct */

    ap::hmap_glist_t elems;
    ap_vector_t<bucket_t> buckets;

#ifdef AP_ENABLE_AUTOINIT
    ap_hashmap_t() {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
    }
#endif

#ifdef AP_ENABLE_AUTOINIT
private:
#endif
    int init(ap_ctx_t *ctx) {
        ASSERT_FN(buckets.init(ctx));
        resize(INITIAL_BUCKET_CNT); /* hardcoded initial */
        elems.o.ctx_id = buckets.ctx_id;
        /* buckets.ctx_id - must be given to each list */
        return 0;
    }
#ifdef AP_ENABLE_AUTOINIT
public:
#endif

/* TODO: all the members bellow should be internals */
    void resize(uint32_t newsz) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return ;

        ap_vector_t<bucket_t> old_buckets = buckets;
        ap_vector_t<bucket_t> new_buckets;
        new_buckets.init(ctx);
        new_buckets.resize(newsz);
        buckets = new_buckets;

        for (auto &buck : new_buckets) {
            buck.nodes.o.ctx_id = buckets.ctx_id;
        }

        for (auto &buck : old_buckets) {
            auto curr = buck.nodes.front();
            while (curr) {
                auto next = buck.nodes.next(curr);
                buck.nodes.pop_front();
                insert(curr);
                curr = next;
            }
        }

        old_buckets.uninit();
    }

    void insert(ap_off_t hn) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return ;
        hmap_node_t *node = (hmap_node_t *)ap_malloc_ptr(ctx, hn);
        uint32_t slot = (ap::ap_hash(node->key)) % buckets.size();
        auto &buck = buckets[slot];
        buck.nodes.push_front(hn);
        buck.cnt++;
        elems.push_front(hn);
        if (buck.cnt > MAX_BUCKET_CNT)
            resize(buckets.size() * 2);
    }

    ap_off_t find(Key key) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return 0;
        uint32_t slot = (ap::ap_hash(key)) % buckets.size();
        auto curr = buckets[slot].nodes.front();
        while (curr) {
            hmap_node_t *node = (hmap_node_t *)ap_malloc_ptr(ctx, curr);
            if (node->key == key)
                return curr;
            curr = buckets[slot].nodes.next(curr);
        }
        return 0;
    }

    ap_off_t erase(Key key) {
        uint32_t slot = (ap::ap_hash(key)) % buckets.size();
        auto node = find(key);
        if (!node)
            return 0;
        elems.remove(node);
        buckets[slot].nodes.remove(node);
        return node;
    }

    ap_off_t alloc_hnode() {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return 0;
        return ap_malloc_alloc(ctx, sizeof(hmap_node_t));
    }

    void free_hnode(ap_off_t off) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return ;
        ap_malloc_free(ctx, off);
    }

    hmap_node_t *deref_hnode(ap_off_t off) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return NULL;
        return (hmap_node_t *)ap_malloc_ptr(ctx, off);
    }

    void iter(void *uctx, iter_fn_t iter_fn) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(buckets.ctx_id);
        if (!ctx)
            return ;
        auto curr = elems.front();
        while (curr) {
            hmap_node_t *node = (hmap_node_t *)ap_malloc_ptr(ctx, curr);
            iter_fn(node, uctx);
            curr = elems.next(curr);
        }
    }
};

#endif
