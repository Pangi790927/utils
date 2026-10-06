/* Test21 - Plugins: a plugin is loaded once for the process, logging into the file its host names,
 * and registered into each state apart.
 *
 * Every module carries its own logger, so a plugin's DBG goes wherever the plugin's own logger was
 * opened. load_plugin() opens it through the plugin's plugin_init, at the logfile it is handed,
 * and touches no state; register_plugin() is what puts the plugin into a state.
 *
 * The claims: the plugin's DBG lands in its file and not in the host's log; loading it again with
 * the same logfile does nothing, and with another is refused and opens nothing; a second state
 * gets the plugin by registering it, with no second load; a plugin not loaded cannot be
 * registered; a logfile the host or another plugin has, and an empty one, are refused, and the
 * plugin may still load afterwards with a file of its own; a plugin without plugin_init is
 * refused; uninit_plugins() runs every plugin_uninit once, however often it is called.
 *
 * 05-10-2026-22:33 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

#include <sstream>

static const char *PLUGIN_A      = "plugins/mock_plugin_a" PLUGIN_EXT;
static const char *PLUGIN_B      = "plugins/mock_plugin_b" PLUGIN_EXT;
static const char *PLUGIN_NOINIT = "plugins/mock_plugin_noinit" PLUGIN_EXT;

/*! Answers what a file holds, empty when there is none. @date 05-10-2026-22:33 */
static std::string read_file(const char *path) {
    std::ifstream f(path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

/*! Expects `path` to hold `want` exactly `times` times. @date 05-10-2026-22:33 */
static bool holds(const char *path, const char *want, int times) {
    std::string s = read_file(path);
    int n = 0;
    for (size_t at = s.find(want); at != std::string::npos; at = s.find(want, at + 1))
        n++;
    if (n != times) {
        DBG("%s holds '%s' %d times, not %d", path, want, n, times);
        return false;
    }
    return true;
}

/*! Binds mock_plugin_a's a_ping in the state, as 021-002 does, and calls it once from Lua.
 * @date 05-10-2026-22:33 */
static bool ping(vc::virt_state_t *vs) {
    auto path = write_temp_yaml("021-008",
        "a_ping:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function call_a_ping() return vc.a_ping() end\n");
    if (vc::parse_config(vs, path.c_str()) != vc::VC_ERROR_OK) {
        DBG("the config binding a_ping did not parse");
        return false;
    }
    auto [r, err] = vc::call_lua<int64_t>(vs, "call_a_ping");
    if (err != vc::VC_ERROR_OK || r != 1000) {
        DBG("a_ping answered %lld err %d", (long long)r, (int)err);
        return false;
    }
    return true;
}

/*! Expects `what` to have answered a refusal, and says `why` when it did not.
 * @date 05-10-2026-22:33 */
static bool refused(int what, const char *why) {
    if (what >= 0) {
        DBG("not refused: %s", why);
        return false;
    }
    return true;
}

int main() {
    bool passed = true;
    /* Earlier runs' logs would count toward what is looked for below. 05-10-2026-22:33 */
    for (auto f : {"021-008-a.tmp.log", "021-008-a.tmp.old.log", "021-008-b.tmp.log",
            "021-008-b.tmp.old.log", "021-008-other.tmp.log"})
        std::filesystem::remove(f);

    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs));
    ASSERT_FN(vc::load_plugin(PLUGIN_A, "021-008-a.tmp"));
    ASSERT_FN(vc::register_plugin(vs.get(), PLUGIN_A));
    passed = ping(vs.get()) && passed;
    passed = holds("021-008-a.tmp.log", "a_ping ran", 1) && passed;
    passed = holds("logfile.log", "a_ping ran", 0) && passed;

    /* Loaded already: the same logfile is nothing new, another is refused. 05-10-2026-22:33 */
    passed = vc::load_plugin(PLUGIN_A, "021-008-a.tmp") == 0 && passed;
    passed = refused(vc::load_plugin(PLUGIN_A, "021-008-other.tmp"),
            "a loaded plugin was given a second logfile") && passed;
    if (std::filesystem::exists("021-008-other.tmp.log")) {
        DBG("a second logfile was opened for a plugin already loaded");
        passed = false;
    }

    /* A second state gets a by registering it, with no load of its own. 05-10-2026-22:33 */
    auto second = vc::create_state();
    ASSERT_FN(CHK_PTR(second));
    ASSERT_FN(vc::register_plugin(second.get(), PLUGIN_A));
    passed = ping(second.get()) && passed;
    passed = holds("021-008-a.tmp.log", "a_ping ran", 2) && passed;

    /* b is not loaded yet, so it cannot be registered; it may not take a's file, nor the host's,
    nor go without one. Each refusal leaves nothing behind, so b still loads with a file of its
    own. 05-10-2026-22:33 */
    passed = refused(vc::register_plugin(vs.get(), PLUGIN_B),
            "a plugin not loaded was registered") && passed;
    passed = refused(vc::load_plugin(PLUGIN_B, "021-008-a.tmp"), "b took a's logfile") && passed;
    passed = refused(vc::load_plugin(PLUGIN_B, "logfile"), "b took the host's logfile") && passed;
    passed = refused(vc::load_plugin(PLUGIN_B, ""), "b was loaded with no logfile") && passed;
    if (vc::load_plugin(PLUGIN_B, "021-008-b.tmp") < 0
            || vc::register_plugin(vs.get(), PLUGIN_B) < 0) {
        DBG("b did not load with a file of its own");
        passed = false;
    }

    passed = refused(vc::load_plugin(PLUGIN_NOINIT, "021-008-noinit.tmp"),
            "a plugin without plugin_init was loaded") && passed;

    vc::uninit_plugins();
    vc::uninit_plugins();
    passed = holds("021-008-a.tmp.log", "mock plugin log closing", 1) && passed;
    passed = holds("021-008-b.tmp.log", "mock plugin log closing", 1) && passed;

    second = nullptr;
    vs = nullptr;
    print_test_result("021-008-plugin_log.cpp", passed);
    return passed ? 0 : 1;
}
