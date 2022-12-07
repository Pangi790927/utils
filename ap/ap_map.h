/* TODO */
#include "ap_malloc.h"
#include "gavl.h"
#include "ap_except.h"

#ifdef AP_ENABLE_AUTOINIT
extern ap_ctx_t *ap_static_ctx;
#endif

namespace ap
{
    template <typename K, typename V>
    struct map_node_t {
        ap_off_t left = 0;
        ap_off_t right = 0;
        int height = 0;
        std::pair<K, V> elem;

        const K& key() const {
            return elem.first;
        }

        V& val() {
            return elem.second;
        }
    };

    template <typename K, typename V>
    struct map_avl_ctx_t {
        using node_t = map_node_t<K, V>;
        using ptr_t = ap_off_t;

        ap_ctx_id_t ctx_id = 0;
        ptr_t root = ptr_t{};

        int init(ap_ctx_t *ctx) {
            /* init the avl tree */
            ctx_id = ctx->ctx_id;
            root = ptr_t{};
            return 0;
        }

        ptr_t       get_root_fn   ()                            { return root; }
        void        set_root_fn   (ptr_t r)                     { root = r; }
        ptr_t       get_left_fn   (ptr_t node)                  { return get_node(node)->left; }
        void        set_left_fn   (ptr_t node, ptr_t newn)      { get_node(node)->left = newn; }
        ptr_t       get_right_fn  (ptr_t node)                  { return get_node(node)->right; }
        void        set_right_fn  (ptr_t node, ptr_t newn)      { get_node(node)->right = newn; }
        int         get_height_fn (ptr_t node)                  { return get_node(node)->height; }
        void        set_height_fn (ptr_t node, int height)      { get_node(node)->height = height; }

        /* cmp is as strcmp */
        int cmp_fn(ptr_t a, ptr_t b) {
            auto na = get_node(a);
            auto nb = get_node(b);
            return na->key() < nb->key() ? -1 : (nb->key() < na->key() ? 1 : 0);
        }

        void same_key_cbk(ptr_t node, ptr_t new_node) {
            /* on insert */
            get_node(node)->val() = get_node(new_node)->val();
        }

        node_t *get_node(ptr_t n) {
            ap_ctx_t *ctx = ap_malloc_get_ctx(ctx_id);
            if (!ctx) {
                AP_EXCEPT("Failed to get ctx");
                return NULL;
            }
            return (node_t *)ap_malloc_ptr(ctx, n);
        }
    };
}

template <typename Key, typename Val>
struct ap_map_t {
    generic_avl_t<ap::map_avl_ctx_t<Key, Val>> avl;
    size_t cnt = 0;

    using ctx_t = ap::map_avl_ctx_t<Key, Val>;
    using node_t = decltype(avl.o)::node_t;
    using pair_t = decltype(node_t::elem);

    struct search_ctx_t {
        ap_map_t *parr;
        const Key *key;

        search_ctx_t(ap_map_t *parr, const Key *key) : parr(parr), key(key) {}
    };

    struct iter_t {
        ap_map_t *parr;
        ap_off_t n;

        iter_t(ap_map_t *parr, ap_off_t n) : parr(parr), n(n) {}

        iter_t &operator ++() {
            if (n) {
                auto &key = parr->get_node(n)->elem.first;
                n = parr->avl.get_succ(key_cmp_fn, search_ctx_t(parr, &key));
            }
            else
                n = parr->avl.get_min();
            return *this;
        }

        iter_t &operator --() {
            if (n) {
                auto &key = parr->get_node(n)->elem.first;
                n = parr->avl.get_pred(key_cmp_fn, search_ctx_t(parr, &key));
            }
            else
                n = parr->avl.get_max();
            return *this;
        }

        iter_t  operator ++(int) { auto iter = (*this); ++(*this); return iter;  }
        iter_t  operator --(int) { auto iter = (*this); --(*this); return iter;  }

        bool operator == (iter_t oth) {
            return n == oth.n && parr == oth.parr;
        }

        bool operator != (iter_t oth) {
            return !((*this) == oth);
        }

        pair_t &operator *() {
            return parr->get_node(n)->elem;
        }

        pair_t *operator ->() {
            return &(parr->get_node(n)->elem);
        }
    };

#ifdef AP_ENABLE_AUTOINIT
    ap_map_t() {
        if (init(ap_static_ctx) < 0)
            AP_EXCEPT("Failed constructor");
    }

    ~ap_map_t() {
        uninit();
    }
#endif

#ifdef AP_ENABLE_AUTOINIT
private:
#endif
    int init(ap_ctx_t *ctx) {
        cnt = 0;
        return avl.o.init(ctx);
    }

    void uninit() {
        clear();
    }
#ifdef AP_ENABLE_AUTOINIT
public:
#endif

    void clear() {
        while (avl.o.root) {
            ap_off_t n;
            avl.remove(avl.o.root, &n);
            free_node(n);
        }
        cnt = 0;
    }

    iter_t insert(const Key& key, const Val& val) {
        auto n = alloc_node();
        construct(n);
        auto node = get_node(n);
        node->elem = pair_t(key, val);
        avl.insert(n);
        cnt++;
        return iter_t(this, n);
    }

    void erase(const Key& key) {
        ap_off_t to_rm;
        avl.remove(key_cmp_fn, search_ctx_t(this, &key), &to_rm);
        if (to_rm) {
            cnt--;
            deconstruct(to_rm);
            free_node(to_rm);
        }
    }

    iter_t find(const Key& key) {
        return iter_t(this, avl.find(key_cmp_fn, search_ctx_t(this, &key)));
    }

    iter_t pred(const Key& key) {
        return iter_t(this, avl.get_pred(key_cmp_fn, search_ctx_t(this, &key)));
    }

    iter_t succ(const Key& key) {
        return iter_t(this, avl.get_succ(key_cmp_fn, search_ctx_t(this, &key)));
    }

    size_t size() {
        return cnt;
    }

    iter_t begin() {
        if (!cnt)
            return iter_t(this, 0);
        return iter_t(this, avl.get_min());
    }

    iter_t end() {
        return iter_t(this, 0);
    }

    iter_t rbegin() {
        if (!cnt)
            return iter_t(this, 0);
        return iter_t(this, avl.get_max());
    }

    iter_t rend() {
        return iter_t(this, 0);
    }

    Val &operator [] (const Key& key) {
        auto it = find(key);
        if (it == end())
            it = insert(key, Val{});
        return it->second;
    }

    const Val &operator [] (const Key& key) const {
        auto it = find(key);
        return *it;
    }

private:
    static int key_cmp_fn(const search_ctx_t& key_ctx, ap_off_t n) {
        auto node = key_ctx.parr->get_node(n);
        return *key_ctx.key < node->key() ? -1 : (node->key() < *key_ctx.key ? 1 : 0);
    }

    void construct(ap_off_t n) {
        auto node = get_node(n);
        new (node) node_t{};
    }

    void deconstruct(ap_off_t n) {
        auto node = get_node(n);
        node->~node_t();
    }

    ap_off_t alloc_node() {
        ap_ctx_t *ctx = ap_malloc_get_ctx(avl.o.ctx_id);
        if (!ctx) {
            AP_EXCEPT("Failed to get ctx: alloc");
            return ap_off_t{};
        }
        return ap_malloc_alloc(ctx, sizeof(node_t));
    }

    void free_node(ap_off_t node) {
        ap_ctx_t *ctx = ap_malloc_get_ctx(avl.o.ctx_id);
        if (!ctx) {
            AP_EXCEPT("Failed to get ctx: free");
            return ;
        }
        ap_malloc_free(ctx, node);
    }

    node_t *get_node(ap_off_t n) {
        return avl.o.get_node(n);
    }
};