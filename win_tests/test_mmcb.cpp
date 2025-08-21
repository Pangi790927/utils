#define MMCB_DEBUG_ENABLE_LOGGING

#include "mmcb.h"

int main(int argc, char const *argv[])
{

    mmcb_t mmcb;

    size_t sz = 4096*20;
    // mmcb.init(sz);
    mmcb.init(sz, MMCB_FLAG_TRICOPY);

    char *base = (char *)mmcb.get_base();
    char *first_zone = base - sz;
    char *second_zone = base;
    char *third_zone = base + sz;

    memset(second_zone, 0, sz * 2);

    strcpy(second_zone, "This is a string that should be in multiple places");
    printf("%s\n", first_zone);
    printf("%s\n", second_zone);
    printf("%s\n", third_zone);


    mmcb.uninit();

    MMCB_DEBUG("DONE");

    /* code */
    return 0;
}