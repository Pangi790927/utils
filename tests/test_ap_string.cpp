#define AP_EXCEPT_STATIC_CBK
#include "ap_string.h"
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

    using str_t = ap_string_t;
    str_t &str = *(str_t *)ap_alloc(sizeof(str_t));
    str.init(ctx);

    DBG("str: %s", str.c_str());
    str = "Ana are mere";
    DBG("str: %s", str.c_str());
    str += ", multe mere";
    DBG("str: %s", str.c_str());

    ap_free(&str);

	return 0;
}