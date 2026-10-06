/* No-init plugin - Category: Plugins
 *
 * A plugin of the shape before plugin_init and plugin_uninit: the three older exports and nothing
 * else. It exists to be refused as not a plugin, since the host has nowhere to tell it where to
 * log.
 *
 * 05-10-2026-22:33 */

#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"
/* The plugin component, right after virt_composer.h: the export macro, the type macro and the
tag. 06-10-2026-00:40 */
#include "../../../virt_composer_plugins.h"

namespace vc = virt_composer;
namespace vo = virt_object;

int _type_offset = 0;

#include "../../../virt_composer_end.h"

VIRT_COMPOSER_PLUGIN_EXPORT const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

VIRT_COMPOSER_PLUGIN_EXPORT int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;
    return 0;
}
