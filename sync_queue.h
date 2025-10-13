#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

#define SYNC_QUEUE_DEFAULT_MAX_SZ 4096
#define SYNC_QUEUE_NOLIMIT_MAX_SZ 0

template <typename T>
struct sync_queue_t {
    uint32_t max_size = SYNC_QUEUE_DEFAULT_MAX_SZ;

    std::queue<T> q;
    std::condition_variable cv;
    std::mutex mu;

    size_t size() {
        std::lock_guard lk(mu);
        return q.size();
    }

    /* ignores the size limit */
    void force_push(const T& t) {
        std::lock_guard lk(mu);
        q.push(t);
        cv.notify_all();
    }

    void push(const T& t) {
        std::unique_lock lk(mu);
        cv.wait(lk, [this]{ return (max_size == 0) || (q.size() < max_size); });
        q.push(t);
        lk.unlock();
        cv.notify_all();
    }

    void pop(T &t) {
        std::unique_lock lk(mu);
        cv.wait(lk, [this]{ return q.size() > 0; });
        t = q.front();
        q.pop();
        lk.unlock();
        cv.notify_all();
    }
};

#endif
