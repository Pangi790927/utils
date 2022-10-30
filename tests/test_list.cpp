#include "glist.h"
#include "debug.h"
#include "misc_utils.h"

#define CHECK_FN_PRESENT_INIT(fn_name, sig)                                                        \
        using fn_name ## _sig_t = sig;                                                             \
        template<typename, typename T>                                                             \
        struct fn_name ## _check_t {                                                               \
            static_assert(std::integral_constant<T, false>::value,                                 \
                    "Second template parameter needs to be of function type.");                    \
        };                                                                                         \
        template<typename C, typename Ret, typename... Args>                                       \
        struct fn_name ## _check_t<C, Ret(Args...)> {                                              \
        private:                                                                                   \
            template<typename T>                                                                   \
            static constexpr auto check(T*)                                                        \
            -> typename                                                                            \
                std::is_same<                                                                      \
                    decltype( std::declval<T>().fn_name(std::declval<Args>()...)),                 \
                    Ret                                                                            \
                >::type;                                                                           \
            template<typename>                                                                     \
            static constexpr std::false_type check(...);                                           \
            typedef decltype(check<C>(0)) type;                                                    \
        public:                                                                                    \
            static constexpr bool value = type::value;                                             \
        };

CHECK_FN_PRESENT_INIT(push_front, void(int));
CHECK_FN_PRESENT_INIT(push_back, void(int));
CHECK_FN_PRESENT_INIT(pop_front, void(void));
CHECK_FN_PRESENT_INIT(pop_back, void(void));
CHECK_FN_PRESENT_INIT(insert_before, void(int, int));
CHECK_FN_PRESENT_INIT(insert_after, void(int, int));
CHECK_FN_PRESENT_INIT(remove, void(int));

#define CHECK_FN_PRESENT(T, fn_name) (fn_name ## _check_t<T, fn_name ## _sig_t>::value)

struct node_t {
    int id;
    int next;
    int prev;
};
static node_t nodes[64] = {0};

struct list_ctx_t {
    using ptr_t = int;

    int  get_next_fn(int n)        { return nodes[nodes[n].next].id; }
    void set_next_fn(int n, int r) { nodes[n].next = r; }
    int  get_prev_fn(int n)        { return nodes[nodes[n].prev].id; }
    void set_prev_fn(int n, int l) { nodes[n].prev = l; }
    int  get_first()               { return first; }
    int  get_last()                { return last; }
    void set_first(int n)          { first = n; }
    void set_last(int n)           { last = n; }

private:
    int first = 0;
    int last = 0;
};

using simple_list_t = generic_list_t<GLIST_FLAG_SIMPLE, list_ctx_t>;
using dlist_t = generic_list_t<GLIST_FLAG_DLIST | GLIST_FLAG_CIRC, list_ctx_t>;

constexpr int GL_D = GLIST_FLAG_DLIST;
constexpr int GL_L = GLIST_FLAG_LAST;
constexpr int GL_C = GLIST_FLAG_CIRC;
constexpr int GL_S = GLIST_FLAG_SLOW;

using list_t = generic_list_t<GLIST_FLAG_SIMPLE, int>;

using list_xxxx_t = generic_list_t< 0   | 0    | 0    | 0   , list_ctx_t>;
using list_xxxs_t = generic_list_t< 0   | 0    | 0    | GL_S, list_ctx_t>;
using list_xxcx_t = generic_list_t< 0   | 0    | GL_C | 0   , list_ctx_t>;
using list_xxcs_t = generic_list_t< 0   | 0    | GL_C | GL_S, list_ctx_t>;
using list_xlxx_t = generic_list_t< 0   | GL_L | 0    | 0   , list_ctx_t>;
using list_xlxs_t = generic_list_t< 0   | GL_L | 0    | GL_S, list_ctx_t>;
using list_xlcx_t = generic_list_t< 0   | GL_L | GL_C | 0   , list_ctx_t>;
using list_xlcs_t = generic_list_t< 0   | GL_L | GL_C | GL_S, list_ctx_t>;

using list_dxxx_t = generic_list_t<GL_D | 0    | 0    | 0   , list_ctx_t>;
using list_dxxs_t = generic_list_t<GL_D | 0    | 0    | GL_S, list_ctx_t>;
using list_dxcx_t = generic_list_t<GL_D | 0    | GL_C | 0   , list_ctx_t>;
using list_dxcs_t = generic_list_t<GL_D | 0    | GL_C | GL_S, list_ctx_t>;
using list_dlxx_t = generic_list_t<GL_D | GL_L | 0    | 0   , list_ctx_t>;
using list_dlxs_t = generic_list_t<GL_D | GL_L | 0    | GL_S, list_ctx_t>;
using list_dlcx_t = generic_list_t<GL_D | GL_L | GL_C | 0   , list_ctx_t>;
using list_dlcs_t = generic_list_t<GL_D | GL_L | GL_C | GL_S, list_ctx_t>;

enum {
    LIST_EXEC_NONE,
    LIST_EXEC_PUSH_BACK,
    LIST_EXEC_PUSH_FRONT,
    LIST_EXEC_POP_BACK,
    LIST_EXEC_POP_FRONT,
    LIST_EXEC_REMOVE,
    LIST_EXEC_INSERT_AFTER,
    LIST_EXEC_INSERT_BEFORE,
};

struct list_exec_t {
    int op;
    int node;
    int newn;
};

static list_exec_t popb = list_exec_t { .op = LIST_EXEC_POP_BACK };
static list_exec_t popf = list_exec_t { .op = LIST_EXEC_POP_FRONT };


static list_exec_t operator "" _pb (unsigned long long int newn) {
    return list_exec_t { .op = LIST_EXEC_PUSH_BACK, .newn = (int)newn };
}

static list_exec_t operator "" _pf (unsigned long long int newn) {
    return list_exec_t { .op = LIST_EXEC_PUSH_FRONT, .newn = (int)newn };
}

static list_exec_t operator "" _rm (unsigned long long int newn) {
    return list_exec_t { .op = LIST_EXEC_REMOVE, .newn = (int)newn };
}

static list_exec_t ia(int node, int newn) {   
    return list_exec_t { .op = LIST_EXEC_INSERT_AFTER, .node = node, .newn = newn };
}

static list_exec_t ib(int node, int newn) {   
    return list_exec_t { .op = LIST_EXEC_INSERT_BEFORE, .node = node, .newn = newn };
}

template <int flags, typename T>
static void print_list(generic_list_t<flags, T>& list) {
    std::string line1 = sformat("start %2d ::", list.front());
    std::string line2 = sformat("          ");
    auto curr = list.front();
    auto orig = curr;
    while (curr) {
        line1 += sformat("  %2d n--> %2d |", curr, list.next(curr));
        if constexpr (flags & GLIST_FLAG_DLIST)
            line2 += sformat("      p--> %2d ", list.prev(curr));
        curr = list.next(curr);
        if (curr == orig)
            break;
    }

    std::string ltype;
    if (!flags)
        ltype = "simple";
    if (flags & GLIST_FLAG_DLIST)
        ltype += "dlist|";
    if (flags & GLIST_FLAG_CIRC)
        ltype += "cicl|";
    if (flags & GLIST_FLAG_LAST)
        ltype += "last|";
    if (flags & GLIST_FLAG_SLOW)
        ltype += "slow|";

    if constexpr (flags & GLIST_FLAG_LAST || flags & GLIST_FLAG_SLOW)
       line1 += sformat(":: end %2d", list.back());
    if constexpr (flags & GLIST_FLAG_DLIST) {
        DBG("%s\n%s\n%s", ltype.c_str(), line1.c_str(), line2.c_str());
    }
    else {
        DBG("%s\n%s", ltype.c_str(), line1.c_str());
    }
}

template <typename LT>
static int test(std::vector<list_exec_t> ops, std::vector<int> expected) {
    DBG_SCOPE();
    LT list;

    std::string ops_str;
    for (auto op : ops) {
        bool op_executed = false;
        switch (op.op) {
            case LIST_EXEC_PUSH_FRONT:    {
                if constexpr (CHECK_FN_PRESENT(LT, push_front)) {
                    list.push_front(op.newn);
                    op_executed = true;
                }
                ops_str += sformat("%d_pf, ", op.newn);
                break;
            }
            case LIST_EXEC_PUSH_BACK:     {
                if constexpr (CHECK_FN_PRESENT(LT, push_back)) {
                    list.push_back(op.newn);
                    op_executed = true;
                }
                ops_str += sformat("%d_pb, ", op.newn);
                break;
            }
            case LIST_EXEC_POP_FRONT:     {
                if constexpr (CHECK_FN_PRESENT(LT, pop_front)) {
                    list.pop_front();        
                    op_executed = true;
                }
                ops_str += sformat("popf, ");
                break;
            }
            case LIST_EXEC_POP_BACK:      {
                if constexpr (CHECK_FN_PRESENT(LT, pop_back)) {
                    list.pop_back();
                    op_executed = true;
                }
                ops_str += sformat("popb, ");
                break;
            }
            case LIST_EXEC_REMOVE:        {
                if constexpr (CHECK_FN_PRESENT(LT, remove)) {
                    list.remove(op.newn);
                    op_executed = true;
                }
                ops_str += sformat("%d_rm, ", op.newn);
                break;
            }
            case LIST_EXEC_INSERT_AFTER:  {
                if constexpr (CHECK_FN_PRESENT(LT, insert_after)) {
                    list.insert_after(op.node, op.newn);
                    op_executed = true;
                }
                ops_str += sformat("ia(%d, %d), ", op.node, op.newn);
                break;
            }
            case LIST_EXEC_INSERT_BEFORE: {
                if constexpr (CHECK_FN_PRESENT(LT, insert_before)) {
                    list.insert_before(op.node, op.newn);
                    op_executed = true;
                }
                ops_str += sformat("ib(%d, %d), ", op.node, op.newn);
                break;
            }
        }
        if (!op_executed) {
            DBG("Function called but is not present: %d[partial: %s]", op.op, ops_str.c_str());
            return -1;
        }
        DBG("op_list: %s", ops_str.c_str());
        print_list(list);
    }

    DBG("op_list: %s", ops_str.c_str());
    print_list(list);

    if (!expected.size() && list.front()) {
        DBG("list should be empty");
        return -1;
    }

    auto curr = list.front();
    auto orig = curr;
    auto first = curr;
    auto last = curr;
    int i = 0;
    while (true) {
        if (!curr)
            break;
        if (i >= expected.size()) {
            DBG("Too many values in list, expected: %ld", expected.size());
            return -1;
        }
        if (expected[i] != curr) {
            DBG("Failed expected value on pos i:%d -> %d vs %d", i, expected[i], curr);
            return -1;
        }
        last = curr;
        curr = list.next(curr);
        i++;
        if (curr == orig)
            break;
    }
    if (i != expected.size()) {
        DBG("Too few values in list %d, expected: %ld", i, expected.size());
        return -1;
    }
    if (first != list.front()) {
        DBG("First mismatch %d vs %d", first, list.front());
        return -1;
    }
    if constexpr (LT::L) {
        if (last != list.back()) {
            DBG("Last mismatch %d vs %d", last, list.back());
            return -1;
        }
    }

    curr = list.front();
    orig = curr;
    first = curr;
    auto prev = last;
    i = 0;
    if constexpr (!LT::C)
        prev = 0;

    while (true) {
        if (!curr)
            break;
        if constexpr (LT::D)
            if (list.prev(curr) != prev) {
                DBG("Prev node is incorrect: %d vs %d", list.prev(curr), prev);
                return -1;
            }
        if (i != expected.size() - 1)
            if (list.next(curr) != expected[i + 1]) {
                DBG("Next node is incorrect: %d vs %d", list.next(curr), expected[i + 1]);
                return -1;
            }
        if (i == expected.size() - 1) {
            if constexpr (LT::C) {
                if (list.next(curr) != expected[0]) {
                    DBG("Failed last from circ list");
                    return -1;
                }
            }
            else {
                if (list.next(curr) != 0) {
                    DBG("Last node next not 0");
                    return -1;
                }
            }
        }
        prev = curr;
        curr = list.next(curr);
        i++;
        if (curr == orig)
            break;
    }

    return 0;
}

int main(int argc, char const *argv[])
{
    DBG_SCOPE();
    /* 0 is not a valid node */
    for (int i = 0; i < 128; i++)
        nodes[i].id = i;

    simple_list_t l1;

    l1.push_front(1);
    l1.push_front(3);
    l1.push_front(2);

    l1.pop_front();
    print_list(l1);

    simple_list_t l2;

    l2.push_front(2);
    l2.push_front(4);
    l2.push_front(5);

    print_list(l2);

    dlist_t d1;

    d1.push_front(3);
    d1.push_front(2);
    d1.push_front(1);

    print_list(d1);

    d1.remove(2);
    // d1.remove(1);
    d1.remove(3);
    print_list(d1);

    /* TODO: make all tests, to do that make some tests */

    /* ops on empty list: */
    ASSERT_FN(test<list_dlcs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dlcs_t>({1_pb}, {1}));

    /* ops on 1 list */
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, ib(1, 2)}, {2, 1}));

    /* ops on 2 list */
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlcs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));

    /* ops on 3 list */
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlcs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dlcx*/
    ASSERT_FN(test<list_dlcx_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dlcx_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlcx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlcx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dlxs*/
    ASSERT_FN(test<list_dlxs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dlxs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlxs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlxs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dlxx*/
    ASSERT_FN(test<list_dlxx_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dlxx_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dlxx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dlxx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dxcs*/
    ASSERT_FN(test<list_dxcs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dxcs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxcs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxcs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dxcx*/
    ASSERT_FN(test<list_dxcx_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dxcx_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxcx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxcx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dxxs*/
    ASSERT_FN(test<list_dxxs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_dxxs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxxs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxxs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*dxxx*/
    ASSERT_FN(test<list_dxxx_t>({1_pf}, {1}));
    // ASSERT_FN(test<list_dxxx_t>({1_pb}, {1}));
    // ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, popf}, {}));
    // ASSERT_FN(test<list_dxxx_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    // ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, popf}, {1}));
    // ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_dxxx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_dxxx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xlcs*/
    ASSERT_FN(test<list_xlcs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xlcs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xlcs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlcs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xlcx*/
    ASSERT_FN(test<list_xlcx_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xlcx_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, popf}, {}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, popb}, {}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, ia(1, 2)}, {1, 2}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, popf}, {1}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, popb}, {2}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, 1_rm}, {2}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xlcx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    // ASSERT_FN(test<list_xlcx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xlxs*/
    ASSERT_FN(test<list_xlxs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xlxs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xlxs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlxs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xlxx*/
    ASSERT_FN(test<list_xlxx_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xlxx_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, popf}, {}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, popb}, {}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, ia(1, 2)}, {1, 2}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, popf}, {1}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, popb}, {2}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, 1_rm}, {2}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xlxx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    // ASSERT_FN(test<list_xlxx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xxcs*/
    ASSERT_FN(test<list_xxcs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xxcs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, popf}, {1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, popb}, {2}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xxcs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xxcs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xxcx - circular on it's own doesn't work*/
    // ASSERT_FN(test<list_xxcx_t>({1_pf}, {1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pb}, {1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pb}, {1, 2}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf}, {2, 1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, popf}, {}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, popb}, {}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 1_rm}, {}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, ia(1, 2)}, {1, 2}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, ib(1, 2)}, {2, 1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, popf}, {1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, popb}, {2}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, 1_rm}, {2}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, 2_rm}, {1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xxcx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    // ASSERT_FN(test<list_xxcx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xxxs*/
    ASSERT_FN(test<list_xxxs_t>({1_pf}, {1}));
    ASSERT_FN(test<list_xxxs_t>({1_pb}, {1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, popf}, {}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, popb}, {}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, ia(1, 2)}, {1, 2}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xxxs_t>({2_pf, 1_pf, popf}, {2}));
    ASSERT_FN(test<list_xxxs_t>({2_pf, 1_pf, popb}, {1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, 1_rm}, {2}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    ASSERT_FN(test<list_xxxs_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xxxs_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));

    /*xxxs*/
    ASSERT_FN(test<list_xxxx_t>({1_pf}, {1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pb}, {1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pb}, {1, 2}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf}, {2, 1}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, popf}, {}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, popb}, {}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 1_rm}, {}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, ia(1, 2)}, {1, 2}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, ib(1, 2)}, {2, 1}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, 3_pf}, {3, 2, 1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, 3_pb}, {2, 1, 3}));
    ASSERT_FN(test<list_xxxx_t>({2_pf, 1_pf, popf}, {2}));
    // ASSERT_FN(test<list_xxxx_t>({2_pf, 1_pf, popb}, {1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, 1_rm}, {2}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, 2_rm}, {1}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, ia(1, 3)}, {2, 1, 3}));
    ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, ia(2, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, ib(1, 3)}, {2, 3, 1}));
    // ASSERT_FN(test<list_xxxx_t>({1_pf, 2_pf, ib(2, 3)}, {3, 2, 1}));
    ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, 4_pf}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, 4_pb}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, popb}, {1, 2}));
    ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, popf}, {2, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, 1_rm}, {2, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, 2_rm}, {1, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, 3_rm}, {1, 2}));
    ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ia(1, 4)}, {1, 4, 2, 3}));
    ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ia(2, 4)}, {1, 2, 4, 3}));
    ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ia(3, 4)}, {1, 2, 3, 4}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ib(1, 4)}, {4, 1, 2, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ib(2, 4)}, {1, 4, 2, 3}));
    // ASSERT_FN(test<list_xxxx_t>({3_pf, 2_pf, 1_pf, ib(3, 4)}, {1, 2, 4, 3}));


    return 0;
}
