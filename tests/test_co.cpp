#include "co_utils.h"
#include "sys_utils.h"

#define SERVER_PORT 5123

struct PACKED_STRUCT msg_t {
    int magic = 123;
    int type;
    char str[256] = "Invalid string";
};

enum {
    MSG_TYPE_START,
    MSG_TYPE_PRINT_HELLO,
    MSG_TYPE_PRINT_WORLD,
    MSG_TYPE_PRINT_STRING,
    MSG_TYPE_STOP,
    MSG_TYPE_ACK,
};

co::sem_t close_sem;

co::task_t client_handle(int sock_fd) {
    DBG_SCOPE();
    FnScope scope([&] {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    });

    while (true) {
        int ret;
        msg_t rcv;
        msg_t rsp{ .type = MSG_TYPE_ACK };

        ASSERT_COFN(ret = co_await co::read_sz(sock_fd, &rcv, sizeof(rcv)));

        if (ret == 0) {
            DBG("Premature close of connection");
            co_return -1;
        }

        bool stopped = false;
        switch (rcv.type) {
            case MSG_TYPE_START: {
                strcpy(rsp.str, "START_RECEIVED");
            } break;
            case MSG_TYPE_PRINT_HELLO: {
                strcpy(rsp.str, "COMMAND_RECEIVED_PRINT_HELLO");
                printf("Hello \n");
            } break;
            case MSG_TYPE_PRINT_WORLD: {
                strcpy(rsp.str, "COMMAND_RECEIVED_PRINT_WORLD");
                printf("world!\n");
            } break;
            case MSG_TYPE_PRINT_STRING: {
                strcpy(rsp.str, "COMMAND_RECEIVED_PRINT_STRING");
                printf("string: %s\n", rcv.str);
            } break;
            case MSG_TYPE_STOP: {
                strcpy(rsp.str, "STOP_RECEIVED");
                stopped = true;
            } break;
        }

        ASSERT_COFN(co_await co::write_sz(sock_fd, &rsp, sizeof(rsp)));

        if (stopped) {
            DBG("Time to stop the connection");
            break;
        }
    }
    co_return 0;
}

co::task_t client(std::string msg_str) {
    DBG_SCOPE();
    int sock_fd;

    sockaddr_in addr = create_sa_ipv4("127.0.0.1", SERVER_PORT);
    socklen_t len = sizeof(addr);

    ASSERT_COFN(sock_fd = socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_COFN(connect(sock_fd, (sockaddr *)&addr, len));

    FnScope scope([&] {
        DBG("Called scoped close");
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        close_sem.rel();
    });

    msg_t msgs[] = {
        { .type = MSG_TYPE_START },
        { .type = MSG_TYPE_PRINT_HELLO },
        { .type = MSG_TYPE_PRINT_WORLD },
        { .type = MSG_TYPE_PRINT_STRING },
        { .type = MSG_TYPE_STOP },
    };

    for (auto &msg : msgs) {
        int ret;
        msg_t rcv;

        if (msg.type == MSG_TYPE_PRINT_STRING) {
            strcpy(msg.str, msg_str.c_str());
        }

        co_await co::sleep_us(100);

        ASSERT_COFN(co_await co::write_sz(sock_fd, &msg, sizeof(msg)));
        ASSERT_COFN(ret = co_await co::read_sz(sock_fd, &rcv, sizeof(rcv)));

        if (ret == 0) {
            DBG("Connection closed too early");
            co_return -1;
        }

        DBG("received: %d, %s", rcv.type, rcv.str);
    }
    co_return 0;
}

int server_fd;
co::task_t server() {
    DBG_SCOPE();
    int enable = 1;

    sockaddr_in addr = create_sa_ipv4(INADDR_ANY, SERVER_PORT);
    socklen_t len = sizeof(addr);

    ASSERT_COFN(server_fd = socket(AF_INET, SOCK_STREAM, 0));
    ASSERT_COFN(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)));
    ASSERT_COFN(bind(server_fd, (sockaddr *)&addr, len));
    ASSERT_COFN(listen(server_fd, 5));

    FnScope scope([&] {
        shutdown(server_fd, SHUT_RDWR);
        close(server_fd);
    });

    DBG("server_fd: %d", server_fd);

    while (true) {
        int client_fd;
        ASSERT_COFN(client_fd = co_await co::accept(server_fd, (sockaddr *)&addr, &len));
        if (client_fd == 0) {
            DBG("Accept closed");
            break;
        }
        co_await co::sched(client_handle(client_fd));
    }
    co_return 0;
}

co::task_t close_task() {
    co_await close_sem;
    shutdown(server_fd, SHUT_RDWR);
    co_return 0;
}

co::sem_t sem(3);

co::task_t producer() {
    for (int i = 0; i < 100; i++) {
        DBG("Produced: %d", i);
        sem.rel();
        co_await co::yield();
    }
    co_return 0;
}

co::task_t consumer() {
    for (int i = 0; i < 100; i++) {
        co_await sem;
        DBG("Consumed: %d", i);
    }
    co_return 0;
}

co::task_t sleep_multiple() {
    auto task = [](int ms, std::string to_print) -> co::task_t {
        ASSERT_COFN(co_await co::sleep_ms(ms));
        DBG(" <:> %s ", to_print.c_str());
        co_return 0;
    };
    co_return co_await when_all(
        task(8, "works"),
        task(7, "sleep"),
        task(6, "if"),
        task(5, "order"),
        task(4, "reverse"),
        task(3, "in"),
        task(2, "print"),
        task(1, "will")
    );
}

co::task_t force_order_multiple() {
    co::sem_t initial_sem(1);
    std::vector<co::sem_t> sems(4);
    auto task = [](std::string to_print, co::sem_t &before, co::sem_t &after) -> co::task_t {
        co_await before;
        DBG(" <:> %s ", to_print.c_str());
        after.rel();
        co_return 0;
    };
    co_return co_await when_all(
        task("0 -> 1", sems[0], sems[1]),
        task("1 -> 2", sems[1], sems[2]),
        task("initial -> 0", initial_sem, sems[0]),
        task("2 -> 3", sems[2], sems[3])
    );
}

co::task_t test_comods_timeo_trace(uint64_t var_sleep1_us, uint64_t var_sleep2_us) {
    co::sem_t break_sem(0);
    co::sem_t wait_done(0);

    co::sleep_handle_t sleep_handle;

    /* test wait on semaphore */
    co_await co::sched([&]() -> co::task_t {
        int ret = co_await co::trace(
            co::timed(
                [&]() -> co::task_t {
                    co_await break_sem;
                    co_return 0;
                }(),
                var_sleep1_us
            ),
            co::default_trace_fn
        );
        sleep_handle.stop();
        DBG("timeo: %ld sig_time: %ld After semaphore wait, return val: %d",
                var_sleep1_us, var_sleep2_us, ret);

        wait_done.rel();
        co_return 0;
    }());

    co_await co::var_sleep_us(var_sleep2_us, &sleep_handle);
    break_sem.rel();
    co_await co::yield();

    co_await wait_done; /* makes sure local variables are not released before usage */
    co_return 0;
}

int main(int argc, char const *argv[])
{
    co::pool_t pool;

    DBG("Will schedule server");
    pool.sched(server());

    close_sem = co::sem_t(-3);

    DBG("Will schedule clients");
    pool.sched(client("client_1"));
    pool.sched(client("client_2"));
    pool.sched(client("client_3"));

    pool.sched(close_task());

    DBG("Will run the pool");
    pool.run();

    /*-------------------------------------*/

    DBG("Will schedule producer:");
    pool.sched(producer());
    DBG("Will schedule consumer:");
    pool.sched(consumer());

    DBG("Will run the pool");
    pool.run();

    /*-------------------------------------*/

    DBG("Will sleep multiple");
    pool.sched(sleep_multiple());

    DBG("Will run the pool");
    pool.run();

    /*-------------------------------------*/

    DBG("Will force order multiple");
    pool.sched(force_order_multiple());

    DBG("Will run the pool");
    pool.run();

    /*-------------------------------------*/

    DBG("Will test comods");
    pool.sched(test_comods_timeo_trace(1'000'000, 0));
    pool.sched(test_comods_timeo_trace(0, 0));
    pool.sched(test_comods_timeo_trace(0, 1'000'000));
    pool.sched(test_comods_timeo_trace(1'000'000, 1'000'000));
    pool.sched(test_comods_timeo_trace(2'000'000, 1'000'000));
    pool.sched(test_comods_timeo_trace(1'000'000, 2'000'000));

    DBG("Will run the pool");
    pool.run();

    return 0;
}
