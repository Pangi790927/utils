/* Test21 - Plugins: a plugin claiming a name another already owns is refused, and the name keeps
 * answering for whoever registered it first.
 *
 * `plugins/mock_plugin_clash.cpp` registers `a_make`, which `plugins/mock_plugin_a.cpp` registered
 * before it. Two plugins are built by two people who never agreed on names, so this is an ordinary
 * accident. What must not come of it is a call written against one plugin running the other's
 * code, which is what replacing the entry would mean and what nothing would have said.
 *
 * The first claimant keeps the name, the second is turned away, and the load it was part of fails
 * rather than half succeeding.
 *
 * 2026-09-20 19:30 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

static const char *PLUGIN_A     = "plugins/mock_plugin_a.so";
static const char *PLUGIN_CLASH = "plugins/mock_plugin_clash.so";

/*! Binds `a_make` and reaches it, so the test can ask which plugin is answering to the name.
 * 2026-09-20 19:30 */
static std::string write_clash_config() {
    return write_temp_yaml("021-005-clash",
        "a_make:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function first_tag() return vc.a_make(1):tag() end\n");
}

static int test21_a_taken_name_is_refused() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    ASSERT_FN(vc::load_plugin(vs.get(), PLUGIN_A));

    if (vc::load_plugin(vs.get(), PLUGIN_CLASH) >= 0) {
        DBG("a plugin claiming a name another owns was accepted");
        return -1;
    }
    return 0;
}

static int test21_the_first_claimant_keeps_the_name() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    ASSERT_FN(vc::load_plugin(vs.get(), PLUGIN_A));
    ASSERT_FN(CHK_BOOL(vc::load_plugin(vs.get(), PLUGIN_CLASH) < 0));

    auto path = write_clash_config();
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    /* mock_plugin_a's a_make answers an object whose tag is 101. The clashing one answers the
    number 999, which has no tag at all, so a refusal that had not held would fail here rather
    than quietly returning the wrong thing. 2026-09-20 19:30 */
    auto [tag, err] = vc::call_lua<int64_t>(vs.get(), "first_tag");
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    if (tag != 101) {
        DBG("a_make answered %lld, so the name no longer belongs to the plugin that took it first",
                (long long)tag);
        return -1;
    }
    return 0;
}

/*! A name a plugin owns is its own to register again, which it does for every state it is loaded
 * into. The guard must not mistake that for a second claimant. 2026-09-20 19:30 */
static int test21_a_plugin_may_keep_its_own_name() {
    auto first = vc::create_state();
    ASSERT_FN(CHK_PTR(first.get()));
    ASSERT_FN(vc::load_plugin(first.get(), PLUGIN_A));

    auto second = vc::create_state();
    ASSERT_FN(CHK_PTR(second.get()));
    ASSERT_FN(vc::load_plugin(second.get(), PLUGIN_A));
    return 0;
}

/*! A second state must not be able to hand the name to someone else.
 *
 * The internal-function table belongs to the module rather than to a state, so a state that
 * forgot who owned `a_make` would let the next plugin change what the first state answers. A
 * per-state record of owners let exactly that through when this was first written.
 * 2026-09-20 19:45 */
static int test21_a_second_state_cannot_take_the_name() {
    auto first = vc::create_state();
    ASSERT_FN(CHK_PTR(first.get()));
    ASSERT_FN(vc::load_plugin(first.get(), PLUGIN_A));

    auto second = vc::create_state();
    ASSERT_FN(CHK_PTR(second.get()));
    if (vc::load_plugin(second.get(), PLUGIN_CLASH) >= 0) {
        DBG("a fresh state let a plugin take a name another plugin already owned");
        return -1;
    }
    return 0;
}

int main() {
    int ret = 0;

    ASSERT_FN(test21_a_taken_name_is_refused());
    ASSERT_FN(test21_a_second_state_cannot_take_the_name());
    ASSERT_FN(test21_the_first_claimant_keeps_the_name());
    ASSERT_FN(test21_a_plugin_may_keep_its_own_name());

    print_test_result("021-005-refused_name.cpp", ret >= 0);
    return ret;
}
