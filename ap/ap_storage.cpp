#include "ap_storage.h"
#include "ap_malloc.h"
#include "gbitmap.h"
#include "debug.h"
#include "misc_utils.h"

#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <filesystem>

#define CTRL_SZ             4096
#define STORAGE_MIN_SZ      4096
#define PAGE_SZ             4096
#define MAX_STORAGE_SPACE   (1ULL * 4 * 1024 * 1024 * 1024 * 1024)

#define STORAGE_MAGIC 0xa1ceface

struct storage_ctrl_t {
    uint32_t magic = STORAGE_MAGIC; /* shows that this ctrl is initialized */
    uint32_t flag_in_use;           /* if this is 1 at load, then data_used points to a corrupted
                                    data backup. While syncing the data_used will change and the
                                    other backup will be used. */
    uint32_t data_used;             /* 1 or 0 */
};

static_assert(sizeof(storage_ctrl_t) < CTRL_SZ);

/* TODO: check all the cases, really not sure if everything is ok(in this entire implementation) */

static uint64_t bmap_get_word(uint64_t i);
static void     bmap_set_word(uint64_t i, uint64_t w);
static uint64_t bmap_get_size();

struct mod_bmap_ctx_t {
    using W = uint64_t;
    using I = size_t;
    using SZ = size_t;

    W    get_word_fn(I i) const { return bmap_get_word(i); }
    void set_word_fn(I i, W w)  { bmap_set_word(i, w); }
    SZ   get_sz_fn()      const { return bmap_get_size(); }
    void resize_fn(SZ sz)       { ; }
};

using mod_bmap_t = generic_bitmap_t<mod_bmap_ctx_t>;

static struct sigaction old_sa;
static storage_ctrl_t *ctrl;
static int ctrl_fd;

static std::string storage_dir;
static std::string storage_name;
static std::string storage_data_path[2];

static int      storage_fd;
static int      backup_fd;
static uint64_t storage_sz;
static ap_ctx_t storage_ctx;
static uint64_t last_storage_sz;

static mod_bmap_t mod_bmap;
static std::vector<uint64_t> mod_bmap_data;

static uint64_t bmap_get_word(uint64_t i)               { return mod_bmap_data[i]; }
static void     bmap_set_word(uint64_t i, uint64_t w)   { mod_bmap_data[i] = w; }
static uint64_t bmap_get_size()                         { return mod_bmap_data.size(); }

static std::string base_name(std::string path) {
    return path.substr(path.find_last_of("/") + 1);
}

static std::string base_dir(std::string path) {
    return path.substr(0, path.find_last_of("/") + 1);
}

static int commit_mem_changes() {
    uint64_t page = mod_bmap.next_one(0);
    while (page != uint64_t(-1)) {
        /* write this page to file */
        void *page_addr = (uint8_t *)storage_ctx.region + page * PAGE_SZ;
        ASSERT_FN(msync(page_addr, PAGE_SZ, MS_SYNC));
        page = mod_bmap.next_one(page + 1);
    }
    return 0;
}

static int increase_storage_by(ap_sz_t sz) {
    /* we know the other storage_data is not mapped and we can't change it's data here, but we
    can make it larger from here */
    ASSERT_FN(commit_mem_changes());
    munmap(storage_ctx.region, storage_sz);
    storage_sz += sz;
    ASSERT_FN(ftruncate(storage_fd, storage_sz));
    ASSERT_FN(fsync(storage_fd));

    storage_ctx.region = mmap(storage_ctx.region, storage_sz, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_FIXED, storage_fd, 0);
    ASSERT_FN((intptr_t)storage_ctx.region);
    ASSERT_FN(mprotect(storage_ctx.region, storage_sz, PROT_READ));
    mod_bmap_data.resize(DIV_UP(storage_sz, PAGE_SZ));

    return 0;
}

static void mprot_handl(int sig, siginfo_t *si, void *uc) {
    if ((uint8_t *)si->si_addr < (uint8_t *)storage_ctx.region || (uint8_t *)si->si_addr >= 
            ((uint8_t *)storage_ctx.region + storage_sz))
    {
        DBG("broke at: %p", si->si_addr);
        /* not the sigsegv that we expected */
        if (old_sa.sa_flags & SA_SIGINFO)
            (*old_sa.sa_sigaction)(sig, si, uc);
        else 
            (*old_sa.sa_handler)(sig);
    }
    else {
        uint64_t page = uintptr_t((uint8_t *)si->si_addr - (uint8_t *)storage_ctx.region) / PAGE_SZ;
        void *addr0 = (void *)((uint8_t *)storage_ctx.region + page * PAGE_SZ);
        mod_bmap.set(page, true);
        if (mprotect(addr0, PAGE_SZ, PROT_READ | PROT_WRITE) < 0) {
            DBGE("Failed mprotect, wierd, but will kill the program");
            exit(-1);
        }
    }
}

int ap_storage_submit_changes(bool reverse_changes) {
    ASSERT_FN(commit_mem_changes());
    /* now all the data is stored in our current file, so we know the current file is valid and
    we are going to change the backup file */

    uint64_t backup_sz;

    if (!reverse_changes) {
        ctrl->data_used = !ctrl->data_used;
        ASSERT_FN(msync(ctrl, CTRL_SZ, MS_SYNC));

        /* now we also increase the size of the other file such that on submit_changes we will have
        enaugh space to submit the changes to the backup */
        ASSERT_FN(ftruncate(backup_fd, storage_sz));
        ASSERT_FN(fsync(backup_fd));
        backup_sz = storage_sz;
    }
    else {
        ASSERT_FN(ftruncate(storage_fd, last_storage_sz));
        ASSERT_FN(fsync(storage_fd));
        backup_sz = last_storage_sz;
    }

    void *oth_region = mmap(NULL, backup_sz, PROT_READ | PROT_WRITE, MAP_SHARED, backup_fd, 0);
    ASSERT_FN(intptr_t(oth_region));
    FnScope scope([&oth_region, &backup_sz]{ munmap(oth_region, backup_sz); });

    uint64_t page = mod_bmap.next_one(0);
    while (page != uint64_t(-1)) {
        /* write this page to file */
        if ((page + 1) * PAGE_SZ > backup_sz)
            break;
        void *page_addr = (uint8_t *)storage_ctx.region + page * PAGE_SZ;
        void *oth_paddr = (uint8_t *)oth_region + page * PAGE_SZ;
        if (!reverse_changes) {
            memcpy(oth_paddr, page_addr, PAGE_SZ);
            ASSERT_FN(msync(oth_paddr, PAGE_SZ, MS_SYNC));
        }
        else {
            /* this case is unsubmitting the changes, loading them from backup */
            memcpy(page_addr, oth_paddr, PAGE_SZ);
            ASSERT_FN(msync(page_addr, PAGE_SZ, MS_SYNC));
        }
        page = mod_bmap.next_one(page + 1);
    }

    mod_bmap_data.clear();
    mod_bmap_data.resize(DIV_UP(backup_sz, PAGE_SZ));

    /* switch back to the storage_data that we where using */
    if (!reverse_changes) {
        last_storage_sz = backup_sz;
        ctrl->data_used = !ctrl->data_used;
        ASSERT_FN(msync(ctrl, CTRL_SZ, MS_SYNC));
    }
    return 0;
}

int ap_storage_init(const char *ctrl_file) {
    if (PAGE_SZ != sysconf(_SC_PAGESIZE)) {
        DBG("Incorect page size: %ld", sysconf(_SC_PAGESIZE));
        return -1;
    }
    struct sigaction sa = {};

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = mprot_handl;
    ASSERT_FN(sigaction(SIGSEGV, &sa, &old_sa));

    int ctrl_fd;
    struct stat st_ctrl;
    ASSERT_FN(ctrl_fd = open(ctrl_file, O_RDWR | O_CREAT, 0666));
    FnScope err_scope([&ctrl_fd]{ close(ctrl_fd); });

    ASSERT_FN(ftruncate(ctrl_fd, CTRL_SZ));
    ASSERT_FN(fstat(ctrl_fd, &st_ctrl));
    ASSERT_FN(fsync(ctrl_fd));

    if (st_ctrl.st_size != CTRL_SZ) {
        DBG("Malformed ctrl block: size");
        return -1;
    }

    ASSERT_FN(ctrl = (storage_ctrl_t *)mmap(NULL, st_ctrl.st_size,
            PROT_READ | PROT_WRITE, MAP_SHARED, ctrl_fd, 0));
    err_scope([]{ munmap(ctrl, CTRL_SZ); });

    if (ctrl->magic != STORAGE_MAGIC && ctrl->magic != 0) {
        DBG("Malformed ctrl block: magic");
        return -1;
    }

    storage_dir = base_dir(ctrl_file);
    storage_name = base_name(ctrl_file);

    storage_data_path[0] = storage_dir + storage_name + "_0.data";
    storage_data_path[1] = storage_dir + storage_name + "_1.data";

    storage_ctx.region = mmap(0, MAX_STORAGE_SPACE, PROT_NONE, MAP_SHARED |
            MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    ASSERT_FN((intptr_t)storage_ctx.region);

    if (ctrl->magic != STORAGE_MAGIC) {
        DBG("Initializing new storage");
        memset(ctrl, 0, CTRL_SZ);
        ASSERT_FN(msync(ctrl, CTRL_SZ, MS_SYNC));

        int fd0;
        unlink(storage_data_path[0].c_str());
        ASSERT_FN(fd0 = open(storage_data_path[0].c_str(), O_RDWR | O_CREAT, 0666));
        FnScope scope([fd0]{ close(fd0); });

        ASSERT_FN(ftruncate(fd0, 0));
        ASSERT_FN(ftruncate(fd0, STORAGE_MIN_SZ));
        ASSERT_FN(fsync(fd0));

        ap_ctx_t tmpctx = {};
        tmpctx.region = mmap(NULL, STORAGE_MIN_SZ, PROT_READ | PROT_WRITE, MAP_SHARED, fd0, 0);
        ASSERT_FN((intptr_t)tmpctx.region);

        scope([&tmpctx]{ munmap(tmpctx.region, STORAGE_MIN_SZ); });

        ASSERT_FN(ap_malloc_init(&tmpctx, STORAGE_MIN_SZ));
        ASSERT_FN(msync(tmpctx.region, STORAGE_MIN_SZ, MS_SYNC));

        scope.call();

        unlink(storage_data_path[1].c_str());
        if (!std::filesystem::copy_file(storage_data_path[0], storage_data_path[1])) {
            DBGE("Can't create backup file");
            return -1;
        }

        /* at this point we have two malloc initialized data storages so we can write the magic
        inside the ctrl struct */
        ctrl->magic = STORAGE_MAGIC;
        ASSERT_FN(msync(ctrl, CTRL_SZ, MS_SYNC));
    }
    else {
        DBG("Storage was already initialized before");
    }

    if (ctrl->flag_in_use) {
        /* This means that ctrl->data_used was corrupted during the last sesion and we must restore
        it from the backup */
        unlink(storage_data_path[ctrl->data_used].c_str());
        if (!std::filesystem::copy_file(storage_data_path[!ctrl->data_used],
                storage_data_path[ctrl->data_used]))
        {
            DBGE("Can't copy from backup file");
            return -1;
        }
    }

    ctrl->flag_in_use = true;
    err_scope([]{ ctrl->flag_in_use = false; });

    /* now ctrl is initialized and both files hold the same data */
    ctrl->data_used = !ctrl->data_used;
    ASSERT_FN(storage_fd = open(storage_data_path[ctrl->data_used].c_str(), O_RDWR));
    err_scope([]{ close(storage_fd); });

    ASSERT_FN(backup_fd = open(storage_data_path[!ctrl->data_used].c_str(), O_RDWR));
    err_scope([]{ close(backup_fd); });

    struct stat st_data;
    ASSERT_FN(fstat(storage_fd, &st_data));
    storage_sz = st_data.st_size;
    last_storage_sz = storage_sz;

    storage_ctx.region = mmap(storage_ctx.region, storage_sz, PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_FIXED, storage_fd, 0);
    storage_ctx.add_mem_fn = increase_storage_by;

    DBG("Storage region %p storage_sz %ld", storage_ctx.region, storage_sz);
    ASSERT_FN((intptr_t)storage_ctx.region);

    ASSERT_FN(mprotect(storage_ctx.region, storage_sz, PROT_READ));
    mod_bmap_data.resize(DIV_UP(storage_sz, PAGE_SZ));

    ASSERT_FN(ap_malloc_init(&storage_ctx, storage_sz));

    err_scope.disable();
    return 0;
}

void ap_storage_uninit() {
    ap_storage_submit_changes(true);
    munmap(storage_ctx.region, MAX_STORAGE_SPACE);

    int ignres;

    ignres = ftruncate(storage_fd, last_storage_sz);
    close(storage_fd);

    ignres = ftruncate(backup_fd, last_storage_sz);
    close(backup_fd);

    (int)ignres;

    ctrl->flag_in_use = false;
    msync(ctrl, CTRL_SZ, MS_SYNC);
    munmap(ctrl, CTRL_SZ);
    close(ctrl_fd);
}

ap_ctx_t *ap_storage_get_mctx() {
    return &storage_ctx;
}
