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

/*! Opens this plugin's own log, `<logfile>.log`, at the path the host resolved.
 *
 * The host calls it once per process, from load_plugin(), before any state asks the plugin to
 * register. What a plugin must do once, not once per state, belongs here. 05-10-2026-22:33 */
VIRT_COMPOSER_PLUGIN_EXPORT int plugin_init(const char *logfile) {
    return logger_init(logfile);
}

/*! Closes this plugin's log. The host calls it through uninit_plugins(), when no state will use
 * the plugin again. 05-10-2026-22:33 */
VIRT_COMPOSER_PLUGIN_EXPORT void plugin_uninit() {
    logger_uninit();
}
