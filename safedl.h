#ifndef SAFEDL_H
#define SAFEDL_H

namespace safedl {

/* The idea:
    A crash inside a shared object is most probably a crash that can be reversed. Simply return -1
    from the function that crashed and unload the SO.

    OBS: If there are objects that where created from the shared object those objects will no longer
    be valid. This safedl is intended to work with the virt_object/virt_composer framework and as
    such, we will be able to remember all those objects and mark null their internals. In this way
    they will allways except out because their internal is no longer there (TODO: inspect this
    further).

    With this addition, the shared objects will be able to provide objects with diverse utility
    and if they crash, they would simply go away.
*/

inline void sigsegv_handle(/*...*/) {
    if (crash_in_managed_so()) {
        siglongjump(sig_jmpbuff_location)
    }
    else {
        forward_to_normal_sigsegv();
    }
}

struct dl_wrapper_t {
    int fd;
    sigjmp_buf sig_jmpbuff_location;

    /* ... */
    auto fn_call_factory(std::string fnname) {
        auto fn = load_fn_from_fd(fd, fnname);
        ASSERT(fn);
        return [fn]<typename ...Args>(Args ...args) -> int /* all fns will return int */ {
            if (sigsetjump(sig_jmpbuff_location)) {
                DBG("So crashed or something")
                return -1;
            }
            return fn(args...);
        };
    }
};

}

#endif