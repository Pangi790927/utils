#include "tests_common.h"

/* Test11 - register_inheritance()
================================================================================================= */

/* register_inheritance<T, U>() (virt_composer.h) tells the framework that one registered type is
a base of another, so members registered on the base later become visible on the derived type too
(see set_lua_class_member()/set_class_member_setter()/set_trivial_copy_member(), which all iterate
`vs->inheritance_table[type]` and write the member into every type in that set). Unlike what the
"runtime relation" framing might suggest, it is NOT free to relate two arbitrary object_t types:
its `requires` clause on top of the compile-time `is_base_of_v<T,U> || is_base_of_v<U,T>` check
(enforced via `demangle_static_assert`, which throws inside a consteval function to force a hard
compile error for an invalid pairing) means `derived_t` below has to actually, genuinely inherit
from `base_t` in C++ - register_inheritance() then only teaches the *framework* (which otherwise
has no idea derived_t is-a base_t) about that relationship, it doesn't invent one. It also does
not retroactively apply to members that were already registered on the base *before*
register_inheritance() was called (order matters - see test11_registration_order_matters below),
and it is not transitive past the one pair it's given (see the two `test11_three_level_hierarchy_*`
tests below, and register_inheritance()'s own doc comment in virt_composer.h for the full writeup -
this used to be logged as a BUGS.md entry until it turned out to have a one-line, fully-supported
workaround, not be an actual defect). */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_BASE);
VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_DERIVED);
VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_GRANDCHILD);

struct base_t : public vc::object_t {
    int64_t base_val = 0;

    base_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~base_t() {}

    static vc::ref_t<base_t> create(int64_t v) {
        auto ret = std::make_shared<base_t>(vc::object_t::Private{type_id_static()});
        ret->base_val = v;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_BASE; }
    static vc::object_type_e type_id_static() { return TC_TYPE_BASE; }

    inline std::string to_string() const override {
        return std::format("base_t[{}]: base_val={}", (void*)this, base_val);
    }
};

/* Real C++ inheritance from base_t, as register_inheritance<base_t, derived_t>()'s constraint
requires - it reuses base_t's `base_val` member rather than redeclaring it. */
struct derived_t : public base_t {
    int64_t derived_val = 0;

    derived_t(vc::object_t::Private priv) : base_t(priv) {}
    virtual ~derived_t() {}

    static vc::ref_t<derived_t> create(int64_t b, int64_t d) {
        auto ret = std::make_shared<derived_t>(vc::object_t::Private{type_id_static()});
        ret->base_val = b;
        ret->derived_val = d;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_DERIVED; }
    static vc::object_type_e type_id_static() { return TC_TYPE_DERIVED; }

    inline std::string to_string() const override {
        return std::format("derived_t[{}]: base_val={} derived_val={}",
                (void*)this, base_val, derived_val);
    }
};

/* A third level, for the transitivity tests below - real C++ inheritance from derived_t (and
therefore, transitively, from base_t too). */
struct grandchild_t : public derived_t {
    grandchild_t(vc::object_t::Private priv) : derived_t(priv) {}
    virtual ~grandchild_t() {}

    static vc::ref_t<grandchild_t> create(int64_t b, int64_t d) {
        auto ret = std::make_shared<grandchild_t>(vc::object_t::Private{type_id_static()});
        ret->base_val = b;
        ret->derived_val = d;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_GRANDCHILD; }
    static vc::object_type_e type_id_static() { return TC_TYPE_GRANDCHILD; }

    inline std::string to_string() const override {
        return std::format("grandchild_t[{}]: base_val={} derived_val={}",
                (void*)this, base_val, derived_val);
    }
};

#include "../../virt_composer_end.h"

int test11_member_visible_on_derived() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    /* register_inheritance() first, THEN register the member on the base - so the member
    propagates to derived_t too (see BUGS-worthy note in test11_registration_order_matters for the
    reverse order). */
    vc::register_inheritance<base_t, derived_t>(vs.get());
    VC_REGISTER_MEMBER_OBJECT(vs.get(), base_t, base_val);

    auto d = derived_t::create(10, 20);

    auto path = write_temp_yaml("011-001-inherit",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function read_base_val(obj) return obj.base_val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    /* base_val was only ever registered against base_t's type id - reading it off a derived_t
    instance only works because of the register_inheritance() call above. */
    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "read_base_val", d);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 10));

    return 0;
}

int test11_registration_order_matters() {
    /* Non-obvious behavior: register_inheritance()/set_lua_class_member() do NOT maintain a
    live/lazy link between base and derived - set_lua_class_member() copies the member map entry
    into every type currently in `inheritance_table[type]` *at the time it's called*. A member
    registered on the base BEFORE register_inheritance() establishes the relation is never
    retroactively propagated to the derived type. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    VC_REGISTER_MEMBER_OBJECT(vs.get(), base_t, base_val); /* registered first, on base_t only */
    vc::register_inheritance<base_t, derived_t>(vs.get());  /* relation established afterwards */

    auto d = derived_t::create(10, 20);

    auto path = write_temp_yaml("011-001-order",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function try_read_base_val(obj) return obj.base_val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "try_read_base_val", d);
    (void)ret;
    /* Fails: derived_t never got the member, because it wasn't in base_t's inheritance_table set
    yet when VC_REGISTER_MEMBER_OBJECT ran. */
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_FAILED_CALL));

    return 0;
}

int test11_three_level_hierarchy_without_all_pairs_fails() {
    /* register_inheritance() is not transitive past the one pair it's given (see its doc comment
    in virt_composer.h). Registering only the adjacent links - base/derived, derived/grandchild -
    is NOT enough to make base_val reach grandchild_t, even though grandchild_t genuinely is a
    base_t in C++. This is expected/documented behavior, not a bug: contrast with
    test11_three_level_hierarchy_needs_every_pair_registered below, which is identical except for
    also registering the non-adjacent base/grandchild pair. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    vc::register_inheritance<base_t, derived_t>(vs.get());
    vc::register_inheritance<derived_t, grandchild_t>(vs.get());
    VC_REGISTER_MEMBER_OBJECT(vs.get(), base_t, base_val);

    auto g = grandchild_t::create(30, 40);

    auto path = write_temp_yaml("011-001-threelevel-fail",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function try_read_base_val(obj) return obj.base_val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "try_read_base_val", g);
    (void)ret;
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_FAILED_CALL));

    return 0;
}

int test11_three_level_hierarchy_needs_every_pair_registered() {
    /* The correct pattern for a 3+-level hierarchy: register every ancestor/descendant pair you
    need visible, not just the adjacent links. register_inheritance<base_t, grandchild_t>() is a
    perfectly valid, independent call - std::is_base_of_v<base_t, grandchild_t> (and therefore
    this function's own compile-time constraint) is satisfied regardless of how many levels
    separate the two types, precisely because C++ inheritance is itself transitive; the framework
    just doesn't derive that same transitivity automatically from the individual pairs it's told
    about. All three relations still have to be registered before the member, per the
    registration-order rule (see test11_registration_order_matters above) - a
    register_inheritance() call never retroactively reaches back over members already added. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    vc::register_inheritance<base_t, derived_t>(vs.get());
    vc::register_inheritance<derived_t, grandchild_t>(vs.get());
    vc::register_inheritance<base_t, grandchild_t>(vs.get());   /* the non-adjacent pair */
    VC_REGISTER_MEMBER_OBJECT(vs.get(), base_t, base_val);

    auto g = grandchild_t::create(30, 40);

    auto path = write_temp_yaml("011-001-threelevel-ok",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function read_base_val(obj) return obj.base_val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "read_base_val", g);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 30));

    return 0;
}

int test11_register_inheritance() {
    ASSERT_FN(test11_member_visible_on_derived());
    ASSERT_FN(test11_registration_order_matters());
    ASSERT_FN(test11_three_level_hierarchy_without_all_pairs_fails());
    ASSERT_FN(test11_three_level_hierarchy_needs_every_pair_registered());
    return 0;
}

int main() {
    int ret = test11_register_inheritance();
    print_test_result("011-001-register_inheritance.cpp", ret >= 0);
    return ret;
}
