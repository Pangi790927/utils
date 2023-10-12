#include "ap_storage.h"
#include "ap_vector.h"
#include "ap_string.h"
#include "debug.h"
#include "misc_utils.h"
#include "sys_utils.h"
#include "ap_ptr.h"
#include "co_utils.h"

#include <sys/wait.h>
#include <unistd.h>
#include <map>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCK_PATH "./ap_storage_test_comm"

void ap_storage_except_cbk(void *ctx, const char *errmsg, ap_except_info_t *ei) {
    DBG("[CRITICAL] %s\n%s", errmsg, ei->bt.c_str());
    exit(1);
}

static co::task_t run_program(const char *prog_name, std::vector<const char *> params, int *retv) {
    int pid = 0;
    ASSERT_COFN(pid = fork());

    if (pid == 0) {
        std::string to_exec = prog_name;
        std::vector<const char *> argv = {prog_name};
        for (auto p : params) {
            argv.push_back(p);
            to_exec += " ";
            to_exec += p;
        }
        argv.push_back(NULL);
        ASSERT_COFN(execv(prog_name, (char* const*)argv.data()));
    }
    else {
        while (true) {
            int status = 0;
            int ret = 0;
            ASSERT_COFN(ret = waitpid(pid, &status, WNOHANG));
            if (ret == 0)
                continue;
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                if (retv)
                    *retv = -WEXITSTATUS(status);
                co_return 0;
            }
            co_await co::sleep_ms(100);
        }
    }
    co_return 0;
}

static co::task_t cleanup() {
    ASSERT_ECOFN(co_await run_program("/usr/bin/ls", {"-l", "data"}, NULL));
    ASSERT_ECOFN(co_await run_program("/usr/bin/unlink", {"data/storage"}, NULL));
    ASSERT_ECOFN(co_await run_program("/usr/bin/unlink", {"data/storage_0.data"}, NULL));
    ASSERT_ECOFN(co_await run_program("/usr/bin/unlink", {"data/storage_1.data"}, NULL));
    co_return 0;
}

static co::task_t host_server() {
    DBG_SCOPE();
    /* this here will receive messages from the guest prograram */
    co_return 0;
}

static co::task_t do_host_stuff(const char *prog_name) {
    DBG_SCOPE();

    co_await co::sched(host_server());

    DBG("A");
    ASSERT_ECOFN(co_await cleanup());
    ASSERT_ECOFN(co_await run_program(prog_name, {"parameter"}, NULL));
    DBG("B");
    ASSERT_ECOFN(co_await cleanup());
    DBG("C");
    co_return 0;
}

static co::task_t do_guest_stuff(const char *_param) {
    DBG_SCOPE();
    std::string param = _param;
    DBG("Running TEST: %s", param.c_str());

    if (param == "init") {
        ASSERT_ECOFN(ap_storage_init("data/storage", ap_storage_except_cbk, NULL));
        ASSERT_ECOFN(ap_storage_do_changes(AP_STORAGE_COMMIT_CHANGES));

        // auto [off, ptr] = ap_storage_construct<test_ap_ptr_t>();
        // ap_malloc_set_usr(ap_static_ctx, off);

        ap_storage_uninit();
    }

    co_return 0;
}

int main(int argc, char const *argv[]) {
    DBG_SCOPE();
    co::pool_t pool;

    if (argc != 1 && argc != 2) {
        DBG("This program must be manually executed without parameters, "
                "the parameters are for internal use");
        return -1;
    }
    if (argc == 1) {
        pool.sched(do_host_stuff(argv[0]));
    }
    else {
        pool.sched(do_guest_stuff(argv[1]));
    }

    ASSERT_FN(pool.run());
    return 0;
}
