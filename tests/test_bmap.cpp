#include "gbitmap.h"
#include "debug.h"
#include <string>

#define BMAP_WC     128

using bmap_word_t = uint8_t;
struct bmap_ctx_t {
    using W = uint64_t;
    using I = size_t;
    using SZ = size_t;

    W    get_word_fn(I i) const { return arr[i]; }
    void set_word_fn(I i, W w)  { arr[i] = w; }
    SZ   get_sz_fn()      const { return sz; }
    void resize_fn(SZ newsz)    {
        if (newsz <= BMAP_WC)
            sz = newsz;
    }

private:
    bmap_word_t arr[BMAP_WC];
    uint64_t sz = 0;
};

template <typename T>
static void print_bmap(const T& bm) {
    std::string bits_str;
    for (auto b : bm)
        bits_str += b ? '1' : '0';
    DBG("bits: %s", bits_str.c_str());
}

int main(int argc, char const *argv[]) {
    DBG_SCOPE();

    using bmap_t = generic_bitmap_t<bmap_ctx_t>;
    bmap_t bmap;

    bmap.set(10, true);
    bmap.set(9, true);
    bmap.set(1, true);
    bmap.set(0, true);
    print_bmap(bmap);
    bmap.set(10, false);
    bmap.set(63, true);
    print_bmap(bmap);
    bmap.set(64, true);
    print_bmap(bmap);

    return 0;
}
