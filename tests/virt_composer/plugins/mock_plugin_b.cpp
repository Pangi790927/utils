/* Mock plugin B - Category: Plugins
 *
 * The same shape as A, answering in the two hundreds, so that a number arriving from the wrong
 * plugin names itself. Built into mock_plugin_b.so and loaded by 021-001 alongside A.
 *
 * Its own member function is `b_only`, which A does not have. See mock_plugin_a.cpp for the pair
 * this forms with it.
 *
 * 2026-09-20 17:14 */

#define MOCK_OWN_MEMBER b_only
#include "mock_common.h"

MOCK_DECLARE_TYPE(b_one_t,   201);
MOCK_DECLARE_TYPE(b_two_t,   202);
MOCK_DECLARE_TYPE(b_three_t, 203);

#include "../../../virt_composer_end.h"

/*! Answers an object of this plugin's nth type, counting from 1.
 *
 * push_vc_object() answers an error code and leaves the object on the stack, so the count of Lua
 * results is ours to return - returning what it answered would say "no results" and the caller
 * would see nil. 2026-09-20 17:22 */
static int b_make(lua_State *L) {
    switch ((int)lua_tointeger(L, 1)) {
        case 1:  vc::push_vc_object(L, b_one_t::create()); return 1;
        case 2:  vc::push_vc_object(L, b_two_t::create()); return 1;
        case 3:  vc::push_vc_object(L, b_three_t::create()); return 1;
        default: lua_pushnil(L); return 1;
    }
}

static int b_ping(lua_State *L) {
    lua_pushinteger(L, 2000);
    return 1;
}

/* Its own VIRT_COMPOSER_ABI, baked in when this file was compiled. Not vc::get_version(), which
resolves to the host's copy and would therefore agree with the host no matter how stale this
plugin is. 2026-09-20 17:14 */
extern "C" const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

extern "C" int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

extern "C" int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;

    MOCK_REGISTER_TYPE(vs, b_one_t);
    MOCK_REGISTER_TYPE(vs, b_two_t);
    MOCK_REGISTER_TYPE(vs, b_three_t);

    vc::c_function_t::add_plugin_internal_func(vs, "b_make", b_make);
    vc::c_function_t::add_plugin_internal_func(vs, "b_ping", b_ping);
    return 0;
}

/*! Answers the id this plugin's nth type ended up with, counting from 1, which is the one thing
 * about the relocation a host cannot ask about on its own. 2026-09-20 17:14 */
extern "C" int plugin_type_id(int n) {
    switch (n) {
        case 1:  return b_one_t_TYPE().value();
        case 2:  return b_two_t_TYPE().value();
        case 3:  return b_three_t_TYPE().value();
        default: return -1;
    }
}
