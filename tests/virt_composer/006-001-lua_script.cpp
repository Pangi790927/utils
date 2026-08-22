#include "tests_common.h"
#include "../../virt_composer_end.h"

#include <filesystem>
#include <fstream>

/* Test6 - Lua Scripts
================================================================================================= */

/* vc::lua_script_t (init_lua_script() in virt_composer.cpp) loads and immediately executes a Lua
chunk, either inline (`m_source`) or from a file (`m_source_path`); the two are mutually exclusive
and at least one is required. Scripts are meant to define functions (see virt_composer.h's
top-of-file docs: "scripts should focus on defining functions rather than performing actions
directly") that get invoked later via call_lua(). */

int test6_inline_source() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("006-001-inline",
        "greet:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function make_greeting(name)\n"
        "      return \"hello \" .. name\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto script_obj = vc::get_ref<vc::lua_script_t>(vs.get(), "greet");
    ASSERT_FN(CHK_PTR(script_obj.get()));
    ASSERT_FN(CHK_BOOL(script_obj->content.find("make_greeting") != std::string::npos));

    auto [ret, err] = vc::call_lua<std::string>(vs.get(), "make_greeting", std::string("world"));
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == "hello world"));

    return 0;
}

int test6_source_path() {
    /* get_file_string_content() (virt_composer.cpp) restricts m_source_path to files under the
    process's startup directory (std::filesystem::canonical("./") captured once, as `app_path`,
    the first time parse_config runs) unless VIRT_COMPOSER_ENABLE_LUA_IO/OS is set - so the script
    file has to live under the test binary's own working directory, same as the *.tmp.yaml files
    write_temp_yaml() produces. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    std::string script_path = "006-001-external.tmp.lua";
    std::ofstream lua_file(script_path, std::ios::trunc);
    lua_file << "function double_it(x) return x * 2 end\n";
    lua_file.close();

    auto path = write_temp_yaml("006-001-path",
        "loader:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source_path: " + script_path + "\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [ret, err] = vc::call_lua<int>(vs.get(), "double_it", 21);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 42));

    std::filesystem::remove(script_path);
    return 0;
}

int test6_both_fields_is_invalid() {
    /* init_lua_script() rejects a node that specifies both m_source and m_source_path (DBG + a
    null return), and neither m_source nor m_source_path (also a null return) - in both cases the
    object is simply never registered (co_return nullptr from init_lua_script(), propagated up
    through build_object()'s `co_return co_await init_lua_script(...)`), no exception is thrown
    and parse_config() still reports VC_ERROR_OK (same "silently missing" shape as an unresolved
    !ref, see 005-001's unresolved-reference case). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("006-001-both",
        "bad:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: \"x = 1\"\n"
        "  m_source_path: \"whatever.lua\"\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs.get(), "bad") == nullptr));

    return 0;
}

int test6_neither_field_is_invalid() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("006-001-neither",
        "bad:\n"
        "  m_type: vc::lua_script_t\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs.get(), "bad") == nullptr));

    return 0;
}

int test6_lua_script() {
    ASSERT_FN(test6_inline_source());
    ASSERT_FN(test6_source_path());
    ASSERT_FN(test6_both_fields_is_invalid());
    ASSERT_FN(test6_neither_field_is_invalid());
    return 0;
}

int main() {
    int ret = test6_lua_script();
    print_test_result("006-001-lua_script.cpp", ret >= 0);
    return ret;
}
