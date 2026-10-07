/* Coroutine plugin - Category: Plugins
 *
 * A plugin whose members wait. Each answers a co::task made by this plugin's own copy of colib,
 * which the host's pool runs and resumes, so the script that called it waits on the host's side for
 * work the plugin does. Loaded by 021-009; never linked into a test.
 *
 * virt_composer_coroutines.h comes right after virt_composer.h, as in every module that uses it,
 * so VC_TYPE_LUA_CORO takes the id the host gave it, and its returner is what lets a member that
 * answers a co::task suspend its caller.
 *
 * 06-10-2026-06:34 */

#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"
#include "../../../virt_composer_coroutines.h"
#include "../../../virt_composer_plugins.h"

#include <chrono>

namespace vc = virt_composer;
namespace vo = virt_object;

/* Written by the host through plugin_register_meta() before anything asks a type for its id.
06-10-2026-06:34 */
int _type_offset = 0;

VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(NAPPER_TYPE);

/*! An object whose members wait, each a coroutine of this plugin's. @date 06-10-2026-06:34 */
struct napper_t : public vc::object_t {
    napper_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~napper_t() {}

    static vc::ref_t<napper_t> create() {
        return std::make_shared<napper_t>(vc::object_t::Private{type_id_static()});
    }

    virtual vc::object_type_e type_id() const override { return NAPPER_TYPE(); }
    static vc::object_type_e type_id_static() { return NAPPER_TYPE(); }
    virtual std::string to_string() const override { return "napper_t"; }

    /*! **Coroutine** that sleeps `ms` milliseconds on the pool it runs on and answers twice `ms`.
     * @date 06-10-2026-06:34 */
    co::task<int64_t> nap(int64_t ms) {
        co_await co::sleep_ms((uint64_t)ms);
        co_return ms * 2;
    }

    /*! **Coroutine** that answers the address of the pool it runs on, as a number, so the host can
     * tell whether a plugin's task runs on the host's pool. @date 06-10-2026-06:34 */
    co::task<int64_t> pool_addr() {
        co::pool_t *pool = co_await co::get_pool();
        co_return (int64_t)(intptr_t)pool;
    }

    /*! **Coroutine** that waits on a semaphore of this plugin's making, made on the pool it runs
     * on, which a second task of the plugin's signals after `ms` milliseconds; answers `ms`.
     * @date 06-10-2026-06:34 */
    co::task<int64_t> handoff(int64_t ms) {
        co::pool_t *pool = co_await co::get_pool();
        auto sem = co::create_sem(pool, 0);
        auto signal = [](co::sem_p s, int64_t ms) -> co::task_t {
            co_await co::sleep_ms((uint64_t)ms);
            s->signal();
            co_return 0;
        };
        pool->sched(signal(sem, ms));
        co_await sem->wait();
        co_return ms;
    }

    /*! **Coroutine** that throws once it has waited, which the calling script hears as a raise.
     * @date 06-10-2026-06:34 */
    co::task<int64_t> fail(int64_t ms) {
        co_await co::sleep_ms((uint64_t)ms);
        throw vc::except_t("napper: asked to fail");
        co_return 0;
    }
};

#include "../../../virt_composer_end.h"

/*! Answers a new napper. @date 06-10-2026-06:34 */
static int napper_make(lua_State *L) {
    vc::push_vc_object(L, napper_t::create());
    return 1;
}

VIRT_COMPOSER_PLUGIN_EXPORT const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;

    VC_REGISTER_MEMBER_FUNCTION(vs, napper_t, nap, int64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, napper_t, pool_addr);
    VC_REGISTER_MEMBER_FUNCTION(vs, napper_t, handoff, int64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, napper_t, fail, int64_t);
    vc::add_plugin_internal_func(vs, "napper_make", napper_make);
    return 0;
}

/*! Opens this plugin's own log. 06-10-2026-06:34 */
VIRT_COMPOSER_PLUGIN_EXPORT int plugin_init(const char *logfile) {
    return logger_init(logfile);
}

/*! Closes this plugin's log. 06-10-2026-06:34 */
VIRT_COMPOSER_PLUGIN_EXPORT void plugin_uninit() {
    logger_uninit();
}
