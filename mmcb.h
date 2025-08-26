#ifndef MMCB_H
#define MMCB_H

/*! @file
 *
 * @brief Memory mapped circular buffer.
 *
 * This header file creates functions for creating a memory mapped circular buffer with page size
 * granularity by installing two(or three) views of the same physical memory into virtual memory
 * space. The two(or three) virtual views are one after another, making the transition from the end
 * of one into the other seamless. A picture is given by:
 *
 * PHYSICAL:
 * [This is a string in memory.]
 *
 * VIRTUAL:
 * [This is a string in memory.][This is a string in memory.][This is a string in memory.]
 * ^                          ^                          ^
 * ^                          ^                          ^
 * (phys: 0, virtual: 0)      (phys: 0, virtual: 28)     (phys: 0, virtual: 56)
 *
 * The string above "This is string in memory." (including the terminating '\0') has 28 bytes in
 * memory. This is not exact since we can't remap only 28 bytes since this is not a multiple of
 * page size, but with that in consideration you can see that a pointer that points to the byte 28
 * in virtual space (The letter 'T') will point to the same data in memory as a pointer to 0 or a
 * pointer to 56. Hence if decrementing it bellow 0, the pointer to 'T' reaches 27, pointing to '.',
 * or a pointer to 55 (that points to '.'), incremented now points to 'T', i.e. 'wrapps arround'.
 *
 * This is helpfull, for example, when receiving streams of messages. If you know that the messages
 * will not be greater than the buffer, you can implement the following schema:
 *
 * 1. p = the pointer at the start of the message
 * 2. m = p
 * 3. receive bytes from stream into p and increment p acordingly (if it passes the boundry,
 * simply wrap it around)
 * 4. if we have a whole message at m dispatch it and afterwards increment m by the message len.
 * 5. jump to 3
 *
 * Further, if you can guarantee that p doesn't ever reach m in the example above, you can even use
 * that to add a header before m and you don't need to care about overflows or underflows.
 */

#if defined(__linux__)
# include <unistd.h>
# include <sys/mman.h>
#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
# define NOMINMAX
# include <Windows.h>
#endif

#include <cstdint>
#include <string>

#include "debug.h"
#include "misc_utils.h"

enum mmcb_e : int32_t {
    MMCB_FLAG_NONE      = 0,
    MMCB_FLAG_TRICOPY   = (1 << 0),
};

/*! This is the main object, you use it to point to the circular buffer. Contents can be copied,
 * but should not uninit the pointed buffer more than once. (one init with one uninit) */
struct mmcb_t {
    size_t get_size() { return _size; }
    void *get_base() { return _base; }

    int init(size_t size, mmcb_e flags = MMCB_FLAG_NONE);
    int uninit();

    bool is_init() { return _base != nullptr; }

    template <typename T>
    T *wrap(T *ptr);

    std::string to_str();

protected:
    void *_base = nullptr;   /*!< start of the buffer */
    size_t _size = 0;        /*!< size in bytes of the area. */
    mmcb_e _flags = MMCB_FLAG_NONE;

#if defined(__linux__)
    int _mem_fd = -1;
#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
    HANDLE _mem_fd = NULL;
#endif
};

inline std::string mmcb_to_str(mmcb_e flags);

/* IMPLEMENTATION
=================================================================================================
=================================================================================================
================================================================================================= */

#if defined(__linux__)

inline int mmcb_t::init(size_t size, mmcb_e flags) {
    /* TODO: handle fail to init */
    if (_size != 0) {
        DBG("FAILED This object was already initialized");
        return -1;
    }
    if (size == 0) {
        DBG("FAILED Can't initialize to 0 size");
        return -1;
    }
    if (size % getpagesize() != 0) {
        DBG("FAILED size must be a multiple of page size: getpagesize()=%d", getpagesize());
        return -1;
    }
    _size = size;
    _flags = flags;

    ASSERT_FN(_mem_fd = memfd_create("", 0));
    FnScope err_scope([this]{ close(_mem_fd); });
    ASSERT_FN(ftruncate(_mem_fd, _size));
    int prot = PROT_READ | PROT_WRITE;
    if (flags & MMCB_FLAG_TRICOPY) {
        /* Reserve for three views into the same memory */
        void *area_start;
        ASSERT_FN(CHK_MMAP(area_start =
                mmap(NULL, size * 3, prot, MAP_ANONYMOUS | MAP_SHARED, -1, 0)));
        err_scope([&]{ munmap(area_start, size * 3); });

        /* create the first view in memory */
        uint8_t *first_zone = (uint8_t *)area_start;
        ASSERT_FN(CHK_MMAP(mmap(first_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)));

        /* create the second view in memory (also the base one) */
        uint8_t *second_zone = first_zone + size;
        ASSERT_FN(CHK_MMAP(mmap(second_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)));
        _base = (void *)second_zone;

        /* create the third view in memory */
        uint8_t *third_zone = second_zone + size;
        ASSERT_FN(CHK_MMAP(mmap(third_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)));
    }
    else {
        /* Reserve for two views into the same memory */
        void *area_start;
        ASSERT_FN(CHK_MMAP(area_start =
                mmap(NULL, size * 2, prot, MAP_ANONYMOUS | MAP_SHARED, -1, 0)));
        err_scope([&]{ munmap(area_start, size * 2); });

        /* create the first view in memory (also the base one) */
        uint8_t *first_zone = (uint8_t *)area_start;
        ASSERT_FN(CHK_MMAP(mmap(first_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)));
        _base = first_zone;

        /* create the second view in memory */
        uint8_t *second_zone = first_zone + size;
        ASSERT_FN(CHK_MMAP(mmap(second_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)));
    }

    err_scope.disable();
    return -1;
}

inline int mmcb_t::uninit() {
    if (_size == 0 || _base == nullptr) {
        DBG("FAILED This object was not initialized");
        return -1;
    }
    if (_flags & MMCB_FLAG_TRICOPY) {
        void *area_start = (uint8_t *)_base - _size;
        ASSERT_FN(munmap(area_start, _size * 3));
    }
    else {
        void *area_start = _base;
        ASSERT_FN(munmap(area_start, _size * 2));
    }
    close(_mem_fd);
    _base = nullptr;
    _size = 0;
    _flags = MMCB_FLAG_NONE;
    _mem_fd = -1;
    return 0;
}

#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) /* end linux */

inline int mmcb_t::init(size_t size, mmcb_e flags) {
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);

    if (_size != 0) {
        DBG("FAILED This object was already initialized");
        return -1;
    }
    if (size == 0) {
        DBG("FAILED Can't initialize to 0 size");
        return -1;
    }
    if (size % sys_info.dwPageSize != 0) {
        DBG("FAILED size must be a multiple of page size: sys_info.dwPageSize=%d",
                sys_info.dwPageSize);
        return -1;
    }
    _size = size;
    _flags = flags;

    _mem_fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(_size >> 32),
            (DWORD)(_size & 0xffffffff), NULL);
    ASSERT_FN(CHK_PTR(_mem_fd));
    FnScope err_scope([this] { CloseHandle(_mem_fd); });

    if (flags & MMCB_FLAG_TRICOPY) {
        /* Reserve for three views into the same memory */
        void* area_start;
        ASSERT_FN(CHK_PTR(area_start = VirtualAlloc2(NULL, NULL, 3 * _size,
                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0)));

        if (!area_start) {
            DBGE("FAILED VirtualAlloc2");
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* split the placeholder into two placeholders 2*size, size */
        if (!VirtualFree(area_start, 2 * _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            VirtualFree(area_start, 0, MEM_RELEASE);
            DBG("FAILED VirtualFree0");
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* split the larger placeholder into two other placeholders to obtain three placeholdrs,
        each of size _size */
        if (!VirtualFree(area_start, _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            DBG("FAILED VirtualFree1");
            VirtualFree(area_start, 0, MEM_RELEASE);
            VirtualFree((uint8_t *)area_start + _size * 2, 0, MEM_RELEASE);
            return MMCB_ERROR_SYSCALL_ERR;
        }

        err_scope([&]{ VirtualFree(first_zone, 0, MEM_RELEASE); });
        /* create the first view in memory */
        uint8_t *first_zone = (uint8_t *)area_start;
        ASSERT_FN(CHK_BOOL(MapViewOfFile3(_mem_fd, NULL, first_zone, 0, _size,
                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0)));
        err_scope([&]{ UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER); });

        /* create the second view in memory (also the base one) */
        uint8_t *second_zone = first_zone + _size;
        err_scope([&]{ VirtualFree(second_zone, 0, MEM_RELEASE); });
        ASSERT_FN(CHK_BOOL(MapViewOfFile3(_mem_fd, NULL, second_zone, 0, _size,
                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0)));
        _base = second_zone;
        err_scope([&]{ UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER); });

        /* create the third view in memory */
        uint8_t *third_zone = second_zone + _size;
        err_scope([&]{ VirtualFree(third_zone, 0, MEM_RELEASE); });
        ASSERT_FN(CHK_BOOL(MapViewOfFile3(_mem_fd, NULL, third_zone, 0, _size,
                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0)));
        err_scope([&]{ UnmapViewOfFileEx(third_zone, MEM_PRESERVE_PLACEHOLDER); });
    }
    else {
        /* Reserve for two views into the same memory */
        void* area_start;
        ASSERT_FN(CHK_PTR(area_start = VirtualAlloc2(NULL, NULL, 2 * _size,
                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0)));

        /* split the placeholder into two placeholders size, size */
        if (!VirtualFree(area_start, _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            DBGE("FAILED VirtualFree");
            VirtualFree(area_start, 0, MEM_RELEASE);
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the first view in memory (also the base one) */
        uint8_t *first_zone = (uint8_t *)area_start;
        err_scope([&]{ VirtualFree(first_zone, 0, MEM_RELEASE); });
        ASSERT_FN(CHK_BOOL(MapViewOfFile3(_mem_fd, NULL, first_zone, 0, _size,
                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0)))
        _base = first_zone;
        err_scope([&]{ UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER); });

        /* create the second view in memory */
        uint8_t *second_zone = first_zone + _size;
        err_scope([&]{ VirtualFree(second_zone, 0, MEM_RELEASE); });
        ASSERT_FN(CHK_BOOL(MapViewOfFile3(_mem_fd, NULL, second_zone, 0, _size,
                MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0)))
        err_scope([&]{ UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER); });
    }

    err_scope.disable();
    return 0;
}

inline int mmcb_t::uninit() {
    if (_size == 0 || _base == nullptr) {
        DBG("FAILED This object was not initialized");
        return -1;
    }
    if (_flags & MMCB_FLAG_TRICOPY) {
        void *area_start = (uint8_t *)_base - _size;
        uint8_t *first_zone = (uint8_t *)area_start;
        uint8_t *second_zone = first_zone + _size;
        uint8_t *third_zone = second_zone + _size;
        /* Shity windows documentation doesn't make it clear that MEM_PRESERVE_PLACEHOLDER is needed
        or that unmaping the file and not calling VirtualFree would suffice. In their example for
        a circular buffer they use both, but without the MEM_PRESERVE_PLACEHOLDER flag, problem is
        that without the flag, VirtualFree will thorw an error.. */

        ASSERT_FN(CHK_BOOL(UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER)));
        ASSERT_FN(CHK_BOOL(UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER)));
        ASSERT_FN(CHK_BOOL(UnmapViewOfFileEx(third_zone, MEM_PRESERVE_PLACEHOLDER)));
        ASSERT_FN(CHK_BOOL(VirtualFree(first_zone, 0, MEM_RELEASE)));
        ASSERT_FN(CHK_BOOL(VirtualFree(second_zone, 0, MEM_RELEASE)));
        ASSERT_FN(CHK_BOOL(VirtualFree(third_zone, 0, MEM_RELEASE)));
    }
    else {
        void *area_start = _base;
        uint8_t *first_zone = (uint8_t *)area_start;
        uint8_t *second_zone = first_zone + _size;

        ASSERT_FN(CHK_BOOL(UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER)));
        ASSERT_FN(CHK_BOOL(UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER)));
        ASSERT_FN(CHK_BOOL(VirtualFree(first_zone, 0, MEM_RELEASE)));
        ASSERT_FN(CHK_BOOL(VirtualFree(second_zone, 0, MEM_RELEASE)));
    }
    CloseHandle(_mem_fd);

    _base = nullptr;
    _size = 0;
    _flags = MMCB_FLAG_NONE;
    _mem_fd = nullptr;
    return MMCB_ERROR_NONE;
}

#endif /* end windows */

template <typename T>
inline T *mmcb_t::wrap(T *_ptr) {
    uint8_t *ptr = (uint8_t *)_ptr;
    uint8_t *base = (uint8_t *)_base;

    if (_ptr < base)
        _ptr = (ptr + _size);
    if (_ptr > base + _size)
        _ptr = (base - _size);
    return (T *)ptr;
}

inline std::string mmcb_t::to_str() {
    char buff[256] = {0};
    std::snprintf(buff, sizeof(buff) - 1, "mmcb: [base: %p size: 0x%zx, flags: %s]",
            _base, _size, mmcb_to_str(_flags).c_str());
    return buff;
}

inline std::string mmcb_to_str(mmcb_e flags) {
    std::string ret = "[";
    if (flags & MMCB_FLAG_TRICOPY)
        ret += "MMCB_FLAG_TRICOPY";
    return ret + "]";
}

#endif
