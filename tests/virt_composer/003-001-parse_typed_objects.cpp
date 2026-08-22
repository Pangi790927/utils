#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test3 - YAML Typed Objects
================================================================================================= */

/* build_object() (virt_composer.cpp) handles nodes that carry an `m_type` field by dispatching on
its value - "vc::integer_t"/"vc::float_t"/"vc::string_t" are handled as builtins, right there in
build_object() itself, before ever reaching a user-registered builder callback. This test parses a
YAML doc that defines one of each and checks the resulting objects via get_ref<T>(). */

static const char *k_yaml = R"YAML(
an_int:
  m_type: vc::integer_t
  value: 42

a_float:
  m_type: vc::float_t
  value: 2.5

a_string:
  m_type: vc::string_t
  value: "hello world"
)YAML";

int test3_typed_objects() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("003-001-typed", k_yaml);
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto i = vc::get_ref<vc::integer_t>(vs.get(), "an_int");
    ASSERT_FN(CHK_PTR(i.get()));
    ASSERT_FN(CHK_BOOL(i->value == 42));

    auto f = vc::get_ref<vc::float_t>(vs.get(), "a_float");
    ASSERT_FN(CHK_PTR(f.get()));
    ASSERT_FN(CHK_BOOL(f->value == 2.5));

    auto s = vc::get_ref<vc::string_t>(vs.get(), "a_string");
    ASSERT_FN(CHK_PTR(s.get()));
    ASSERT_FN(CHK_BOOL(s->value == "hello world"));

    return 0;
}

int test3_unknown_type_fails() {
    /* build_object() throws vc::except_t for an m_type it doesn't recognize (no builtin match, no
    registered builder callback), which parse_config() turns into VC_ERROR_GENERIC. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("003-001-unknown",
            "bogus:\n  m_type: vc::not_a_real_type\n  value: 1\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_GENERIC));

    return 0;
}

int test3_parse_typed_objects() {
    ASSERT_FN(test3_typed_objects());
    ASSERT_FN(test3_unknown_type_fails());
    return 0;
}

int main() {
    int ret = test3_parse_typed_objects();
    print_test_result("003-001-parse_typed_objects.cpp", ret >= 0);
    return ret;
}
