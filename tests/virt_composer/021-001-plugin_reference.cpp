/* Test21 - Plugins: a plugin written the ordinary way is loaded and used from Lua.
 *
 * The first of the plugin tests on purpose. `plugins/reference_plugin.cpp` is the file to read
 * before the others and the one to copy when writing a plugin. It is assembled the way a plugin
 * should be: its three types live in composer headers of their own under `plugins/reference/`,
 * each with its own `register_meta()`, and the final file only gathers them, closes the
 * registrations and puts the three exports on the outside - the same division a host keeps
 * between its `*_composer.h` files and its `main.cpp`. The plugins the later tests use share a
 * header and a declaration macro, which keeps them short at the cost of being a poor thing to
 * learn the shape from.
 *
 * What this shows, end to end: a shared object built on its own is loaded into a state that knew
 * nothing of it, its functions are bound by name from a yaml config, and a Lua script builds its
 * objects, reads and writes their members and calls their methods.
 *
 * 2026-09-20 18:45 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

static const char *REFERENCE_PLUGIN = "plugins/reference_plugin.so";

/*! Loads the reference plugin and parses a config that binds its three functions and drives them.
 *
 * Each function needs a yaml entry of its own whose key is the name the plugin registered, since
 * that key is what c_function_t::init() looks up. 2026-09-20 18:45 */
static int start_with_reference_plugin(std::shared_ptr<vc::virt_state_t>& out) {
    out = vc::create_state();
    ASSERT_FN(CHK_PTR(out.get()));

    ASSERT_FN(vc::load_plugin(out.get(), REFERENCE_PLUGIN));

    auto path = write_temp_yaml("021-001-reference",
        "ref_vec2:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "ref_rect:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "ref_circle:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function len2(x, y)    return vc.ref_vec2(x, y):len2() end\n"
        "    function area(w, h)    return vc.ref_rect(w, h):area() end\n"
        "    function circle(r)     return vc.ref_circle(r):area_x100() end\n"
        "    function read_x(x, y)  return vc.ref_vec2(x, y).x end\n"
        "    function moved(x, y)\n"
        "      local v = vc.ref_vec2(x, y)\n"
        "      v.x = v.x + 10\n"
        "      return v.x\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(out.get(), path.c_str()) == vc::VC_ERROR_OK));
    return 0;
}

/*! Each of the three functions builds an object of its own type, and each type's method answers
 * for that object. 2026-09-20 18:45 */
static int test21_three_types_and_three_functions(vc::virt_state_t *vs) {
    auto [len2, e1] = vc::call_lua<int64_t>(vs, "len2", 3, 4);
    ASSERT_FN(CHK_BOOL(e1 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(len2 == 25));

    auto [area, e2] = vc::call_lua<int64_t>(vs, "area", 6, 7);
    ASSERT_FN(CHK_BOOL(e2 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(area == 42));

    auto [circ, e3] = vc::call_lua<int64_t>(vs, "circle", 10);
    ASSERT_FN(CHK_BOOL(e3 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(circ == 31400));
    return 0;
}

/*! A member registered as an object is readable and writable from Lua, where one registered as a
 * function is called. Both kinds appear in the reference plugin and they are not the same thing.
 * 2026-09-20 18:45 */
static int test21_members_are_read_and_written(vc::virt_state_t *vs) {
    auto [x, e1] = vc::call_lua<int64_t>(vs, "read_x", 5, 9);
    ASSERT_FN(CHK_BOOL(e1 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(x == 5));

    auto [moved, e2] = vc::call_lua<int64_t>(vs, "moved", 5, 9);
    ASSERT_FN(CHK_BOOL(e2 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(moved == 15));
    return 0;
}

/*! The plugin's types sit past the host's own, which is the whole of what loading one arranges.
 * 2026-09-20 18:45 */
static int test21_its_types_sit_past_the_hosts(vc::virt_state_t *vs) {
    /* The reference plugin exports no way to ask for its ids, on purpose - it is meant to be
    copied, and a test hook has no business in it. Asking Lua what it built answers just as well:
    a type id below the host's count would be one of the host's own types. 2026-09-20 18:45 */
    auto [len2, err] = vc::call_lua<int64_t>(vs, "len2", 1, 0);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(len2 == 1));

    auto obj = vc::get_ref<vc::object_t>(vs, "ref_vec2");
    ASSERT_FN(CHK_PTR(obj.get()));
    return 0;
}

int main() {
    int ret = 0;

    std::shared_ptr<vc::virt_state_t> vs;
    ASSERT_FN(start_with_reference_plugin(vs));

    ASSERT_FN(test21_three_types_and_three_functions(vs.get()));
    ASSERT_FN(test21_members_are_read_and_written(vs.get()));
    ASSERT_FN(test21_its_types_sit_past_the_hosts(vs.get()));

    print_test_result("021-001-plugin_reference.cpp", ret >= 0);
    return ret;
}
