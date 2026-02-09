#ifndef VIRT_COMPOSER_H
#define VIRT_COMPOSER_H

/*!
 * Virtual Composer is a C++ framework designed to create a pool of objects whose members and
 * member functions are backed by C++ code but can be configured via YAML and scripted using Lua.
 *
 * The framework relies on the concept of a translation unit. All logic intended for object linking
 * must be included in a single translation unit. The header `virt_composer.h` should be included
 * in every file using the composer. Thanks to include guards, it will only be processed once per
 * translation unit. This header generates compile-time increments to assign unique IDs to different
 * object types, which are later used to instantiate objects from YAML config files and Lua scripts.
 * 
 * While all type registrations and parser links must be included in a single translation unit (to
 * ensure unique compile-time IDs), the actual implementation of functions, logic, or behaviors can
 * reside in separate .cpp files. This keeps the framework flexible: you only need to centralize
 * the type declarations and their association with the parser, while the rest of your codebase can
 * remain modular and organized. An example is exactly virt_composer.cpp, that needs to be compiled
 * and linked to the final object.
 *
 * User-defined object types and their associated logic should be declared in separate headers,
 * preferably named as `*_composer.h`. These types and their members will be accessible from both
 * YAML configs and Lua scripts. After including all relevant headers, `virt_composer_end.h` must
 * be included to finalize internal counters and values.
 *
 * The public interface of this header provides functions that apply to all user-defined
 * `*_composer.h` files, effectively creating a virt-composer-parser in the translation unit.
 *
 * Users of the parser can instantiate a `virt-state`, which maintains a map of objects (as
 * references: `vc::ref_t<vc::object_t>`, where `object_t` is the base class for all objects in the
 * system and `vc` stands for virt_composer). The object pool can be dynamically enriched or
 * modified by reading YAML config files. Additionally, users can execute Lua scripts to interact
 * with the object pool. A single Lua state is shared across all objects in the pool.
 *
 * Lua scripts are primarily used to define functions for later execution. While basic
 * initialization in scripts is acceptable, the order of script execution is not guaranteed.
 * Therefore, scripts should focus on defining functions rather than performing actions directly.
 *
 * User-defined composers must register object types, members, and functions to make them available
 * to the parser. Users can also attach custom Lua functions to objects during registration.
 */

#include "virt_object.h"

/*! Use this to register a new type. This is only used to register the enumerator. */
#define VIRT_COMPOSER_REGISTER_TYPE(type) \
        constexpr virt_composer::object_type_e type{\
        virt_object::compile_unique_id<virt_composer::virt_tag_t>(), #type}

namespace virt_composer {

namespace vc = virt_composer; 
namespace vo = virt_object;

struct virt_tag_t {};

/* This is a common type enumeration for all the types that can be derived from vku_object_t */
using object_type_e = vo::EnumClass<virt_tag_t>;

struct virt_traits_t {
    using ret_t = int64_t;
    using type_t = object_type_e;
};

using object_t = vo::object_t<virt_traits_t>;

template <typename T>
using ref_t = vo::ref_t<T, virt_traits_t>;

using ret_t = virt_traits_t::ret_t;

};

/*! IMPLEMENTATION
 * ==============================================================================================
 * ==============================================================================================
 * ============================================================================================== */

#endif
