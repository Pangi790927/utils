#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test5 - References & Dependency Resolution
================================================================================================= */

/* Every top-level YAML entry is scheduled as its own coroutine (build_schema()'s `co_await
co::sched(build_object(...))` / `co::sched(build_pseudo_object(...))` loop), so objects are built
concurrently, not strictly top-to-bottom. A `!ref name` value resolves through
depend_resolver_t<T>: if the referenced object isn't registered yet, the referencing coroutine
suspends itself on virt_state_t::wanted_objects and is resumed later by mark_dependency_solved()
(called by whichever builder finishes constructing that name) - see resolve_int/resolve_float's
`node.get_tag_name() == "!ref"` branch. This test defines the reference BEFORE its target in
document order specifically to exercise that suspend/resume path rather than the (uninteresting)
case where the dependency already exists by the time it's needed. */

static const char *k_yaml = R"YAML(
depends_on_later:
  m_type: vc::integer_t
  value: !ref later_defined

later_defined:
  m_type: vc::integer_t
  value: 99
)YAML";

int test5_forward_reference() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("005-001-fwd", k_yaml);
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto later = vc::get_ref<vc::integer_t>(vs.get(), "later_defined");
    ASSERT_FN(CHK_PTR(later.get()));
    ASSERT_FN(CHK_BOOL(later->value == 99));

    auto early = vc::get_ref<vc::integer_t>(vs.get(), "depends_on_later");
    ASSERT_FN(CHK_PTR(early.get()));
    /* resolve_int()'s !ref branch follows the reference to the target integer_t and copies its
    ->value out - it does not alias the same object. */
    ASSERT_FN(CHK_BOOL(early->value == 99));
    ASSERT_FN(CHK_BOOL(early.get() != later.get()));

    return 0;
}

int test5_backward_reference() {
    /* Same mechanism, but the dependency already exists by the time it's referenced - the
    depend_resolver_t<T>::await_ready() fast path (no suspension needed) should give the same
    result as the forward case above. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("005-001-back",
        "earlier:\n"
        "  m_type: vc::integer_t\n"
        "  value: 5\n"
        "depends_on_earlier:\n"
        "  m_type: vc::integer_t\n"
        "  value: !ref earlier\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto later = vc::get_ref<vc::integer_t>(vs.get(), "depends_on_earlier");
    ASSERT_FN(CHK_PTR(later.get()));
    ASSERT_FN(CHK_BOOL(later->value == 5));

    return 0;
}

int test5_unresolved_reference_leaves_object_unregistered() {
    /* Non-obvious behavior: parse_config() does NOT turn a leftover (never-resolved) entry in
    wanted_objects into an error. It only logs a DBG warning for each name still outstanding after
    build_schema()'s pool finishes running, and still returns VC_ERROR_OK. The coroutine that was
    waiting on the missing dependency simply never completes; the object that reference was
    supposed to populate is never registered. get_ref_base() is the safe way to check that (a
    plain get_ref<T>() would dereference a null base ref_t and crash - see get_ref_base's
    `->to_related<T>()` on whatever get_ref_base() returns). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("005-001-unresolved",
        "depends_on_missing:\n"
        "  m_type: vc::integer_t\n"
        "  value: !ref does_not_exist_anywhere\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));

    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs.get(), "depends_on_missing") == nullptr));

    return 0;
}

int test5_references() {
    ASSERT_FN(test5_forward_reference());
    ASSERT_FN(test5_backward_reference());
    ASSERT_FN(test5_unresolved_reference_leaves_object_unregistered());
    return 0;
}

int main() {
    int ret = test5_references();
    print_test_result("005-001-forward_reference.cpp", ret >= 0);
    return ret;
}
