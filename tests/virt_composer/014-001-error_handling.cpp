#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test14 - Error Handling
================================================================================================= */

int test14_missing_file_returns_parse_error() {
    /* parse_config() opens the path with a plain std::ifstream and hands it straight to
    fkyaml::node::deserialize() - a missing file doesn't fail at the ifstream::open() call site
    (no explicit good()/is_open() check there, unlike get_file_string_content() for
    m_source_path), it's fkyaml choking on an empty/unreadable stream that throws, which lands in
    the fkyaml::exception catch clause. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto err = vc::parse_config(vs.get(), "014-001-this-file-does-not-exist.yaml");
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_PARSE_YAML));

    return 0;
}

int test14_malformed_yaml_returns_parse_error() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    /* Mismatched flow-collection brackets (opened with '[', closed with '}') - deliberately
    invalid YAML syntax that fkyaml rejects while actually parsing (as opposed to e.g. an
    unterminated sequence, which fkyaml tolerates by implicitly closing it at EOF - that produces
    a structurally-valid-but-unrecognized node instead of a parse error, and is covered by
    test14_unrecognized_pseudo_object_shape_is_silently_missing below instead). */
    auto path = write_temp_yaml("014-001-malformed", "bad: [1, 2}\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_PARSE_YAML));

    return 0;
}

int test14_unrecognized_pseudo_object_shape_is_silently_missing() {
    /* Non-obvious behavior, found while picking a malformed-YAML example above: an unterminated
    flow sequence ("[1, 2," with no closing ']') is NOT a parse error - fkyaml tolerates it and
    implicitly closes the sequence at EOF, producing a structurally valid node build_pseudo_object()
    then doesn't recognize (it's a sequence, not a mapping/int/float/string) and gives up on
    (`co_return -1` from build_pseudo_object()'s final fallback). That -1 is silently discarded by
    build_schema()'s `co_await co::sched(build_pseudo_object(...))` call (the return value isn't
    even read), so parse_config() still reports VC_ERROR_OK - same "quietly missing object" shape
    as an unresolved !ref (005-001) or a malformed lua_script_t node (006-001). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("014-001-unterminated", "bad: [1, 2,\n");
    auto err = vc::parse_config(vs.get(), path.c_str());
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(vc::get_ref_base(vs.get(), "bad") == nullptr));

    return 0;
}

int test14_duplicate_name_throws() {
    /* mark_dependency_solved() throws vc::except_t{"Tag name already exists: ..."} if the name is
    already registered - two top-level YAML keys can't collide (YAML mappings can't have duplicate
    keys anyway), but a builder that calls mark_dependency_solved() twice for the same name (e.g. a
    named builder callback with a bug) hits this. Simulated here directly rather than by writing an
    actually-duplicate-keyed YAML doc (fkyaml would just silently keep the last of two identical
    mapping keys, never reaching mark_dependency_solved() twice). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto a = vc::integer_t::create(1);
    vc::mark_dependency_solved(vs.get(), "dup", a->to_related<vc::object_t>());

    bool threw = false;
    try {
        auto b = vc::integer_t::create(2);
        vc::mark_dependency_solved(vs.get(), "dup", b->to_related<vc::object_t>());
    }
    catch (vc::except_t &) {
        threw = true;
    }
    ASSERT_FN(CHK_BOOL(threw));

    /* The original registration must be left untouched by the failed second attempt. */
    auto still_a = vc::get_ref<vc::integer_t>(vs.get(), "dup");
    ASSERT_FN(CHK_PTR(still_a.get()));
    ASSERT_FN(CHK_BOOL(still_a->value == 1));

    return 0;
}

int test14_null_object_throws() {
    /* mark_dependency_solved() also rejects a null ref_t outright, before ever touching the name
    tables - see its `if (!depend) throw ...` guard. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    bool threw = false;
    try {
        vc::mark_dependency_solved(vs.get(), "whatever", vc::ref_t<vc::object_t>{});
    }
    catch (vc::except_t &) {
        threw = true;
    }
    ASSERT_FN(CHK_BOOL(threw));

    return 0;
}

int test14_error_handling() {
    ASSERT_FN(test14_missing_file_returns_parse_error());
    ASSERT_FN(test14_malformed_yaml_returns_parse_error());
    ASSERT_FN(test14_unrecognized_pseudo_object_shape_is_silently_missing());
    ASSERT_FN(test14_duplicate_name_throws());
    ASSERT_FN(test14_null_object_throws());
    return 0;
}

int main() {
    int ret = test14_error_handling();
    print_test_result("014-001-error_handling.cpp", ret >= 0);
    return ret;
}
