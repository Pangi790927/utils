/* Mock plugin A - Category: Plugins
 *
 * Three types and two functions, all answering in the hundreds so that anything of B's showing up
 * here is obvious. Built into mock_plugin_a.so and loaded by 021-001; never linked into a test.
 *
 * Its own member function is `a_only`, which B does not have. A B object asked for it must be
 * refused, and that is how the test shows one plugin's members did not land on another's types.
 *
 * 2026-09-20 17:14 */

#define MOCK_OWN_MEMBER a_only
#include "mock_common.h"

MOCK_DECLARE_TYPE(a_one_t,   101);
MOCK_DECLARE_TYPE(a_two_t,   102);
MOCK_DECLARE_TYPE(a_three_t, 103);

#include "../../../virt_composer_end.h"

/*! Answers an object of this plugin's nth type, counting from 1.
 *
 * push_vc_object() answers an error code and leaves the object on the stack, so the count of Lua
 * results is ours to return - returning what it answered would say "no results" and the caller
 * would see nil. 2026-09-20 17:22 */
static int a_make(lua_State *L) {
    switch ((int)lua_tointeger(L, 1)) {
        case 1:  vc::push_vc_object(L, a_one_t::create()); return 1;
        case 2:  vc::push_vc_object(L, a_two_t::create()); return 1;
        case 3:  vc::push_vc_object(L, a_three_t::create()); return 1;
        default: lua_pushnil(L); return 1;
    }
}

/* Logs a line, so 021-008 can see which log a plugin's DBG lands in. 05-10-2026-22:33 */
static int a_ping(lua_State *L) {
    DBG("a_ping ran");
    lua_pushinteger(L, 1000);
    return 1;
}

/* Its own VIRT_COMPOSER_ABI, baked in when this file was compiled, and not anything its host could
answer, which would agree with the host no matter how stale this plugin is. 30-09-2026-16:00 */
VIRT_COMPOSER_PLUGIN_EXPORT const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;

    MOCK_REGISTER_TYPE(vs, a_one_t);
    MOCK_REGISTER_TYPE(vs, a_two_t);
    MOCK_REGISTER_TYPE(vs, a_three_t);

    vc::add_plugin_internal_func(vs, "a_make", a_make);
    vc::add_plugin_internal_func(vs, "a_ping", a_ping);
    return 0;
}

/*! Answers where this plugin finds a piece of state: 0 the owners of names, which must be its
 * host's, then 1 its logger, 2 its type count and 3 its own internal-function table, which must be
 * its own. 021-007 compares them with the host's. 28-09-2026-12:00 */
VIRT_COMPOSER_PLUGIN_EXPORT const void *plugin_state(int which) {
    switch (which) {
        case 0:  return &vc::name_owner();
        case 1:  return &_logger_data;
        case 2:  return &vc::VIRT_TYPE_CNT;
        case 3:  return vc::c_function_t::own_internal_funcs();
        default: return nullptr;
    }
}

/*! Answers the id this plugin's nth type ended up with, counting from 1, which is the one thing
 * about the relocation a host cannot ask about on its own. 2026-09-20 17:14 */
VIRT_COMPOSER_PLUGIN_EXPORT int plugin_type_id(int n) {
    switch (n) {
        case 1:  return a_one_t_TYPE().value();
        case 2:  return a_two_t_TYPE().value();
        case 3:  return a_three_t_TYPE().value();
        default: return -1;
    }
}
