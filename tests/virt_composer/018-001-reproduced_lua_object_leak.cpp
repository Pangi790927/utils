#include "tests_common.h"

/* Test18 - Reproduced Bugs: every object pushed into Lua was leaked for the virt_state_t's
lifetime (fixed)
================================================================================================= */

/* Was an open bug (BUGS.md carried this entry): every vc::object_t exposed to Lua via
push_vc_object()/mark_dependency_solved() was pushed as *light* userdata, so the "__vc_metatable"
__gc metamethod could never fire - light userdata isn't a GC-tracked heap object, it never receives
finalization at all. virt_state_t::obj_keepalive then permanently pinned every such object for the
whole virt_state_t's lifetime regardless of Lua/C++ reachability, no matter how many times a full
GC was forced.

Fixed by switching push_vc_object()/mark_dependency_solved() to *full* userdata (box_t, which owns
a real vc::ref_t<vc::object_t>), backed by a per-virt_state_t weak-value ("__mode" = "v") cache
table keyed by the raw object pointer, so repeated pushes of the same object reuse the same box
(preserving Lua-side `==` identity) without the cache table itself keeping anything alive. __gc now
actually fires and drops the box's strong ref - see virt_composer.cpp's push_vc_object()/box_t/
the "__vc_metatable" __gc handler in luaopen_vc(). obj_keepalive was removed entirely: an object's
lifetime is now governed uniformly by ordinary Lua/C++ reachability (the `vc` module table's own
strong reference is what keeps named/anonymous registered objects alive, same as anything else).

This file is the regression test for that fix: it must show the object's destructor actually
running after the only C++-side ref_t is dropped and a couple of full GC cycles are forced - before
the fix, alive_count stayed 1 forever (the object only died when the whole virt_state_t was torn
down). The two forced lua_gc(LUA_GCCOLLECT) calls are not redundant - Lua 5.4 (confirmed to be what
minilua.h actually is: LUA_VERSION_NUM 504, verbatim lgc.c/lauxlib.c) finalizes objects with a __gc
metamethod one GC step after they're found unreachable, so a single collectgarbage() call can still
observe the pre-collection state even on a correctly-fixed build. */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_LEAKY);

static int g_alive_count = 0;

struct leaky_t : public vc::object_t {
    leaky_t(vc::object_t::Private priv) : vc::object_t(priv) { g_alive_count++; }
    virtual ~leaky_t() { g_alive_count--; }

    static vc::ref_t<leaky_t> create() {
        return std::make_shared<leaky_t>(vc::object_t::Private{type_id_static()});
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_LEAKY; }
    static vc::object_type_e type_id_static() { return TC_TYPE_LEAKY; }
    inline std::string to_string() const override { return "leaky_t"; }
};

#include "../../virt_composer_end.h"

int test18_lua_object_actually_gets_collected() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("018-001-leak",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function touch(obj) return 1 end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    ASSERT_FN(CHK_BOOL(g_alive_count == 0));
    {
        auto obj = leaky_t::create();
        ASSERT_FN(CHK_BOOL(g_alive_count == 1));

        /* Pushes obj into Lua as a call_lua() argument - luaw_push_cpp_object()'s is_vc_ref
        branch -> push_vc_object() - the exact path that used to leak. */
        auto [ret, err] = vc::call_lua<int>(vs.get(), "touch", obj);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(ret == 1));

        /* `obj` (our only C++-side ref_t) goes out of scope here. */
    }

    /* Force two full GC cycles - see the header comment for why one isn't enough. */
    auto L = vc::luaw_get_lua_state(vs.get());
    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCCOLLECT, 0);

    ASSERT_FN(CHK_BOOL(g_alive_count == 0));

    return 0;
}

int main() {
    int ret = test18_lua_object_actually_gets_collected();
    print_test_result("018-001-reproduced_lua_object_leak.cpp", ret >= 0);
    return ret;
}
