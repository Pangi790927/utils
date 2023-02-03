#include "co_utils.h"

#define SERVER_PORT 5123

sockaddr_in create_sa_ipv4(uint32_t addr, uint16_t port) {
    sockaddr_in sa_addr = {};
    sa_addr.sin_family = AF_INET;
    sa_addr.sin_addr.s_addr = INADDR_ANY;
    sa_addr.sin_port = htons(SERVER_PORT);
    return sa_addr;
}

sockaddr_in create_sa_ipv4(std::string addr, uint16_t port) {
    return create_sa_ipv4(inet_addr(addr.c_str()), port);
}

struct PACKED_STRUCT msg_t {
    int magic = 'goku';
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

/* The semaphore is an object with an internal counter and is usefull in signaling and mutual
exclusion:
    1. co_await - suspends the current coroutine if the counter is zero, else decrements the counter
                  returns an unlocker_t object that has a member function .unlock()
    2. rel      - increments the counter.
    3. rel_all  - increments the counter with the amount of waiting coroutines on this semaphore

    At a suspension point coroutines that suspended on a wait are candidates for rescheduling. Upon
    rescheduling the internal counter will be decremented.

    You must manually use (co_await co::yield()) to suspend the current coroutine if you want the
    notified coroutine to have a chance to be rescheduled. 
*/

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

int main(int argc, char const *argv[])
{
    co::pool_t pool;

    DBG("Will schedule server");
    pool.sched(server());

    close_sem = co::sem_t(-3);

    DBG("Will schedule client");
    pool.sched(client("client_1"));
    pool.sched(client("client_2"));
    pool.sched(client("client_3"));

    pool.sched(close_task());

    // DBG("Will schedule producer:");
    // pool.sched(producer());
    // DBG("Will schedule consumer:");
    // pool.sched(consumer());

    DBG("Will run the pool");

    uint64_t start_us = get_time_us();
    pool.run();
    uint64_t stop_us = get_time_us();
    DBG("Run time diff: %ld", stop_us - start_us);
    return 0;
}
