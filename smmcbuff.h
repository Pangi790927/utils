#ifndef SMMCBUFF_T
#define SMMCBUFF_T

#include "misc_utils.h"
#include "co_utils.h"

#include <unistd.h>
#include <sys/mman.h>


/* Sync Memory Mapped Circuler Buffer */

/* This circular buffer is meant for packet transfers, for reading a packet from the start of the
buffer and for pushing a packet at the end of the buffer. The reason it has sync in the name is that
this buffer is meant to be able to lock both ends of the buffer and send availability notifications.
The reason it is called memory mapped is that it uses memory mapping to make the circular buffer
appear continuous.

For reading the packets of the buffer you would lock the begining of the buffer and you would
receive a token that: holds a reference to the used lock, holds the number of bytes available at the
respective moment and holds a pointer to the memory area. You would read however many bytes you want
from the 'read' end of the buffer and move the head pointer to signal that the area has been
received, that is if you want that. If for example, your packet is not complete at the moment you
would unlock the read end of the buffer without moving the head of the buffer.

You could also place a request for memory, ie give the smmcbuff a signaling object that should be
used to signal you when a given amount of memory is available to be read. At the end of the
operation you should receive a locked head with the total memory available.

For writing to the end of the buffer you would also get a locked tail, but this time with the memory
that is available to be written. In the same maner as for reading you could choose not to advance
the tail of the buffer at the end of the operation. This is usefull if you decide along the way that
the message was malformed in some way and want to throw the written bytes away, in that case you
would just not advance the buffer tail and just return some sort of error.

The same as for reading you can ask for the buffer to notify you when it has suficient memory for
you to write to it. */

/* The main idea is that you can think of free space as the dual of used space, so a circular buffer
has N used bytes and SZ-N free bytes. Both the free and used parts can be seen as a circular buffer
of their own, where head is the start of the used part and tail is the start of the free part, from
this observation we can see that most of the operations needed for the two are actually quite
simetric. */

struct co_smmcbuff_t;

struct mmcbuff_cbk_t {
    using fn_t = void (*)(void *, size_t adv_len);

    fn_t fn = NULL;
    void *ctx = NULL;
};

struct mmcbuff_t {
    // structure to hold the lock of the area written or read

    struct head_t {
        head_t();

        /* When copied from another head_t this head_t becomes an "staged copy" to the actual head,
        to actually make the changes you must asign this head_t to the returned by ref_*() functions
         */
        head_t(const head_t&);
        head_t(head_t&&) = delete;

        head_t &operator = (const head_t&);
        head_t &operator = (head_t&&);

        /* advances the head of the buffer internaly */
        int advance(size_t adv_len);

        /* gives addr to the first valid byte */
        void *addr();

        /* gives the valid area of this respective head */
        size_t size();

    protected:
        size_t base;
        size_t *other = NULL;

        mmcbuff_t *cbuff = NULL;
        mmcbuff_cbk_t change_cbk;

        friend mmcbuff_t;
        friend co_smmcbuff_t;
    };

    mmcbuff_t();
    ~mmcbuff_t();

    mmcbuff_t(const mmcbuff_t&) = delete;
    mmcbuff_t(mmcbuff_t&&);

    mmcbuff_t &operator = (const mmcbuff_t&) = delete;
    mmcbuff_t &operator = (mmcbuff_t&&);

    int init(size_t _sz, mmcbuff_cbk_t change_read = mmcbuff_cbk_t{},
            mmcbuff_cbk_t change_write = mmcbuff_cbk_t{});
    void uninit();

    head_t &ref_read();
    head_t &ref_write();

    size_t size();
    size_t capacity();

    char *cbase();
protected:

    size_t sz; // size of the hole buffer, ie how many bytes can fit in

    head_t head;
    head_t tail;

    int fd;
    void *base;

    friend co_smmcbuff_t;
};

struct co_smmcbuff_t {
    mmcbuff_t cbuff;
    size_t write_sz;
    size_t read_sz;
    size_t req_write = 0;
    size_t req_read = 0;

    co::sem_t read_sem;
    co::sem_t write_sem;

    int init(size_t _sz) {
        ASSERT_FN(cbuff.init(_sz));
        write_sz = cbuff.capacity();
        read_sz = 0;
        cbuff.head.change_cbk = mmcbuff_cbk_t{
            .fn = +[](void *_own, size_t len){
                auto own = (co_smmcbuff_t *)_own;
                own->write_sz += len;
                own->read_sz -= len;
                if (own->req_write && own->write_sz >= own->req_write) {
                    own->write_sem.rel();
                    own->req_write = 0;
                }
            },
            .ctx = this
        };
        cbuff.tail.change_cbk = mmcbuff_cbk_t{
            .fn = +[](void *_own, size_t len){
                auto own = (co_smmcbuff_t *)_own;
                own->write_sz -= len;
                own->read_sz += len;
                if (own->req_read && own->read_sz >= own->req_read) {
                    own->read_sem.rel();
                    own->req_read = 0;
                }
            },
            .ctx = this
        };
        return 0;
    }

    void uninit() {
        cbuff.uninit();
    }

    size_t size() {
        return cbuff.size();
    }
    size_t capacity() {
        return cbuff.capacity();
    }

    co::task_t ref_read(mmcbuff_t::head_t **ret, size_t minsz) {
        if (read_sz < minsz) {
            req_read = minsz;
            co_await read_sem;
        }
        if (ret)
            *ret = &cbuff.ref_read();
        co_return 0;
    }

    co::task_t ref_write(mmcbuff_t::head_t **ret, size_t minsz) {
        if (minsz > cbuff.capacity()) {
            DBG("Can't wait for more space than the capacity of the cbuff req: %ld capacity: %ld",
                    minsz, cbuff.capacity());
            co_return -1;
        }
        if (write_sz < minsz) {
            req_write = minsz;
            co_await write_sem;
        }
        if (ret)
            *ret = &cbuff.ref_write();
        co_return 0;
    }
};


/* IMPLEMENTATION:
================================================================================================= */

inline mmcbuff_t::mmcbuff_t() {
    fd = -1;
}

inline mmcbuff_t::~mmcbuff_t() {
    if (fd != -1)
        DBG("ERROR: You forgot to uninit mmcbuff_t");
}

inline mmcbuff_t::mmcbuff_t(mmcbuff_t&& oth) {
    *this = std::move(oth);   
}

inline mmcbuff_t &mmcbuff_t::operator = (mmcbuff_t&& oth) {
    sz = oth.sz;
    head = oth.head;
    tail = oth.tail;
    base = oth.base;
    fd = oth.fd;
    oth.fd = -1;
    head.cbuff = this;
    tail.cbuff = this;
    head.other = &tail.base;
    tail.other = &head.base;
    head.change_cbk = oth.head.change_cbk;
    tail.change_cbk = oth.tail.change_cbk;
    return *this;
}

inline char *mmcbuff_t::cbase() {
    return (char *)base;
}

inline int mmcbuff_t::init(size_t _sz, mmcbuff_cbk_t change_read, mmcbuff_cbk_t change_write) {
    sz = UP_ALIGN(_sz, sysconf(_SC_PAGESIZE));
    head.cbuff = this;
    head.base = 0;
    head.other = &tail.base;
    head.change_cbk = change_read;
    tail.cbuff = this;
    tail.base = 0;
    tail.other = &head.base;
    tail.change_cbk = change_write;

    /* create a memory file to hold our buffer data and to give us a handler to pass to mmap */
    ASSERT_FN(fd = memfd_create("", 0));
    ASSERT_FN(ftruncate(fd, sz));

    int prot = PROT_READ | PROT_WRITE;

    /* reserve twice as much virtual memory as needed for the circular buffer */
    ASSERT_FN(intptr_t(base = mmap(NULL, sz * 2, prot, MAP_ANONYMOUS | MAP_SHARED, -1, 0)));
    
    /* map the memory file twice inside our reserved virtual mapping */
    ASSERT_FN(intptr_t(mmap(cbase(), sz, prot, MAP_FIXED | MAP_SHARED, fd, 0)));
    ASSERT_FN(intptr_t(mmap(cbase() + sz, sz, prot, MAP_FIXED | MAP_SHARED, fd, 0)));
    return 0;
}

inline void mmcbuff_t::uninit() {
    munmap(base, sz * 2);
    close(fd);
    fd = -1;
}

inline size_t mmcbuff_t::size() {
    return head.size();
}

inline size_t mmcbuff_t::capacity() {
    return sz;
}

mmcbuff_t::head_t::head_t() {}

/* This makes a fake copy(maybe I should invent a different type for this operation?) This copy has
a pointer to the other current valid head, but the other head does not have a valid copy to this one
 */
mmcbuff_t::head_t::head_t(const head_t& oth) {
    base = oth.base;
    cbuff = oth.cbuff;
    other = oth.other;
}

mmcbuff_t::head_t &mmcbuff_t::head_t::operator = (const mmcbuff_t::head_t& oth) {
    if (!cbuff) {
        cbuff = oth.cbuff;
        other = oth.other;
    }
    if (cbuff != oth.cbuff)
        throw std::runtime_error("Can't have transfers between heads of different cbufs");
    if (change_cbk.fn && base != oth.base) {
        size_t adv_len = 0;
        if (oth.base > base)
            adv_len = oth.base - base;
        else
            adv_len = cbuff->sz + oth.base - base;
        change_cbk.fn(change_cbk.ctx, adv_len);
    }
    base = oth.base;
    return *this;
}

mmcbuff_t::head_t &mmcbuff_t::head_t::operator = (mmcbuff_t::head_t&& oth) {
    return (*this) = oth;
}

inline mmcbuff_t::head_t &mmcbuff_t::ref_read() {
    return head;
}

inline mmcbuff_t::head_t &mmcbuff_t::ref_write() {
    return tail;
}

inline int mmcbuff_t::head_t::advance(size_t adv_len) {
    // for read this pops data, for write this pushes data, it does both operations without actually
    // changing the buffer, you need to call commit to achieve that
    size_t curr_sz = size();
    if (adv_len > curr_sz)
        return -1;
    if (change_cbk.fn && adv_len)
        change_cbk.fn(change_cbk.ctx, adv_len);
    base = (base + adv_len) % cbuff->sz;
    return 0;
}

inline void *mmcbuff_t::head_t::addr() {
    return cbuff->cbase() + base;
}

inline size_t mmcbuff_t::head_t::size() {
    if (other != &cbuff->tail.base) {
        if (*other > base)
            return *other - base;
        else
            return cbuff->sz + *other - base;
    }
    else {
        if (*other >= base)
            return *other - base;
        else
            return cbuff->sz + *other - base;
    }
}

#endif
