#include "ap_malloc.h"
#include "debug.h"
#include "misc_utils.h"
#include "bit_utils.h"
#include "time_utils.h"

#include <unordered_map>

#define AP_MALLOC_MAGIC     0xd0caffe
#define ALIGNMENT           16
#define PAGE_SZ             4096
#define SZ_DIV_ASK          (16 * PAGE_SZ)

#define MIN_MEM_SZ          sizeof(chunk_border_t)
#define BOREDR_SZ           ((sizeof(ap_sz_t) + sizeof(border_sz_t)))
#define INIT_OFF            (((sizeof(mem_hdr_t) + BOREDR_SZ) / 16 + 1) * 16 - BOREDR_SZ)
#define CB_ALIGN_OFF        (BOREDR_SZ % 16 ? (16 - BOREDR_SZ % 16) : 0)

#define PACKED_STRUCT       __attribute__((packed))

/* TODO: replace /16 %16 with >>4 &0xf */

struct PACKED_STRUCT border_sz_t {
    ap_sz_t is_free : 1;
    ap_sz_t is_node : 1; // this is for nodes in the avl tree(not implemented)
    ap_sz_t is_sbin : 1; // this bit is allways 0 except for small bins(not implemented)
    ap_sz_t reserve : 5;
    ap_sz_t sz      : ((sizeof(ap_sz_t) - 1) * 8);
};

/* TODO: create chunk borders that are nodes in an avl */
struct PACKED_STRUCT chunk_border_t {
    ap_sz_t     prev_sz;
    border_sz_t sz;
    ap_off_t    prev_free;
    ap_off_t    next_free;
};

struct mem_hdr_t {
    uint64_t null;         // nothing can point here
    uint64_t magic;

    ap_sz_t sz;
    ap_ctx_id_t ctx_id;

    ap_off_t usr_slot;

    /* TODO: solution - implement an avl */
    /* TODO: find a way to fix this, it can work really badly, meaning a lot of subsized free
    pieces can stand inside the list apriori of the searched node */
    uint64_t free_bmap;
    ap_off_t free_lists[64];
};

static_assert(sizeof(mem_hdr_t) < INIT_OFF);
static_assert(sizeof(chunk_border_t) % 16 == 0);

static std::unordered_map<ap_ctx_id_t, ap_ctx_t *> id_ctx_map;

static ap_ctx_id_t generate_ctx_id() {
    ap_ctx_id_t ret = (1ULL << (sizeof(ap_ctx_id_t) * 8 - 1));
    return ret + get_time_us();
}

static chunk_border_t *get_first_border(ap_ctx_t *ctx) {
    return (chunk_border_t *)((uint8_t *)ctx->region + INIT_OFF);
}

static chunk_border_t *get_last_border(ap_ctx_t *ctx) {
    auto hdr = (mem_hdr_t *)ctx->region;
    return (chunk_border_t *)((uint8_t *)ctx->region + hdr->sz - sizeof(chunk_border_t));
}

static void *cb2usr(chunk_border_t *cb) {
    return (void *)&cb->prev_free;
}

static chunk_border_t *usr2cb(void *usr) {
    return (chunk_border_t *)((uint8_t *)usr -
            sizeof(chunk_border_t::sz) - sizeof(chunk_border_t::prev_sz));
}

static chunk_border_t *next_border(chunk_border_t *cb) {
    return (chunk_border_t *)((uint8_t *)cb2usr(cb) + cb->sz.sz);
}

static chunk_border_t *prev_border(chunk_border_t *cb) {
    return usr2cb((uint8_t *)cb - cb->prev_sz);
}

static ap_off_t get_offset(ap_ctx_t *ctx, void *addr) {
    return (uint8_t *)addr - (uint8_t *)ctx->region;
}

static void *get_ptr(ap_ctx_t *ctx, ap_off_t off) {
    return (uint8_t *)ctx->region + off;
}

static void remove_from_free_list(ap_ctx_t *ctx, chunk_border_t *cb) {
    uint32_t l2 = log2_64(cb->sz.sz);
    auto hdr = (mem_hdr_t *)ctx->region;
    auto &first_free = hdr->free_lists[l2];

    // DBG("Remove %lx from %d", get_offset(ctx, cb), l2);

    cb->sz.is_free = false;
    if (cb->prev_free)
        ((chunk_border_t *)get_ptr(ctx, cb->prev_free))->next_free = cb->next_free;
    if (cb->next_free)
        ((chunk_border_t *)get_ptr(ctx, cb->next_free))->prev_free = cb->prev_free;
    if (get_offset(ctx, cb) == first_free) {
        first_free = cb->next_free;
        if (!first_free)
            hdr->free_bmap &= ~(1 << l2);
    }
}

static void add_to_free_list(ap_ctx_t *ctx, chunk_border_t *cb) {
    uint32_t l2 = log2_64(cb->sz.sz);
    auto hdr = (mem_hdr_t *)ctx->region;
    auto &first_free = hdr->free_lists[l2];
    hdr->free_bmap |= (1 << l2);

    // DBG("Insert %lx to %d", get_offset(ctx, cb), l2);

    if (first_free)
        ((chunk_border_t *)get_ptr(ctx, first_free))->prev_free = get_offset(ctx, cb);
    cb->next_free = first_free;
    cb->prev_free = 0;
    first_free = get_offset(ctx, cb);
    cb->sz.is_free = true;
}

static chunk_border_t *split_chunk(chunk_border_t *cb, ap_sz_t split_loc) {
    /* splits the current chunk into two chunks, the first one will remain the current chunk and
    the second one will be returned, split_loc must reside in the usr space of the old chunk and
    it will be the usr space of the new chunk */

    auto new_cb = usr2cb((uint8_t *)cb2usr(cb) + split_loc);
    auto next_cb = next_border(cb);

    if (split_loc < BOREDR_SZ)
        DBG("OOOPS");

    ap_sz_t old_sz1 = cb->sz.sz;
    ap_sz_t new_sz1 = split_loc - BOREDR_SZ;
    ap_sz_t new_sz2 = old_sz1 - split_loc;

    new_cb->sz.sz = new_sz2;
    next_cb->prev_sz = new_sz2;

    cb->sz.sz = new_sz1;
    new_cb->prev_sz = new_sz1;

    return new_cb;
}

/* we assume a, b are free, b is not the last chunk border */
static chunk_border_t *merge_chunks(ap_ctx_t *ctx, chunk_border_t *a, chunk_border_t *b) {
    auto next_cb = next_border(b);
    next_cb->prev_sz = a->sz.sz + BOREDR_SZ + b->sz.sz;
    a->sz.sz = next_cb->prev_sz;
    return a;
}

static ap_off_t try_alloc_in_free(ap_ctx_t *ctx, size_t sz) {
    auto hdr = (mem_hdr_t *)ctx->region;

    if (sz % 16 != 0)
        sz = (sz / 16 + 1) * 16;

    /* first we will try to look in the list of similar sizes  */
    uint32_t l2 = log2_64(sz);
    ap_off_t curr_free = hdr->free_lists[l2];
    bool found = false;

    while (curr_free) {
        auto cb = (chunk_border_t *)get_ptr(ctx, curr_free);
        if (cb->sz.sz >= sz)
            break;
        curr_free = cb->next_free;
    }

    /* if in this list we didn't find anything we must look in larger chunks */
    if (!curr_free) {
        l2 = log2_64(next_pow2(sz));
        uint64_t bmap = hdr->free_bmap >> l2;
        while (bmap) {
            if (bmap & 1)
                break;
            bmap >>= 1;
            l2++;
        }
        if (bmap)
            curr_free = hdr->free_lists[l2];
    }

    /* if we had no luck then we have failed to find enaugh space */
    if (!curr_free)
        return 0;

    /* else we can allocate */
    auto cb = (chunk_border_t *)get_ptr(ctx, curr_free);
    if (cb->sz.sz >= sz + CB_ALIGN_OFF + sizeof(chunk_border_t)) {
        /* In this case we must split the current border in two and apend one to the free
        list */
        ap_sz_t split_loc = sz + BOREDR_SZ + CB_ALIGN_OFF;
        remove_from_free_list(ctx, cb);
        auto new_cb = split_chunk(cb, split_loc);
        if (next_border(new_cb) != get_last_border(ctx) && next_border(new_cb)->sz.is_free) {
            remove_from_free_list(ctx, next_border(new_cb));
            merge_chunks(ctx, new_cb, next_border(new_cb));
        }
        add_to_free_list(ctx, new_cb);
        return get_offset(ctx, cb2usr(cb));
    }
    else {
        remove_from_free_list(ctx, cb);
        return get_offset(ctx, cb2usr(cb));
    }
    return 0;
}

static ap_off_t alloc_increasing_space(ap_ctx_t *ctx, size_t sz) {
    /* This means we have no more space in our current region so we must increase the space by at
    least sz. We will increase it by more than sz, more exactly by multiples of SZ_DIV_ASK */
    ap_sz_t ask_sz = sz + sizeof(chunk_border_t);
    if (ask_sz % SZ_DIV_ASK != 0)
        ask_sz = (ask_sz / SZ_DIV_ASK + 1) * SZ_DIV_ASK;

    /* we will ask for more space */
    if (ctx->add_mem_fn) {
        int ret = 0;
        if ((ret = ctx->add_mem_fn(ask_sz)) < 0) {
            DBG("Failed to add more memory!");
            return 0;
        }
    }
    else {
        DBG("Asking for memory is disabled!");
        return 0;
    }

    auto last_border = get_last_border(ctx);
    auto hdr = (mem_hdr_t *)ctx->region;
    hdr->sz += ask_sz;

    last_border->sz.sz = ask_sz - BOREDR_SZ;
    auto new_last_border = get_last_border(ctx);
    new_last_border->prev_sz = last_border->sz.sz;

    new_last_border->sz.is_free = false;

    add_to_free_list(ctx, last_border);

    /* now this should work */
    return try_alloc_in_free(ctx, sz);
}

static int register_ctx_id(ap_ctx_t *ctx, ap_ctx_id_t ctx_id) {
    id_ctx_map[ctx_id] = ctx;
    ctx->ctx_id = ctx_id;
    return 0;
}

int ap_malloc_init(ap_ctx_t *ctx, ap_sz_t sz) {
    auto hdr = (mem_hdr_t *)ctx->region;

    if (hdr->magic == AP_MALLOC_MAGIC) { /* It means that this region is already initialized */
        DBG("using existing malloc: ctx_id: %ld", hdr->ctx_id);
        ASSERT_FN(register_ctx_id(ctx, hdr->ctx_id));
        return 0;
    }

    ap_ctx_id_t desired_ctx_id = generate_ctx_id();
    if (ctx->ctx_id)
        desired_ctx_id = ctx->ctx_id;
    DBG("Registering ctx: %p as %ld", ctx, desired_ctx_id);
    ASSERT_FN(register_ctx_id(ctx, desired_ctx_id));
    FnScope err_scope([desired_ctx_id]{ id_ctx_map.erase(desired_ctx_id); });

    sz = (sz / 16) * 16;
    if (sz < INIT_OFF + 2 * sizeof(chunk_border_t)) {
        DBG("Failed to initialize allocator, must provide more initial space");
        return -1;
    }
    if ((intptr_t)ctx->region & 0xf) {
        DBG("Mem region must be aligned to 16 bytes");
        return -1;
    }

    // 4r453 - not sure what it means, but my cat wrote that

    hdr->magic = AP_MALLOC_MAGIC;
    hdr->ctx_id = ctx->ctx_id;

    hdr->sz = sz;
    hdr->free_bmap = 0;
    memset(hdr->free_lists, 0, sizeof(hdr->free_lists));

    auto border0 = get_first_border(ctx);
    auto usr0 = (uint8_t *)cb2usr(border0);
    auto end = (uint8_t *)ctx->region + hdr->sz;

    ap_sz_t first_sz = end - usr0 - sizeof(chunk_border_t);
    border0->sz.sz = first_sz;
    border0->prev_sz = 0;
    auto border1 = next_border(border0);
    border1->prev_sz = first_sz;
    border1->sz.sz = 0;
    border1->sz.is_free = false;
    border1->prev_free = 0;
    border1->next_free = 0;

    add_to_free_list(ctx, border0);
    err_scope.disable();
    return 0;
}

ap_ctx_t *ap_malloc_get_ctx(ap_ctx_id_t ctx_id) {
    if (!HAS(id_ctx_map, ctx_id)) {
        DBG("This context was not registered: %ld", ctx_id);
        return NULL;
    }
    return id_ctx_map[ctx_id];
}

void ap_malloc_set_usr(ap_ctx_t *ctx, ap_off_t val) {
    auto hdr = (mem_hdr_t *)ctx->region;
    hdr->usr_slot = val;
}

ap_off_t ap_malloc_get_usr(ap_ctx_t *ctx) {
    auto hdr = (mem_hdr_t *)ctx->region;
    return hdr->usr_slot;
}

void *ap_malloc_ptr(ap_ctx_t *ctx, ap_off_t off) {
    if (off == 0)
        return NULL;
    return (uint8_t *)ctx->region + off;
}

ap_off_t ap_malloc_off(ap_ctx_t *ctx, void *ptr) {
    if (!ptr)
        return 0;
    return ap_off_t((uint8_t *)ptr - (uint8_t *)ctx->region);
}

ap_off_t ap_malloc_alloc(ap_ctx_t *ctx, size_t sz) {
    if (!sz)
        return 0;
    ap_off_t ret = 0;
    if ((ret = try_alloc_in_free(ctx, sz)))
        return ret;
    return alloc_increasing_space(ctx, sz);
}

void ap_malloc_free(ap_ctx_t *ctx, ap_off_t ptroff) {
    /* a user gives us a pointer and we must free it, in case the boundry that gets freed
    is adjiacent to another free boundry, we can merge it to reduce fragmentation */
    auto hdr = (mem_hdr_t *)ctx->region;
    auto cb = usr2cb(ap_malloc_ptr(ctx, ptroff));
    if (cb->sz.is_free) {
        DBG("Double free");
        return ;
    }
    if (cb != get_last_border(ctx) && next_border(cb)->sz.is_free) {
        remove_from_free_list(ctx, next_border(cb));
        merge_chunks(ctx, cb, next_border(cb));
    }
    if (cb != get_first_border(ctx) && prev_border(cb)->sz.is_free) {
        remove_from_free_list(ctx, prev_border(cb));
        cb = merge_chunks(ctx, prev_border(cb), cb);
    }
    add_to_free_list(ctx, cb);
}

void ap_malloc_dbg_print(ap_ctx_t *ctx) {
    auto hdr = (mem_hdr_t *)ctx->region;
    std::string free_list_str;
    for (int i = 0; i < sizeof(hdr->free_lists) / sizeof(hdr->free_lists[0]); i++) {
        free_list_str += sformat("[%2d:%16lx]", i, hdr->free_lists[i]);
        if (i % 8 == 0)
            free_list_str += "\n";
    }

    DBG("AP_MALLOC:");
    DBG("\tregion:     %p" , ctx->region);
    DBG("\tsz:         %ld", (int64_t)hdr->sz);
    DBG("\tadd_mem():  %p" , ctx->add_mem_fn);
    DBG("\tfree_bmap:  %lx", (uint64_t)hdr->free_bmap);
    // DBG("\tfree_str:   %s" , free_list_str.c_str());

    auto cb = get_first_border(ctx);
    auto last_border = get_last_border(ctx);
    while (cb != last_border) {
        std::string offsets = sformat("%lx/%lx", get_offset(ctx, cb), get_offset(ctx, cb2usr(cb)));
        DBG("[CB] off/usr: %16s sz: %8ld prev_sz: %8ld is_free: %1d prev_free: %16lx next_free: %16lx",
                offsets.c_str(), cb->sz.sz, cb->prev_sz, cb->sz.is_free, cb->prev_free, cb->next_free);
        cb = next_border(cb);
    }
    DBG("\\AP_MALLOC");
}
