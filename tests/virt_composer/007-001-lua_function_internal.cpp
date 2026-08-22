#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test7 - Lua Functions (internal C++ callbacks exposed to Lua)
================================================================================================= */

/* lua_function_t wraps a native C++ callback and exposes it as a callable Lua value through the
"__vc_metatable" __call metamethod (see luaopen_vc()'s __call handler, which dispatches to
lua_function_t::call()). Callbacks are registered ahead of time with
lua_function_t::add_internal_func(name, fn) - this is exactly the pattern
tests/test_vulkan_composer.cpp uses for "fill_buffer_with_quad_vertices" - then a yaml node with
`m_type: vc::lua_function_t` and `m_source: "[INTERNAL]"` looks the name up in that registry
(lua_function_t::init()) and binds it. Once bound, a Lua script can call it as `vc.<name>(...)`. */

static int g_last_arg_seen = -1;

int test7_internal_func_called_from_lua() {
    /* lua_function_t::init() looks the registry up by m_name, which is set to the YAML object's
    own key ("doubler" below) - not by any name chosen here - so the registered name must match
    that key exactly. */
    vc::lua_function_t::add_internal_func("doubler",
        [](lua_State *L) -> int {
            int x = (int)lua_tointeger(L, 1);
            g_last_arg_seen = x;
            lua_pushinteger(L, x * 2);
            return 1;
        });

    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    /* `vc` is not a Lua global by default - luaopen_vc() only registers the "virt_composer"
    module under package.loaded, so any script that wants to reach vc.<object_name> has to
    `require` it first (matching virt_composer.h's own top-of-file doc example). */
    auto path = write_temp_yaml("007-001-internal",
        "doubler:\n"
        "  m_type: vc::lua_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "caller:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function invoke_doubler(x)\n"
        "      return vc.doubler(x)\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto fn_obj = vc::get_ref<vc::lua_function_t>(vs.get(), "doubler");
    ASSERT_FN(CHK_PTR(fn_obj.get()));
    ASSERT_FN(CHK_BOOL(fn_obj->m_name == "doubler"));

    auto [ret, err] = vc::call_lua<int>(vs.get(), "invoke_doubler", 21);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 42));
    ASSERT_FN(CHK_BOOL(g_last_arg_seen == 21));

    return 0;
}

int test7_unknown_internal_func_fails_init() {
    /* lua_function_t::init() returns VC_ERROR_GENERIC when m_source == "[INTERNAL]" but no
    matching name was ever registered via add_internal_func() - lua_function_t::create() turns
    that into a thrown vc::except_t ("Failed lua_function_t init"), which build_object() lets
    propagate, and parse_config()'s outer catch(std::exception&) turns into VC_ERROR_GENERIC. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("007-001-unknown",
        "missing:\n"
        "  m_type: vc::lua_function_t\n"
        "  m_source: \"[INTERNAL]\"\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_GENERIC));

    return 0;
}

int test7_lua_function() {
    ASSERT_FN(test7_internal_func_called_from_lua());
    ASSERT_FN(test7_unknown_internal_func_fails_init());
    return 0;
}

int main() {
    int ret = test7_lua_function();
    print_test_result("007-001-lua_function_internal.cpp", ret >= 0);
    return ret;
}
