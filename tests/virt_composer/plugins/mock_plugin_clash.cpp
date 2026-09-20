/* Clashing plugin - Category: Plugins
 *
 * A plugin that claims a name mock_plugin_a already registered. It exists to be turned away: two
 * plugins are built by two people who never agreed on names, so the same name twice is an ordinary
 * accident rather than a far-fetched one, and what must not happen is one plugin's Lua calls
 * quietly running the other's code.
 *
 * It registers no types. The clash is over a function name, and that is enough.
 *
 * 2026-09-20 19:30 */

#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"

namespace vc = virt_composer;
namespace vo = virt_object;

int _type_offset = 0;

#include "../../../virt_composer_end.h"

/*! Answers a number belonging to no one else, so a caller reaching this rather than
 * mock_plugin_a's a_make says so plainly. 2026-09-20 19:30 */
static int a_make(lua_State *L) {
    lua_pushinteger(L, 999);
    return 1;
}

extern "C" const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

extern "C" int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

extern "C" int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;

    /* The name mock_plugin_a owns. Answering 0 here is deliberate: the plugin believes it
    registered, and only the host knows it did not. 2026-09-20 19:30 */
    vc::c_function_t::add_plugin_internal_func(vs, "a_make", a_make);
    return 0;
}
