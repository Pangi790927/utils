#ifndef VIRT_COMPOSER_END_H
#define VIRT_COMPOSER_END_H

/*! @file
 * Closes the type registrations of one translation unit and publishes what they came to.
 *
 * It is included once, after the last `*_composer.h`, by the one translation unit that registers
 * types. Counting the types is what makes them final, so nothing may register after it: a further
 * `VIRT_COMPOSER_REGISTER_TYPE` in the same unit fails to compile rather than going unnoticed.
 *
 * A host publishes its count where `create_state()` reads it. A plugin counts its types just the
 * same, and closes them just the same, but publishes nothing: the variables it would publish into
 * are the host's own, which it would overwrite while it loads. It answers for its types through
 * `plugin_type_cnt()` instead, which the host asks as it loads it. See
 * VIRT_COMPOSER_PLUGIN_COUNTERS in virt_composer.h.
 *
 * @date 2026-09-20 17:22
 */

namespace virt_composer {

namespace vo = virt_object;
namespace vc = virt_composer;

/* Total number of different types. This file consumes this counter, so all types must be known
before this file */
constexpr vc::object_type_e _VIRT_TYPE_CNT{vo::compile_max_id<vc::virt_tag_t>() + 1};

/* The registrations are closed here, by initialising a variable nobody reads: an initialiser is
the one thing that still runs at namespace scope, and the count is only knowable once every
VIRT_COMPOSER_REGISTER_TYPE in this translation unit has had its turn. 2026-09-20 08:52 */
#ifndef VIRT_COMPOSER_PLUGIN_COUNTERS
inline size_t _virt_composer_var_here_for_the_side_effect = (
    VIRT_TYPES_INITIALIZED = true,
    VIRT_TYPE_CNT = _VIRT_TYPE_CNT.value()
);
#endif /* ifndef VIRT_COMPOSER_PLUGIN_COUNTERS */

}

#endif

