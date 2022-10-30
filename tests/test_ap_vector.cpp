#define AP_EXCEPT_STATIC_CBK

#include "ap_vector.h"
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

    using vec_t = ap_vector_t<int>;
    vec_t &vec = *(vec_t *)ap_alloc(sizeof(vec_t));
    vec.init(ctx);

    /* TODO: more tests */

    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);
    vec.push_back(6);
    vec.pop_back();
    vec.push_back(7);
    vec.push_back(8);
    vec.push_back(9);
    vec.push_back(10);
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }
    vec.clear();
    vec.push_back(101);
    vec.push_back(102);
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }
    DBG("vec[-1]: %d vec[-2]: %d", vec[-1], vec[-2]);
    std::vector<char> v = {1, 2, 3, 4, 5};
    vec.insert(vec.begin(), v.begin(), v.end());
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }
    vec.insert(vec.begin() + 2, 54321);
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }
    vec.insert(vec.end(), {-1, -2, -3, -4, -5});
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }
    vec.erase(vec.begin() + 3, vec.end() - 3);
    DBG("") for (auto &a : vec) { DBG("in vec: %d", a); }

    ap_free(&vec);

    return 0;
}