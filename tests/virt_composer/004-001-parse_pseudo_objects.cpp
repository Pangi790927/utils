#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test4 - YAML Pseudo (auto-identified) Objects
================================================================================================= */

/* A YAML node with no `m_type` field goes through build_pseudo_object() instead of build_object()
(see build_schema()'s branch on `node.contains("m_type")`). build_pseudo_object() tries registered
analyser callbacks first (none here), then falls back to its own builtin recognizers: a bare
integer/float/string scalar becomes an integer_t/float_t/string_t without ever mentioning
"vc::integer_t" etc. This is the shorthand form; 003-001 covers the explicit `m_type:` form for
the same three builtins. */

static const char *k_yaml = R"YAML(
an_int: 7
a_float: 1.5
a_string: "plain scalar"
)YAML";

int test4_pseudo_objects() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("004-001-pseudo", k_yaml);
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto i = vc::get_ref<vc::integer_t>(vs.get(), "an_int");
    ASSERT_FN(CHK_PTR(i.get()));
    ASSERT_FN(CHK_BOOL(i->value == 7));

    auto f = vc::get_ref<vc::float_t>(vs.get(), "a_float");
    ASSERT_FN(CHK_PTR(f.get()));
    ASSERT_FN(CHK_BOOL(f->value == 1.5));

    auto s = vc::get_ref<vc::string_t>(vs.get(), "a_string");
    ASSERT_FN(CHK_PTR(s.get()));
    ASSERT_FN(CHK_BOOL(s->value == "plain scalar"));

    return 0;
}

int test4_pseudo_object_wrong_cast_throws() {
    /* get_ref<T>() calls to_related<T>() under the hood, which throws std::runtime_error for a
    type mismatch (see test1_to_related in 001-001) - confirm that also holds for objects that
    came from the pseudo-object path, not just objects built directly in C++. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("004-001-wrongcast", "an_int: 7\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    bool threw = false;
    try {
        vc::get_ref<vc::string_t>(vs.get(), "an_int");
    }
    catch (std::runtime_error &) {
        threw = true;
    }
    ASSERT_FN(CHK_BOOL(threw));

    return 0;
}

int test4_parse_pseudo_objects() {
    ASSERT_FN(test4_pseudo_objects());
    ASSERT_FN(test4_pseudo_object_wrong_cast_throws());
    return 0;
}

int main() {
    int ret = test4_parse_pseudo_objects();
    print_test_result("004-001-parse_pseudo_objects.cpp", ret >= 0);
    return ret;
}
