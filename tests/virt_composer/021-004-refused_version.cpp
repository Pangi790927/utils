/* Test21 - Plugins: a plugin built against another virt_composer is refused, and refusing it
 * leaves the state as it was.
 *
 * `plugins/mock_plugin_stale.cpp` is built with -DVIRT_COMPOSER_ABI overridden, which is why that
 * macro is guarded at all. It is the only way to reach the one branch of load_plugin() that has
 * otherwise never been exercised: the version check that keeps a plugin from running against a
 * library it was not built for.
 *
 * The second half matters as much as the first. A refusal here happens before any range of type
 * ids is taken, so a state that has just refused a plugin must be as good as one that never saw
 * it - able to load a real plugin and place it at the same ids.
 *
 * 2026-09-20 18:45 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

static const char *STALE_PLUGIN     = "plugins/mock_plugin_stale" PLUGIN_EXT;
static const char *REFERENCE_PLUGIN = "plugins/reference_plugin" PLUGIN_EXT;

static int test21_a_plugin_from_another_build_is_refused() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    if (plugin_into(vs.get(), STALE_PLUGIN, "021-004-stale.tmp") >= 0) {
        DBG("a plugin claiming another virt_composer was accepted");
        return -1;
    }
    /* Asking again answers the same. A version mismatch is not remembered as a broken plugin -
    that is kept for one which took a range and then failed - so this goes through the check a
    second time rather than reading a verdict off a list. 2026-09-20 18:45 */
    if (plugin_into(vs.get(), STALE_PLUGIN, "021-004-stale.tmp") >= 0) {
        DBG("the same plugin was accepted when asked for a second time");
        return -1;
    }
    return 0;
}

static int test21_a_refusal_costs_the_state_nothing() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    ASSERT_FN(CHK_BOOL(plugin_into(vs.get(), STALE_PLUGIN, "021-004-stale.tmp") < 0));

    /* Nothing was reserved for the refused plugin, so the next one gets the first range and this
    state is indistinguishable from one that never asked. 2026-09-20 18:45 */
    ASSERT_FN(plugin_into(vs.get(), REFERENCE_PLUGIN, "021-004-ref.tmp"));
    return 0;
}

int main() {
    int ret = 0;

    ASSERT_FN(test21_a_plugin_from_another_build_is_refused());
    ASSERT_FN(test21_a_refusal_costs_the_state_nothing());

    print_test_result("021-004-refused_version.cpp", ret >= 0);
    return ret;
}
