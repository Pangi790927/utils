#include "tests_common.h"
#include "../../virt_composer_end.h"

/* Test1 - Builtin Objects
================================================================================================= */

/* vc::integer_t/float_t/string_t are the three builtin object_t types virt_composer registers for
itself (see virt_composer.h's VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_*) block). This test exercises
their C++-side API directly (create/value/type_id/to_string), with no yaml or Lua involved -
everything else in this directory builds on top of these three types. */

int test1_integer() {
    auto i = vc::integer_t::create(42);
    ASSERT_FN(CHK_PTR(i.get()));
    ASSERT_FN(CHK_BOOL(i->value == 42));
    ASSERT_FN(CHK_BOOL(i->type_id() == vc::VC_TYPE_INTEGER));
    ASSERT_FN(CHK_BOOL(vc::integer_t::type_id_static() == vc::VC_TYPE_INTEGER));
    ASSERT_FN(CHK_BOOL(!i->to_string().empty()));
    return 0;
}

int test1_float() {
    auto f = vc::float_t::create(3.5);
    ASSERT_FN(CHK_PTR(f.get()));
    ASSERT_FN(CHK_BOOL(f->value == 3.5));
    ASSERT_FN(CHK_BOOL(f->type_id() == vc::VC_TYPE_FLOAT));
    return 0;
}

int test1_string() {
    auto s = vc::string_t::create("hello");
    ASSERT_FN(CHK_PTR(s.get()));
    ASSERT_FN(CHK_BOOL(s->value == "hello"));
    ASSERT_FN(CHK_BOOL(s->type_id() == vc::VC_TYPE_STRING));
    return 0;
}

int test1_to_related() {
    /* to_related<T>() is the up/down-cast helper every ref_t goes through inside the parser (see
    build_object()'s `obj->to_related<vc::object_t>()` calls) - confirm it succeeds for a valid
    cast to a base type and throws std::runtime_error for a cast to an unrelated type. */
    auto i = vc::integer_t::create(7);
    vc::ref_t<vc::object_t> base = i->to_related<vc::object_t>();
    ASSERT_FN(CHK_PTR(base.get()));

    bool threw = false;
    try {
        base->to_related<vc::string_t>();
    }
    catch (std::runtime_error &) {
        threw = true;
    }
    ASSERT_FN(CHK_BOOL(threw));

    return 0;
}

int test1_builtin_objects() {
    ASSERT_FN(test1_integer());
    ASSERT_FN(test1_float());
    ASSERT_FN(test1_string());
    ASSERT_FN(test1_to_related());
    return 0;
}

int main() {
    int ret = test1_builtin_objects();
    print_test_result("001-001-builtin_object_create.cpp", ret >= 0);
    return ret;
}
