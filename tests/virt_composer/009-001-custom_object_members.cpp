#include "tests_common.h"

/* Test9 - Custom Object Types & Member Registration
================================================================================================= */

/* Real composers (see vulkan/vulkan_composer.h) define their own vc::object_t-derived types and
expose C++ members to Lua/yaml with VC_REGISTER_MEMBER_OBJECT/VC_REGISTER_MEMBER_FUNCTION. Doing
that requires: (1) VIRT_COMPOSER_REGISTER_TYPE(...) for a new enum value, which must happen in
this translation unit *before* virt_composer_end.h locks in VIRT_TYPE_CNT (that's why this file's
`#include "../../virt_composer_end.h"` is down here, after the type + registration, instead of
right under tests_common.h like every other test in this directory); (2) a struct following the
exact Private-constructor/create()/type_id()/type_id_static()/to_string() shape integer_t etc. use
(see virt_composer.h's own vc::integer_t); (3) registering it during/after create_state(). */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_POINT);

/* Exercises luaw_returner_t<Enum> (the is_vc_enum-constrained specialization living alongside
Integer/Floating in virt_composer.h) via quadrant() below. Making an enum usable there at all needs
the same get_enum_val<T> specialization every enum in vulkan_composer.h goes through (see
virt_composer.h's get_enum_val() doc comment): a string-name table, forwarded to the two-argument
overload. Declared before point_t since it's used as a member function's return type there. */
enum class point_quadrant_e {
    ORIGIN,
    I,
    II,
    III,
    IV,
};

namespace virt_composer {

inline std::unordered_map<std::string, point_quadrant_e> point_quadrant_from_str = {
    {"ORIGIN", point_quadrant_e::ORIGIN},
    {"I", point_quadrant_e::I},
    {"II", point_quadrant_e::II},
    {"III", point_quadrant_e::III},
    {"IV", point_quadrant_e::IV},
};

template <> inline point_quadrant_e get_enum_val<point_quadrant_e>(fkyaml::node &n) {
    return get_enum_val(n, point_quadrant_from_str);
}

} /* virt_composer */

struct point_t : public vc::object_t {
    int64_t x = 0;
    int64_t y = 0;

    point_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~point_t() {}

    static vc::ref_t<point_t> create(int64_t x, int64_t y) {
        auto ret = std::make_shared<point_t>(vc::object_t::Private{type_id_static()});
        ret->x = x;
        ret->y = y;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_POINT; }
    static vc::object_type_e type_id_static() { return TC_TYPE_POINT; }

    int64_t manhattan_len() const { return (x < 0 ? -x : x) + (y < 0 ? -y : y); }

    point_quadrant_e quadrant() const {
        if (x == 0 && y == 0) return point_quadrant_e::ORIGIN;
        if (x > 0 && y > 0)   return point_quadrant_e::I;
        if (x < 0 && y > 0)   return point_quadrant_e::II;
        if (x < 0 && y < 0)   return point_quadrant_e::III;
        return point_quadrant_e::IV;
    }

    inline std::string to_string() const override {
        return std::format("point_t[{}]: x={} y={}", (void*)this, x, y);
    }
};

#include "../../virt_composer_end.h"

static vc::ref_t<vc::virt_state_t> make_state_with_point_registered() {
    auto vs = vc::create_state();
    if (!vs)
        return nullptr;
    VC_REGISTER_MEMBER_OBJECT(vs.get(), point_t, x);
    VC_REGISTER_MEMBER_OBJECT(vs.get(), point_t, y);
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), point_t, manhattan_len);
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), point_t, quadrant);
    return vs;
}

int test9_member_object_read_write_from_lua() {
    auto vs = make_state_with_point_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto p = point_t::create(3, -4);

    /* Note: call_lua<R>() only ever requests one Lua result (lua_pcall's nresults=1), and
    luaw_lua_to_cpp_object()'s tuple branch expects that one result to be a table - so read_xy
    must return `{pt.x, pt.y}` (a table), not Lua's native multi-return `return pt.x, pt.y`
    (which call_lua<tuple<...>>() would silently truncate to just the first value). */
    auto path = write_temp_yaml("009-001-memberobj",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function read_xy(pt) return {pt.x, pt.y} end\n"
        "    function write_x(pt, v) pt.x = v end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [xy, err] = vc::call_lua<std::tuple<int64_t, int64_t>>(vs.get(), "read_xy", p);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(std::get<0>(xy) == 3));
    ASSERT_FN(CHK_BOOL(std::get<1>(xy) == -4));

    auto [write_ret, werr] = vc::call_lua<void>(vs.get(), "write_x", p, 99);
    (void)write_ret;
    ASSERT_FN(CHK_BOOL(werr == vc::VC_ERROR_OK));
    /* the setter (luaw_member_setter_object_wrapper) mutates the same C++ object `p` points to -
    it's not a copy that gets thrown away once control returns to Lua. */
    ASSERT_FN(CHK_BOOL(p->x == 99));

    return 0;
}

int test9_member_function_call_from_lua() {
    auto vs = make_state_with_point_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto p = point_t::create(3, -4);

    auto path = write_temp_yaml("009-001-memberfn",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function get_len(pt) return pt:manhattan_len() end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [len, err] = vc::call_lua<int64_t>(vs.get(), "get_len", p);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(len == 7));

    return 0;
}

int test9_member_function_enum_return_from_lua() {
    auto vs = make_state_with_point_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto p = point_t::create(3, -4);

    auto path = write_temp_yaml("009-001-memberfn-enum",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function get_quadrant(pt) return pt:quadrant() end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    /* quadrant() returns point_quadrant_e - luaw_returner_t<Enum> pushes it as a plain lua_Number
    (not a string), so the Lua side hands back a number matching the underlying int value. */
    auto [q, err] = vc::call_lua<int>(vs.get(), "get_quadrant", p);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(q == (int)point_quadrant_e::IV));

    return 0;
}

int test9_custom_object_members() {
    ASSERT_FN(test9_member_object_read_write_from_lua());
    ASSERT_FN(test9_member_function_call_from_lua());
    ASSERT_FN(test9_member_function_enum_return_from_lua());
    return 0;
}

int main() {
    int ret = test9_custom_object_members();
    print_test_result("009-001-custom_object_members.cpp", ret >= 0);
    return ret;
}
