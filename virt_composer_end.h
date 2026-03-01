#ifndef VIRT_COMPOSER_END_H
#define VIRT_COMPOSER_END_H

namespace virt_composer {

namespace vo = virt_object;
namespace vc = virt_composer;

/* Total number of different types. This file consumes this counter, so all types must be known
before this file */
constexpr vc::object_type_e _VIRT_TYPE_CNT{vo::compile_max_id<vc::virt_tag_t>() + 1};

inline size_t _unused_variable_only_here_for_the_side_effect = (
    VIRT_TYPES_INITIALIZED = true,    
    VIRT_TYPE_CNT = _VIRT_TYPE_CNT.value()
);

}

#endif

