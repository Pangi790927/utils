#define AP_EXCEPT_STATIC_CBK
#include "ap_storage.h"
#include "ap_vector.h"
#include "debug.h"

#include <unistd.h>
#include <map>

void ap_except_cbk(const char *errmsg, ap_except_info_t *ei) {
    DBG("[CRITICAL] %s\n%s", errmsg, ei->bt.c_str());
    exit(1);
}

static std::map<void *, ap_off_t> ptr_off;
static void *ap_alloc(ap_sz_t sz) {
    ap_ctx_t *ctx = ap_storage_get_mctx();
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
    ap_ctx_t *ctx = ap_storage_get_mctx();
    ap_malloc_free(ctx, off);
}

int main(int argc, char const *argv[]) {
    DBG_SCOPE();

    // unlink("storage");
    ASSERT_FN(ap_storage_init("data/storage"));

    using vec_t = ap_vector_t<int>;
    vec_t &vec = *(vec_t *)ap_alloc(sizeof(vec_t));
    vec.init(ap_storage_get_mctx());

    for (int i = 0; i < 4096; i++)
        ASSERT_FN(vec.push_back(0xf1f1f1f1));

    ASSERT_FN(ap_storage_submit_changes());
    ap_storage_uninit();

    return 0;
}