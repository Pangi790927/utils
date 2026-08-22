#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test13 - Expression Resolution (resolve_int/resolve_float via tinyexpr)
================================================================================================= */

/* resolve_int()/resolve_float() treat a plain YAML *string* scalar (with no `!ref` tag) as a
math expression to evaluate through tinyexpr (see resolve_string_as_expression() in
virt_composer.cpp) - not as a literal string value. resolve_int() additionally rounds the
evaluated double to the nearest integer (`std::round(...)`). virt_state_t also seeds a fixed table
of `constants` (SIZEOF_INT32, SIZEOF_VEC_3F, ...) available to any expression - this is what makes
"3 * SIZEOF_INT32" a valid `value:` for an integer_t. */

static const char *k_yaml = R"YAML(
plain_int: 42
expr_int:
  m_type: vc::integer_t
  value: "2 + 3 * 4"

rounded_int:
  m_type: vc::integer_t
  value: "7 / 2"

expr_float:
  m_type: vc::float_t
  value: "1.5 + 2.5"

sizeof_expr:
  m_type: vc::integer_t
  value: "3 * SIZEOF_INT32"
)YAML";

int test13_expressions() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("013-001-expr", k_yaml);
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto expr_int = vc::get_ref<vc::integer_t>(vs.get(), "expr_int");
    ASSERT_FN(CHK_PTR(expr_int.get()));
    ASSERT_FN(CHK_BOOL(expr_int->value == 14));

    /* 7 / 2 = 3.5, resolve_int() rounds to nearest -> 4, not truncated to 3. */
    auto rounded = vc::get_ref<vc::integer_t>(vs.get(), "rounded_int");
    ASSERT_FN(CHK_PTR(rounded.get()));
    ASSERT_FN(CHK_BOOL(rounded->value == 4));

    auto expr_float = vc::get_ref<vc::float_t>(vs.get(), "expr_float");
    ASSERT_FN(CHK_PTR(expr_float.get()));
    ASSERT_FN(CHK_BOOL(expr_float->value == 4.0));

    auto sizeof_expr = vc::get_ref<vc::integer_t>(vs.get(), "sizeof_expr");
    ASSERT_FN(CHK_PTR(sizeof_expr.get()));
    ASSERT_FN(CHK_BOOL(sizeof_expr->value == 3 * (int64_t)sizeof(int32_t)));

    return 0;
}

int test13_invalid_expression_fails_parse() {
    /* A string that tinyexpr can't parse (te_compile() returns null) throws vc::except_t, which
    parse_config()'s outer catch(std::exception&) turns into VC_ERROR_GENERIC. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("013-001-badexpr",
        "bad:\n"
        "  m_type: vc::integer_t\n"
        "  value: \"2 + + + \"\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_GENERIC));

    return 0;
}

int test13_resolve_expressions() {
    ASSERT_FN(test13_expressions());
    ASSERT_FN(test13_invalid_expression_fails_parse());
    return 0;
}

int main() {
    int ret = test13_resolve_expressions();
    print_test_result("013-001-resolve_expressions.cpp", ret >= 0);
    return ret;
}
