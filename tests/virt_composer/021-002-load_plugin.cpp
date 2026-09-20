/* Test21 - Plugins: two shared objects, each built on its own, are loaded into one state and keep
 * their types and functions apart.
 *
 * The plugins are plugins/mock_plugin_a.cpp and plugins/mock_plugin_b.cpp, three types and two
 * functions each. Every number A answers with is in the hundreds and every number B answers with
 * is in the two hundreds, so a value arriving from the wrong plugin names itself.
 *
 * What is actually under test is that nothing is shared that should not be. Six types must take
 * six distinct ids, in two runs of three, the first starting where the host's own types end.
 * Ten things must be callable one at a time and answer for themselves: six `tag()` methods and
 * four registered functions. And a member registered by one plugin must not have landed on the
 * other's types, which is what `a_only`/`b_only` are for.
 *
 * Checking the ids by their exact values rather than by "not zero" is the point: an offset that
 * never arrives leaves a plugin's first type at 0, which is VC_TYPE_STRING, and a plugin quietly
 * answering "I am a string" is the failure this whole mechanism exists to prevent.
 *
 * 2026-09-20 17:14 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

#include <dlfcn.h>

static const char *PLUGIN_A = "plugins/mock_plugin_a.so";
static const char *PLUGIN_B = "plugins/mock_plugin_b.so";

/*! Answers the id a loaded plugin gave its nth type, counting from 1, or -1 if it cannot be asked.
 *
 * load_plugin() opens a plugin RTLD_LOCAL, so its symbols are not in the global namespace and a
 * handle of our own is the only way to reach them. Opening the same path again answers with the
 * handle that is already there rather than loading a second copy. 2026-09-20 17:14 */
static int ask_plugin_type_id(const char *path, int n) {
    void *h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        DBG("the test could not open %s itself: %s", path, dlerror());
        return -1;
    }
    auto fn = (int (*)(int))dlsym(h, "plugin_type_id");
    if (!fn) {
        DBG("plugin_type_id is missing from %s", path);
        return -1;
    }
    return fn(n);
}

/*! Writes the config both plugins' functions are bound through, and a script that reaches each of
 * them one at a time. A name here must match the name the plugin registered, since that is what
 * c_function_t::init() looks up. 2026-09-20 17:14 */
static std::string write_plugin_config() {
    return write_temp_yaml("021-002-plugins",
        "a_make:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "a_ping:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "b_make:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "b_ping:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function a_tag(n)   return vc.a_make(n):tag() end\n"
        "    function b_tag(n)   return vc.b_make(n):tag() end\n"
        "    function a_mine(n)  return vc.a_make(n):a_only() end\n"
        "    function b_mine(n)  return vc.b_make(n):b_only() end\n"
        "    function call_a_ping() return vc.a_ping() end\n"
        "    function call_b_ping() return vc.b_ping() end\n"
        "    function b_asked_for_a(n) return vc.b_make(n):a_only() end\n");
}

/*! Loads both plugins into a state and parses the config that binds their functions.
 *
 * One state for every check below, and it has to be: a plugin holds a single offset belonging to
 * the state that reserved it, so a second state asking for the same plugin is refused. That is the
 * rule load_plugin() states, and test21_a_plugin_serves_one_state is what holds it to it.
 * 2026-09-20 17:22 */
static int start_with_both_plugins(std::shared_ptr<vc::virt_state_t>& out) {
    out = vc::create_state();
    ASSERT_FN(CHK_PTR(out.get()));

    ASSERT_FN(vc::load_plugin(out.get(), PLUGIN_A));
    ASSERT_FN(vc::load_plugin(out.get(), PLUGIN_B));

    auto path = write_plugin_config();
    ASSERT_FN(CHK_BOOL(vc::parse_config(out.get(), path.c_str()) == vc::VC_ERROR_OK));
    return 0;
}

static int test21_refuses_what_it_cannot_use() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    if (vc::load_plugin(vs.get(), "plugins/there_is_no_such_plugin.so") >= 0) {
        DBG("a plugin that does not exist was accepted");
        return -1;
    }
    /* A real file that resolves and opens as nothing. It must be refused for missing exports
    rather than crashing on them. 2026-09-20 17:14 */
    if (vc::load_plugin(vs.get(), "021-002-load_plugin.cpp") >= 0) {
        DBG("a file that is not a shared object was accepted");
        return -1;
    }
    return 0;
}

static int test21_six_types_take_six_ids(vc::virt_state_t *vs) {
    /* This translation unit registers no types of its own, so the host's count is the six that
    virt_composer.h registers, and A's range starts exactly there. 2026-09-20 17:14 */
    int base = (int)vc::VIRT_TYPE_CNT;

    for (int n = 1; n <= 3; n++) {
        int want = base + n - 1;
        int got = ask_plugin_type_id(PLUGIN_A, n);
        if (got != want) {
            DBG("A's type %d has id %d, expected %d", n, got, want);
            return -1;
        }
    }
    for (int n = 1; n <= 3; n++) {
        int want = base + 3 + n - 1;
        int got = ask_plugin_type_id(PLUGIN_B, n);
        if (got != want) {
            DBG("B's type %d has id %d, expected %d - B did not start where A ended", n, got, want);
            return -1;
        }
    }
    return 0;
}

static int test21_every_type_answers_for_itself(vc::virt_state_t *vs) {
    for (int n = 1; n <= 3; n++) {
        auto [tag, err] = vc::call_lua<int64_t>(vs, "a_tag", n);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        if (tag != 100 + n) {
            DBG("A's type %d tagged itself %lld, expected %d", n, (long long)tag, 100 + n);
            return -1;
        }
    }
    for (int n = 1; n <= 3; n++) {
        auto [tag, err] = vc::call_lua<int64_t>(vs, "b_tag", n);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        if (tag != 200 + n) {
            DBG("B's type %d tagged itself %lld, expected %d", n, (long long)tag, 200 + n);
            return -1;
        }
    }
    return 0;
}

static int test21_every_function_answers_for_itself(vc::virt_state_t *vs) {
    auto [a, a_err] = vc::call_lua<int64_t>(vs, "call_a_ping");
    ASSERT_FN(CHK_BOOL(a_err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(a == 1000));

    auto [b, b_err] = vc::call_lua<int64_t>(vs, "call_b_ping");
    ASSERT_FN(CHK_BOOL(b_err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(b == 2000));

    /* a_make and b_make are the other two of the four, and every call above has already gone
    through one of them. Asked directly here so that each of the four is named once.
    2026-09-20 17:14 */
    for (int n = 1; n <= 3; n++) {
        auto [av, aerr] = vc::call_lua<int64_t>(vs, "a_mine", n);
        ASSERT_FN(CHK_BOOL(aerr == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(av == 100 + n));

        auto [bv, berr] = vc::call_lua<int64_t>(vs, "b_mine", n);
        ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(bv == 200 + n));
    }
    return 0;
}

static int test21_one_plugins_members_do_not_reach_the_other(vc::virt_state_t *vs) {
    /* a_only was registered on A's three types and on nothing else. A B object answering it would
    mean one plugin's registrations landed on another's rows. 2026-09-20 17:14 */
    for (int n = 1; n <= 3; n++) {
        auto [val, err] = vc::call_lua<int64_t>(vs, "b_asked_for_a", n);
        if (err == vc::VC_ERROR_OK) {
            DBG("B's type %d answered A's a_only with %lld", n, (long long)val);
            return -1;
        }
    }
    return 0;
}

/*! A plugin's range belongs to the process, so a second state may load the same plugin and must
 * see its types at the same ids. This is also the shape a rebuilt state takes - the old one is
 * destroyed and a new one asks for the same plugins - and a new state landing on the address the
 * old one had must not change the answer. 2026-09-20 17:45 */
static int test21_a_plugin_serves_many_states() {
    int want = ask_plugin_type_id(PLUGIN_A, 1);

    auto second = vc::create_state();
    ASSERT_FN(CHK_PTR(second.get()));
    ASSERT_FN(vc::load_plugin(second.get(), PLUGIN_A));

    if (ask_plugin_type_id(PLUGIN_A, 1) != want) {
        DBG("A's first type moved when a second state loaded it");
        return -1;
    }

    /* B is left out of this state on purpose: A's range is where it always was, and this state has
    to reach past nothing to find it. The other order is the interesting one, so a third state
    takes B alone and must find it past A's range, on rows it has no types for.
    2026-09-20 17:45 */
    int want_b = ask_plugin_type_id(PLUGIN_B, 1);

    auto third = vc::create_state();
    ASSERT_FN(CHK_PTR(third.get()));
    ASSERT_FN(vc::load_plugin(third.get(), PLUGIN_B));

    if (ask_plugin_type_id(PLUGIN_B, 1) != want_b) {
        DBG("B's first type moved when a state loaded it without A");
        return -1;
    }

    /* Asking a state for a plugin it already has does nothing and answers success. Nothing here
    can watch it do nothing, but the state would grow a second time if it did, so B's id moving
    would say so. 2026-09-20 18:55 */
    ASSERT_FN(vc::load_plugin(third.get(), PLUGIN_B));
    if (ask_plugin_type_id(PLUGIN_B, 1) != want_b) {
        DBG("B's first type moved when its own state was asked for it twice");
        return -1;
    }

    auto path = write_plugin_config();
    ASSERT_FN(CHK_BOOL(vc::parse_config(third.get(), path.c_str()) == vc::VC_ERROR_OK));

    /* The point of the skipped range: B's types work in a state that never loaded A, and the rows
    A would have used sit there unasked. 2026-09-20 17:45 */
    auto [tag, err] = vc::call_lua<int64_t>(third.get(), "b_tag", 1);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(tag == 201));
    return 0;
}

int main() {
    int ret = 0;

    ASSERT_FN(test21_refuses_what_it_cannot_use());

    std::shared_ptr<vc::virt_state_t> vs;
    ASSERT_FN(start_with_both_plugins(vs));

    ASSERT_FN(test21_six_types_take_six_ids(vs.get()));
    ASSERT_FN(test21_every_type_answers_for_itself(vs.get()));
    ASSERT_FN(test21_every_function_answers_for_itself(vs.get()));
    ASSERT_FN(test21_one_plugins_members_do_not_reach_the_other(vs.get()));
    ASSERT_FN(test21_a_plugin_serves_many_states());

    print_test_result("021-002-load_plugin.cpp", ret >= 0);
    return ret;
}
