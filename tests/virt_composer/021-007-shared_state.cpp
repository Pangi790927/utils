/* Test21 - Plugins: a plugin reaches its host's owners of names, and keeps everything else its own.
 *
 * Every module carries its own copy of virt_composer, and the ABI hash keeps the copies in
 * agreement. What one of them must share with the host is marked VC_API and found in the main
 * program; today that is the table of who owns each registered name, without which two plugins
 * could each take the same `[INTERNAL]` name. Everything else stays each module's own: its logger,
 * its type counters, its internal-function table, which no state made by the host ever reads.
 *
 * So the plugin is asked where it finds each of them: the owners of names must be where the host
 * finds them, and the rest must not.
 *
 * 28-09-2026-12:00 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

static const char *PLUGIN_A = "plugins/mock_plugin_a" PLUGIN_EXT;

int main() {
    bool passed = true;
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs));
    ASSERT_FN(plugin_into(vs.get(), PLUGIN_A, "021-007-a.tmp"));

    auto where = (const void *(*)(int))plugin_symbol(PLUGIN_A, "plugin_state");
    ASSERT_FN(CHK_PTR((void *)where));

    const void *host[] = {&vc::name_owner(), &_logger_data, &vc::VIRT_TYPE_CNT,
            vc::c_function_t::own_internal_funcs()};
    const char *what[] = {"the owners of names", "the logger", "the type count",
            "the internal-function table"};
    for (int i = 0; i < 4; i++) {
        const void *plugin = where(i);
        bool shared = plugin == host[i];
        bool want_shared = i == 0;
        DBG("%s: host %p, plugin %p", what[i], host[i], plugin);
        if (shared != want_shared) {
            DBG("claim failed: the plugin %s %s", want_shared ? "does not share" : "shares",
                    what[i]);
            passed = false;
        }
    }

    vs = nullptr;
    print_test_result("021-007-shared_state.cpp", passed);
    return passed ? 0 : 1;
}
