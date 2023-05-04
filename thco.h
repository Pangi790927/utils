#ifndef THCO_H
#define THCO_H

#include "co_utils.h"
#include "sys_utils.h"
#include <mutex>
#include <queue>

/*  Sometimes you need to do blocking IO in a separate thread because there is no way to async wait
for that event, for whatever reason. If done inside one of the coroutines handled by the main thread
the coroutine engine would be blocked. This header introduces mechanisms which can be used to bypass
this specific shortcomming of coroutines.
    To do this I assumed the blocking IO would be handled by a separate thread. As such thread-coro
comms need to be established. This task will probably be usually achieved by using pipes. */

template <typename T>
struct thco_queue_t {
    std::queue<T>   que;
    std::mutex      que_mu;
    int             que_sig[2];

    thco_queue_t() {
        int ret = pipe(que_sig);
        if (ret < 0) {
            DBGE("Very unusual pipe fail");
        }
    }

    ~thco_queue_t() {
        close(que_sig[0]);
        close(que_sig[1]);
    }

    thco_queue_t(const thco_queue_t&) = delete;
    thco_queue_t(thco_queue_t&&) = delete;
    thco_queue_t& operator = (const thco_queue_t&) = delete;
    thco_queue_t& operator = (thco_queue_t&&) = delete;

    void push(const T& t) {
        select_wrap(std::vector<int>{}, std::vector<int>{(int)que_sig[1]}, std::vector<int>{});
        std::lock_guard guard(que_mu);
        que.push(t);
        int ret = write(que_sig[1], "", 1);
        if (ret <= 0) {
            DBGE("Very unusual pipe fail");
        }
    }

    co::task_t pop(T &ret) {
        uint8_t aux;
        co_await co::read(que_sig[0], &aux, 1);
        std::lock_guard guard(que_mu);
        ret = que.front();
        que.pop();
        co_return 0;
    }
};

#endif
