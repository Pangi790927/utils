#ifndef GAVL_H
#define GAVL_H

#include "debug.h"

/* changed the whole function thing: made a class that holds the required functions for
a generic*-struct, made sure the functions are present using c++ concepts, hold only a single ctx
member inside each generic*-struct. This way we are able to hold those implementations inside
shared memory locations, kernel space, alloc-free spaces, custom alloced spaces, mapped files, etc.
*/

namespace ap
{
    template <typename T>
    concept generic_avl_ctx_req = requires(T) {
        { sizeof(typename T::ptr_t)                            } -> std::same_as<size_t>;
        { T{}.get_root_fn()                                    } -> std::same_as<typename T::ptr_t>;
        { T{}.set_root_fn(typename T::ptr_t{})                 } -> std::same_as<void>;
        { T{}.cmp_fn(typename T::ptr_t{}, typename T::ptr_t{}) } -> std::same_as<int>;

        { T{}.get_left_fn(typename T::ptr_t{})                 } -> std::same_as<typename T::ptr_t>;
        { T{}.get_right_fn(typename T::ptr_t{})                } -> std::same_as<typename T::ptr_t>;
        { T{}.get_height_fn(typename T::ptr_t{})               } -> std::same_as<int>;

        { T{}.set_left_fn(typename T::ptr_t{}, typename T::ptr_t{})  } -> std::same_as<void>;
        { T{}.set_right_fn(typename T::ptr_t{}, typename T::ptr_t{}) } -> std::same_as<void>;
        { T{}.set_height_fn(typename T::ptr_t{}, int{})              } -> std::same_as<void>;
        { T{}.same_key_cbk(typename T::ptr_t{}, typename T::ptr_t{}) } -> std::same_as<void>;
    };

    template <typename T> requires generic_avl_ctx_req<T>
    struct generic_avl_ctx_wrap_t { T o; };

    inline void FIX_THE_SYNTAX_IN_ST4__AVL (){ /* TODO: remove when fixed */ }

    struct avl_ctx_example_t {
        struct node_t {
            node_t *left;
            node_t *right;
            int val;
            int height;
        };

        using ptr_t = node_t *;

        node_t *    get_root_fn   () const                          { return root; }
        void        set_root_fn   (node_t *r)                       { root = r; }

        /* cmp is as strcmp */
        int         cmp_fn        (node_t *a, node_t *b)            { return a->val - b->val; }
        node_t *    get_left_fn   (node_t *node) const              { return node->left; }
        void        set_left_fn   (node_t *node, node_t *newn)      { node->left = newn; }
        node_t *    get_right_fn  (node_t *node) const              { return node->right; }
        void        set_right_fn  (node_t *node, node_t *newn)      { node->right = newn; }
        int         get_height_fn (node_t *node) const              { return node->height; }
        void        set_height_fn (node_t *node, int height)        { node->height = height; }
        void        same_key_cbk  (node_t *node, node_t *new_node)  { node->val = new_node->val; }

    private:
        node_t *root = nullptr;
    };
}

/* TODO: like for lists add the option to add a parent and others? */

/* avl_ptr_t{} must point to "NULL", whatever that means for you, and casting avl_ptr_t to bool
should return false if avl_ptr_t is "NULL" */
template <typename avl_ctx_t>
struct generic_avl_t : public ap::generic_avl_ctx_wrap_t<avl_ctx_t>{
    using avl_ptr_t = typename avl_ctx_t::ptr_t;
    using ctx_t = ap::generic_avl_ctx_wrap_t<avl_ctx_t>;

    using iter_ctx_t      = void *;
    using iter_cbk_t      = void        (*)(avl_ptr_t node, avl_ptr_t par, int lvl, iter_ctx_t c);

    void insert(avl_ptr_t new_node) {
        auto root = get_root();
        if (!root) {
            set_root(new_node);
            return ;
        }
        auto new_root = rec_insert(root, new_node);
        if (new_root != root)
            set_root(new_root);
    }

    template <typename KeyCmp, typename KeyCtx>
    void remove(KeyCmp key_cmp, const KeyCtx &key_ctx, avl_ptr_t *to_remove) {
        auto target = find(key_cmp, key_ctx);
        if (!target)
            return ;
        remove(target, to_remove);
    }

    void remove(avl_ptr_t target, avl_ptr_t *to_remove) {
        auto root = get_root();
        auto new_root = rec_remove(root, target, to_remove);
        if (new_root != root)
            set_root(new_root);
    }

    template <typename KeyCmp, typename KeyCtx>
    avl_ptr_t find(KeyCmp key_cmp, const KeyCtx &key_ctx) {
        return rec_find(get_root(), key_cmp, key_ctx);
    }

    avl_ptr_t get_min() const {
        return _get_min(get_root());
    }

    avl_ptr_t get_max() const {
        return _get_max(get_root());
    }

    template <typename KeyCmp, typename KeyCtx>
    avl_ptr_t get_succ(KeyCmp key_cmp, const KeyCtx &key_ctx) const {
        auto succ = avl_ptr_t{};
        auto curr = get_root();
        while (curr) {
            int cmpv = -key_cmp(key_ctx, curr);
            if (cmpv < 0) {
                curr = get_right(curr);
            }
            else if (cmpv > 0) {
                succ = curr;
                curr = get_left(curr);
            }
            else {
                curr = get_right(curr);
                if (curr)
                    succ = _get_min(curr);
                break;
            }
        }
        return succ;
    }

    template <typename KeyCmp, typename KeyCtx>
    avl_ptr_t get_pred(KeyCmp key_cmp, const KeyCtx &key_ctx) const {
        auto pred = avl_ptr_t{};
        auto curr = get_root();
        while (curr) {
            int cmpv = -key_cmp(key_ctx, curr);
            if (cmpv > 0) {
                curr = get_left(curr);
            }
            else if (cmpv < 0) {
                pred = curr;
                curr = get_right(curr);
            }
            else {
                curr = get_left(curr);
                if (curr)
                    pred = _get_max(curr);
                break;
            }
        }
        return pred;
    }

    void iter(iter_cbk_t cbk, iter_ctx_t c) {
        rec_iter(get_root(), avl_ptr_t{}, 0, cbk, c);
    }

    void rev_iter(iter_cbk_t cbk, iter_ctx_t c) {
        rec_rev_iter(get_root(), avl_ptr_t{}, 0, cbk, c);
    }

private:
    avl_ptr_t _get_min(avl_ptr_t node) const {
        avl_ptr_t curr = node;
        while (curr && get_left(curr))
            curr = get_left(curr);
        return curr;
    }

    avl_ptr_t _get_max(avl_ptr_t node) const {
        avl_ptr_t curr = node;
        while (curr && get_right(curr))
            curr = get_right(curr);
        return curr;
    }

    int compute_height(avl_ptr_t node) {
        int left_h = get_left(node) ? get_heigth(get_left(node)) : 0;
        int right_h = get_right(node) ? get_heigth(get_right(node)) : 0;

        return (left_h > right_h ? left_h : right_h) + 1;
    }

    int compute_bal(avl_ptr_t node) {
        if (!node)
            return 0;

        int left_h = get_left(node) ? get_heigth(get_left(node)) : 0;
        int right_h = get_right(node) ? get_heigth(get_right(node)) : 0;

        return left_h - right_h;
    }

    // void dbg_print() {
    //     dbg_print_rec(get_root(), 0);
    // }

    // void dbg_print_rec(avl_ptr_t node, int lvl) {
    //     if (!node)
    //         return ;
    //     std::string padd = std::string(lvl, '.');
    //     std::string rpadd = std::string(10 - lvl, ' ');
    //     dbg_print_rec(get_left(node), lvl + 1);
    //     DBG("%s > %2d%s[%2d]", padd.c_str(), node->val, rpadd.c_str(), node->height);
    //     dbg_print_rec(get_right(node), lvl + 1);
    // }

    avl_ptr_t rec_insert(avl_ptr_t node, avl_ptr_t new_node) {
        if (!node) {
            set_heigth(new_node, 1);
            set_left(new_node, avl_ptr_t{});
            set_right(new_node, avl_ptr_t{});
            return new_node;
        }
        // DBG("node: %p", node);
        // DBG("new_node: %p", new_node);
        // DBG("node: %d", node->val);
        // DBG("new_node: %d", new_node->val);

        if (cmp(new_node, node) < 0) {
            // DBG("<node: %p", node);
            set_left(node, rec_insert(get_left(node), new_node));
            // DBG("<?node: %p", node);
        }
        else if (cmp(new_node, node) > 0) {
            // DBG(">node: %p", node);
            set_right(node, rec_insert(get_right(node), new_node));
            // DBG(">?node: %p", node);
        }
        else
            same_key(node, new_node);

        set_heigth(node, compute_height(node));
        // DBG("u blake?");
        // dbg_print();
        return balance_insert(node, new_node);
    }

    avl_ptr_t balance_insert(avl_ptr_t node, avl_ptr_t new_node) {
        int bal = compute_bal(node);

        if (bal > 1 && get_left(node) && cmp(new_node, node) < 0)
            return right_rotate(node);

        // DBG("1blake?");

        if (bal < -1 && cmp(new_node, get_right(node)) > 0)
            return left_rotate(node);
        // DBG("2blake?");

        if (bal > 1 && cmp(new_node, get_left(node)) > 0) {
            // DBG("boohooo: %p", node);
            set_left(node, left_rotate(get_left(node)));
            // DBG("_2blake?");
            return right_rotate(node);
        }

        // DBG("3blake?");

        if (bal < -1 && cmp(new_node, get_right(node)) < 0) {
            set_right(node, right_rotate(get_right(node)));
            return left_rotate(node);
        }

        // DBG("4blake?");
        return node;
    }

    avl_ptr_t rec_remove(avl_ptr_t node, avl_ptr_t target, avl_ptr_t *to_remove) {
        if (!node)
            return avl_ptr_t{};
        int cmpv = cmp(target, node);
        if (cmpv < 0)
            set_left(node, rec_remove(get_left(node), target, to_remove));
        else if (cmpv > 0)
            set_right(node, rec_remove(get_right(node), target, to_remove));
        else {
            if (!get_left(node) && !get_right(node)) {
                *to_remove = node;
                node = avl_ptr_t{};
            }
            else if (!get_left(node) || !get_right(node)) {
                avl_ptr_t tmp = get_left(node) ? get_left(node) : get_right(node);
                *to_remove = node;
                node = tmp;
            }
            else {
                avl_ptr_t oth;
                avl_ptr_t next = _get_min(get_right(node));

                set_right(next, rec_remove(get_right(node), next, &oth));
                set_left(next, get_left(node));
                *to_remove = node;
                node = next;
            }
        }

        if (!node)
            return avl_ptr_t{};

        set_heigth(node, compute_height(node));
        return balance_remove(node);
    }

    avl_ptr_t balance_remove(avl_ptr_t node) {
        int bal = compute_bal(node);

        if (bal > 1 && compute_bal(get_left(node)) >= 0) {
            return right_rotate(node);
        }

        if (bal > 1 && compute_bal(get_left(node)) < 0) {
            set_left(node, left_rotate(get_left(node)));
            return right_rotate(node);
        }

        if (bal < -1 && compute_bal(get_right(node)) <= 0) {
            return left_rotate(node);
        }

        if (bal < -1 && compute_bal(get_right(node)) > 0) {
            set_right(node, right_rotate(get_right(node)));
            return left_rotate(node);
        }
        return node;
    }

    template <typename KeyCmp, typename KeyCtx>
    avl_ptr_t rec_find(avl_ptr_t node, KeyCmp key_cmp, const KeyCtx &key_ctx) {
        if (!node)
            return avl_ptr_t{};
        int cmpv = key_cmp(key_ctx, node);
        if (cmpv == 0)
            return node;
        else if (cmpv < 0)
            return rec_find(get_left(node), key_cmp, key_ctx);
        else
            return rec_find(get_right(node), key_cmp, key_ctx);
    }

    avl_ptr_t right_rotate (avl_ptr_t node) {
        avl_ptr_t left = get_left(node);

        set_left(node, get_right(left));
        set_right(left, node);

        set_heigth(node, compute_height(node));
        set_heigth(left, compute_height(left));

        return left;
    }

    avl_ptr_t left_rotate (avl_ptr_t node) {
        avl_ptr_t right = get_right(node);

        set_right(node, get_left(right));
        set_left(right, node);

        set_heigth(node, compute_height(node));
        set_heigth(right, compute_height(right));

        return right;
    }

    void rec_iter(avl_ptr_t node, avl_ptr_t par, int level, iter_cbk_t cbk, iter_ctx_t c) {
        if (node) {
            rec_iter(get_left(node), node, level + 1, cbk, c);
            cbk(node, par, level, c);
            rec_iter(get_right(node), node, level + 1, cbk, c);
        }
    }

    void rec_rev_iter(avl_ptr_t node, avl_ptr_t par, int level, iter_cbk_t cbk, iter_ctx_t c) {
        if (node) {
            rec_iter(get_right(node), node, level + 1, cbk, c);
            cbk(node, par, level, c);
            rec_iter(get_left(node), node, level + 1, cbk, c);
        }
    }

    avl_ptr_t   get_root() const                    { return ctx_t::o.get_root_fn(); }
    void        set_root(avl_ptr_t root)            { return ctx_t::o.set_root_fn(root); }
    int         cmp(avl_ptr_t a, avl_ptr_t b)       { return ctx_t::o.cmp_fn(a, b); }
    avl_ptr_t   get_left(avl_ptr_t n) const         { return ctx_t::o.get_left_fn(n); }
    void        set_left(avl_ptr_t n, avl_ptr_t l)  { ctx_t::o.set_left_fn(n, l); }
    avl_ptr_t   get_right(avl_ptr_t n) const        { return ctx_t::o.get_right_fn(n); }
    void        set_right(avl_ptr_t n, avl_ptr_t r) { ctx_t::o.set_right_fn(n, r); }
    int         get_heigth(avl_ptr_t n) const       { return ctx_t::o.get_height_fn(n); }
    void        set_heigth(avl_ptr_t n, int h)      { ctx_t::o.set_height_fn(n, h); }
    void        same_key(avl_ptr_t a, avl_ptr_t b)  { ctx_t::o.same_key_cbk(a, b); }
};

#endif
