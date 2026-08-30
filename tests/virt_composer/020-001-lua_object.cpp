#include "tests_common.h"

/* Test20 - Lua Objects (Lua values held and later invoked from C++)
=================================================================================================

Covers vc::lua_object_t - the reverse direction of c_function_t (a C++ callback exposed to Lua):
here a *Lua* value (a function, in every test below) is captured by C++ and kept alive - a strong
ref into its own dedicated registry sub-table, separate from weak_cache_ref - so C++ can
push()/call() it again later, possibly long after the Lua call that handed it over has returned.

Three single-purpose operations:
- create() - the normal factory (same as any other vc object), makes an empty shell with nothing
  captured yet.
- capture(L) - an instance method: replaces *this* instance's held value with whatever is on top
  of L's stack, releasing whatever it previously held first.
- capture_lua_object(L, ref, idx) - a static convenience that duplicates the value at stack index
  idx onto the top and calls ref->capture(L). It does not create anything - ref must already exist.

Two distinct paths through the type, on purpose:
- As a member-function *parameter* (vc::ref_t<lua_object_t>), passing a Lua value captures it
  fresh - the whole point of declaring that parameter type is "capture whatever's given here."
- As a member-function *return value*, the captured lua_object_t is boxed the exact same way any
  other vc::ref_t<T> would be (push_vc_object(), normal "__vc_metatable" userdata) - Lua gets a
  portable reference it can hold/pass around like any other vc object, not the raw underlying
  function unwrapped from its shell. Passing that reference back into another capture point
  recognizes it's already a lua_object_t and reuses it as-is, rather than double-wrapping the
  userdata box as if it were some opaque foreign value. */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_HOLDER);

/* A minimal object that can hold one Lua callback and invoke it later - mirrors how a real
composer would store an event handler/completion callback handed to it from a Lua script. */
struct holder_t : public vc::object_t {
    vc::ref_t<vc::lua_object_t> cb;

    holder_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~holder_t() {}

    static vc::ref_t<holder_t> create() {
        return std::make_shared<holder_t>(vc::object_t::Private{type_id_static()});
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_HOLDER; }
    static vc::object_type_e type_id_static() { return TC_TYPE_HOLDER; }

    inline std::string to_string() const override {
        return std::format("holder_t[{}]", (void*)this);
    }

    void set_callback(vc::ref_t<vc::lua_object_t> new_cb) { cb = new_cb; }
    vc::ref_t<vc::lua_object_t> get_callback() { return cb; }

    int64_t invoke(int64_t x) {
        if (!cb)
            return -1;
        auto [ret, err] = cb->call<int64_t>(x);
        return err == vc::VC_ERROR_OK ? ret : -1;
    }
};

#include "../../virt_composer_end.h"

static vc::ref_t<vc::virt_state_t> make_state_with_holder_registered() {
    auto vs = vc::create_state();
    if (!vs)
        return nullptr;
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), holder_t, set_callback, vc::ref_t<vc::lua_object_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), holder_t, get_callback);
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), holder_t, invoke, int64_t);
    return vs;
}

int test20_capture_and_call_typed() {
    /* h:set_callback(add_one) captures add_one - via luaw_param_t<vc::ref_t<lua_object_t>,
    index>'s special case, since holder_t::set_callback takes vc::ref_t<lua_object_t> - into cb.
    Later, invoke() calls it via lua_object_t::call<R>(args...), the call_lua<R>()-shaped
    convenience, long after the set_callback() call that handed it over has already returned. */
    auto vs = make_state_with_holder_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-typed",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function add_one(x) return x + 1 end\n"
        "    function bind_and_forget(h) h:set_callback(add_one) end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto h = holder_t::create();
    auto [bret, berr] = vc::call_lua<void>(vs.get(), "bind_and_forget", h);
    (void)bret;
    ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));

    ASSERT_FN(CHK_BOOL(h->invoke(41) == 42));

    return 0;
}

int test20_reference_relayed_through_lua() {
    /* h1:get_callback() returns the captured lua_object_t through the exact same path as any
    other vc::ref_t<T> return value (luaw_returner_t -> push_vc_object()) - Lua gets a normal
    boxed reference, not the raw underlying function unwrapped. That reference can be handed to
    h2:set_callback() and relayed straight through: luaw_param_t's lua_object_t special case
    recognizes the incoming value is already a lua_object_t (rather than capturing the userdata
    box itself as if it were some opaque foreign value) and just reuses it - so h1 and h2 end up
    sharing the identical lua_object_t instance, never touching doubler a second time. */
    auto vs = make_state_with_holder_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-pushback",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function doubler(x) return x * 2 end\n"
        "    function bind(h) h:set_callback(doubler) end\n"
        "    function relay(h1, h2)\n"
        "      local f = h1:get_callback()\n"
        "      h2:set_callback(f)\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto h1 = holder_t::create();
    auto h2 = holder_t::create();

    auto [bret, berr] = vc::call_lua<void>(vs.get(), "bind", h1);
    (void)bret;
    ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));

    auto [rret, rerr] = vc::call_lua<void>(vs.get(), "relay", h1, h2);
    (void)rret;
    ASSERT_FN(CHK_BOOL(rerr == vc::VC_ERROR_OK));

    ASSERT_FN(CHK_BOOL(h1->cb.get() == h2->cb.get()));
    ASSERT_FN(CHK_BOOL(h2->invoke(21) == 42));

    return 0;
}

int test20_explicit_push_unwraps_to_raw_value() {
    /* lua_object_t::push() is also registered as a Lua-visible member function (see
    create_state()) - the explicit way for a script to unwrap a boxed reference back to the raw
    captured value. h:get_callback() alone gives Lua a normal vc reference (not directly
    callable); calling :push() on it hands back the original doubler function itself, which Lua
    can then call directly. */
    auto vs = make_state_with_holder_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-explicit-push",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function doubler(x) return x * 2 end\n"
        "    function bind(h) h:set_callback(doubler) end\n"
        "    function unwrap_and_call(h, x)\n"
        "      local ref = h:get_callback()\n"
        "      local raw = ref:push()\n"
        "      return raw(x)\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto h = holder_t::create();
    auto [bret, berr] = vc::call_lua<void>(vs.get(), "bind", h);
    (void)bret;
    ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));

    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "unwrap_and_call", h, 21);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 42));

    return 0;
}

int test20_capture_replaces_previous_value() {
    /* capture(L) replaces whatever this instance previously held - releasing the old value first
    (see release()) - rather than just adding a second one alongside it. Exercised directly here
    via create() + capture_lua_object(L, ref, idx), the low-level combination the generic
    conversion machinery builds on. */
    vc::c_function_t::add_internal_func("recapture_and_invoke", [](lua_State *L) -> int {
        auto obj = vc::lua_object_t::create();
        vc::lua_object_t::capture_lua_object(L, obj, 1); // first captures whatever's at arg 1...
        vc::lua_object_t::capture_lua_object(L, obj, 2); // ...then replaces it with arg 2 instead
        lua_remove(L, 1);
        lua_remove(L, 1);
        return obj->call(L, lua_gettop(L));
    });

    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-recapture",
        "recapture_and_invoke:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function first(x) return x + 1 end\n"
        "    function second(x) return x * 10 end\n"
        "    function test_recapture(x)\n"
        "      return vc.recapture_and_invoke(first, second, x)\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    /* If capture() had left `first` still captured (added second alongside rather than replacing
    it), this would call first(5) == 6, not second(5) == 50. */
    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "test_recapture", 5);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(ret == 50));

    return 0;
}

int test20_capture_and_release_from_lua() {
    /* lua_object_t::capture()/release() are also registered as plain Lua-visible member
    functions (see create_state()), so a script holding a boxed reference can re-target or empty
    it directly - `ref:capture(x)`/`ref:release()` - without needing any C++ round trip. Since
    h->cb and the `ref` the script gets from h:get_callback() are the SAME underlying lua_object_t
    instance, mutating it from Lua is immediately visible on the C++ side too. */
    auto vs = make_state_with_holder_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-luaside",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function doubler(x) return x * 2 end\n"
        "    function tripler(x) return x * 3 end\n"
        "    function bind(h) h:set_callback(doubler) end\n"
        "    function recapture_via_lua(h)\n"
        "      local ref = h:get_callback()\n"
        "      ref:capture(tripler)\n"
        "    end\n"
        "    function release_via_lua(h)\n"
        "      local ref = h:get_callback()\n"
        "      ref:release()\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto h = holder_t::create();
    auto [bret, berr] = vc::call_lua<void>(vs.get(), "bind", h);
    (void)bret;
    ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));

    auto [rret, rerr] = vc::call_lua<void>(vs.get(), "recapture_via_lua", h);
    (void)rret;
    ASSERT_FN(CHK_BOOL(rerr == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(h->invoke(10) == 30)); /* tripler now, not doubler */

    auto [lret, lerr] = vc::call_lua<void>(vs.get(), "release_via_lua", h);
    (void)lret;
    ASSERT_FN(CHK_BOOL(lerr == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(h->invoke(10) == -1)); /* empty now - fails cleanly, doesn't crash */

    return 0;
}

int test20_capture_nil_is_a_release() {
    /* capture(nil) is treated as an explicit release(), not just a "captured nil" value - without
    that, luaL_ref would hand back LUA_REFNIL (a valid-looking ref distinct from LUA_NOREF),
    leaving push()/call()'s "nothing captured" guards (ref == LUA_NOREF) unable to recognize it.
    invoke() must fail cleanly afterward, the same as after an explicit :release(). */
    auto vs = make_state_with_holder_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-capturenil",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function doubler(x) return x * 2 end\n"
        "    function bind(h) h:set_callback(doubler) end\n"
        "    function capture_nil_via_lua(h)\n"
        "      local ref = h:get_callback()\n"
        "      ref:capture(nil)\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto h = holder_t::create();
    auto [bret, berr] = vc::call_lua<void>(vs.get(), "bind", h);
    (void)bret;
    ASSERT_FN(CHK_BOOL(berr == vc::VC_ERROR_OK));

    auto [nret, nerr] = vc::call_lua<void>(vs.get(), "capture_nil_via_lua", h);
    (void)nret;
    ASSERT_FN(CHK_BOOL(nerr == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(h->invoke(10) == -1));

    return 0;
}

int test20_raw_call_multiple_results() {
    /* Exercises lua_object_t::call(L, nargs) directly - the raw primitive, not the typed
    convenience - registered as a plain internal c_function_t so it's reachable as
    vc.invoke_with_2_args(...) from Lua. Confirms it correctly forwards LUA_MULTRET (the captured
    function here returns two values) rather than just the first, and that
    capture_lua_object() leaves the caller's stack net-unchanged (the original callee argument at
    index 1 is still there, untouched, after capturing it). */
    vc::c_function_t::add_internal_func("invoke_with_2_args", [](lua_State *L) -> int {
        auto obj = vc::lua_object_t::create();
        vc::lua_object_t::capture_lua_object(L, obj, 1);
        return obj->call(L, 2);
    });

    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("020-001-multret",
        "invoke_with_2_args:\n"
        "  m_type: vc::c_function_t\n"
        "  m_source: \"[INTERNAL]\"\n"
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    vc = require(\"virt_composer\")\n"
        "    function subtract_and_sum(a, b) return a - b, a + b end\n"
        "    function test_invoke(a, b)\n"
        "      local diff, sum = vc.invoke_with_2_args(subtract_and_sum, a, b)\n"
        "      return {diff, sum}\n"
        "    end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto [res, err] = vc::call_lua<std::tuple<int64_t, int64_t>>(vs.get(), "test_invoke", 10, 3);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(std::get<0>(res) == 7));
    ASSERT_FN(CHK_BOOL(std::get<1>(res) == 13));

    return 0;
}

int test20_lua_object() {
    ASSERT_FN(test20_capture_and_call_typed());
    ASSERT_FN(test20_reference_relayed_through_lua());
    ASSERT_FN(test20_explicit_push_unwraps_to_raw_value());
    ASSERT_FN(test20_capture_replaces_previous_value());
    ASSERT_FN(test20_capture_and_release_from_lua());
    ASSERT_FN(test20_capture_nil_is_a_release());
    ASSERT_FN(test20_raw_call_multiple_results());
    return 0;
}

int main() {
    int ret = test20_lua_object();
    print_test_result("020-001-lua_object.cpp", ret >= 0);
    return ret;
}
