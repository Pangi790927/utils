#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test2 - Virt State
================================================================================================= */

/* create_state() is the entry point of the whole framework: it builds the Lua state, registers
the three builtin types (integer_t/float_t/string_t) and returns a shared_ptr<virt_state_t>. This
test checks basic lifecycle: creation succeeds, two independent states don't share objects, and
looking up a name that was never defined returns null rather than crashing. */

int test2_create_state_basic() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));
    return 0;
}

int test2_two_independent_states() {
    /* Each virt_state_t owns its own Lua state and its own name_to_object map - defining "x" in
    one must not make it visible from the other. */
    auto vs1 = vc::create_state();
    auto vs2 = vc::create_state();
    ASSERT_FN(CHK_PTR(vs1.get()));
    ASSERT_FN(CHK_PTR(vs2.get()));

    auto path = write_temp_yaml("002-001-vs1", "x:\n  m_type: vc::integer_t\n  value: 1\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs1.get(), path.c_str()) == vc::VC_ERROR_OK));

    ASSERT_FN(CHK_PTR(vc::get_ref_base(vs1.get(), "x").get()));
    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs2.get(), "x") == nullptr));

    return 0;
}

int test2_unknown_name_returns_null() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));
    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs.get(), "does_not_exist") == nullptr));
    return 0;
}

int test2_virt_state() {
    ASSERT_FN(test2_create_state_basic());
    ASSERT_FN(test2_two_independent_states());
    ASSERT_FN(test2_unknown_name_returns_null());
    return 0;
}

int main() {
    int ret = test2_virt_state();
    print_test_result("002-001-virt_state_lifecycle.cpp", ret >= 0);
    return ret;
}
