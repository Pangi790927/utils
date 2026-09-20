/* Stale plugin - Category: Plugins
 *
 * A plugin that claims a virt_composer it was not built against. linux.makefile builds it with
 * -DVIRT_COMPOSER_ABI overridden, which is the only way to produce one - and the only reason
 * VIRT_COMPOSER_ABI is guarded rather than defined outright. See its doc comment in
 * virt_composer.h: overriding it anywhere but here defeats the check instead of passing it.
 *
 * It registers nothing and has no types, because it must be refused before the host ever asks how
 * many it has.
 *
 * 2026-09-20 18:45 */

#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"

namespace vc = virt_composer;
namespace vo = virt_object;

int _type_offset = 0;

#include "../../../virt_composer_end.h"

extern "C" const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

extern "C" int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

extern "C" int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;
    return 0;
}
