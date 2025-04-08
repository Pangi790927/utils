#include "coro.h"

#include <string.h>
#include <thread>
#include <sys/un.h>

/* Test Utils
================================================================================================= */

/* Assert Corutine Function */
#define ASSERT_COFN(fn_call) do { \
    int res = (int)(fn_call); \
    if (res < 0) { \
        DBG("Assert[%s] FAILED[ret: %d, errno: %d[%s]]", #fn_call, res, errno, strerror(errno)); \
        co_return res; \
    } \
} while (0)

/* Assert Function */
#define ASSERT_FN(fn_call) do { \
    int res = (int)(fn_call); \
    if (res < 0) { \
        DBG("Assert[%s] FAILED[ret: %d, errno: %d[%s]]", #fn_call, res, errno, strerror(errno)); \
        return res; \
    } \
} while (0)

#define CHK_BOOL(val) ((val) ? 0 : -1)
#define CHK_PTR(val) ((val) != nullptr ? 0 : -1)

#define DBG(fmt, ...) co::dbg(__FILE__, __func__, __LINE__, fmt, ##__VA_ARGS__)

namespace co = coro;

/* Test1
================================================================================================= */

/* testing ping-pong usage of the semaphore */

bool test1_done = false;

co::task_t test1_co_even(co::sem_p a, co::sem_p b) {
    volatile int64_t i = 0;
    while (true) {
        co_await a->wait();
        if (i % 100'000L == 0)
            DBG("the coro: %ld", i);
        i = i + 1;
        b->signal();
        if (i > 1'000'000) {
            test1_done = true;
            break;
        }
    }
    co_return 0;
}

co::task_t test1_co_odd(co::sem_p a, co::sem_p b) {
    while (true) {
        co_await b->wait();
        if (test1_done)
            break;
        a->signal();
    }
    co_return 0;
}

int test1_semaphore() {
    auto pool = co::create_pool();
    auto a = co::create_sem(pool, 1);
    auto b = co::create_sem(pool, 0);

    pool->sched(test1_co_odd(a, b));
    pool->sched(test1_co_even(a, b));

    ASSERT_FN(pool->run());

    return 0;
}

/* Test2
================================================================================================= */

/* testing the multi-wait initialization of a semaphore */

int test2_counter = 0;
int test2_counter_after = 0;
int test2_counter_before = 0;

co::task_t test2_co_signaler1(co::sem_p a) {
    for (int i = 0; i < 50; i++) {
        a->signal();
        test2_counter++;
    }
    co_return 0;
}

co::task_t test2_co_signaler2(co::sem_p a) {
    for (int i = 0; i < 50; i++) {
        a->signal();
        test2_counter++;
    }
    co_return 0;
}

co::task_t test2_co_waiter(co::sem_p a) {
    test2_counter_before = test2_counter; /* we know that co_awaiter is executed first because
                                          it is the first that was scheduled */
    co_await a->wait();
    test2_counter_after = test2_counter;  /* observe that sem_t::signal does not suspend the
                                          corutine */
    co_return 0;
}

int test2_semaphore() {
    auto pool = co::create_pool();
    auto a = co::create_sem(pool, -100);

    pool->sched(test2_co_waiter(a));
    pool->sched(test2_co_signaler1(a));
    pool->sched(test2_co_signaler2(a));

    ASSERT_FN(pool->run());
    ASSERT_FN(CHK_BOOL(test2_counter_after == 100));
    ASSERT_FN(CHK_BOOL(test2_counter_before == 0));

    return 0;
}

/* Test3
================================================================================================= */

/* testing the  */
int test3_counter = 0;
int test3_counter_end = 0;

co::task_t test3_co_protect(co::sem_p a) {
    co_await a->wait();
    for (int i = 0; i < 3; i++) {
        test3_counter++;
        co_await co::yield();
    }
    test3_counter_end = test3_counter;
    a->signal();
}

co::task_t test3_co_access(co::sem_p a) {
    co_await a->wait();
    for (int i = 0; i < 3; i++) {
        test3_counter = 0;
        co_await co::yield();
    }
    a->signal();
}

int test3_semaphore() {
    auto pool = co::create_pool();

    /* the first one is going to be test3_co_protect, meaning it takes the lock, increment the
    value 3 times and finally the test3_co_access will set it back to 0*/
    auto a = co::create_sem(pool, 1);
    pool->sched(test3_co_protect(a));
    pool->sched(test3_co_access(a));

    ASSERT_FN(pool->run());
    ASSERT_FN(CHK_BOOL(test3_counter_end == 3));
    ASSERT_FN(CHK_BOOL(test3_counter == 0));

    /* this time both of them will have access to the resource, test3_co_access will be able to
    0 the variable each time */
    test3_counter_end = 0;
    a = co::create_sem(pool, 2);
    pool->sched(test3_co_protect(a));
    pool->sched(test3_co_access(a));

    ASSERT_FN(pool->run());
    ASSERT_FN(CHK_BOOL(test3_counter_end == 0));
    ASSERT_FN(CHK_BOOL(test3_counter == 0));

    return 0;
}

/* Test4
================================================================================================= */

int test4_counter_a = 0;
int test4_counter_b = 0;
int test4_counter_c = 0;
int test4_counter_sum = 0;

co::task_t test4_signal(co::sem_p a) {
    for (int i = 0; i < 6; i++) {
        a->signal();
        co_await co::yield();
        test4_counter_sum += test4_counter_a;
        test4_counter_sum += test4_counter_b;
        test4_counter_sum += test4_counter_c;
    }
    co_return 0;
}

co::task_t test4_wait_a(co::sem_p a) {
    co_await a->wait();
    test4_counter_a++;
    co_await a->wait();
    test4_counter_a++;
    co_return 0;
}

co::task_t test4_wait_b(co::sem_p a) {
    co_await a->wait();
    test4_counter_b++;
    co_await a->wait();
    test4_counter_b++;
    co_return 0;
}

co::task_t test4_wait_c(co::sem_p a) {
    co_await a->wait();
    test4_counter_c++;
    co_await a->wait();
    test4_counter_c++;
    co_return 0;
}

int test4_semaphore() {
    auto pool = co::create_pool();
    auto a = co::create_sem(pool, 0);

    pool->sched(test4_wait_a(a));
    pool->sched(test4_wait_b(a));
    pool->sched(test4_wait_c(a));
    pool->sched(test4_signal(a));

    /* 
    yield_num   a   b   c   sum
    1           1   0   0   1
    2           1   1   0   3
    3           1   1   1   6
    4           2   1   1   10
    5           2   2   1   15
    6           2   2   2   21
    */
    /* Obs: as you can see, the order is very clear */
    ASSERT_FN(pool->run());
    ASSERT_FN(CHK_BOOL(test4_counter_sum == 21));
    ASSERT_FN(CHK_BOOL(test4_counter_a == 2));
    ASSERT_FN(CHK_BOOL(test4_counter_b == 2));
    ASSERT_FN(CHK_BOOL(test4_counter_c == 2));

    return 0;
}

/* Test5
================================================================================================= */

int test5_counter = 0;
int test5_stopped = 0;
int test5_increment = 0;

co::task_t test5_co_stop() {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++)
            test5_counter++;
        co_await co::force_stop(i);
    }
    co_return 0;
}

int test5_stopping() {
    auto pool = co::create_pool();
    pool->sched(test5_co_stop());

    while (true) {
        auto ret = pool->run();
        test5_stopped++;
        test5_increment += pool->stopval;
        // pool->stopval = 0; // <- if the stopval is not reset, it keeps it's value 
        if (ret == co::RUN_OK)
            break;
        if (ret != co::RUN_STOPPED) {
            DBG("Something went wrong: %s", co::dbg_enum(ret).c_str());
            return -1;
        }
    }

    ASSERT_FN(CHK_BOOL(test5_stopped == 6));    /* 5 stops + 1 end */
    ASSERT_FN(CHK_BOOL(test5_counter == 25));   /* 5*5 == 25 */
    ASSERT_FN(CHK_BOOL(test5_increment == 14)); /* 0+1+2+3+4+4 the last one from not reseting stopval */

    return 0;
}

/* Test6
================================================================================================= */

int test6_num = 0;

co::task_t test6_co_sleep_1000us() {
    co_await co::sleep_us(1000);
    test6_num = 1;
    DBG("Done");
    co_return 0;
}

co::task_t test6_co_sleep_100ms() {
    co_await co::sleep_ms(100);
    test6_num *= 100;
    DBG("Done");
    co_return 0;
}

co::task_t test6_co_sleep_1s() {
    co_await co::sleep_s(1);
    test6_num += 5;
    DBG("Done");
    co_return 0;
}

int test6_sleeping() {
    auto pool = co::create_pool();
    pool->sched(test6_co_sleep_1s());
    pool->sched(test6_co_sleep_100ms());
    pool->sched(test6_co_sleep_1000us());

    ASSERT_FN(pool->run());
    ASSERT_FN(CHK_BOOL(test6_num == 105));
    return 0;
}

/* Test7
================================================================================================= */

int test7_destruct_cnt = 0;
std::string test7_prev = "";
struct test7_destruct_t {
    std::string curr;
    std::string prev;
    test7_destruct_t(std::string curr, std::string prev) : prev(prev), curr(curr) {}
    ~test7_destruct_t() {
        /* we make sure their order is clear and known */
        if (prev == test7_prev)
            test7_destruct_cnt++;
        test7_prev = curr;
    }
};

co::task_t test7_on_semaphore(co::sem_p sem, co::sem_p done){
    /* the semaphore wait dies first after timer because it is the first wait on the semaphore sem,
    so the first in queue */
    test7_destruct_t d("semaphore", "timer");
    done->signal();
    co_await sem->wait();
    co_return 0;
}

co::task_t test7_on_timer(co::sem_p done){
    /* The timer dies first because it is io, no previous */
    test7_destruct_t d("timer", "");
    done->signal();
    co_await co::sleep_ms(1000);
    co_return 0;
}

co::task_t test7_callee(co::sem_p sem, co::sem_p done) {
    /* calle is the second waiter on the semaphore sem, so he dies after semaphore */
    test7_destruct_t d("callee", "semaphore");
    done->signal();
    co_await sem->wait();
    co_return 0;
}

co::task_t test7_on_call(co::sem_p sem, co::sem_p done) {
    /* call is the caller of callee, so he dies right after callee */
    test7_destruct_t d("call", "callee");
    done->signal();
    co_await test7_callee(sem, done);
    co_return 0;
}

co::task_t test7_stopper(co::sem_p done) {
    /* stopper initiated the force_stop so he is in the ready_task queue, he will die last */
    test7_destruct_t d("stopper", "call");
    co_await done->wait();
    co_await co::force_stop();
    co_return 0;
}

int test7_clearing() {
    auto pool = co::create_pool();
    auto sem = co::create_sem(pool, 0);
    auto done = co::create_sem(pool, -4);

    pool->sched(test7_on_semaphore(sem, done));
    pool->sched(test7_on_timer(done));
    pool->sched(test7_on_call(sem, done));
    pool->sched(test7_stopper(done));

    auto ret = pool->run();
    DBG("ret: %s", co::dbg_enum(ret).c_str());
    ASSERT_FN(CHK_BOOL(ret == co::RUN_STOPPED));
    ASSERT_FN(CHK_BOOL(test7_destruct_cnt == 0));
    ASSERT_FN(pool->clear());
    ASSERT_FN(CHK_BOOL(test7_destruct_cnt == 5));
    return 0;
}

/* Test8
================================================================================================= */

int test8_server_fd;
int test8_pass_cnt = 0;
int test8_client_done = 0;
const char *test8_server_path = "./test-coro-8-server.socket";

co::task_t test8_server_conn(int fd) {
    uint32_t uints[4] = {0};
    ASSERT_COFN(co_await co::read_sz(fd, uints, sizeof(uints)));
    ASSERT_COFN(CHK_BOOL(uints[0] == 2 && uints[1] == (uint32_t)-15
            && uints[2] == 0xbadc0ffe && uints[3] == 41));
    for (int i = 0; i < 4; i++)
        if (uints[i] = i * 13 + 2);
            test8_pass_cnt++;

    ASSERT_COFN(co_await co::write_sz(fd, uints, sizeof(uints)));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::write_sz(fd, uints, sizeof(uints[0]) * 2));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::write_sz(fd, &uints[2], sizeof(uints[2]) * 2));

    test8_pass_cnt++;
    co_return 0;
}

co::task_t test8_server() {
    unlink(test8_server_path);
    ASSERT_COFN(test8_server_fd = socket(AF_UNIX, SOCK_STREAM, 0));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, test8_server_path);

    ASSERT_COFN(bind(test8_server_fd, (const struct sockaddr *)&addr,
            sizeof(struct sockaddr_un)));
    ASSERT_COFN(listen(test8_server_fd, 2));

    while (true) {
        int fd;
        fd = co_await co::accept(test8_server_fd, NULL, NULL);
        if (fd == co::ERROR_WAKEUP)
            break;
        ASSERT_COFN(fd);

        co_await co::sched(test8_server_conn(fd));
    }

    ASSERT_COFN(shutdown(test8_server_fd, SHUT_RDWR));
    close(test8_server_fd);

    test8_pass_cnt++;
    co_return 0;
}

co::task_t test8_client() {
    int fd;

    ASSERT_COFN(fd = socket(AF_UNIX, SOCK_STREAM, 0));

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, test8_server_path);

    int retest = 3;
    while (--retest >= 0) {
        int ret = co_await co::connect(fd, (struct sockaddr *)&addr,
                sizeof(struct sockaddr_un));
        if (ret < 0) {
            co_await co::sleep_ms(50);
            DBG("Failed connect...");
        }
    }
    ASSERT_COFN(retest);

    uint32_t uints1[] = { 2, (uint32_t)-15, 0xbadc0ffe, 41 };
    ASSERT_COFN(co_await co::write_sz(fd, uints1, sizeof(uints1)));

    uint32_t uints2[4] = {0};
    ASSERT_COFN(co_await co::read_sz(fd, uints2, sizeof(uints2)));
    ASSERT_COFN(CHK_BOOL(uints2[0] == 2 && uints2[1] == (uint32_t)-15
            && uints2[2] == 0xbadc0ffe && uints2[3] == 41));

    uint32_t uints3[4] = {0};
    ASSERT_COFN(co_await co::read_sz(fd, uints2, sizeof(uints2[0]) * 2));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::read_sz(fd, &uints2[2], sizeof(uints2[2]) * 2));
    co_await co::sleep_ms(10);
    ASSERT_COFN(CHK_BOOL(uints3[0] == 2 && uints3[1] == (uint32_t)-15
            && uints3[2] == 0xbadc0ffe && uints3[3] == 41));

    test8_client_done++;
    if (test8_client_done == 3) {
        co_await co::stopfd(test8_server_fd);
    }

    test8_pass_cnt++;
    co_return 0;
}

int test8_io() {
    auto pool = co::create_pool();
    pool->sched(test8_server());
    pool->sched(test8_client());
    pool->sched(test8_client());
    pool->sched(test8_client());
    co::run_e ret = pool->run();
    DBG("ret: %s", co::dbg_enum(ret).c_str());
    ASSERT_FN(CHK_BOOL(ret == co::RUN_STOPPED));
    ASSERT_FN(CHK_BOOL(test8_pass_cnt == 4));
    return 0;
}

/* Main:
================================================================================================= */

int main(int argc, char const *argv[]) {
    std::pair<std::function<int(void)>, std::string> tests[] = {
        { test1_semaphore, "test1_semaphore" },
        { test2_semaphore, "test2_semaphore" },
        { test3_semaphore, "test3_semaphore" },
        { test4_semaphore, "test4_semaphore" },
        { test5_stopping,  "test5_stopping" },
        { test6_sleeping,  "test6_sleeping" },
        { test7_clearing,  "test7_clearing" },
        { test8_io,        "test8_io"},
    };

    for (auto test : tests) {
        // DBG("[STARTED]: %s", test.second.c_str());
        int ret = test.first();
        if (ret >= 0)
            DBG("[+PASSED]: %s", test.second.c_str());
        else
            DBG("[-FAILED]: %s, ret", test.second.c_str(), ret);
    }
    return 0;
}
