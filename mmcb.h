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

#ifdef MMCB_DEBUG_ENABLE_LOGGING
# ifndef MMCB_DEBUG
#  define MMCB_DEBUG(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
# endif
#else
# ifndef MMCB_DEBUG
#  define MMCB_DEBUG(...) do {} while (0)
# endif
#endif

enum mmcb_e : int32_t {
    MMCB_FLAG_NONE      = 0,
    MMCB_FLAG_TRICOPY   = (1 << 0),
};

enum mmcb_err_e : int32_t {
    MMCB_ERROR_NONE = 0,
    MMCB_ERROR_ALREADY_INIT = -1,
    MMCB_ERROR_INVALID_PARAM = -2,
    MMCB_ERROR_SYSCALL_ERR = -3,
};

/*! This is the main object, you use it to point to the circular buffer. Contents can be copied,
 * but should not uninit the pointed buffer more than once. (one init with one uninit) */
struct mmcb_t {
    size_t get_size() { return _size; }
    void *get_base() { return _base; }

    mmcb_err_e init(size_t size, mmcb_e flags = MMCB_FLAG_NONE);
    mmcb_err_e uninit();

    bool is_init() { return base != nullptr; }

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

inline mmcb_err_e mmcb_t::init(size_t size, mmcb_e flags) {
    /* TODO: handle fail to init */
    if (_size != 0) {
        MMCB_DEBUG("FAILED This object was already initialized");
        return MMCB_ERROR_ALREADY_INIT;
    }
    if (size == 0) {
        MMCB_DEBUG("FAILED Can't initialize to 0 size");
        return MMCB_ERROR_INVALID_PARAM;
    }
    if (size % getpagesize() != 0) {
        MMCB_DEBUG("FAILED size must be a multiple of page size: getpagesize()=%d", getpagesize());
        return MMCB_ERROR_INVALID_PARAM;
    }
    _size = size;
    _flags = flags;

    if ((_mem_fd = memfd_create("", 0)) < 0) {
        MMCB_DEBUG("FAILED memfd_create: %s[%d]", strerror(errno), errno);
        return MMCB_ERROR_SYSCALL_ERR;
    }
    if (ftruncate(_mem_fd, _size) < 0) {
        MMCB_DEBUG("FAILED ftruncate: %s[%d]", strerror(errno), errno);
        return MMCB_ERROR_SYSCALL_ERR;
    }

    int prot = PROT_READ | PROT_WRITE;
    if (flags & MMCB_FLAG_TRICOPY) {
        /* Reserve for three views into the same memory */
        void *area_start = mmap(NULL, size * 3, prot, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (area_start == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the first view in memory */
        uint8_t *first_zone = (uint8_t *)area_start;
        if ((mmap(first_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)) == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap0: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the second view in memory (also the base one) */
        uint8_t *second_zone = first_zone + size;
        if ((mmap(second_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)) == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap1: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
        _base = (void *)second_zone;

        /* create the third view in memory */
        uint8_t *third_zone = second_zone + size;
        if ((mmap(third_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)) == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap2: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    else {
        /* Reserve for two views into the same memory */
        void *area_start = mmap(NULL, size * 2, prot, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (area_start == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the first view in memory (also the base one) */
        uint8_t *first_zone = (uint8_t *)area_start;
        if ((mmap(first_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)) == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap0: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
        _base = first_zone;

        /* create the second view in memory */
        uint8_t *second_zone = first_zone + size;
        if ((mmap(second_zone, size, prot, MAP_FIXED | MAP_SHARED, _mem_fd, 0)) == MAP_FAILED) {
            MMCB_DEBUG("FAILED mmap1: %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    return MMCB_ERROR_NONE;
}

inline mmcb_err_e mmcb_t::uninit() {
    if (_size == 0 || _base == nullptr) {
        MMCB_DEBUG("FAILED This object was not initialized");
        return MMCB_ERROR_INVALID_PARAM;
    }
    if (_flags & MMCB_FLAG_TRICOPY) {
        void *area_start = (uint8_t *)_base - _size;
        if (munmap(area_start, _size * 3) < 0) {
            MMCB_DEBUG("FAILED munmap %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    else {
        void *area_start = _base;
        if (munmap(area_start, _size * 2) < 0) {
            MMCB_DEBUG("FAILED munmap %s[%d]", strerror(errno), errno);
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    close(_mem_fd);
    _base = nullptr;
    _size = 0;
    _flags = MMCB_FLAG_NONE;
    _mem_fd = -1;
    return MMCB_ERROR_NONE;
}

#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)) /* end linux */

inline std::string mmcb_get_last_error_as_string() {
    // https://stackoverflow.com/questions/1387064/
    // how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

    // Get the error message ID, if any.
    DWORD err_msg_id = ::GetLastError();
    if (err_msg_id == 0) {
        return "NO_ERROR"; //No error message has been recorded
    }

    LPSTR message_buffer = nullptr;

    // Ask Win32 to give us the string version of that message ID.
    // The parameters we pass in, tell Win32 to create the buffer that holds the message for us
    // (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err_msg_id,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&message_buffer, 0, NULL);

    // Copy the error message into a std::string.
    std::string message(message_buffer, size);

    // Free the Win32's string's buffer.
    LocalFree(message_buffer);

    return message + "[" + std::to_string(err_msg_id) +"]";
}

inline mmcb_err_e mmcb_t::init(size_t size, mmcb_e flags) {
    SYSTEM_INFO sys_info;

    GetSystemInfo(&sys_info);

    if (_size != 0) {
        MMCB_DEBUG("FAILED This object was already initialized");
        return MMCB_ERROR_ALREADY_INIT;
    }
    if (size == 0) {
        MMCB_DEBUG("FAILED Can't initialize to 0 size");
        return MMCB_ERROR_INVALID_PARAM;
    }
    if (size % sys_info.dwPageSize != 0) {
        MMCB_DEBUG("FAILED size must be a multiple of page size: sys_info.dwPageSize=%d",
                sys_info.dwPageSize);
        return MMCB_ERROR_INVALID_PARAM;
    }
    _size = size;
    _flags = flags;

    _mem_fd = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, (DWORD)(_size >> 32),
            (DWORD)(_size & 0xffffffff), NULL);

    if (!_mem_fd) {
        MMCB_DEBUG("FAILED CreateFileMapping: %s", mmcb_get_last_error_as_string().c_str());
        return MMCB_ERROR_SYSCALL_ERR;
    }

    if (flags & MMCB_FLAG_TRICOPY) {
        /* Reserve for three views into the same memory */
        void* area_start = VirtualAlloc2(NULL, NULL, 3 * _size,
                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
        if (!area_start) {
            MMCB_DEBUG("FAILED VirtualAlloc2: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* split the placeholder into two placeholders 2*size, size */
        if (!VirtualFree(area_start, 2 * _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED VirtualFree0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* split the larger placeholder into two other placeholders to obtain three placeholdrs,
        each of size _size */
        if (!VirtualFree(area_start, _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED VirtualFree1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the first view in memory */
        uint8_t *first_zone = (uint8_t *)area_start;
        if (!MapViewOfFile3(_mem_fd, NULL, first_zone, 0, _size, MEM_REPLACE_PLACEHOLDER,
                PAGE_READWRITE, NULL, 0))
        {
            MMCB_DEBUG("FAILED MapViewOfFile3_0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the second view in memory (also the base one) */
        uint8_t *second_zone = first_zone + _size;
        if (!MapViewOfFile3(_mem_fd, NULL, second_zone, 0, _size, MEM_REPLACE_PLACEHOLDER,
                PAGE_READWRITE, NULL, 0))
        {
            MMCB_DEBUG("FAILED MapViewOfFile3_1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        _base = second_zone;

        /* create the third view in memory */
        uint8_t *third_zone = second_zone + _size;
        if (!MapViewOfFile3(_mem_fd, NULL, third_zone, 0, _size, MEM_REPLACE_PLACEHOLDER,
                PAGE_READWRITE, NULL, 0))
        {
            MMCB_DEBUG("FAILED MapViewOfFile3_2: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    else {
        /* Reserve for two views into the same memory */
        void* area_start = VirtualAlloc2(NULL, NULL, 2 * _size,
                MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
        if (!area_start) {
            MMCB_DEBUG("FAILED VirtualAlloc2: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* split the placeholder into two placeholders size, size */
        if (!VirtualFree(area_start, _size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED VirtualFree: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }

        /* create the first view in memory (also the base one) */
        uint8_t *first_zone = (uint8_t *)area_start;
        if (!MapViewOfFile3(_mem_fd, NULL, first_zone, 0, _size, MEM_REPLACE_PLACEHOLDER,
                PAGE_READWRITE, NULL, 0))
        {
            MMCB_DEBUG("FAILED MapViewOfFile3_0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        _base = first_zone;

        /* create the second view in memory */
        uint8_t *second_zone = first_zone + _size;
        if (!MapViewOfFile3(_mem_fd, NULL, second_zone, 0, _size, MEM_REPLACE_PLACEHOLDER,
                PAGE_READWRITE, NULL, 0))
        {
            MMCB_DEBUG("FAILED MapViewOfFile3_1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }

    return MMCB_ERROR_NONE;
}

inline mmcb_err_e mmcb_t::uninit() {
    if (_size == 0 || _base == nullptr) {
        MMCB_DEBUG("FAILED This object was not initialized");
        return MMCB_ERROR_INVALID_PARAM;
    }
    if (_flags & MMCB_FLAG_TRICOPY) {
        void *area_start = (uint8_t *)_base - _size;
        uint8_t *first_zone = (uint8_t *)area_start;
        uint8_t *second_zone = first_zone + _size;
        uint8_t *third_zone = second_zone + _size;
        /* Shity windows documentation doesn't make it clear that MEM_PRESERVE_PLACEHOLDER is needed
        or that unmaping the file and not calling VirtualFree would suffice. In their example for
        a circular buffer they use both, but without the MEM_PRESERVE_PLACEHOLDER flag, but without
        the flag, VirtualFree will thorw an error.. */
        if (!UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED UnmapViewOfFileEx0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED UnmapViewOfFileEx1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!UnmapViewOfFileEx(third_zone, MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED UnmapViewOfFileEx2: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!VirtualFree(first_zone, 0, MEM_RELEASE)) {
            MMCB_DEBUG("FAILED VirtualFree0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!VirtualFree(second_zone, 0, MEM_RELEASE)) {
            MMCB_DEBUG("FAILED VirtualFree1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!VirtualFree(third_zone, 0, MEM_RELEASE)) {
            MMCB_DEBUG("FAILED VirtualFree2: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
    }
    else {
        void *area_start = _base;
        uint8_t *first_zone = (uint8_t *)area_start;
        uint8_t *second_zone = first_zone + _size;
        if (!UnmapViewOfFileEx(first_zone, MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED UnmapViewOfFileEx0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!UnmapViewOfFileEx(second_zone, MEM_PRESERVE_PLACEHOLDER)) {
            MMCB_DEBUG("FAILED UnmapViewOfFileEx1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!VirtualFree(first_zone, 0, MEM_RELEASE)) {
            MMCB_DEBUG("FAILED VirtualFree0: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
        if (!VirtualFree(second_zone, 0, MEM_RELEASE)) {
            MMCB_DEBUG("FAILED VirtualFree1: %s", mmcb_get_last_error_as_string().c_str());
            return MMCB_ERROR_SYSCALL_ERR;
        }
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
