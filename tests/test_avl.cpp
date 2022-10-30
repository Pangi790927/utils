#include "gavl.h"
#include "debug.h"
#include "misc_utils.h"

#include <set>

struct node_t {
    int val;

    int height = 0;
    node_t *left = NULL;
    node_t *right = NULL;
};

struct avl_ctx_t {
    using ptr_t = node_t *;

    node_t *    get_root_fn   ()                        { return root; }
    void        set_root_fn   (node_t *r)               { root = r; }

    /* cmp is as strcmp */
    int         cmp_fn        (node_t *a, node_t *b)    { return a->val - b->val; }
    node_t *    get_left_fn   (node_t *n)               {
        // DBG("n: %p", n);
        return n->left;
    }
    void        set_left_fn   (node_t *n, node_t *newn) { n->left = newn; }
    node_t *    get_right_fn  (node_t *n)               { return n->right; }
    void        set_right_fn  (node_t *n, node_t *newn) { n->right = newn; }
    int         get_height_fn (node_t *n)               { return n->height; }
    void        set_height_fn (node_t *n, int height)   { n->height = height; }
    void        same_key_cbk  (node_t *a, node_t *b)    { DBG("SAME KEY!: %d", b->val); }

private:
    node_t *root = nullptr;
};

enum {
    AVL_EXEC_INSERT = 0,
    AVL_EXEC_ERASE = 1,
};

struct avl_exec_t {
    int op;
    int key;

    bool operator < (const avl_exec_t& oth) {
        if (op < oth.op)
            return true;
        if (oth.op < op)
            return false;
        if (key < oth.key)
            return true;
        return false;
    }
};

static avl_exec_t operator "" _i (unsigned long long int key) {
    return avl_exec_t { .op = AVL_EXEC_INSERT, .key = (int)key };
}

static avl_exec_t operator "" _e (unsigned long long int key) {
    return avl_exec_t { .op = AVL_EXEC_ERASE, .key = (int)key };
}

using avl_t = generic_avl_t<avl_ctx_t>;

static int key_cmp_node(node_t *ctx, node_t *oth) {
    return avl_ctx_t{}.cmp_fn(ctx, oth);
}

static int key_cmp_val(int val, node_t *oth) {
    node_t val_node;
    val_node.val = val;
    return avl_ctx_t{}.cmp_fn(&val_node, oth);
}

// I think those cases should be enaugh:
//        1
//    1       1
//  1   1   1   1
// 1 1 1 1 1

static uint64_t get_perm_cnt(uint64_t sz) {
    uint64_t res = 1;
    for (int i = 1; i <= sz; i++)
        res *= i;
    return res;
}

static int do_test(const std::vector<avl_exec_t>& exec_vec) {
    std::set<int> ref;

     auto iter_fn = [](node_t *n, node_t *p, int lvl, void *_avl) {
        auto avl = (avl_t *)_avl;
        std::string pad(lvl, ' ');
        std::string npad(16 - lvl, ' ');
        auto succ = avl->get_succ(key_cmp_node, n);
        auto pred = avl->get_pred(key_cmp_node, n);

        DBG("%s|-> %4d[%2d] %s . succ[%s] pred[%s]", pad.c_str(), n->val, n->height, npad.c_str(),
                succ ? sformat("%d", succ->val).c_str() : "(none)",
                pred ? sformat("%d", pred->val).c_str() : "(none)");
    };

    avl_t avl;
    auto insert = [&](int val) {
        avl.insert(new node_t{ .val = val });
    };
    node_t key;
    auto remove = [&](int val) {
        node_t *to_rm = NULL;
        avl.remove(key_cmp_val, val, &to_rm);
        if (to_rm) {
            delete to_rm;
        }
    };

    std::string ops;
    for (auto &exec : exec_vec) {
        // std::string last_op;
        // if (exec.op == AVL_EXEC_INSERT) {
        //     last_op = sformat("%d_i, ", exec.key);
        //     ops += last_op;
        // }
        // else if (exec.op == AVL_EXEC_ERASE) {
        //     if (HAS(ref, exec.key)) {
        //         last_op = sformat("%d_e, ", exec.key);
        //         ops += last_op;
        //     }
        // }
        // DBG(">>>: %s", ops.c_str());
        // DBG("---: %s", last_op.c_str());
        if (exec.op == AVL_EXEC_INSERT) {
            insert(exec.key);
            ref.insert(exec.key);
        }
        else if (exec.op == AVL_EXEC_ERASE) {
            if (HAS(ref, exec.key)) {
                remove(exec.key);
                ref.erase(exec.key);
            }
            else {
                auto curr = avl.get_min();
                while (curr) {
                    remove(curr->val);
                    curr = avl.get_min();
                }
                return 0;
            }
        }
        // for (auto elem : ref)
        //     DBG("ref: %d", elem);
        // avl.iter(iter_fn, (void *)&avl);

        auto avl_it = avl.get_min();
        std::string elems;
        for (auto ref_it = ref.begin(); ref_it != ref.end(); ref_it++) {
            elems += sformat("{%d=%d}", *ref_it, avl_it->val);
            if (*ref_it != avl_it->val) {
                DBG("failed: %s", ops.c_str());
                DBG("expected: %s", elems.c_str());
                avl.iter(iter_fn, (void *)&avl);
                return -1;
            }
            avl_it = avl.get_succ(key_cmp_node, avl_it);
        }
    }

    auto curr = avl.get_min();
    while (curr) {
        remove(curr->val);
        curr = avl.get_min();
    }
    return 0;
}

static std::string get_perm_str(const std::vector<avl_exec_t>& exec_vec) {
    std::string ops;
    for (auto &exec : exec_vec) {
        std::string last_op;
        if (exec.op == AVL_EXEC_INSERT) {
            last_op = sformat("%d_i, ", exec.key);
            ops += last_op;
        }
        else if (exec.op == AVL_EXEC_ERASE) {
            last_op = sformat("%d_e, ", exec.key);
            ops += last_op;
        }
    }
    return ops;
}

int main(int argc, char const *argv[]) {
    setlocale(LC_ALL,"");
    DBG_SCOPE();

    // DBG("Start custom test");
    // std::vector<avl_exec_t> custom_test_vec {
    //     1_i, 2_i, 3_i, 4_i, 5_i,
    //     1_e, 2_e, 3_e, 4_e, 5_e,
    // };
    // ASSERT_FN(do_test(custom_test_vec));
    DBG("Done custom test");

    // std::vector<avl_exec_t> test_vec {
    //     1_i, 2_i, 3_i, 4_i, 5_i, 6_i,
    //     1_e, 2_e, 3_e, 4_e, 5_e, 6_e,
    // };

    std::vector<avl_exec_t> test_vec {
        // 1_i, 2_i, 3_i, 4_i, 5_i, 6_i, 7_i, 8_i, 9_i, 10_i, 11_i, 12_i,
        // 1_i, 11_i, 8_i, 5_i, 7_i, 6_i, 4_i, 2_i, 3_i, 9_i, 12_i, 10_i,
        // 1_i, 2_i, 3_i, 4_i, 5_i, 6_i, 7_i, 8_i, 9_i, 10_i
        1_i, 2_i, 3_i, 4_i, 5_i, 6_i, 7_i, 8_i, 9_i
    };
    auto sz = test_vec.size();
    uint64_t perm_cnt = get_perm_cnt(sz);
    for (uint64_t i = 0; i < perm_cnt; i++) {
        // DBG("====================================================================================");
        ASSERT_FN(do_test(test_vec));
        next_permutation(test_vec.begin(), test_vec.end());
        if (i % 10000 == 0)
            DBG("%'9ld/%'9ld %s", i, perm_cnt, get_perm_str(test_vec).c_str());
    }

    return 0;
}