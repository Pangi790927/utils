/* Reference plugin - Category: Plugins
 *
 * A plugin assembled the way one should be, and the file to copy when starting a new one. It holds
 * no types of its own: they live in composer headers beside it, under reference/, each with its
 * own register_meta(), exactly as a host keeps its types in *_composer.h files. This file is only
 * the final one - the single translation unit that gathers them, closes the registrations and puts
 * the three exports on the outside.
 *
 * Two composers for three types, on purpose: vec2 is alone and the two shapes share one, because a
 * composer is a subject rather than a type. Splitting or joining them changes nothing else here.
 *
 * The other plugins in this directory share a header and a declaration macro. That keeps them
 * short and makes them a poor thing to learn the shape from; this one is the shape.
 *
 * The order below is the whole of what makes a translation unit a plugin, and none of it moves:
 *
 *   1. VIRT_COMPOSER_PLUGIN_COUNTERS, before virt_composer.h. It says this unit counts its types
 *      but publishes no count, leaving the host's alone. Each composer header refuses to compile
 *      without it, so the order cannot be got wrong quietly.
 *   2. `_type_offset`, this plugin's own, before any composer. The host writes it while loading
 *      and every type id in every composer is read through it.
 *   3. The composers, one include each.
 *   4. virt_composer_end.h, after the last of them. It closes the registrations: a type declared
 *      below this line would not compile.
 *   5. plugin_get_version, plugin_type_cnt and plugin_register_meta, exported "C" so the host
 *      finds them by name.
 *
 * 2026-09-20 18:12 */

/* 1. */
#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"

namespace vc = virt_composer;
namespace vo = virt_object;

/* 2. Written by the host through plugin_register_meta() before anything asks a type for its id.
Until then every id in every composer is a bare index and means nothing. 2026-09-20 18:12 */
int _type_offset = 0;

/* 3. The composers. Adding one is a line here and a line in plugin_register_meta below; adding a
type to one that already exists is neither. 2026-09-20 18:12 */
#include "reference/vec2_composer.h"
#include "reference/shapes_composer.h"

/* 4. */
#include "../../../virt_composer_end.h"

/* 5. The three exports the host looks for by name, which is why they are extern "C". */

/*! Answers the virt_composer this plugin was compiled against.
 *
 * It must be this plugin's own VIRT_COMPOSER_ABI and not vc::get_version(), which resolves to the
 * host's copy and would agree with the host however stale this plugin is. 2026-09-20 18:12 */
extern "C" const char *plugin_get_version() {
    return VIRT_COMPOSER_ABI;
}

/*! Answers how many type ids this plugin needs.
 *
 * Counted rather than written down, so it cannot drift from the composers above: a count too small
 * and the host reserves too little room for them. 2026-09-20 18:12 */
extern "C" int plugin_type_cnt() {
    return vo::compile_max_id<vc::plugin_tag_t>() + 1;
}

/*! Takes the offset the host reserved and hands each composer the state to register itself into.
 *
 * Runs once per state rather than once per process, so everything reached from here is about the
 * state it is handed. Registering the same thing twice is harmless - each step is an assignment -
 * but allocating or opening something would happen again for the next state. 2026-09-20 18:12 */
extern "C" int plugin_register_meta(vc::virt_state_t *vs, int type_offset) {
    _type_offset = type_offset;

    ASSERT_FN(vec2_composer::register_meta(vs));
    ASSERT_FN(shapes_composer::register_meta(vs));
    return 0;
}
