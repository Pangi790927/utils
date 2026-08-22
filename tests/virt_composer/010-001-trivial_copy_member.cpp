#include "tests_common.h"

/* Test10 - Trivially-Copyable Member Access (register_trivially_copyable_member / resolve_memb)
================================================================================================= */

/* VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER registers a memcpy-based accessor for a POD member (see
register_trivially_copyable_member() in virt_composer.h and resolve_memb_data() in
virt_composer.cpp) - this is meant for yaml fields tagged `!copy`, which resolve_memb<T>() reads
via the `object`/`member` sub-fields of the node (see resolve_memb()'s
`node.get_tag_name() != "!copy"` check). It's a separate mechanism from
VC_REGISTER_MEMBER_OBJECT/luaw_register_member_object - that one is Lua-facing (get/set through
the __index/__newindex metamethods); this one is yaml-facing (used from inside a builder callback,
not from a running Lua script). */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_VEC3);

struct vec3_t : public vc::object_t {
    float x = 0, y = 0, z = 0;

    vec3_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~vec3_t() {}

    static vc::ref_t<vec3_t> create(float x, float y, float z) {
        auto ret = std::make_shared<vec3_t>(vc::object_t::Private{type_id_static()});
        ret->x = x; ret->y = y; ret->z = z;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_VEC3; }
    static vc::object_type_e type_id_static() { return TC_TYPE_VEC3; }

    inline std::string to_string() const override {
        return std::format("vec3_t[{}]: x={} y={} z={}", (void*)this, x, y, z);
    }
};

#include "../../virt_composer_end.h"

/* resolve_memb<T>() is documented as "to be used inside the build_object callback" - it is NOT
wired into the builtin vc::float_t/vc::integer_t builders (those only ever call resolve_float()/
resolve_int(), which know about `!ref` but nothing about `!copy`). So exercising it needs a small
custom builder of our own that explicitly does `co_await resolve_memb<float>(vs, node["value"])` -
that's what "tc::float_copy_t" is below. */
static vc::err_e register_float_copy_builder(vc::virt_state_t *vs) {
    return add_named_builder_callback(vs, "tc::float_copy_t",
        [](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
        {
            auto val = co_await vc::resolve_memb<float>(vs, node["value"]);
            auto obj = vc::float_t::create(val);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return obj->to_related<vc::object_t>();
        });
}

int test10_resolve_memb_copy() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));
    VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs.get(), vec3_t, x);

    ASSERT_FN(add_named_builder_callback(vs.get(), "tc::vec3_t",
        [](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
        {
            auto x = co_await vc::resolve_float(vs, node["x"]);
            auto y = co_await vc::resolve_float(vs, node["y"]);
            auto z = co_await vc::resolve_float(vs, node["z"]);
            auto obj = vec3_t::create((float)x, (float)y, (float)z);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return obj->to_related<vc::object_t>();
        }));
    ASSERT_FN(register_float_copy_builder(vs.get()));

    auto path = write_temp_yaml("010-001-copy",
        "source_vec:\n"
        "  m_type: tc::vec3_t\n"
        "  x: 11\n"
        "  y: 22\n"
        "  z: 33\n"
        "copy_of_x:\n"
        "  m_type: tc::float_copy_t\n"
        "  value: !copy\n"
        "    object: source_vec\n"
        "    member: x\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto src = vc::get_ref<vec3_t>(vs.get(), "source_vec");
    ASSERT_FN(CHK_PTR(src.get()));
    ASSERT_FN(CHK_BOOL(src->x == 11.0f));

    /* The interesting assertion: copy_of_x is a plain vc::float_t whose value was memcpy'd out of
    source_vec's `x` field by resolve_memb_data(), entirely independent of source_vec afterwards. */
    auto dst = vc::get_ref<vc::float_t>(vs.get(), "copy_of_x");
    ASSERT_FN(CHK_PTR(dst.get()));
    ASSERT_FN(CHK_BOOL(dst->value == 11.0f));

    return 0;
}

int test10_resolve_memb_unregistered_member_fails() {
    /* resolve_memb_data() throws vc::except_t if the target member was never registered with
    register_trivially_copyable_member() for that object's type - here, `y` was never registered
    (only `x` was, in the previous test's own separately-registered state), so referencing !copy
    on it must fail the whole parse rather than silently reading garbage. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));
    VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs.get(), vec3_t, x); /* only x, not y */

    ASSERT_FN(add_named_builder_callback(vs.get(), "tc::vec3_t",
        [](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
        {
            auto x = co_await vc::resolve_float(vs, node["x"]);
            auto obj = vec3_t::create((float)x, 0, 0);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return obj->to_related<vc::object_t>();
        }));
    ASSERT_FN(register_float_copy_builder(vs.get()));

    auto path = write_temp_yaml("010-001-unregistered",
        "source_vec:\n"
        "  m_type: tc::vec3_t\n"
        "  x: 1\n"
        "bad_copy:\n"
        "  m_type: tc::float_copy_t\n"
        "  value: !copy\n"
        "    object: source_vec\n"
        "    member: y\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_GENERIC));

    return 0;
}

int test10_trivial_copy_member() {
    ASSERT_FN(test10_resolve_memb_copy());
    ASSERT_FN(test10_resolve_memb_unregistered_member_fails());
    return 0;
}

int main() {
    int ret = test10_trivial_copy_member();
    print_test_result("010-001-trivial_copy_member.cpp", ret >= 0);
    return ret;
}
