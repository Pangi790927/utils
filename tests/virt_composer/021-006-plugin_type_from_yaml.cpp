/* Test21 - Plugins: a config names a plugin's type and gets an object of it.
 *
 * The last thing a plugin's types had not been shown doing. A plugin's type reaches yaml the same
 * way a host's custom type does: build_object() knows the built-in types by name and hands
 * everything else to the builders registered with add_named_builder_callback(), so what makes
 * `m_type: shapes::rect_t` mean anything is the builder shapes_composer registers, not anything
 * about being a plugin.
 *
 * Worth saying because the reverse is easy to assume: a plugin's type is not nameable in a config
 * merely by existing, and a plugin that registers no builder has types Lua can use and yaml
 * cannot.
 *
 * 2026-09-20 20:00 */

#include "tests_common.h"
#include "../../virt_composer_end.h"

static const char *REFERENCE_PLUGIN = "plugins/reference_plugin.so";

/*! Builds a state holding the reference plugin and a config that names one of its types.
 * 2026-09-20 20:00 */
static int start_with_a_rect_in_the_config(std::shared_ptr<vc::virt_state_t>& out) {
    out = vc::create_state();
    ASSERT_FN(CHK_PTR(out.get()));

    ASSERT_FN(vc::load_plugin(out.get(), REFERENCE_PLUGIN));

    auto path = write_temp_yaml("021-006-rect",
        "table_top:\n"
        "  m_type: shapes::rect_t\n"
        "  w: 6\n"
        "  h: 7\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function top_area() return vc.table_top:area() end\n"
        "    function top_w()    return vc.table_top.w end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(out.get(), path.c_str()) == vc::VC_ERROR_OK));
    return 0;
}

/*! The config's node became an object of the plugin's type, carrying what the node said.
 * 2026-09-20 20:00 */
static int test21_the_config_built_the_plugins_type(vc::virt_state_t *vs) {
    auto obj = vc::get_ref<vc::object_t>(vs, "table_top");
    ASSERT_FN(CHK_PTR(obj.get()));

    /* Past the host's own types, so it is the plugin's and not something the host would have
    built from the same node. 2026-09-20 20:00 */
    if (obj->type_id().value() < (int)vc::VIRT_TYPE_CNT) {
        DBG("table_top has id %d, which is one of the host's own types", obj->type_id().value());
        return -1;
    }
    return 0;
}

/*! An object the config built is reached from Lua by its config name, and answers through the
 * members and methods the plugin registered for its type. 2026-09-20 20:00 */
static int test21_lua_uses_what_the_config_built(vc::virt_state_t *vs) {
    auto [area, e1] = vc::call_lua<int64_t>(vs, "top_area");
    ASSERT_FN(CHK_BOOL(e1 == vc::VC_ERROR_OK));
    if (area != 42) {
        DBG("table_top's area is %lld, expected 42 from w: 6 and h: 7", (long long)area);
        return -1;
    }

    auto [w, e2] = vc::call_lua<int64_t>(vs, "top_w");
    ASSERT_FN(CHK_BOOL(e2 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(w == 6));
    return 0;
}

/*! A type the plugin registered no builder for cannot be named in a config, and the parse says so
 * rather than building something else. 2026-09-20 20:00 */
static int test21_a_type_without_a_builder_is_refused() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    ASSERT_FN(vc::load_plugin(vs.get(), REFERENCE_PLUGIN));

    /* vec2_composer registers no builder, so this type exists and is usable from Lua but has no
    way into a config. 2026-09-20 20:00 */
    auto path = write_temp_yaml("021-006-novec",
        "somewhere:\n"
        "  m_type: vec2::vec2_t\n"
        "  x: 1\n"
        "  y: 2\n");
    if (vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK) {
        DBG("a config named a type no builder was registered for, and was accepted");
        return -1;
    }
    return 0;
}

int main() {
    int ret = 0;

    std::shared_ptr<vc::virt_state_t> vs;
    ASSERT_FN(start_with_a_rect_in_the_config(vs));

    ASSERT_FN(test21_the_config_built_the_plugins_type(vs.get()));
    ASSERT_FN(test21_lua_uses_what_the_config_built(vs.get()));
    ASSERT_FN(test21_a_type_without_a_builder_is_refused());

    print_test_result("021-006-plugin_type_from_yaml.cpp", ret >= 0);
    return ret;
}
