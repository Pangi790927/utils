#define CORO_ENABLE_DEBUG_NAMES true

#include "coro.h"

#include <string.h>
#include <stdexcept>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>

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

struct FnScope {
    using fn_t = std::function<void(void)>;
    std::vector<fn_t> fns;
    bool done = false;

    FnScope(fn_t fn) {
        add(fn);
    }

    FnScope() {}

    ~FnScope() {
        call();
    }

    void operator() (fn_t fn) {
        add(fn);
    }

    void add(fn_t fn) {
        fns.push_back(fn);
    }

    void disable() {
        fns.clear();
    }

    void call() {
        for (auto &f : fns)
            f();
        fns.clear();
    }
};

/* Test1 - Semaphores
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

/* Test2 - Semaphores
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

/* Test3 - Semaphores
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

/* Test4 - Semaphores
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

/* Test5 - Force Stop
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

/* Test6 - Sleep
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

/* Test7 - Clear
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

/* Test8 - IO
================================================================================================= */

int test8_server_fd;
int test8_pass_cnt = 0;
int test8_client_done = 0;
const char *test8_local_ip = "127.0.0.1";
const int test8_port = 3000;

static void test8_set_msg(uint32_t uints[4]) {
    uints[0] = 2;
    uints[1] = uint32_t(-15);
    uints[2] = 0xbadc0ffe;
    uints[3] = 41;
}

static int test8_chk_msg(uint32_t uints[4]) {
    if (!(uints[0] == 2 && uints[1] == uint32_t(-15) && uints[2] == 0xbadc0ffe && uints[3] == 41)) {
        DBG("WRONG MESSAGE: %u %d 0x%x %u", uints[0], uints[1], uints[2], uints[3]);
        return -1;
    }
    return 0;
}

co::task_t test8_server_conn(int fd) {
    uint32_t uints[4] = {0};
    FnScope scope([fd]{ close(fd); });

    ASSERT_COFN(co_await co::read_sz(fd, uints, sizeof(uints)));
    test8_set_msg(uints);

    ASSERT_COFN(co_await co::write_sz(fd, uints, sizeof(uints)));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::write_sz(fd, uints, sizeof(uints[0]) * 3));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::write_sz(fd, &uints[3], sizeof(uints[3]) * 1));

    test8_pass_cnt++;
    co_return 0;
}

co::task_t test8_server() {
    ASSERT_COFN(test8_server_fd = socket(AF_INET, SOCK_STREAM, 0));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(test8_port);
    addr.sin_addr.s_addr = inet_addr(test8_local_ip);

    const int enable = 1;
    ASSERT_COFN(setsockopt(test8_server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)));

    ASSERT_COFN(bind(test8_server_fd, (const struct sockaddr *)&addr,
            sizeof(struct sockaddr_in)));
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

    co::pool_t *pool = co_await co::get_pool();
    FnScope scope([pool]{
        test8_client_done++;
        if (test8_client_done == 3)
            pool->stop_fd(test8_server_fd);
    });

    ASSERT_COFN(fd = socket(AF_INET, SOCK_STREAM, 0));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(test8_port);
    addr.sin_addr.s_addr = inet_addr(test8_local_ip);

    int retest = 3;
    while (--retest >= 0) {
        int ret = co_await co::connect(fd, (struct sockaddr *)&addr,
                sizeof(struct sockaddr_in));
        if (ret < 0) {
            co_await co::sleep_ms(50);
            DBG("Failed connect...");
        }
        else
            break;
    }
    ASSERT_COFN(retest);

    uint32_t uints1[4] = {0};
    test8_set_msg(uints1);
    ASSERT_COFN(test8_chk_msg(uints1));
    ASSERT_COFN(co_await co::write_sz(fd, uints1, sizeof(uints1)));

    uint32_t uints2[4] = {0};
    ASSERT_COFN(co_await co::read_sz(fd, uints2, sizeof(uints2)));
    ASSERT_COFN(test8_chk_msg(uints2));

    uint32_t uints3[4] = {0};
    ASSERT_COFN(co_await co::read_sz(fd, uints3, sizeof(uints3[0]) * 2));
    co_await co::sleep_ms(10);
    ASSERT_COFN(co_await co::read_sz(fd, &uints3[2], sizeof(uints3[2]) * 2));
    co_await co::sleep_ms(10);
    ASSERT_COFN(test8_chk_msg(uints3));

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
    ASSERT_FN(CHK_BOOL(ret == co::RUN_OK));
    ASSERT_FN(CHK_BOOL(test8_pass_cnt == 7));
    return 0;
}

/* Test9 - DBG Trace
================================================================================================= */

/* TODO: more tests here */
/* This test requires visual inspection */

co::task_t test9_dbg_call() {
    DBG("called...");
    co_return 0;
}

co::task_t test9_dbg_sched() {
    DBG("scheduled...");
    co_await CORO_REGNAME(test9_dbg_call());
    co_return 0;
};

int test9_dbg_trace() {
    auto pool = co::create_pool();
    auto trace = co::dbg_create_tracer(pool.get());
    pool->sched(CORO_REGNAME(test9_dbg_sched()), trace);
    co::run_e ret = pool->run();
    ASSERT_FN(CHK_BOOL(ret == co::RUN_OK));
    return 0;
}

/* Test10 - Futures
================================================================================================= */

struct test10_data_t {
    std::string name;
    int val = 0;
};

co::task<test10_data_t> test10_future_result() {
    co_return test10_data_t{ .name = "test10", .val = 10 };
}

co::task_t test10_co_futures() {
    auto t = test10_future_result();
    auto f = co::create_future(co_await co::get_pool(), t);
    co_await co::sched(t);
    auto [name, val] = co_await f;
    ASSERT_COFN(CHK_BOOL(name == "test10" && val == 10));
    co_return 0;
}

int test10_futures() {
    auto pool = co::create_pool();
    pool->sched(test10_co_futures());
    co::run_e ret = pool->run();
    ASSERT_FN(CHK_BOOL(ret == co::RUN_OK));
    return 0;
}

/* Test11 - wait_all
================================================================================================= */

co::task_t test11_co_wait_all() {
    auto ret = co_await co::wait_all(
        []() -> co::task<std::string> { co_return "test11"; }(),
        []() -> co::task<int> { co_return 11; }(),
        []() -> co::task<float> { co_return 0.11; }()
    );

    bool is_ok = ret == std::tuple<std::string, int, float>{"test11", 11, 0.11};
    ASSERT_COFN(CHK_BOOL(is_ok));

    co_return 0;
}

int test11_wait_all() {
    auto pool = co::create_pool();
    pool->sched(test11_co_wait_all());
    co::run_e ret = pool->run();
    ASSERT_FN(CHK_BOOL(ret == co::RUN_OK));
    return 0;
}

/* Test12 - Yielding
================================================================================================= */

co::task_t test12_co_yielder() {
    for (int i = 0; i < 14; i++) {
        co_yield i;
    }
    co_return 14;
}

co::task_t test12_yield_test() {
    auto yielder = test12_co_yielder();
    for (int i = 0; i < 15; i++)
        if (i != co_await yielder) {
            DBG("Bad yield");
            co_return -1;
        }
    co_return 0;
}

int test12_yielding() {
    auto pool = co::create_pool();
    pool->sched(test12_yield_test());
    co::run_e ret = pool->run();
    ASSERT_FN(CHK_BOOL(ret == co::RUN_OK));
    return 0;
}

/* Test13 - Exceptions
================================================================================================= */

int test13_val_inc = 0;

co::task_t test13_c() {
    test13_val_inc += 1;
    FnScope scope([]{
        test13_val_inc += 10;
    });
    throw std::runtime_error("test13");
    test13_val_inc += 1;
    co_return 0;
}

co::task_t test13_b() {
    FnScope scope([]{
        test13_val_inc += 1000;
    });
    test13_val_inc += 100;
    co_await test13_c();
    test13_val_inc += 100;
    co_return 0;
}

co::task_t test13_a() {
    try {
        test13_val_inc += 10000;
        co_await test13_b();
        test13_val_inc += 10000;
    }
    catch (const std::exception& ex) {
        ASSERT_COFN(CHK_BOOL(std::string(ex.what()) == "test13"));
        test13_val_inc += 100000;
    }
    test13_val_inc += 1000000;
    co_return 0;
}

co::task_t test13_exception_test() {
    co_await test13_a();
    test13_val_inc += 10000000;
    throw std::runtime_error("Custom exception");
    co_return 0;
}

int test13_except() {
    auto pool = co::create_pool();
    pool->sched(test13_exception_test());
    bool excepted = false;
    try {
        pool->run();
    }
    catch (std::exception &ex) {
        excepted = true;
        ASSERT_FN(CHK_BOOL(std::string(ex.what()) == "Custom exception"));
    }
    ASSERT_FN(CHK_BOOL(excepted));
    ASSERT_FN(CHK_BOOL(test13_val_inc == 11111111));
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
        { test8_io,        "test8_io" },
        { test9_dbg_trace, "test9_dbg_trace" },
        { test10_futures,  "test10_futures" },
        { test11_wait_all, "test11_wait_all" },
        { test12_yielding, "test12_yielding" },
        { test13_except,   "test13_except" },
    };

    for (auto test : tests) {
        int ret = test.first();
        if (ret >= 0)
            DBG("[+PASSED]: %s", test.second.c_str());
        else
            DBG("[-FAILED]: %s, ret", test.second.c_str(), ret);
    }
    return 0;
}
