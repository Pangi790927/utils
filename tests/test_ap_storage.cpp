#include "ap_storage.h"
#include "ap_vector.h"
#include "ap_string.h"
#include "debug.h"
#include "misc_utils.h"
#include "sys_utils.h"

#include <sys/wait.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./ap_storage_test_comm"

struct test_struct_t {
    ap_vector_t<int> vec;
    ap_string_t str;
};

void ap_storage_except_cbk(void *ctx, const char *errmsg, ap_except_info_t *ei) {
    DBG("[CRITICAL] %s\n%s", errmsg, ei->bt.c_str());
    exit(1);
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
    ASSERT_FN(run_program(prog_name, "read_test"));
    ASSERT_FN(unlink("data/storage"));
    ASSERT_FN(unlink("data/storage_0.data"));
    ASSERT_FN(unlink("data/storage_1.data"));

    DBG("UNINIT");

    return 0;
}

static int do_guest_stuff(const char *_param) {
    std::string param = _param;
    if (param == "init_exit") {
        DBG("Start INIT_EXIT");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
        DBG("Done INIT_EXIT");
    }
    else if (param == "base_test") {
        DBG("Start BASE_TEST");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        DBG("Will allocate vector on storage");

        {
            /* OBS: Don't use ap_vector_t before/after initing/uniniting the ap_storage, because you
            will get a strange segfault */
            ap_vector_t<int> vec;

            for (int i = 0; i < 4096; i++) {
                ASSERT_FN(vec.push_back(0xf1f1f1f1));
            }
        }

        ap_off_t off;
        ASSERT_FN(CHK_BOOL(off = ap_malloc_alloc(ap_static_ctx, sizeof(test_struct_t))));
        test_struct_t *test_struct = (test_struct_t *)ap_malloc_ptr(ap_static_ctx, off);
        new (test_struct) test_struct_t;
        ap_malloc_set_usr(ap_static_ctx, off);

        test_struct->vec.push_back(1);
        test_struct->vec.push_back(2);
        test_struct->vec.push_back(4);
        test_struct->vec.push_back(8);
        test_struct->vec.push_back(16);
        test_struct->vec.push_back(32);
        test_struct->vec.push_back(64);
        test_struct->vec.push_back(128);
        test_struct->str = "Ana are mere";

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        test_struct->vec.push_back(256);
        test_struct->vec.push_back(512);
        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES)); /* TODO: fix, second commit does nothing */
        // test_struct->vec.push_back(1023);
        // test_struct->vec.push_back(2047);
        // ASSERT_FN(ap_storage_do_changes(AP_STORAGE_REVERT_CHANGES));
        // test_struct->vec.push_back(1024);
        // test_struct->vec.push_back(2048);
        // ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        // test_struct->vec.push_back(777);
        // test_struct->vec.push_back(888);

        ap_storage_uninit();
        DBG("Done BASE_TEST");
    }
    else if (param == "read_test") {
        DBG("Start READ_TEST");
        ASSERT_FN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        DBG("Will read storage");

        {
            ap_off_t off = ap_malloc_get_usr(ap_static_ctx);
            test_struct_t *test_struct = (test_struct_t *)ap_malloc_ptr(ap_static_ctx, off);

            DBG("str: %s", test_struct->str.c_str());
            for (auto i : test_struct->vec)
                DBG("%d", i);
        }

        ASSERT_FN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));
        ap_storage_uninit();
        DBG("Done READ_TEST");
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
