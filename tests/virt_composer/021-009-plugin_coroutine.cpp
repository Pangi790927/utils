/* Test21 - Plugins: a plugin's coroutine runs on its host's pool.
 *
 * A plugin carries its own copy of the library and of colib. A member of its that answers a
 * co::task is awaited by a script on the host's lua_coro_t: the task is made by the plugin's colib,
 * scheduled and resumed by the host's pool, and its result pushed back into the host's state.
 *
 * The claims: a plugin's nap waits and answers; two scripts waiting on the plugin at once overlap
 * on the one pool rather than run one after the other; a plugin's task runs on the host's pool, the
 * one luaw_get_pool() answers; a semaphore the plugin makes on that pool hands off between two of
 * its tasks; a plugin's task that throws raises in the script that waited on it, and the next wait
 * still works.
 *
 * 06-10-2026-06:34 */

#include "tests_common.h"
#include "../../virt_composer_coroutines.h"
#include "../../virt_composer_end.h"

#include <chrono>

static const char *PLUGIN = "plugins/mock_plugin_coro" PLUGIN_EXT;

/*! Answers milliseconds since an arbitrary start. @date 06-10-2026-06:34 */
static int64_t now_ms() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

/*! Answers a state with the coroutine component, the plugin registered, and the scripts below.
 * @date 06-10-2026-06:34 */
static std::shared_ptr<vc::virt_state_t> make_state() {
    auto vs = vc::create_state();
    if (!vs || vc::coroutines_register_meta(vs.get()) != vc::VC_ERROR_OK
            || plugin_into(vs.get(), PLUGIN, "021-009-coro.tmp") < 0)
        return nullptr;
    auto path = write_temp_yaml("021-009",
        "napper_make:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function naps(ms) return vc.napper_make():nap(ms) end\n"
        "    function pool_of() return vc.napper_make():pool_addr() end\n"
        "    function hands_off(ms) return vc.napper_make():handoff(ms) end\n"
        "    function fails(ms) return vc.napper_make():fail(ms) end\n");
    if (vc::parse_config(vs.get(), path.c_str()) != vc::VC_ERROR_OK)
        return nullptr;
    return vs;
}

/*! Answers a coroutine set to call `fn(arg)`, or null. @date 06-10-2026-06:34 */
static vc::ref_t<vc::lua_coro_t> call(vc::virt_state_t *vs, const char *fn, int64_t arg) {
    auto co = vc::lua_coro_t::create(vs);
    if (co->set_call(fn, arg) != vc::VC_ERROR_OK) {
        DBG("set_call %s was refused", fn);
        return nullptr;
    }
    return co;
}

/*! Expects `co` to have answered `want` without error. @date 06-10-2026-06:34 */
static bool answered(vc::ref_t<vc::lua_coro_t> co, const char *what, int64_t want) {
    auto [got, err] = co->result<int64_t>();
    if (err != vc::VC_ERROR_OK || got != want) {
        DBG("%s answered %lld err %d, not %lld", what, (long long)got, (int)err, (long long)want);
        return false;
    }
    return true;
}

/*! One nap waits its time and answers; two at once overlap. @date 06-10-2026-06:34 */
static bool naps_wait_and_overlap(vc::virt_state_t *vs) {
    auto pool = vc::luaw_get_pool(vs);
    auto one = call(vs, "naps", 40);
    if (!one)
        return false;
    int64_t t0 = now_ms();
    pool->sched(one->run());
    pool->run();
    int64_t took = now_ms() - t0;
    bool ok = answered(one, "a nap of 40", 80);
    if (took < 40) {
        DBG("a nap of 40 ms took %lld ms", (long long)took);
        ok = false;
    }

    auto a = call(vs, "naps", 120);
    auto b = call(vs, "naps", 60);
    if (!a || !b)
        return false;
    t0 = now_ms();
    pool->sched(a->run());
    pool->sched(b->run());
    pool->run();
    took = now_ms() - t0;
    ok = answered(a, "a nap of 120", 240) && ok;
    ok = answered(b, "a nap of 60", 120) && ok;
    /* One after the other would take 180 ms; overlapping takes the longer, 120. 06-10-2026-06:34 */
    if (took < 120 || took >= 175) {
        DBG("two naps of 120 and 60 ms took %lld ms together", (long long)took);
        ok = false;
    }
    return ok;
}

/*! The plugin's task runs on the pool the host's state runs. @date 06-10-2026-06:34 */
static bool runs_on_the_hosts_pool(vc::virt_state_t *vs) {
    auto pool = vc::luaw_get_pool(vs);
    auto co = call(vs, "pool_of", 0);
    if (!co)
        return false;
    pool->sched(co->run());
    pool->run();
    return answered(co, "the plugin's pool", (int64_t)(intptr_t)pool.get());
}

/*! A semaphore the plugin makes on the host's pool hands off between two of its tasks.
 * @date 06-10-2026-06:34 */
static bool hands_off_through_a_semaphore(vc::virt_state_t *vs) {
    auto pool = vc::luaw_get_pool(vs);
    auto co = call(vs, "hands_off", 30);
    if (!co)
        return false;
    pool->sched(co->run());
    pool->run();
    return answered(co, "the handoff", 30);
}

/*! A throwing task raises in the script, and the next wait still works. @date 06-10-2026-06:34 */
static bool a_throw_raises_in_the_script(vc::virt_state_t *vs) {
    auto pool = vc::luaw_get_pool(vs);
    auto co = call(vs, "fails", 10);
    if (!co)
        return false;
    pool->sched(co->run());
    pool->run();
    auto [got, err] = co->result<int64_t>();
    bool ok = true;
    if (err == vc::VC_ERROR_OK) {
        DBG("a throwing task answered %lld without error", (long long)got);
        ok = false;
    }
    auto after = call(vs, "naps", 10);
    if (!after)
        return false;
    pool->sched(after->run());
    pool->run();
    return answered(after, "a nap after the throw", 20) && ok;
}

int main() {
    bool passed = true;
    auto vs = make_state();
    if (!vs) {
        print_test_result("021-009-plugin_coroutine.cpp", false);
        return 1;
    }

    passed = naps_wait_and_overlap(vs.get()) && passed;
    passed = runs_on_the_hosts_pool(vs.get()) && passed;
    passed = hands_off_through_a_semaphore(vs.get()) && passed;
    passed = a_throw_raises_in_the_script(vs.get()) && passed;

    vs = nullptr;
    vc::uninit_plugins();
    print_test_result("021-009-plugin_coroutine.cpp", passed);
    return passed ? 0 : 1;
}
