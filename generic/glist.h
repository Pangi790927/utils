#ifndef GLIST_H
#define GLIST_H

#include <type_traits>
#include "debug.h"

#define GLIST_FLAG_SIMPLE   0
#define GLIST_FLAG_LAST     1
#define GLIST_FLAG_DLIST    2
#define GLIST_FLAG_CIRC     4
#define GLIST_FLAG_SLOW     8

#define GLIST_D             (!!(flags & GLIST_FLAG_DLIST))
#define GLIST_L             (!!(flags & GLIST_FLAG_LAST))
#define GLIST_C             (!!(flags & GLIST_FLAG_CIRC))
#define GLIST_SLOW          (!!(flags & GLIST_FLAG_SLOW))

#define GLIST_XLCX          (!GLIST_D && !GLIST_SLOW && (GLIST_C || GLIST_L))
#define GLIST_XXCX          (!GLIST_D && !GLIST_SLOW && GLIST_C && !GLIST_L)
#define GLIST_XXXX          (!GLIST_D && !GLIST_SLOW && !GLIST_C && !GLIST_L)

#define GLIST_ENIF(c, T)    template <typename U = T> std::enable_if_t<c && std::is_same_v<T, U>, T>

/*
This is a generic list. The idea is to abstractize the generic list operations

s - O(N + len_concat) or slow
d - double linked
l - remembers last
c - circular

*/

namespace ap
{
    template <class...>
    constexpr std::false_type always_false{};

    template <typename T>
    concept generic_list_ctx_last_req = requires(T) {
        { T{}.get_last()                    } -> std::same_as<typename T::ptr_t>;
        { T{}.set_last(typename T::ptr_t{}) } -> std::same_as<void>;
    };

    template <typename T>
    concept generic_list_ctx_double_req = requires(T) {
        { T{}.get_prev_fn(typename T::ptr_t{}) } -> std::same_as<typename T::ptr_t>;

        { T{}.set_prev_fn(typename T::ptr_t{}, typename T::ptr_t{}) } -> std::same_as<void>;
    };

    template <typename T>
    concept generic_list_ctx_base_req = requires(T) {
        { sizeof(typename T::ptr_t)            } -> std::same_as<size_t>;
        { T{}.get_next_fn(typename T::ptr_t{}) } -> std::same_as<typename T::ptr_t>;
        { T{}.get_first()                      } -> std::same_as<typename T::ptr_t>;
        { T{}.set_first(typename T::ptr_t{})   } -> std::same_as<void>;

        { T{}.set_next_fn(typename T::ptr_t{}, typename T::ptr_t{}) } -> std::same_as<void>;
    };

    template <int flags, typename T>
    concept generic_list_ctx_req = true
            && generic_list_ctx_base_req<T>
            && (!GLIST_D || generic_list_ctx_double_req<T>)
            && (!GLIST_L || generic_list_ctx_last_req<T>);

    template <int flags, typename T> requires generic_list_ctx_req<flags, T>
    struct generic_list_ctx_wrap_t { T o; };

    inline void FIX_THE_SYNTAX_IN_ST4__LISTS (){ /* TODO: remove when fixed */ }

    struct list_ctx_example_t {
        struct node_t {
            node_t *next;
            node_t *prev;
        };

        using ptr_t = node_t *;

        node_t *get_next_fn(node_t *n)            { return n->next; }
        void    set_next_fn(node_t *n, node_t *r) { n->next = r; }
        node_t *get_prev_fn(node_t *n)            { return n->prev; }
        void    set_prev_fn(node_t *n, node_t *l) { n->prev = l; }
        node_t *get_first()                       { return first; }
        node_t *get_last()                        { return last; }
        void    set_first(node_t *n)              { first = n; }
        void    set_last(node_t *n)               { last = n; }

    private:
        node_t *first;
        node_t *last;
    };
}

template <int flags, typename list_ctx_t>
struct generic_list_t : public ap::generic_list_ctx_wrap_t<flags, list_ctx_t> {
    using list_ptr_t = typename list_ctx_t::ptr_t;
    using ctx_t = ap::generic_list_ctx_wrap_t<flags, list_ctx_t>;

    static constexpr bool SLOW = flags & GLIST_FLAG_SLOW;
    static constexpr bool D = flags & GLIST_FLAG_DLIST;
    static constexpr bool L = flags & GLIST_FLAG_LAST;
    static constexpr bool C = flags & GLIST_FLAG_CIRC;

    void insert_after(list_ptr_t node, list_ptr_t newn) {
        if (!node || !newn)
            return;
        list_ptr_t next = get_next(node);
        set_next(node, newn);
        set_next(newn, next);
        if constexpr (D) {
            set_prev(next, newn);
            set_prev(newn, node);
        }
        if constexpr (L) {
            if (get_last() == node)
                set_last(newn);
        }
    }

    GLIST_ENIF(!GLIST_XLCX && !GLIST_XXXX, void)
    insert_before(list_ptr_t node, list_ptr_t newn) {
        if (!node || !newn)
            return;
        if (node == get_first()) {
            push_front(newn);
            return ;
        }
        if constexpr (D) {
            list_ptr_t prev = get_prev(node);
            set_next(prev, newn);
            set_next(newn, node);
            set_prev(node, newn);
            set_prev(newn, prev);
            return ;
        }
        else if constexpr (SLOW) {
            list_ptr_t prevn = prev(node);
            set_next(prevn, newn);
            set_next(newn, node);
            return ;
        }
        else {
            static_assert(ap::always_false<generic_list_t>, "Can't get prev so can't pre-insert");
        }
    }

    GLIST_ENIF(GLIST_D, void)
    remove(list_ptr_t node) {
        if (!node)
            return ;

        if (remove_ends(node))
            return ;

        /* not at ends: */
        list_ptr_t prev = get_prev(node);
        list_ptr_t next = get_next(node);
        if (prev)
            set_next(prev, get_next(node));
        if (next)
            set_prev(next, get_prev(node));
        disable_node(node);
        return ;
    }

    GLIST_ENIF(!GLIST_D && !GLIST_XLCX && !GLIST_XXXX, void)
    remove(list_ptr_t node) {
        if (!node)
            return ;

        if (remove_ends(node))
            return ;

        list_ptr_t prevn = prev(node);
        list_ptr_t next = get_next(node);
        if (prevn)
            set_next(prevn, next);
        disable_node(node);
        return ;
    }

    list_ptr_t next(list_ptr_t node) {
        if (!node)
            return list_ptr_t{};
        return get_next(node);
    }

    GLIST_ENIF(GLIST_D, list_ptr_t)
    prev(list_ptr_t node) {
        if (!node)
            return list_ptr_t{};
        return get_prev(node);
    }

    GLIST_ENIF(!GLIST_D && GLIST_SLOW, list_ptr_t)
    prev(list_ptr_t node) {
        list_ptr_t ret = list_ptr_t{};
        list_ptr_t curr = get_first();
        while (curr) {
            ret = curr;
            curr = get_next(curr);
            if (curr == node)
                break;
        }
        if (curr == node)
            return ret;
        return list_ptr_t{};
    }

    list_ptr_t front() {
        return get_first();
    }

    GLIST_ENIF(GLIST_L, list_ptr_t)
    back() {
        return get_last();
    }

    GLIST_ENIF(GLIST_D && GLIST_C && !GLIST_L, list_ptr_t)
    back() {
        if (get_first())
            return get_prev(get_first());
        return list_ptr_t{};
    }

    /* TODO: those enable ifs are kinda random, should select better ones */
    GLIST_ENIF(!GLIST_L && !(GLIST_D && GLIST_C) && GLIST_SLOW, list_ptr_t)
    back() {
        list_ptr_t ret = list_ptr_t{};
        list_ptr_t curr = get_first();
        list_ptr_t orig;
        if constexpr (C)
            orig = curr;
        while (curr) {
            ret = curr;
            curr = next(curr);
            if constexpr (C)
                if (curr == orig)
                    break;
        }
        return ret;
    }

    GLIST_ENIF(!GLIST_XXCX, void)
    push_front(list_ptr_t node) {
        if (!get_first()) {
            push_first_node(node);
            return ;
        }
        set_next(node, get_first());
        if constexpr (D) {
            list_ptr_t prev = get_prev(get_first());
            if (prev)
                set_next(prev, node);
            set_prev(get_first(), node);
            set_prev(node, prev);
            set_first(node);
            return ;
        }
        else if constexpr (C) {
            if constexpr (L) {
                list_ptr_t prev = get_last();
                set_next(prev, node);
                set_first(node);
                return ;
            }
            else if constexpr (SLOW) {
                list_ptr_t prevn = prev(get_first());
                set_next(prevn, node);
                set_first(node);
                return ;
            }
            else {
                static_assert(ap::always_false<generic_list_t>, "Can't fix circ list");
            }
        }
        set_first(node);
        return ;
    }

    GLIST_ENIF(GLIST_L || (!GLIST_L && GLIST_SLOW) || (GLIST_C && GLIST_D), void)
    push_back(list_ptr_t node) {
        if (!get_first()) {
            push_first_node(node);
            return ;
        }
        insert_after(back(), node);
    }

    GLIST_ENIF(!GLIST_XXCX, void)
    pop_front() {
        remove_ends(front());
    }

    GLIST_ENIF((SLOW || (GLIST_C && GLIST_D)) || (GLIST_L && (GLIST_SLOW || GLIST_D)), void)
    pop_back() {
        if constexpr (L && (SLOW || D))
            remove_ends(back());
        else if constexpr (SLOW || (C && D)) {
            remove(back());
        }
    }

    list_ptr_t get_next  (list_ptr_t n)               { return ctx_t::o.get_next_fn(n); }
    void       set_next  (list_ptr_t n, list_ptr_t x) { ctx_t::o.set_next_fn(n, x); }
    list_ptr_t get_prev  (list_ptr_t n)               { return ctx_t::o.get_prev_fn(n); }
    void       set_prev  (list_ptr_t n, list_ptr_t p) { ctx_t::o.set_prev_fn(n, p); }
    list_ptr_t get_first()                            { return ctx_t::o.get_first(); }
    list_ptr_t get_last()                             { return ctx_t::o.get_last(); }
    void       set_first(list_ptr_t n)                { ctx_t::o.set_first(n); }
    void       set_last(list_ptr_t n)                 { ctx_t::o.set_last(n); }

private:
    void push_first_node(list_ptr_t node) {
        set_first(node);
        if constexpr (L)
            set_last(node);
        if constexpr (C) {
            set_next(node, node);
            if constexpr (D)
                set_prev(node, node);
        }
        else {
            set_next(node, list_ptr_t{});
            if constexpr (D)
                set_prev(node, list_ptr_t{});
        }
    }

    void disable_node(list_ptr_t node) {
        set_next(node, list_ptr_t{});
        set_prev(node, list_ptr_t{});
    }

    /* allways removes "first" on equality, removes "last" only if L */
    bool remove_ends(list_ptr_t node) {
        /* you could say it would be better to just crash but I disagree, this class could be used
        with set/get that don't crash on bad pointers so*/
        if (!node)
            return false;

        if constexpr (L) {
            if (get_first() == get_last() && get_first() == node) {
                set_first(list_ptr_t{});
                set_last(list_ptr_t{});
                disable_node(node);
                return true;
            }
        }

        if (remove_end0(node))
            return true;
        if constexpr (D || SLOW)
            if (remove_end1(node))
                return true;
        return false;
    }
    bool remove_end0(list_ptr_t node) {
        if (node == get_first()) {
            list_ptr_t next = get_next(node);
            if (next == node) {
                set_first(list_ptr_t{});
                disable_node(node);
                return true;
            }
            if constexpr (C) {
                if constexpr (D) {
                    list_ptr_t prev = get_prev(node);
                    set_next(prev, next);
                    set_prev(next, prev);
                }
                else if constexpr (L) {
                    set_next(get_last(), next);
                }
                else if constexpr (SLOW) {
                    list_ptr_t prevn = prev(node);
                    set_next(prevn, next);
                }
                else {
                    /* This means we can't remove the start */
                    static_assert(ap::always_false<generic_list_t>, "Can't remove the start");
                }
            }
            else if constexpr (D) {
                set_prev(next, list_ptr_t{});
            }
            set_first(next);
            disable_node(node);
            return true;
        }
        return false;
    }
    bool remove_end1(list_ptr_t node) {
        if constexpr (L) {
            if (node == get_last()) {
                list_ptr_t prevn;
                if constexpr (C) {                
                    if constexpr (D) {
                        prevn = get_prev(node);
                        list_ptr_t next = get_next(node);
                        set_next(prevn, next);
                        set_prev(next, prevn);
                    }
                    else if constexpr (SLOW) {
                        prevn = prev(node);
                        list_ptr_t next = get_next(node);
                        set_next(prevn, next);
                    }
                    else {
                        static_assert(ap::always_false<generic_list_t>, "Can't remove the end");
                    }
                }
                else if constexpr (D) {
                    prevn = get_prev(node);
                    set_next(prevn, list_ptr_t{});
                }
                else if constexpr (SLOW) {
                    prevn = prev(node);
                    set_next(prevn, list_ptr_t{});
                }
                else {
                    static_assert(ap::always_false<generic_list_t>, "Can't remove the end");
                }
                set_last(prevn);
                disable_node(node);
                return true;
            }
        }
        return false;
    }

};

#endif
