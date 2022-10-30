#define AP_EXCEPT_STATIC_CBK

#include "ap_hashmap.h"
#include "debug.h"

#include <map>

#define TOTAL_MEM   (1024*1024)
#define INIT_MEM    (4096)

void ap_except_cbk(const char *errmsg, ap_except_info_t *ei) {
    DBG("[CRITICAL] %s\n%s", errmsg, ei->bt.c_str());
    exit(1);
}

static int add_mem_fn(ap_sz_t sz);

static uint8_t mem[TOTAL_MEM];
static ap_ctx_t mc = {
    .region = mem,
    .add_mem_fn = add_mem_fn
};

static ap_sz_t tot_mem = INIT_MEM;
static int add_mem_fn(ap_sz_t sz) {
    tot_mem += sz;
    if (tot_mem > TOTAL_MEM)
        return -1;
    return 0;
}

static std::map<void *, ap_off_t> ptr_off;
static void *ap_alloc(ap_sz_t sz) {
    ap_ctx_t *ctx = &mc;
    auto off = ap_malloc_alloc(ctx, sz);
    if (!off) {
        DBG("alloc failed");
        exit(-1);
    }
    void *p = ap_malloc_ptr(ctx, off);
    ptr_off[p] = off;
    return p;
}

static void ap_free(void *p) {
    auto off = ptr_off[p];
    ap_ctx_t *ctx = &mc;
    ap_malloc_free(ctx, off);
}

int main(int argc, char const *argv[])
{
    DBG_SCOPE();
    ap_ctx_t *ctx = &mc;
    ASSERT_FN(ap_malloc_init(ctx, INIT_MEM));

    using hmap_t = ap_hashmap_t<int, int>;
    hmap_t &hmap = *(hmap_t *)ap_alloc(sizeof(hmap_t));
    hmap.init(ctx);

    auto n = hmap.alloc_hnode();
    auto node = hmap.deref_hnode(n);
    node->key = 1;
    node->val = 2;
    hmap.insert(n);

    hmap.iter(NULL, [](hmap_t::hmap_node_t *node, void *) {
        DBG("key: %d val: %d", node->key, node->val);
    });

    hmap.erase(1);
    hmap.free_hnode(n);
    DBG("!!");

    hmap.iter(NULL, [](hmap_t::hmap_node_t *node, void *) {
        DBG("key: %d val: %d", node->key, node->val);
    });

    for (int i = 0; i < 64; i++) {
        auto n = hmap.alloc_hnode();
        auto node = hmap.deref_hnode(n);
        node->key = i;
        node->val = i * 2;
        hmap.insert(n);
    }

    DBG("!!");

    hmap.iter(NULL, [](hmap_t::hmap_node_t *node, void *) {
        DBG("key: %d val: %d", node->key, node->val);
    });

    ap_free(&hmap);

    return 0;
}