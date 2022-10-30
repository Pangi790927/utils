#include "ap_malloc.h"
#include <cstddef>
#include <map>
#include "debug.h"
#include "bit_utils.h"

#define TOTAL_MEM   (1024*1024)
#define INIT_MEM    (4096)

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

static void *p(ap_off_t off) {
    ap_ctx_t *ctx = &mc;
    auto ret = ap_malloc_ptr(ctx, off);
    if (!ret) {
        DBG("Deoffset failed");
        exit(-1);
    }
    if (ret < mem) {
        DBG("invalid ptr(<) : %p", ret);
        exit(-1);
    }
    if (ret >= mem + sizeof(mem)) {
        DBG("invalid ptr(>) : %p", ret);
        exit(-1);
    }
    return ret;
}

static std::map<ap_off_t, ap_sz_t> off_sz;
static ap_off_t alloc(ap_sz_t sz) {
    ap_ctx_t *ctx = &mc;
    auto ret = ap_malloc_alloc(ctx, sz);
    if (!ret) {
        DBG("alloc failed");
        exit(-1);
    }
    memset(p(ret), 0xff, sz);
    off_sz[ret] = sz;
    return ret;
}

static void free(ap_off_t off) {
    ap_ctx_t *ctx = &mc;
    memset(p(off), 0xff, off_sz[off]);
    off_sz.erase(off);
    ap_malloc_free(ctx, off);
}

static void tdbg() {
    ap_ctx_t *ctx = &mc;
    ap_malloc_dbg_print(ctx);
}

int main(int argc, char const *argv[])
{
    DBG_SCOPE();
    ap_ctx_t *ctx = &mc;
    ASSERT_FN(ap_malloc_init(ctx, INIT_MEM));

    tdbg();
    auto p0 = alloc(928);
    tdbg();
    free(p0);
    tdbg();
    auto p1 = alloc(1);
    auto p2 = alloc(10);
    tdbg();
    auto p3 = alloc(129);
    auto p4 = alloc(128);
    auto p5 = alloc(512);
    tdbg();
    auto p6 = alloc(1024);
    tdbg();
    auto p7 = alloc(1024);
    tdbg();
    auto p8 = alloc(1024);
    free(p3);
    tdbg();
    free(p4);
    tdbg();
    free(p6);
    tdbg();
    free(p8);
    tdbg();
    free(p7);

    auto p9 = alloc(288);
    auto p10 = alloc(65568);
    tdbg();
    auto p11 = alloc(65568);
    tdbg();
    free(p11);
    tdbg();
    free(p10);
    free(p9);
    tdbg();
    free(p1);
    free(p2);
    free(p5);
    tdbg();

    /* TODO: more tests */
    return 0;
}

