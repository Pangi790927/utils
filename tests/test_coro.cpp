#include "coro.h"

#include <string.h>
#include <thread>

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
