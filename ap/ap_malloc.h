#ifndef AP_MALLOC_H
#define AP_MALLOC_H

#include <cstdint>

/* I hope this is the final version of my alocator. It is meant to be really easy to use for later
projects, in C++, The idea is that you will provide it with some functions with wich it can extend
it's space and he does the rest. The assumption is that the 'heap' only grows, maybe later I will
add buckets but I don't know about that. The allocator will have one important feature, it will
use only relative pointers. The address 0 will not be accesible so if the pointer is at 0 it means
this can return NULL. */

/* The idea is that all pointers that are stored inside the shared memory must be offsets, while
calculations are done outside of the shared memory they can stay as pointers. So use ap_off_t
inside your shared or saved structs and normal pointers in the rest of the program. Be aware of
pointer lifespan, as you need to take care of that yourself, as a rule of thumb: don't store
those pointers anywhere else besides in the shared memory or in imediate stack memory. */

using ap_off_t      = uint64_t;
using ap_sz_t       = uint64_t;
using ap_ctx_id_t   = uint64_t;

/* This function will get the new requested size as a parameter and will return the real allocated
size. It must alocate at least sz or return negative on error. */
using ap_add_mem_fn_t = int (*)(ap_sz_t sz);

struct ap_ctx_t {
    // The base of the region needs to be provided by the user. This is the memory that is used
    // by malloc to hold the data.
    // 
    // It is not private in case the user wants to relocate stuff between calls to ap_malloc*
    // functions. Changing this pointer will change the base for all the pointers allocated by the
    // malloc instance that is referenced by this ap_ctx_t.
    void *region;

    // this is the function that will be used inside malloc to add more continuous space to the
    // malloc region.
    ap_add_mem_fn_t add_mem_fn;

    // This is a number that you can assign to a malloc instance. If a region was not initialized,
    // it will set this number into the malloc header for that region. If the region was initialized,
    // this number will be overrided into this structure. Specialized data structures will use this number
    // to get the ap_ctx and as such vectors can rezide inside malloc-ed regions. If an id is not
    // provided on first init the time in microseconds will be used as an id (0 is not valid).
    ap_ctx_id_t ctx_id = 0;
};

/* The alocator will use a minimum requested size and will require the user to provide a function
for allocations. The memory inside region MUST be zero if the alocator was not initialized
before. sz must be aligned to 16 bytes */
int ap_malloc_init(ap_ctx_t *ctx, ap_sz_t sz);

/* returns the ap_ctx_t that is reflected by ctx_id */
ap_ctx_t *ap_malloc_get_ctx(ap_ctx_id_t ctx_id);

/* self evident */
ap_off_t ap_malloc_alloc(ap_ctx_t *ctx, ap_sz_t sz);
void ap_malloc_free(ap_ctx_t *ctx, ap_off_t ptr);

/* A special slot is held inside the malloc header. This slot is meant to be populated by an
aplication and it will hold a user provided number. For example it can hold the starting point of
the data, in this way if an application allocated some data, another application using this same
malloc would be able to access it as it would have an initial pointer to the said data. */
void ap_malloc_set_usr(ap_ctx_t *ctx, ap_off_t val);
ap_off_t ap_malloc_get_usr(ap_ctx_t *ctx);

/* transforms an ofset into a pointer or reverse */
void *ap_malloc_ptr(ap_ctx_t *ctx, ap_off_t off);
ap_off_t ap_malloc_off(ap_ctx_t *ctx, void *ptr);

void ap_malloc_dbg_print(ap_ctx_t *ctx);

#endif
