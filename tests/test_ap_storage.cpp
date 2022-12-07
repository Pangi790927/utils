#define AP_EXCEPT_STATIC_CBK
#include "ap_storage.h"
#include "ap_vector.h"
#include "debug.h"

#include <sys/wait.h>
#include <unistd.h>
#include <map>

#define SOCK_PATH "./ap_storage_test_comm"

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

static int run_program(const char *prog_name, const char *param) {
    int pid;
    ASSERT_FN(pid = fork());
    if (pid == 0) {
        DBG("Will exec: %s %s", prog_name, param);
        std::vector<const char *> argv = {prog_name, param, NULL};
        ASSERT_FN(execv(prog_name, (char* const*)argv.data()));
    }
    else {
        int status = 0;
        ASSERT_FN(waitpid(pid, &status, 0));
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            DBG("Guest program failed");
            return -1;
        }
    }
    return 0;
}

static int do_host_stuff(const char *prog_name) {
    int comm_fd;

    unlink(SOCK_PATH);
    ASSERT_FN(comm_fd = socket(AF_UNIX, SOCK_STREAM, 0));

    unlink("data/storage");
    unlink("data/storage_0.data");
    unlink("data/storage_1.data");

    DBG("######################### init_exit:");
    ASSERT_FN(run_program(prog_name, "init_exit"));
    ASSERT_FN(run_program(prog_name, "init_exit"));
    ASSERT_FN(unlink("data/storage"));
    ASSERT_FN(unlink("data/storage_0.data"));
    ASSERT_FN(unlink("data/storage_1.data"));

    DBG("######################### base_test:");
    ASSERT_FN(run_program(prog_name, "base_test"));
    ASSERT_FN(run_program(prog_name, "init_exit"));
    ASSERT_FN(unlink("data/storage"));
    ASSERT_FN(unlink("data/storage_0.data"));
    ASSERT_FN(unlink("data/storage_1.data"));
    return 0;
}

static int do_guest_stuff(const char *_param) {
    std::string param = _param;
    if (param == "init_exit") {
        ASSERT_FN(ap_storage_init("data/storage"));
        ASSERT_FN(ap_storage_submit_changes());
        ap_storage_uninit();
    }
    else if (param == "base_test") {
        ASSERT_FN(ap_storage_init("data/storage"));

        using vec_t = ap_vector_t<int>;
        vec_t &vec = *(vec_t *)ap_alloc(sizeof(vec_t));
        vec.init(ap_storage_get_mctx());

        for (int i = 0; i < 4096; i++)
            ASSERT_FN(vec.push_back(0xf1f1f1f1));

        ASSERT_FN(ap_storage_submit_changes());
        ap_storage_uninit();
    }
    return 0;
}

int main(int argc, char const *argv[]) {
    DBG_SCOPE();

    /* ap storage should be able to exit at all times and the data should be always consistent.
    Changes need to be saved using ap_storage_submit_changes */

    /* This db, if I can call it a db, works as follows: it creates a header file of 4Kb and two
    copies of the memory. The copies are filled in turn such that at least one of them is valid
    at any point in time. The header remembers the last valid copy such that it can recover from
    it in case of a malfunction of any kind. Both copies are mapped in virtual memory, the base
    one is always mapped and the backup is only mapped during a submit. */

    /* To test the storage we need to cover the following cases:
        1. creation of storage if no storage exists
        2. loading of storage if storage already exists
        3. having more than one page allocated
        4. having sparse pages modified
        5. data integrity should be checked
        6.* for all cases sudden sigkills should be tried and data integrity checked

        Those tests should be also done with the two init modes of vec, map, hmap, string
        a. manual init
        b. auto init

        Finally the tests should also be done using the different methods of error passing
    */

    /* On data integrity checking: Data is required to be in order, not necessarily complete,
    meaning that data between save points should be valid, not necessarily all data that was
    inserted. To test that we will mimic the behavior of ap_* objects with srd::* objects and
    remember the state at the beginning of each submit. We will denote the current submit by
    curr_sub and the submit before that as last_sub. We have the following cases:
        1. reload == curr_sub, means that the submit was successful and everything is ok
        2. reload == last_sub, means that the submit was not successful, but that is expected, so ok
        3. if the reload matches neither of the two then the program is known to be broken
    All tests will be executed without fault signals to ensure that on a normal run the program
    would behave properly.
    To avoid loosing a lot of time designing exact tests for each possible break point the guest
    program will auto send the signal based on the different parameters received and send the
    curr_sub and last_sub via unix_sockets to the host program.
    */

    /* TODO: add all the needed tests */
    /* TODO: implement communication channel with guest */

    if (argc != 1 && argc != 2) {
        DBG("This program must be manually executed without parameters, "
                "the parameters are for internal use");
        return -1;
    }
    if (argc == 1) {
        ASSERT_FN(do_host_stuff(argv[0]));
    }
    else {
        ASSERT_FN(do_guest_stuff(argv[1]));
    }

    return 0;
}
