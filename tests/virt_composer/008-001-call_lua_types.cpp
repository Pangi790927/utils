#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test8 - call_lua() argument/return type conversions
================================================================================================= */

/* call_lua<R>(vs, name, args...) pushes each C++ arg through luaw_push_cpp_object() and converts
the single Lua return value back through luaw_lua_to_cpp_object() (see virt_composer.h's
call_lua()). Both of those are constexpr-dispatched on type, with explicit support for: string,
bool, integral, floating point, std::vector<T>, std::tuple<Args...>, std::pair<A,B> and
vc::ref_t<T> - this test exercises one call per supported shape, plus the void-return and
failed-call paths. */

int test8_scalars() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("008-001-scalars",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function add(a, b) return a + b end\n"
        "    function shout(s) return s .. \"!\" end\n"
        "    function negate(b) return not b end\n"
        "    function set_flag() flag_was_called = true end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    {
        auto [ret, err] = vc::call_lua<int>(vs.get(), "add", 3, 4);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == 7));
    }
    {
        auto [ret, err] = vc::call_lua<double>(vs.get(), "add", 1.5, 2.25);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == 3.75));
    }
    {
        auto [ret, err] = vc::call_lua<std::string>(vs.get(), "shout", std::string("hi"));
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == "hi!"));
    }
    {
        auto [ret, err] = vc::call_lua<bool>(vs.get(), "negate", true);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == false));
    }
    {
        /* void return: call_lua<void>() reports success/failure via `err` alone, `ret` is a
        placeholder int (see call_lua()'s `std::conditional_t<!std::is_void_v<R>, R, int>`). */
        auto [ret, err] = vc::call_lua<void>(vs.get(), "set_flag");
        (void)ret;
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));

        auto [flag, ferr] = vc::call_lua<bool>(vs.get(), "add", true, false);
        (void)flag; (void)ferr; /* just making sure the state is still usable afterwards */
    }

    return 0;
}

int test8_vector_and_tuple() {
    /* vector<T>/tuple<Args...> are exercised both as call_lua() *arguments* (luaw_push_cpp_object())
    and as its *return type* R (luaw_lua_to_cpp_object() + the pair<R, err_e> construction in
    call_lua() itself). The return-type direction used to fail to compile - call_lua<R>()'s
    argument-push failure path did `return {0, VC_ERROR_FAILED_CALL};`, and that literal `0`
    couldn't construct a pair<vector<T>, err_e>/pair<tuple<...>, err_e> (see BUGS.md history: fixed
    by value-initializing a same-typed placeholder instead of using `0`). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("008-001-vectuple",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function sum_vec(v)\n"
        "      local total = 0\n"
        "      for i = 1, #v do total = total + v[i] end\n"
        "      return total\n"
        "    end\n"
        "    function sum_tuple(t) return t[1] + t[2] end\n"
        "    function double_each(v)\n"
        "      local out = {}\n"
        "      for i = 1, #v do out[i] = v[i] * 2 end\n"
        "      return out\n"
        "    end\n"
        "    function make_pair()\n"
        "      return {11, \"eleven\"}\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    {
        std::vector<int> v{1, 2, 3, 4};
        auto [ret, err] = vc::call_lua<int>(vs.get(), "sum_vec", v);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == 10));
    }
    {
        auto [ret, err] = vc::call_lua<int>(vs.get(), "sum_tuple", std::make_tuple(3, 4));
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == 7));
    }
    {
        std::vector<int> v{1, 2, 3};
        auto [ret, err] = vc::call_lua<std::vector<int>>(vs.get(), "double_each", v);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret.size() == 3));
        ASSERT_FN(CHK_BOOL(ret[0] == 2 && ret[1] == 4 && ret[2] == 6));
    }
    {
        auto [ret, err] = vc::call_lua<std::tuple<int, std::string>>(vs.get(), "make_pair");
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(std::get<0>(ret) == 11));
        ASSERT_FN(CHK_BOOL(std::get<1>(ret) == "eleven"));
    }
    {
        /* the actual regression case: an unknown function forces call_lua<R>() through the
        argument-push-independent lua_pcall-failure path with R = vector<int>, which previously
        never even got a chance to run because the *whole function* failed to compile for this R. */
        auto [ret, err] = vc::call_lua<std::vector<int>>(vs.get(), "no_such_function_at_all");
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_FAILED_CALL));
        ASSERT_FN(CHK_BOOL(ret.empty()));
    }

    return 0;
}

int test8_ref_argument() {
    /* vc::ref_t<T> args go through luaw_push_cpp_object()'s is_vc_ref branch (push_vc_object as
    light userdata), and a Lua function can read them back out via the object's registered member
    getters (VC_REGISTER_MEMBER_OBJECT for `value` is already wired up for integer_t inside
    create_state() itself). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("008-001-refarg",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function read_value(obj) return obj.value end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto i = vc::integer_t::create(1234);
    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "read_value", i);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 1234));

    return 0;
}

int test8_unknown_function_fails() {
    /* call_lua() on a name that was never defined as a Lua global: lua_getglobal() pushes nil,
    lua_pcall() on a nil value fails, and call_lua() reports VC_ERROR_FAILED_CALL rather than
    throwing or crashing. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto [ret, err] = vc::call_lua<int>(vs.get(), "this_function_was_never_defined", 1);
    (void)ret;
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_FAILED_CALL));

    return 0;
}

int test8_call_lua_types() {
    ASSERT_FN(test8_scalars());
    ASSERT_FN(test8_vector_and_tuple());
    ASSERT_FN(test8_ref_argument());
    ASSERT_FN(test8_unknown_function_fails());
    return 0;
}

int main() {
    int ret = test8_call_lua_types();
    print_test_result("008-001-call_lua_types.cpp", ret >= 0);
    return ret;
}
