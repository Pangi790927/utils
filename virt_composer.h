#ifndef VIRT_COMPOSER_H
#define VIRT_COMPOSER_H

/*!
 * @brief A pool of C++-backed objects whose members and member functions can be configured from
 * YAML and driven from Lua.
 *
 * Core:
 *   - Object types, their members and their functions are declared in C++ and registered with the
 *     parser. Once registered they are reachable from both YAML configs and Lua scripts, under the
 *     names they were registered with.
 *   - Only the declarations need to be centralised. The implementations behind those functions may
 *     sit in any .cpp file, which is what keeps the rest of a codebase modular; virt_composer.cpp
 *     is exactly that, compiled and linked separately.
 *   - A virt-state owns one object pool and one Lua state shared by every object in it. The pool
 *     is a name-indexed lookup reached through accessors such as `get_ref()`, which hand back
 *     `vc::ref_t<vc::object_t>` handles; the pool itself does not hold ownership that way.
 *   - The pool can be enriched or modified at run time by reading further YAML configs, and driven
 *     by running Lua scripts against it.
 *
 * Detail:
 *   - `virt_composer.h` goes into every file that uses the composer; include guards mean it is
 *     processed once per translation unit.
 *   - User-defined types belong in headers of their own, by convention named `*_composer.h`. After
 *     the last of them, `virt_composer_end.h` must be included: it finalises the compile-time
 *     counters and the values derived from them.
 *   - `vc` is the namespace abbreviation for `virt_composer`, and `object_t` is the base class of
 *     everything in the pool.
 *
 * @note The order in which Lua scripts run is not guaranteed, so a script should define functions
 *       rather than perform actions. Basic initialisation is acceptable; anything order-dependent
 *       is not.
 *
 * @date 2026-09-08 06:54
 */

/* TODO: all yaml nodes should be able to define dependencies, those dependencies would be
especially usefull for things like shaders, lua scripts, expressions, etc. This would in a sense
create a strict ordering that and I should check if it can create cycles (deadlocks).

Plugins want the same thing and want it more, because what a plugin brings is not an object another
node can wait for. A node's `m_type` is resolved while that node is built and build_object() throws
on one it does not know; a `vc::c_function_t` with `m_source: "[INTERNAL]"` is looked up while it
is built and create() throws when the name is not registered yet. Neither can wait the way an
unresolved `!ref` waits, so a config that loads a plugin and uses what the plugin brought, in the
same file, is right or wrong according to the order its nodes happen to be built in.

Both failures are loud - parse_config() answers an error and names what it did not know - so this
costs a config that refuses to load rather than one that loads wrongly. Until a node can say what
it depends on, the way around it is to load plugins in a pass of their own, before the config that
uses them is parsed. 2026-09-20 18:40 */

#include  <typeindex>

#include "virt_object.h"
#include "co_utils.h"
#include "yaml.h"
#include "minilua.h"
#include "demangle.h"

#if defined(_MSC_VER) && !defined(ssize_t)
using ssize_t = ptrdiff_t;
#endif

/*!
 * @def VIRT_COMPOSER_ABI
 * @brief Identifies the build of virt_composer a translation unit was compiled against.
 *
 * Core:
 *   - A plugin bakes this in when it is compiled and reports it back through its own
 *     `plugin_get_version()`. The host compares it with its own and refuses a plugin whose value
 *     differs.
 *   - It is written by hand and raised by hand. Anything that changes what crosses between a host
 *     and a plugin - `object_t`'s layout, a signature, the meaning of a type id - wants it raised
 *     in the same edit, because nothing else will notice.
 *
 * @warning A build may override it, and only a test has any business doing so: it is the one way
 *       to produce a plugin that disagrees with its host on purpose, and the refusal in
 *       `load_plugin()` cannot be exercised otherwise. Overriding it anywhere else defeats the
 *       check rather than passing it - a plugin told to claim the host's version still runs
 *       against a library it was not built for, which is the whole of what this guards against.
 *       Rebuild the plugin instead.
 * @warning Forgetting to raise it is caught nowhere in the library. A plugin built against an
 *       older header passes the check and runs against a library it no longer agrees with, which
 *       is the failure this macro exists to prevent. The test suite watches for that - see
 *       `tests/virt_composer/021-003-abi_hash.cpp`, which hashes the files whose contents cross
 *       the boundary and fails when this no longer matches them. Deriving the value from the
 *       source was tried and dropped: `__TIMESTAMP__` written in a header answers for the
 *       translation unit's main file rather than for the header, so it compared two unrelated
 *       source files and refused every honest plugin.
 *
 * @see load_plugin
 *
 * @date 2026-09-20 18:45
 */
#ifndef VIRT_COMPOSER_ABI
# define VIRT_COMPOSER_ABI  "0.3-0efaff81"
#endif

/*!
 * @def VIRT_COMPOSER_PLUGIN_COUNTERS
 * @brief Marks a translation unit as a plugin's, so that it counts its types without publishing
 * the answer.
 *
 * Core:
 *   - Left undefined, the translation unit is a host's: `virt_composer_end.h` publishes the type
 *     count in `VIRT_TYPE_CNT` and raises `VIRT_TYPES_INITIALIZED`, which is what `create_state()`
 *     reads and what it refuses to run without.
 *   - Defined, the translation unit is a plugin's: the counting still happens and still closes the
 *     registrations, but nothing is published. A plugin answers for its own types through
 *     `plugin_type_cnt()` instead, and the host asks it there.
 *   - Only whether it is defined matters, not what it is defined to. It must be defined before
 *     this header is included.
 *
 * Detail:
 *   - Both published variables are `inline`, so a plugin resolving this library's symbols from its
 *     host resolves those two as well - they are the host's, not copies. A plugin that published
 *     its own count would therefore overwrite the host's while it loads, which happens before the
 *     host can check anything about the plugin, its version included. Publishing nothing is what
 *     avoids that.
 *   - Publishing nothing rather than hiding the host's symbols from the plugin: a hidden symbol
 *     would give the plugin a second, zero-valued copy, and a read would quietly answer 0 instead
 *     of the host's count.
 *
 * @see load_plugin, VIRT_COMPOSER_REGISTER_PLUGIN_TYPE
 *
 * @date 2026-09-20 17:14
 */
#ifndef VIRT_COMPOSER_PLUGIN_COUNTERS
// Nothing here, this is only for documentation purposes
#endif

/*!
 * @def VIRT_COMPOSER_ENABLE_LUA_IO
 * @brief Exposes Lua's standard `io` library to every Lua script running in this virt_state_t.
 *
 * Core:
 *   - Disabled (`0`) by default. While disabled, `luaw_init()` skips
 *     `luaL_requiref(L, LUA_IOLIBNAME, ...)`, so scripts have no `io.*` at all and cannot read or
 *     write arbitrary files.
 *   - Must be `#define`d before `virt_composer.h` is included, as every other `VIRT_COMPOSER_*`
 *     configuration macro must. It is read while this header is processed, so a definition placed
 *     after the include has no effect.
 *
 * @date 2026-09-08 06:54
 */
#ifndef VIRT_COMPOSER_ENABLE_LUA_IO
# define VIRT_COMPOSER_ENABLE_LUA_IO 0
#endif

/*!
 * @def VIRT_COMPOSER_ENABLE_LUA_OS
 * @brief Exposes Lua's standard `os` library to every Lua script running in this virt_state_t.
 *
 * Core:
 *   - Disabled (`0`) by default. While disabled, `luaw_init()` skips
 *     `luaL_requiref(L, LUA_OSLIBNAME, ...)`, so scripts have no `os.*` at all: no `os.execute`,
 *     `os.remove`, `os.getenv` or the rest.
 *   - Must be `#define`d before `virt_composer.h` is included, as every other `VIRT_COMPOSER_*`
 *     configuration macro must. It is read while this header is processed, so a definition placed
 *     after the include has no effect.
 *
 * @date 2026-09-08 06:54
 */
#ifndef VIRT_COMPOSER_ENABLE_LUA_OS
# define VIRT_COMPOSER_ENABLE_LUA_OS 0
#endif


/* TODO: VIRT_COMPOSER_ENABLE_LUA_IO/_OS are currently a single compile-time, process-wide switch -
we may want Lua io/os access to be enabled per virt_state_t instead (so e.g. a "trusted" state and
a "sandboxed" state can coexist in the same process) and/or toggleable at runtime rather than only
at compile time, for whichever of the two is already enabled via these macros. */

/*!
 * @def VIRT_COMPOSER_REGISTER_TYPE(type)
 * @brief Registers an object type with the framework, giving it the unique id the parser uses to
 * instantiate it.
 *
 * Core:
 *   - Generates a unique enumerator value for `type` through
 *     `virt_object::compile_unique_id<virt_composer::virt_tag_t>()`, and associates it with the
 *     stringified enum name (`#type`).
 *   - The value it produces is what the corresponding object, inheriting `vc::object_t`, is
 *     expected to return from `type_id()` and `type_id_static()`.
 *   - Used together with `virt_composer_end.h`, which computes `VIRT_TYPE_CNT` right before
 *     `create_state()`, it guarantees the two properties the framework actually depends on: every
 *     type id is distinct, and `virt_composer::VIRT_TYPE_CNT` is greater than all of them. Arrays
 *     of that length are declared, so breaking either is not diagnosed, it is a bad access.
 *
 * Detail:
 *   - Neither property requires this macro. Any type-id scheme is allowed as long as it keeps
 *     both, and that is the only reason to care which one is in use.
 *
 * @param type The enum name associated to a type to register as an enumerator.
 *
 * @see object_type_e
 *
 * @date 2026-09-08 06:54
 */
#define VIRT_COMPOSER_REGISTER_TYPE(type) \
        constexpr virt_composer::object_type_e type{ \
        virt_object::compile_unique_id<virt_composer::virt_tag_t>(), #type}


/*!
 * @def VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(type)
 * @brief Registers a plugin's object type, as a function answering the id rather than a constant.
 *
 * Core:
 *   - Defines `type()`, which answers the `object_type_e` the object inheriting `vc::object_t` is
 *     expected to return from `type_id()` and `type_id_static()`. It is called, not read:
 *     `VC_TYPE_FOO()` where a host's type would be written `VC_TYPE_FOO`.
 *   - The id it answers is `_type_offset` plus an index counted under
 *     `virt_composer::plugin_tag_t`. The plugin's translation unit must define `_type_offset`, and
 *     one that does not fails to link, so the offset cannot be left out quietly.
 *   - It answers correctly only once the host has set `_type_offset`, which `load_plugin()` does
 *     before it lets the plugin register. Read before that, every id is its bare index and
 *     collides with the host's built-in types.
 *   - It also defines `type##_local`, the index on its own, so two types whose names differ only
 *     by that suffix collide.
 *
 * Detail:
 *   - A function rather than a constant precisely because of the ordering above. A namespace-scope
 *     constant is initialised while the shared object loads, which is before the host can hand
 *     over an offset, so it would capture 0 and say nothing about it.
 *   - `plugin_tag_t` counts from 0 independently of `virt_tag_t`, so a plugin's first type is its
 *     index 0 and the offset needs no adjustment for the six types this header registers.
 *
 * @param type The enum name associated to a type to register as an enumerator.
 *
 * @see VIRT_COMPOSER_REGISTER_TYPE, plugin_tag_t, load_plugin
 *
 * @date 2026-09-20 16:07
 */
#define VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(type) \
        constexpr int type##_local = \
                virt_object::compile_unique_id<virt_composer::plugin_tag_t>(); \
        inline virt_composer::object_type_e type() { \
            return virt_composer::object_type_e{_type_offset + type##_local, #type}; }

/*!
 * @def VC_REGISTER_MEMBER_OBJECT(vs, obj_type, memb)
 * @brief Makes a member variable of a registered type readable and writable from Lua.
 *
 * Core:
 *   - Expands to a call to `luaw_register_member_object`, binding `memb` of `obj_type` under its
 *     own name (`#memb`) in the given virt-state.
 *   - Only types the library knows how to carry across the Lua boundary may be registered: string,
 *     bool, int and double, vector, tuple and pair, `vc::ref_t<T>` objects, and `vc::bm_t<T>`
 *     (Lua to C++ only - see its own doc). For anything else the library has no conversion in
 *     either direction.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param memb      The member variable to register.
 *
 * @see luaw_register_member_object
 *
 * @example
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_cmdpool)
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_host_free)
 *
 * @date 2026-09-08 06:54
 */
#define VC_REGISTER_MEMBER_OBJECT(vs, obj_type, memb)   \
virt_composer::luaw_register_member_object<             \
/* self        */   obj_type,                           \
/* member      */   &obj_type::memb>                    \
/* member name */   (vs, #memb)

/*!
 * @def VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs, obj_type, memb)
 * @brief Lets a YAML field take a raw value straight out of an already-built object's member,
 * byte for byte.
 *
 * Core:
 *   - Expands to a call to `register_trivially_copyable_member`, which registers a memcpy-based
 *     accessor for `memb` of `obj_type`.
 *   - What reaches that accessor is a YAML field tagged `!copy`, carrying `object` and `member`
 *     sub-fields, resolved from inside a builder callback through `resolve_memb<T>(vs, node)`.
 *   - The copy is untyped, so `memb` must satisfy `std::is_trivially_copyable_v`; a
 *     `static_assert` enforces it.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param memb      The member variable to register. Must be trivially copyable.
 *
 * @see register_trivially_copyable_member, resolve_memb
 *
 * @example
 * VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs, vec3_t, x);
 *
 * // yaml:
 * // some_float:
 * //   m_type: my_float_copy_t
 * //   value: !copy
 * //     object: some_vec3
 * //     member: x
 *
 * @date 2026-09-08 06:54
 */
#define VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs, obj_type, memb)   \
virt_composer::register_trivially_copyable_member<                  \
/* self        */   obj_type,                                       \
/* member      */   &obj_type::memb>                                \
/* member name */   (vs, #memb)

/*!
 * @def VC_REGISTER_MEMBER_FUNCTION(vs, obj_type, fn, ...)
 * @brief Makes a member function of a registered type callable from Lua.
 *
 * Core:
 *   - Expands to a call to `luaw_register_member_function`, binding `fn` of `obj_type` under its
 *     own name (`#fn`) in the given virt-state.
 *   - The parameter types are part of the registration and must be listed after `fn`; they are
 *     what the call from Lua is parsed into.
 *   - Only types the library knows how to carry across the Lua boundary may appear there: string,
 *     bool, int and double, vector, tuple and pair, `vc::ref_t<T>` objects, and `vc::bm_t<T>`
 *     (Lua to C++ only - see its own doc). For anything else the library has no conversion in
 *     either direction.
 *   - A function registered this way is reachable as a method, `obj:fn(...)`. Functions attached
 *     by other means, such as a bare `lua_setfield()` on the shared metatable, are not: method
 *     calls go through the per-class `__index` dispatch, which only sees what was registered here.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param fn        The member function to register.
 * @param ...       Parameter types of the member function.
 *
 * @see luaw_register_member_function
 *
 * @example
 * VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin_rpass, vc::ref_t<vku::framebuffs_t>, uint32_t);
 *
 * @date 2026-09-08 06:54
 */
#define VC_REGISTER_MEMBER_FUNCTION(vs, obj_type, fn, ...)  \
virt_composer::luaw_register_member_function<               \
/* self     */  obj_type,                                   \
/* function */  &obj_type::fn,                              \
/* params   */  ##__VA_ARGS__>                              \
/* fn name  */  (vs, #fn)


namespace virt_composer {

namespace vc = virt_composer; 
namespace vo = virt_object;

/*!
 * @brief Error codes returned throughout the virt_composer public API.
 *
 * Core:
 *   - `VC_ERROR_OK` is `0` and means success. Every other value is negative and means failure,
 *     following this codebase's general convention (see e.g. `debug.h`'s `ASSERT_FN`).
 *
 * @date 2026-09-08 06:54
 */
enum err_e : int32_t {
    VC_ERROR_OK = 0,           /*!< Success. */
    VC_ERROR_GENERIC = -1,     /*!< Catch-all failure - e.g. an unrecognized `m_type`, a
                                     coroutine-pool run failing, or a caught std::exception during
                                     parse_config()/object construction that doesn't fit one of the
                                     more specific codes below. */
    VC_ERROR_PARSE_YAML = -2,  /*!< The YAML document itself failed to parse (a caught
                                     fkyaml::exception) - malformed syntax, not a semantic error in
                                     an otherwise-valid document. */
    VC_ERROR_FAILED_CALL = -3, /*!< A Lua call failed - lua_pcall()/luaL_dostring() returned
                                     non-OK, e.g. from call_lua(), or from executing a
                                     lua_script_t's m_source/m_source_path content. */
    VC_ERROR_REDEFINED = -4,   /*!< A name is already registered by someone else and was not taken
                                     from them. The first claimant of a name keeps it, so this says
                                     the registration did not happen, not that anything was
                                     replaced. */
};

/*!
 * @brief Identifies a Lua arithmetic, relational or misc metamethod slot (`__add`, `__eq`,
 * `__unm`, ...).
 *
 * Core:
 *   - One `lua_CFunction` may be registered per `object_type_e` per entry here, through
 *     @ref set_class_operator.
 *   - Every virt_composer object shares a single Lua metatable, so these slots are the only
 *     per-type customization point for operators.
 *
 * @see set_class_operator, which carries the exact dispatch rules.
 *
 * @date 2026-09-08 06:54
 */
enum operator_e : int32_t {
    VC_OPERATOR_ADD,
    VC_OPERATOR_SUB,
    VC_OPERATOR_MUL,
    VC_OPERATOR_DIV,
    VC_OPERATOR_MOD,
    VC_OPERATOR_POW,
    VC_OPERATOR_IDIV,
    VC_OPERATOR_BAND,
    VC_OPERATOR_BOR,
    VC_OPERATOR_BXOR,
    VC_OPERATOR_SHL,
    VC_OPERATOR_SHR,
    VC_OPERATOR_UNM,
    VC_OPERATOR_BNOT,
    VC_OPERATOR_CONCAT,
    VC_OPERATOR_LEN,
    VC_OPERATOR_EQ,
    VC_OPERATOR_LT,
    VC_OPERATOR_LE,
    VC_OPERATOR_CNT,
};

/*!
 * @brief The exception type virt_composer throws for errors in its own model.
 *
 * Core:
 *   - Raised throughout the parser and the Lua bridge for conditions specific to virt_composer: an
 *     invalid or unknown object type, a duplicate or missing name, a malformed YAML node shape, an
 *     unknown enum string value, and similar.
 *   - The constructor takes only the error message. It prepends a C++ backtrace
 *     (`cpp_backtrace()`) to `err_str`, so `what()` already carries call-stack context.
 *   - A failed `ref_t<T>` cast does not come through here: it throws a plain `std::runtime_error`
 *     instead, and so is reported differently.
 *
 * Detail:
 *   - The backtrace is a real one on Linux and Unix where boost::stacktrace or <backtrace.h> is
 *     available, and a fixed placeholder string on MSVC - see cpp_backtrace.h.
 *
 * @note `luaw_catch_exception()` catches `except_t` specifically, ahead of its generic
 *       `std::exception` fallback, so one thrown from inside a Lua-callable wrapper reaches Lua as
 *       `"Invalid call: <message>"` rather than the more generic `"std::exception: <message>"`.
 *
 * @date 2026-09-08 06:54
 */
struct except_t : public std::exception {
    std::string err_str;

    except_t(const std::string& str);
    const char *what() const noexcept override { return err_str.c_str(); };
};

/*!
 * @brief Opaque handle for one independent virt_composer instance: one Lua state, one pool of
 * named objects, one set of registered types, members and functions.
 *
 * Core:
 *   - Obtained from `create_state()`, and passed to essentially every other function in this file.
 *   - Two instances are fully independent - separate Lua states, separate name tables, separate
 *     object pools - and nothing built in one is visible from the other.
 *   - The single exception is `c_function_t::internal_funcs`, the table `add_internal_func()`
 *     registers into. It is a process-wide static rather than per-instance, so registering an
 *     internal function once makes it visible to every `virt_state_t` in the process, not only to
 *     the one that was in mind.
 *   - Every `vc::object_t` obtained from a state - through `get_ref`, `call_lua`, a member getter,
 *     anything - is valid only while that state is alive, since its destruction closes the
 *     underlying Lua state. Nothing here enforces that; the caller carries it.
 *
 * Detail:
 *   - The type is fully defined in `virt_composer.cpp`, not in this header, so user code only ever
 *     sees it as a `virt_state_t *`.
 *
 * @see create_state, get_ref, parse_config, call_lua
 *
 * @date 2026-09-08 06:54
 */
struct virt_state_t;


/*!
 * @brief Counts the type ids belonging to a program itself, the ones registered with
 * VIRT_COMPOSER_REGISTER_TYPE.
 *
 * Core:
 *   - Serves two unrelated purposes: it is the tag `compile_unique_id` counts against, and the tag
 *     that makes `object_type_e` a type of its own. Every id is an `object_type_e`, whichever
 *     counter produced it.
 *   - The count restarts at 0 in every translation unit, so the six types this header registers
 *     always take 0 through 5, in a plugin exactly as in its host. Plugins depend on that
 *     agreement, since those six are the host's and are shared with it.
 *
 * @see plugin_tag_t, VIRT_COMPOSER_REGISTER_TYPE, object_type_e
 *
 * @date 2026-09-20 16:07
 */
struct virt_tag_t {};


/*!
 * @brief Counts the type ids belonging to a plugin, separately from its host's.
 *
 * Core:
 *   - A plugin's types count from 0 under this tag while the built-in types count from 0 under
 *     `virt_tag_t`, so the two never interleave and a plugin's first type is its index 0. What
 *     keeps a plugin's ids clear of its host's is the offset added on top, not this tag.
 *   - Each shared object counts on its own, the counter being per translation unit, so one tag
 *     serves every plugin.
 *
 * @see virt_tag_t, VIRT_COMPOSER_REGISTER_PLUGIN_TYPE, load_plugin
 *
 * @date 2026-09-20 16:07
 */
struct plugin_tag_t {};

/*!
 * @brief The type-id enumeration for every object derived from `vc::object_t`.
 *
 * Core:
 *   - Identifies object types uniquely across the framework; it is what `type_id()` and
 *     `type_id_static()` hand back.
 *   - New types are registered with @ref VIRT_COMPOSER_REGISTER_TYPE(name_of_enum), which
 *     generates the unique enumerator. Registrations are tracked up to and including
 *     `virt_composer_end.h`.
 *
 * @see VIRT_COMPOSER_REGISTER_TYPE
 *
 * @date 2026-09-08 06:54
 */
using object_type_e = vo::EnumClass<virt_tag_t>;

/* The said registrations for internal objects(other libraries will also hold their own): */
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_STRING);
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_FLOAT);
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_INTEGER);
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_LUA_SCRIPT);
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_C_FUNCTION);
VIRT_COMPOSER_REGISTER_TYPE(VC_TYPE_LUA_OBJECT);

/*!
 * @brief Base object type for virt_composer: everything the pool holds derives from it.
 *
 * Core:
 *   - An alias for `virt_object::object_t<object_type_e>` - the generic `object_t<Id>` from
 *     `virt_object.h`, parameterized here by this file's own type-id enum.
 *   - Every derived type is ultimately one of these: `integer_t`, `float_t`, `string_t`, and any
 *     user-defined type.
 *
 * @date 2026-09-08 06:54
 */
using object_t = vo::object_t<object_type_e>;

/*!
 * @brief Return type for object operations.
 *
 * Core:
 *   - An alias for `virt_object::ret_t` (`int64_t`), used as the return type of `init()` and
 *     `uninit()`-style functions on objects derived from @ref object_t.
 *   - It carries an @ref err_e code, so `VC_ERROR_OK` means success and negative values mean
 *     failure.
 *
 * @date 2026-09-08 06:54
 */
using ret_t = vo::ret_t;

/*!
 * @brief Reference to a virt_composer object, and the handle user code actually holds.
 *
 * Core:
 *   - A template alias for `virt_object::ref_t<T>`, where `T` must derive from
 *     `virt_composer::object_t`.
 *   - Valid only while the `virt_state_t` it came from is alive; see @ref virt_state_t for what
 *     that costs the caller.
 *
 * @tparam T The object type that is held by this reference.
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
using ref_t = vo::ref_t<T>;

/*!
 * @brief Declares that a Lua argument is an enum or bitmask, so a script may pass it as a string,
 * an integer, or a table of either.
 *
 * Core:
 *   - Used as a parameter type in @ref VC_REGISTER_MEMBER_FUNCTION or @ref luaw_function_wrapper,
 *     it accepts three Lua spellings of the same value: a string naming the enumerator
 *     (`"vc.READ"`), an integer cast to `T`, or a table whose entries are combined into a single
 *     bitmask (`{vc.READ, vc.WRITE}`).
 *   - `T` must be convertible from both strings and integers. The string form needs a
 *     `get_enum_val<T>` specialisation in the `virt_composer` namespace; without one the
 *     registration will not work.
 *   - The conversion is one-way, Lua to C++. `bm_t<T>` describes how an incoming Lua value is
 *     parsed and has no existence past that point.
 *
 * Detail:
 *   - The wrapper is stripped back to a plain `T` right after parsing, so a `T`-typed member or
 *     return value is pushed back to Lua as a plain enum value and never re-wrapped into the
 *     string or table forms.
 *
 * @tparam T The enum/bitmask type (e.g., `OpenFlagBits`).
 *
 * @example
 * // C++:
 * VC_REGISTER_MEMBER_FUNCTION(vku::some_object_t, open, bm_t<OpenFlagBits>);
 *
 * // Lua:
 * vc.object:open("fname", {vc.READ, vc.WRITE}) -- Table of enum strings or integers
 * vc.object:open("fname", "READ")              -- Single enum string
 * vc.object:open("fname", 1)                   -- Single enum integer value
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
struct bm_t {
    using type = T;
};

/*!
 * @brief Wraps a 64-bit integer as a composer object, so a plain number can be named, referenced
 * and held in the pool like any other object.
 *
 * Core:
 *   - `value` is the stored integer and is public. Nothing guards it and nothing observes changes
 *     to it; the object exists to give a number an identity, not to protect it.
 *   - Built from YAML by structure rather than by `m_type`: a config entry whose value is a plain
 *     integer produces one of these.
 *
 * @param value Initial integer value, taken by `create(value)`.
 *
 * @date 2026-09-08 06:54
 */
struct integer_t : public vc::object_t {
    int64_t value = 0;

    integer_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~integer_t() {}

    static vc::ref_t<integer_t> create(int64_t value) {
        auto ret = std::make_shared<integer_t>(vc::object_t::Private{type_id_static()});
        ret->value = value;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return VC_TYPE_INTEGER; }
    static vc::object_type_e type_id_static() { return VC_TYPE_INTEGER; }

    inline std::string to_string() const override {
        return std::format("vc::integer[{}]: value={} ", (void*)this, value);
    }
};

/*!
 * @brief Wraps a double-precision float as a composer object, so a plain number can be named,
 * referenced and held in the pool like any other object.
 *
 * Core:
 *   - `value` is the stored double and is public. Nothing guards it and nothing observes changes
 *     to it; the object exists to give a number an identity, not to protect it.
 *   - Built from YAML by structure rather than by `m_type`: a config entry whose value is a plain
 *     floating-point number produces one of these.
 *
 * @param value Initial floating-point value, taken by `create(value)`.
 *
 * @date 2026-09-08 06:54
 */
struct float_t : public vc::object_t {
    double value = 0;

    float_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~float_t() {}

    static vc::ref_t<float_t> create(double value) {
        auto ret = std::make_shared<float_t>(vc::object_t::Private{type_id_static()});
        ret->value = value;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return VC_TYPE_FLOAT; }
    static vc::object_type_e type_id_static() { return VC_TYPE_FLOAT; }

    inline std::string to_string() const override {
        return std::format("vc::float[{}]: value={} ", (void*)this, value);
    }

};

/*!
 * @brief Wraps a string as a composer object, so text can be named, referenced and held in the
 * pool like any other object.
 *
 * Core:
 *   - `value` is the stored string and is public. Nothing guards it and nothing observes changes
 *     to it; the object exists to give the text an identity, not to protect it.
 *   - Built from YAML by structure rather than by `m_type`: a config entry whose value is a plain
 *     string produces one of these.
 *
 * @param value Initial string content, taken by `create(value)`.
 *
 * @date 2026-09-08 06:54
 */
struct string_t : public vc::object_t {
    std::string value;

    string_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~string_t() {}

    static vc::ref_t<string_t> create(const std::string& value) {
        auto ret = std::make_shared<string_t>(vc::object_t::Private{type_id_static()});
        ret->value = value;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return VC_TYPE_STRING; }
    static vc::object_type_e type_id_static() { return VC_TYPE_STRING; }

    inline std::string to_string() const override {
        return std::format("vc::string[{}]: value={} ", (void*)this, value);
    }
};

/* TODO: add `!lua` */
/*!
 * @brief Holds Lua source as a named composer object, so a script can sit in the pool and be
 * loaded or executed against a Lua state.
 *
 * Core:
 *   - `content` is the script text and is public.
 *   - Built from YAML by `m_type`, carrying exactly one of `m_source`, which holds the text
 *     inline, or `m_source_path`, which names a file to read it from. Supplying both, or neither,
 *     fails the build.
 *   - Running it is one of the things that can produce @ref VC_ERROR_FAILED_CALL.
 *
 * @param content The Lua script source code as a string, taken by `create(content)`.
 *
 * @date 2026-09-08 06:54
 */
struct lua_script_t : public vc::object_t {
    std::string content;

    lua_script_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~lua_script_t() {}

    static vc::object_type_e type_id_static() { return VC_TYPE_LUA_SCRIPT; }
    static vc::ref_t<lua_script_t> create(std::string content) {
        auto ret = std::make_shared<lua_script_t>(vc::object_t::Private{type_id_static()});
        ret->content = content;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return VC_TYPE_LUA_SCRIPT; }

    inline std::string to_string() const override {
        return std::format("vc::lua_script[{}]: m_content=\n{}", (void*)this, content);
    }
};

/* Does this really have any irl usage? ANSW: YES! It holds (should hold) C lua callbacks */
/*!
 * @brief Exposes a C++ callback to Lua as a named composer object, so a script can call into
 * native code the way it calls anything else.
 *
 * Core:
 *   - The callback is bound once, when the object is created, not looked up at call time. A name
 *     that was never registered fails `create()` instead of producing an object that dies on its
 *     first call.
 *   - `add_internal_func(name, fn)` is what puts a callback within reach of `create()`. It is
 *     static and shared by the whole process, so it has to run before any object naming that
 *     function is built.
 *   - `call(L)` invokes the bound callback with the given Lua state; `-1` means nothing was ever
 *     successfully bound.
 *
 * Detail:
 *   - `m_source` says where the callback comes from and currently accepts only `"[INTERNAL]"`,
 *     which searches the table `add_internal_func()` fills. Loading from a DLL or shared object is
 *     planned - `dll_handles` and `dll_funcs` are placeholders for it - but no other value works
 *     yet.
 *
 * @param name   Name of the function as seen from Lua, and the key `add_internal_func()` was
 * given.
 * @param source Must currently be `"[INTERNAL]"`.
 * @throws vc::except_t if `init()` fails: a `source` other than `"[INTERNAL]"`, or a `name` that
 * was never passed to `add_internal_func()`.
 *
 * @date 2026-09-08 06:54
 */
struct c_function_t : public vc::object_t {
    std::string m_name;
    std::string m_source;

    c_function_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~c_function_t() { uninit(); }

    static vc::ref_t<c_function_t> create(virt_state_t *vs, std::string name, std::string source);

    virtual vc::object_type_e type_id() const override { return VC_TYPE_C_FUNCTION; }
    static vc::object_type_e type_id_static() { return VC_TYPE_C_FUNCTION; }

    int call(lua_State *L);

    inline std::string to_string() const override {
        return std::format("vc::c_function[{}]: m_name={} m_source={}",
                (void*)this, m_name, m_source);
    }

    /*! Registers a callback under a name, for a `[INTERNAL]` c_function_t to bind later.
     *
     * This one fills the table belonging to the module it is called from, which is what the host
     * wants: it may be called before any state exists. A plugin wants
     * @ref add_plugin_internal_func instead. @date 2026-09-20 19:10 */
    static void add_internal_func(std::string name, std::function<int(lua_State *L)> fn) {
        c_function_t::internal_funcs[name] = fn;
    }

    /*! Registers a callback into the table the given state binds its `[INTERNAL]` names from.
     *
     * Core:
     *   - The same as @ref add_internal_func except for which table it fills, and a plugin must
     *     use this one.
     *   - The table add_internal_func() fills is a static of this header. A plugin carrying its
     *     own copy of this library therefore fills a table its host never reads, and every
     *     `[INTERNAL]` name the plugin meant to serve goes missing. Where the two share one copy
     *     the calls are the same, which is what makes the mistake invisible until it is ported.
     *
     * @date 2026-09-20 19:10 */
    static void add_plugin_internal_func(virt_state_t *vs, std::string name,
            std::function<int(lua_State *L)> fn);

    /*! [INTERNAL] Answers the internal-function table belonging to the module this is compiled
     * into, which is the one a state made by that module is created pointing at.
     * @date 2026-09-20 19:10 */
    static std::map<std::string, std::function<int(lua_State *L)>> *own_internal_funcs() {
        return &internal_funcs;
    }

private:
    std::function<int(lua_State *L)> _fn;
    static std::map<std::string, std::function<int(lua_State *L)>> internal_funcs;

    /* TODO: unused placeholders for loading a function out of a shared object.
    WARNING: load_plugin() answers that question now, and building on these would make a second
    answer to it. They want deleting rather than filling in. 2026-09-20 19:10 */
    static std::map<std::string, void *> dll_handles;
    static std::map<std::string, std::function<int(lua_State *L)>> dll_funcs;

    vc::ret_t init(virt_state_t *vs);
    vc::ret_t uninit() { return VC_ERROR_OK; }
};

inline std::map<std::string, std::function<int(lua_State *L)>>  c_function_t::internal_funcs;
inline std::map<std::string, std::function<int(lua_State *L)>>  c_function_t::dll_funcs;
inline std::map<std::string, void *>                            c_function_t::dll_handles;

/*!
 * @brief Holds a strong reference to an arbitrary Lua value - a function, table, string or
 * anything else - so C++ can keep it alive past the call that handed it over and use it later.
 *
 * Core:
 *   - The reverse direction of @ref c_function_t: that exposes a C++ callback to Lua, this keeps
 *     a Lua value reachable from C++.
 *   - The value is held in a dedicated sub-table, one per `virt_state_t` (`table_ref`), at its own
 *     slot within it (`ref`), which is what keeps it out of Lua's reach for collection.
 *   - `create()` makes an empty shell with nothing captured; a fresh object holds no value.
 *   - `capture_ref(L)` replaces the held value with whatever is on top of `L`'s stack, releasing
 *     what was held before.
 *   - `capture(oth)`, the Lua-visible "capture", takes its own independent reference to `oth`'s
 *     value rather than aliasing `oth`'s registry slot, since `oth` may be a reference someone
 *     else still holds.
 *   - `push(L)` pushes the captured value back onto `L`, and `call(L, nargs)` or `call<R>(args...)`
 *     invokes it as a function.
 *
 * Detail:
 *   - `capture_lua_object(L, ref, idx)` duplicates the value at stack index `idx` into an existing
 *     `ref`.
 *   - `push` is registered as a raw `lua_CFunction` rather than through
 *     @ref VC_REGISTER_MEMBER_FUNCTION, so that it always receives the real calling `L` instead of
 *     `this->L`. That is what keeps it correct when it is called from a coroutine running on a
 *     different thread than the one the value was captured on.
 *
 * @warning `push(L)` and `call(L, nargs)` raise Lua errors through `luaw_push_error()` on failure,
 *       so they are only safe where a `lua_pcall` is already active further up the call stack: as
 *       a registered `lua_CFunction`, or inside `call_lua()`.
 *
 * @date 2026-09-08 06:54
 */
struct lua_object_t : public vc::object_t {
    lua_State *L = nullptr;
    int table_ref = LUA_NOREF; /* which dedicated sub-table (one per virt_state_t) */
    int ref = LUA_NOREF;       /* this value's slot within that sub-table */

    lua_object_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~lua_object_t();

    static vc::ref_t<lua_object_t> create() {
        return std::make_shared<lua_object_t>(vc::object_t::Private{type_id_static()});
    }

    virtual vc::object_type_e type_id() const override { return VC_TYPE_LUA_OBJECT; }
    static vc::object_type_e type_id_static() { return VC_TYPE_LUA_OBJECT; }

    inline std::string to_string() const override {
        return std::format("vc::lua_object_t[{}]: table_ref={} ref={}",
                (void*)this, table_ref, ref);
    }

    /*! Replaces the captured value with whatever is on top of `L`'s stack, releasing whatever was
     * held before. @date 2026-09-08 06:54 */
    void capture_ref(lua_State *L);

    /*! Lua-visible "capture" (`self:capture(x)`). Takes its own independent reference to `oth`'s
     * value rather than mutating `oth`'s registry slot, since `oth` may be a reference someone
     * else still holds. A missing or nil argument is a release. @date 2026-09-08 06:54 */
    void capture(vc::ref_t<lua_object_t> oth);

    /*!
     * @brief Pushes the captured value back onto `L`.
     *
     * Core:
     *   - The ambient `L` is given explicitly, so the push stays correct when it happens on a
     *     different thread - a coroutine, say - than the one this value was captured on.
     *
     * Detail:
     *   - This is also the Lua-visible "push", registered as a raw `lua_CFunction` for exactly
     *     that reason: the member-function macro has no way to supply the real calling `L`.
     *
     * @date 2026-09-08 06:54
     */
    void push(lua_State *L);

    /*! Raw primitive: `nargs` argument values are already on `L` - pushes the captured callee
     * below them and pcalls with `LUA_MULTRET`. @date 2026-09-08 06:54 */
    int call(lua_State *L, int nargs);

    /*! `call_lua<R>()`-shaped convenience: typed C++ args in, typed C++ result out, paired with an
     * @ref err_e. @date 2026-09-08 06:54 */
    template <typename R, typename ...Args>
    std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
    call(Args&& ...args);

    /*! Unrefs whatever is currently held, if anything. @date 2026-09-08 06:54 */
    void release();

    /*! Duplicates the value at stack index `idx` and captures it into an existing `ref`, which
     * must already have been made by `create()`. @date 2026-09-08 06:54 */
    static void capture_lua_object(lua_State *L, vc::ref_t<lua_object_t> ref, int idx);
};

/*!
 * @brief Human-readable description of an `object_type_e` value: its registered name.
 *
 * Core:
 *   - The name is the stringified `#type` that `VIRT_COMPOSER_REGISTER_TYPE` recorded, e.g.
 *     `"VC_TYPE_INTEGER"`.
 *
 * @date 2026-09-08 06:54
 */
inline std::string to_string(object_type_e type);

/* To string for own objects: */

/*!
 * @brief Human-readable description of a virt_composer object reference.
 *
 * Core:
 *   - `"ref: "` followed by the object's own `to_string()`, which is pure virtual on `object_t`,
 *     so every concrete type supplies its own.
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
inline std::string to_string(ref_t<T> ref);

/*!
 * @brief Human-readable description of a virt_composer object: the same as calling
 * `ref.to_string()` directly.
 *
 * Core:
 *   - A convenience overload for when a plain reference is at hand rather than a `ref_t<T>`, such
 *     as inside a member function of the object itself (`*this`), or after dereferencing one.
 *
 * @date 2026-09-08 06:54
 */
inline std::string to_string(const object_t& ref);

/*!
 * @brief Creates a new virt-state: one Lua state, one object pool, one set of registrations.
 *
 * Core:
 *   - Failure is reported as `nullptr`, not as an exception.
 *
 * @return A shared pointer to the newly created @c virt_state_t object, or `nullptr` if this
 *         translation unit never included `virt_composer_end.h` - so `VIRT_TYPE_CNT` was never
 *         finalized - or if the underlying Lua state failed to initialize.
 *
 * @date 2026-09-08 06:54
 */
std::shared_ptr<virt_state_t> create_state();

/*!
 * @brief Loads a plugin into a state, giving it a private range of type ids to register into.
 *
 * Core:
 *   - Enlarges the state to fit the plugin's types, hands the plugin the offset of that range and
 *     lets it register. Afterwards its types are constructible from YAML and usable from Lua like
 *     any other.
 *   - The plugin must export `plugin_get_version`, `plugin_type_cnt` and `plugin_register_meta`,
 *     answering its own VIRT_COMPOSER_ABI and never the host's, which would agree with the host
 *     whatever the plugin was built against. It must
 *     have been built against this same virt_composer. A plugin built against another is refused.
 *   - A plugin that has taken a range keeps it for the life of the process, and every state that
 *     loads it afterwards places its types at that same range. So a plugin may serve any number of
 *     states, and its types carry one id throughout.
 *   - Asking a state for a plugin it already has, by any path that resolves to the same file, does
 *     nothing and answers success. Anything that fails before the range is taken may be corrected
 *     and asked for again; a plugin that took a range and then failed to register is not asked
 *     again at all.
 *   - A plugin does not load a plugin. Only whoever owns the state calls this, and a plugin that
 *     called it would be reaching for bookkeeping that belongs to the host - which on a platform
 *     where a plugin carries its own copy of this library is its own, and would hand out ranges
 *     from a counter the host knows nothing about.
 *   - `plugin_register_meta` therefore runs once per state, not once per process. A plugin that
 *     does something there which is not about the state it is handed will find it happening again
 *     for the next one.
 *   - A state pays for the ranges it skips. Loading only the second of two plugins still grows the
 *     state past the first's range, leaving rows nothing will ever index.
 *
 * @warning A plugin's types do not inherit from its host's. A plugin type may derive from a host
 *       type in C++, but the base's members are not carried over to it, and nothing says so at the
 *       point it fails - the member is simply absent in Lua.
 * @warning A plugin must not keep a `vc::ref_t` in static or global storage. It outlives the state
 *       it came from, and releasing it at process exit reaches through a dead `lua_State`.
 *
 * @param vs    The state to load into.
 * @param path  The plugin's path, in any spelling that resolves to the file.
 *
 * @return 0 when the plugin is loaded and registered, -1 otherwise.
 *
 * @date 2026-09-20 08:35
 */
int load_plugin(virt_state_t *vs, const char *path);

/*! [INTERNAL] Answers the internal-function table the given state binds its `[INTERNAL]` names
 * from. It exists because virt_state_t is only declared in this header, so the inline bodies below
 * cannot reach into one themselves. @date 2026-09-20 19:10 */
std::map<std::string, std::function<int(lua_State *L)>> *state_internal_funcs(virt_state_t *vs);

/*!
 * @brief Finds a previously-named object by name and casts it to the requested type.
 *
 * Core:
 *   - `name` is looked up in the virt-state's name table, the one `mark_dependency_solved()`
 *     populates: every top-level YAML entry, plus anything explicitly named through
 *     `vc.create_object(name, ...)`.
 *   - The result is cast with `object_t::to_related<T>()`, so a name that exists but holds another
 *     type is an error rather than an empty result.
 *
 * @tparam T  The expected concrete type of the named object.
 * @param vs    Pointer to the virtual state (`virt_state_t`).
 * @param name  The name the object was registered under.
 *
 * @return A `ref_t<T>` to the object, or `nullptr` if no object is registered under `name`.
 *
 * @throws std::runtime_error (via `to_related<T>()`) if an object *is* found under `name` but
 *         isn't actually a `T` (or derived from it).
 *
 * @see get_ref_base, which hands back the untyped `ref_t<object_t>` and does not cast.
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
ref_t<T> get_ref(virt_state_t *vs, const std::string& name);

/*!
 * @brief Converts a YAML or Lua node into an enum/bitmask value of type `T`, given an explicit
 * string-name to value table.
 *
 * Core:
 *   - Handles three node shapes: a string (e.g. `"READ"`), looked up in `enum_vals`; an integer,
 *     cast directly through `T(node.as_int())`; and a sequence (e.g. `[READ, WRITE]`), whose
 *     elements are resolved recursively and OR'd together for combining bitmask flags.
 *   - This is what the single-argument `get_enum_val<T>(node)` overload below is meant to be
 *     implemented in terms of. Write the `enum_vals` table once per enum type and forward to this
 *     function rather than reimplementing the three shapes.
 *
 * @tparam T  The enum/bitmask type to produce.
 * @param node       The YAML/Lua-derived node to convert.
 * @param enum_vals  Table mapping each valid string name to its `T` value.
 *
 * @throws vc::except_t if `node` is a string not present in `enum_vals`, or is not a string,
 *         integer or sequence at all.
 *
 * @see get_enum_val(fkyaml::node&), bm_t
 *
 * @example
 * // The pattern every enum specialization in vulkan/vulkan_composer.h uses:
 * inline std::unordered_map<std::string, VkImageTiling> vk_image_tiling_from_str = { ... };
 * template <> inline VkImageTiling get_enum_val<VkImageTiling>(fkyaml::node &n) {
 *     return get_enum_val(n, vk_image_tiling_from_str);
 * }
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
inline T get_enum_val(fkyaml::node &node, const std::unordered_map<std::string, T>& enum_vals);

/*!
 * @brief Per-enum-type entry point for enum/bitmask conversion: deleted by default, and meant to
 * be specialized once per enum type.
 *
 * Core:
 *   - Specialize it for a type `T` to make `T` usable as `bm_t<T>` in
 *     @ref VC_REGISTER_MEMBER_FUNCTION or `luaw_function_wrapper`, and as a plain YAML/Lua
 *     convertible enum value. The specialization should forward to
 *     `get_enum_val<T>(node, enum_vals)` with its own string-name table.
 *   - The primary template is deliberately `= delete`d and is never meant to be called for an
 *     unspecialized `T`.
 *
 * Detail:
 *   - The deletion is what `is_vc_enum<T>` rests on. That is a `requires` expression checking
 *     whether `get_enum_val<T>(n)` is well-formed; for a `T` with no specialization it resolves to
 *     this deleted primary, and calling a deleted function is ill-formed. The `requires`
 *     expression is SFINAE-friendly about that, so it evaluates to `false` and reports "not a
 *     known enum" instead of breaking compilation wherever it is checked.
 *
 * @tparam T  The enum/bitmask type. Must have its own explicit specialization to be usable.
 *
 * @see get_enum_val(fkyaml::node&, const std::unordered_map<std::string,T>&), is_vc_enum, bm_t
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
inline T get_enum_val(fkyaml::node &n);

/*! The names of @ref err_e, so an error can be written as one in a config and compared against
 * one from Lua. @date 2026-09-22 06:50 */
extern inline std::unordered_map<std::string, err_e> err_e_from_str;
template <> inline err_e get_enum_val<err_e>(fkyaml::node &n);

/* Virt Composer - YAML Parser API
------------------------------------------------------------------------------------------------- */

/*!
 * @brief Parses a YAML configuration file and builds the objects it describes into the given
 * virtual state.
 *
 * Core:
 *   - The objects constructed from the file are stored in the `virt_state_t`, where they can be
 *     retrieved by name from C++ and referenced from Lua scripts.
 *   - An unresolved `!ref`, naming an object that never gets built, is not a failure. It is left
 *     unresolved silently, so a successful return does not mean every reference was satisfied.
 *
 * @param vs    Pointer to the virtual state structure to be populated with the parsed
 *              configuration.
 * @param path  Path to the YAML configuration file to parse.
 *
 * @return virt_composer::err_e
 *         - @c VC_ERROR_OK on success.
 *         - @c VC_ERROR_PARSE_YAML if the YAML file itself is malformed (fails to deserialize).
 *         - @c VC_ERROR_GENERIC for any other failure - schema construction failing, or an object
 *           with an unrecognized `m_type`.
 *
 * @date 2026-09-08 06:54
 */
err_e parse_config(virt_state_t *vs, const char *path);

/*!
 * @brief Registers a builder callback for typed objects, keyed by `m_type`.
 *
 * Core:
 *   - The callback is invoked when a YAML node carrying a matching `m_type` field is met during
 *     parsing, and is responsible for constructing the object from that node.
 *   - Only typed objects, the ones with an `m_type` field, can be nested. Auto-identified objects
 *     cannot, so a structure that must appear inside another has to be typed.
 *
 * @param vs      Pointer to the virtual state.
 * @param match   The `m_type` string to match against YAML nodes.
 * @param builder The callback coroutine to invoke when a match is found. Parameters: virtual
 *                state, node name, and the YAML node itself.
 *
 * @return `VC_ERROR_OK`; this function currently has no failure path.
 *
 * @date 2026-09-08 06:54
 */
err_e add_named_builder_callback(virt_state_t *vs, const std::string& match,
        std::function<co::task<vc::ref_t<vc::object_t>> (
                virt_state_t *, const std::string&, fkyaml::node&)> builder);

/*!
 * @brief Registers a builder callback for auto-identified objects, keyed by node structure
 * instead of by `m_type`.
 *
 * Core:
 *   - The pair is invoked when a YAML node matches what the analyser recognises: the analyser
 *     returns `true` for a node it knows, and the builder is then called to construct the object.
 *   - Only typed objects, the ones with an `m_type` field, can be nested. Objects identified this
 *     way cannot, so anything that must appear inside another node has to be typed instead.
 *
 * @param vs       Pointer to the virtual state.
 * @param analyser Function that checks whether a YAML node matches the expected structure.
 * @param builder  Coroutine that constructs the object once the analyser has returned `true`.
 *                 Parameters: virtual state, node name, and the YAML node itself. Returns `0` on
 *                 success, or a negative value on error.
 *
 * @return `VC_ERROR_OK`; this function currently has no failure path.
 *
 * @date 2026-09-08 06:54
 */
err_e add_auto_builder_callback(virt_state_t *vs,
        std::function<bool(const std::string&, fkyaml::node& node)> analyser,
        std::function<co::task_t(virt_state_t *, const std::string&, fkyaml::node&)> builder);

/*!
 * @brief Marks a dependency as resolved and wakes everything waiting on it. Called from inside a
 * builder callback.
 *
 * Core:
 *   - Registers the newly constructed object in the virtual state under `depend_name`, which is
 *     what later makes it findable through `get_ref()`.
 *   - Exposes it to Lua as `vc.<depend_name>`.
 *   - Resumes any coroutines that suspended while waiting for this dependency, which is how the
 *     parser's ordering is resolved at all.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`), which manages objects and
 *                      dependencies.
 * @param depend_name   The name/identifier of the dependency being resolved.
 * @param depend        The object reference (`vc::ref_t<vc::object_t>`) to register.
 *
 * @throws vc::except_t if the object is null, or if the dependency name is already taken.
 *
 * @example
 * // After constructing an object, mark it as resolved:
 * mark_dependency_solved(vs, "my_object", my_object_ref);
 *
 * @date 2026-09-08 06:54
 */
void mark_dependency_solved(virt_state_t *vs, std::string depend_name, vc::ref_t<vc::object_t> dep);

/*!
 * @brief Resolves a YAML node to an `int64_t`, following references and evaluating expressions.
 * To be used inside the build_object callback.
 *
 * Core:
 *   - Handles three shapes: a reference node (`!ref object_name`), which resolves the referenced
 *     integer object; a string node, evaluated as a mathematical expression through `texpr`; and a
 *     direct integer node, returned as is.
 *   - Being a coroutine, it suspends until a referenced object exists, so a builder need not care
 *     about the order things are declared in YAML.
 *
 * Detail:
 *   - The result of an evaluated expression is rounded to the nearest integer.
 *
 * @param vs    Pointer to the virtual state (`virt_state_t`), providing parsing context and
 *              dependency management.
 * @param node  The YAML node to resolve. Can be a reference, a string expression, or a direct
 *              integer.
 *
 * @return A coroutine task that yields the resolved `int64_t` value.
 *
 * @example
 * // Resolve a reference or expression:
 * int64_t val = co_await resolve_int(vs, yaml_node);
 *
 * @date 2026-09-08 06:54
 */
co::task<int64_t> resolve_int(virt_state_t *vs, fkyaml::node& node);

/*!
 * @brief Resolves a YAML node to a `double`, following references and evaluating expressions.
 * To be used inside the build_object callback.
 *
 * Core:
 *   - Handles four shapes: a reference node (`!ref object_name`), which resolves the referenced
 *     float object; a string node, evaluated as a mathematical expression through `texpr`; an
 *     integer node, cast directly to `double`; and a direct float node, returned as is.
 *   - Being a coroutine, it suspends until a referenced object exists, so a builder need not care
 *     about the order things are declared in YAML.
 *
 * @param vs   Pointer to the virtual state (`virt_state_t`), providing parsing context and
 *             dependency management.
 * @param node The YAML node to resolve. Can be a reference, a string expression, an integer, or a
 *             direct float.
 *
 * @return A coroutine task that yields the resolved `double` value.
 *
 * @example
 * // Resolve a reference or direct float:
 * double val = co_await resolve_float(vs, yaml_node);
 *
 * @date 2026-09-08 06:54
 */
co::task<double> resolve_float(virt_state_t *vs, fkyaml::node& node);

/*!
 * @brief Resolves a YAML node to a `std::string`, following a reference if that is what it holds.
 * To be used inside the build_object callback.
 *
 * Core:
 *   - Handles two shapes: a reference node (`!ref object_name`), which resolves the referenced
 *     string object, and a direct string node, whose value is returned as is.
 *   - Being a coroutine, it suspends until a referenced object exists, so a builder need not care
 *     about the order things are declared in YAML.
 *   - Unlike @ref resolve_int and @ref resolve_float, a string is never treated as an expression
 *     here; it is taken literally.
 *
 * @param vs   Pointer to the virtual state (`virt_state_t`), providing parsing context and
 *             dependency management.
 * @param node The YAML node to resolve. Can be a reference or a direct string.
 *
 * @return A coroutine task that yields the resolved `std::string` value.
 *
 * @example
 * // Resolve a reference or direct string:
 * std::string val = co_await resolve_str(vs, yaml_node);
 *
 * @date 2026-09-08 06:54
 */
co::task<std::string> resolve_str(virt_state_t *vs, fkyaml::node& node);

/*!
 * @brief Resolves a YAML node into a strongly-typed object reference, whether it names one,
 * inlines one, or tags one. To be used inside the build_object callback.
 *
 * Core:
 *   - Supports three shapes: a reference node (`m_field: !ref object_name`), a tagged mapping
 *     node (`m_field: tag_name: m_type: "..."`), and an inlined object node
 *     (`m_field: m_type: "..."`).
 *   - Being a coroutine, it suspends if the object is not yet available and resumes once it is,
 *     so a builder need not care about the order things are declared in YAML.
 *
 * @tparam T The expected type of the resolved object.
 * @param vs   Pointer to the virtual state (`virt_state_t`), providing parsing context and
 *             dependency management.
 * @param node The YAML node to resolve: a reference, a tagged mapping, or an inlined object.
 *
 * @return A coroutine task that yields a `vc::ref_t<T>` to the resolved object.
 *
 * @throws vc::except_t if the node format is invalid or unsupported in the current context.
 *
 * @example
 * // Resolve a reference:
 * auto ref = co_await resolve_obj<my_type_t>(vs, yaml_node);
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
co::task<vc::ref_t<T>> resolve_obj(virt_state_t *vs, fkyaml::node& node);

/*!
 * @brief Reads a trivially-copyable value out of an already-built object's member, by raw memcpy.
 * To be used inside a builder callback, on a YAML node tagged `!copy`.
 *
 * Core:
 *   - The node must be tagged `!copy` and carry `object` and `member` string fields naming the
 *     source object and the member to read, e.g. `value: !copy\n  object: some_vec3\n  member: x`.
 *   - The member must have been registered ahead of time on the *source* object's type, with
 *     @ref VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER or `register_trivially_copyable_member`.
 *   - The registration is looked up by the source object's runtime `type_id()`, not by `T`, so `T`
 *     must match the exact type the member was registered with. The check is a runtime
 *     `std::type_index` comparison, not a compile-time one.
 *
 * Detail:
 *   - The coroutine engine is used here only to allow objects to be declared in whatever order is
 *     convenient in YAML, never to wait on real I/O or an external event. If the source object has
 *     not been built yet, construction of the *current* object pauses, other objects keep being
 *     built, and this resumes once the source becomes available.
 *
 * @tparam T  The C++ type to copy the member's bytes into. Must match the registered member's
 *            type.
 * @param vs    Pointer to the virtual state (`virt_state_t`).
 * @param node  The YAML node. Must be tagged `!copy` and contain `object`/`member` string fields.
 *
 * @return A coroutine task that yields the copied `T` value.
 *
 * @throws vc::except_t if `node` is not tagged `!copy`, if the source object's type never
 *         registered `member` for trivial-copy access, or if the registered member's type does not
 *         match `T`.
 *
 * @see VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER
 *
 * @example
 * // yaml:
 * // some_float:
 * //   m_type: my_float_copy_t
 * //   value: !copy
 * //     object: some_vec3
 * //     member: x
 *
 * // inside a builder callback for my_float_copy_t:
 * float val = co_await resolve_memb<float>(vs, node["value"]);
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
co::task<T> resolve_memb(virt_state_t *vs, fkyaml::node& node);

/* Virt Composer - LUA API
------------------------------------------------------------------------------------------------- */

/*!
 * @brief Adds free (non-member) functions to the `vc` Lua module table, callable as
 * `vc.<name>(...)` from any script.
 *
 * Core:
 *   - These sit on the `vc` table with no receiver, unlike @ref VC_REGISTER_MEMBER_FUNCTION, which
 *     registers a function ON a type (`obj:fn(...)`).
 *   - A function added here is callable the moment this returns and no config mentions it, where
 *     one given to `add_internal_func()` stays a name in a registry until a yaml node binds it as
 *     an object. Reach for this one unless the config must hold the function itself.
 *   - Safe to call more than once; each call appends to what was registered before.
 *
 * @param vs           Pointer to the virtual state (`virt_state_t`).
 * @param vc_tab_funcs The functions to add, as `{name, lua_CFunction}` pairs (`luaL_Reg`). For a
 *                     C++ function whose arguments and result convert themselves, wrap it with
 *                     `luaw_function_wrapper<...>` rather than writing a raw one.
 *
 * @return `VC_ERROR_OK`; this function currently has no failure path.
 *
 * @code
 * add_lua_tab_funcs(vs, {{"my_func", luaw_function_wrapper<&my_free_function, int, int>}});
 *
 * // Raw, for what the wrapper cannot describe - here answering an object. push_vc_object()
 * // leaves it on the stack and answers an error code, so the result count is ours to give.
 * static int make_point(lua_State *L) {
 *     auto obj = point_t::create(lua_tointeger(L, 1), lua_tointeger(L, 2));
 *     vc::push_vc_object(L, obj->to_related<vc::object_t>());
 *     return 1;
 * }
 * add_lua_tab_funcs(vs, {{"make_point", make_point}});   // Lua: vc.make_point(3, 4)
 * @endcode
 *
 * @date 2026-09-20 18:40
 */
err_e add_lua_tab_funcs(virt_state_t *vs, const std::vector<luaL_Reg>& vc_tab_funcs);

/*!
 * @brief Adds integer constants directly onto the `vc` Lua module table, e.g. `vc.READ = 1`.
 *
 * Core:
 *   - Without this, a name like `vc.READ` simply does not exist as a field on the `vc` table.
 *   - This is one of two independent ways a script can hand an enum or flag value to a `bm_t<T>`
 *     parameter: as a named integer constant registered here, which Lua evaluates before the call
 *     happens, or as a bare string literal (`"READ"`), resolved separately through a
 *     `get_enum_val<T>` specialization.
 *
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param mapping The constants to add, as `{integer_value, name}` pairs.
 *
 * @return `VC_ERROR_OK`; this function currently has no failure path.
 *
 * @see get_enum_val, bm_t. The `unordered_map<std::string, T>` overload just below forwards here.
 *
 * @example
 * add_lua_flag_mapping(vs, {{1, "READ"}, {2, "WRITE"}});
 * // Lua: vc.READ == 1, vc.WRITE == 2
 *
 * @date 2026-09-08 06:54
 */
err_e add_lua_flag_mapping(virt_state_t *vs,
        const std::vector<std::pair<lua_Integer, std::string>> &mapping);

/*!
 * @brief Adds integer constants onto the `vc` Lua module table from an existing enum name table.
 *
 * Core:
 *   - A convenience wrapper: `mapping` is converted into the `{value, name}` pair form and
 *     forwarded to the `vector<pair<lua_Integer,string>>` overload.
 *   - The point is to reuse the *same* table already written for a `get_enum_val<T>`
 *     specialization, so one `std::unordered_map<std::string, T>` backs both the C++ side
 *     string-to-enum lookup and the Lua side `vc.<NAME>` constants, instead of two lists that have
 *     to be kept in step.
 *
 * Detail:
 *   - That is the actual pattern in `vulkan_composer.h`, where `shader_stage_from_string` backs
 *     both `get_enum_val<vku_shader_stage_e>`'s specialization and this function's registration.
 *
 * @tparam T  The enum/flag type. Only its integer values matter here; they are cast to
 *            `lua_Integer`.
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param mapping The name to value table to expose as `vc.<name>` constants.
 *
 * @return `VC_ERROR_OK`; this function currently has no failure path.
 *
 * @see add_lua_flag_mapping(virt_state_t*, const std::vector<std::pair<lua_Integer,std::string>>&),
 *      get_enum_val
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
err_e add_lua_flag_mapping(virt_state_t *vs, const std::unordered_map<std::string, T>& mapping);

/*!
 * @brief Wraps a C++ function as a `lua_CFunction`, converting arguments and return values
 * automatically.
 *
 * Core:
 *   - Lua arguments are converted into the declared `Params...` and the result is converted back,
 *     so the wrapped function is written in plain C++ terms.
 *   - `vc::ref_t<T>` references are recognised, so composer objects pass through in either
 *     direction.
 *   - Exceptions thrown by `function` are caught and turned into a Lua error, so no try/catch is
 *     needed around it.
 *
 * @tparam function The C++ function to wrap. Must be callable with the provided `Params...`.
 * @tparam Params   The types of the parameters expected by the wrapped function.
 *
 * @param L The Lua state.
 *
 * @return The number of values returned to Lua: 0 for void, 1 otherwise.
 *
 * @date 2026-09-08 06:54
 */
template <auto function, typename ...Params>
inline int luaw_function_wrapper(lua_State *L);

/*!
 * @brief Registers a member function of a C++ class so Lua scripts can call it on objects of that
 * type.
 *
 * Core:
 *   - `T` must inherit from `virt_composer::object_t`. The function becomes reachable as
 *     `obj:function_name(...)`.
 *   - Only types the library knows how to carry across the Lua boundary may appear in `Params`:
 *     string, bool, int and double, vector, tuple and pair, `vc::ref_t<T>` objects, and
 *     `vc::bm_t<T>` (Lua to C++ only - see its own doc). For anything else the library has no
 *     conversion in either direction.
 *   - This is the only route onto the per-class `__index` dispatch. A function attached by other
 *     means, such as a bare `lua_setfield()` on the shared metatable, is not reachable as a
 *     method.
 *
 * Detail:
 *   - Normally reached through the @ref VC_REGISTER_MEMBER_FUNCTION macro rather than called
 *     directly.
 *
 * @tparam T            The C++ class type (must inherit from `virt_composer::object_t`).
 * @tparam member_ptr   Pointer to the member function to register.
 * @tparam Params       Parameter types of the member function.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`)
 * @param function_name The name of the function as it will be exposed in Lua.
 *
 * @see VC_REGISTER_MEMBER_FUNCTION
 *
 * @date 2026-09-08 06:54
 */
template <typename T, auto member_ptr, typename ...Params>
void luaw_register_member_function(virt_state_t *vs, const char *function_name);

/*!
 * @brief Registers a member variable of a C++ class so Lua scripts can read and write it.
 *
 * Core:
 *   - `T` must inherit from `virt_composer::object_t`. The variable becomes reachable as
 *     `obj.member_name`, for both reading and assignment.
 *   - Only types the library knows how to carry across the Lua boundary may be registered: string,
 *     bool, int and double, vector, tuple and pair, `vc::ref_t<T>` objects, and `vc::bm_t<T>`
 *     (Lua to C++ only - see its own doc). For anything else the library has no conversion in
 *     either direction.
 *
 * Detail:
 *   - Normally reached through the @ref VC_REGISTER_MEMBER_OBJECT macro rather than called
 *     directly.
 *
 * @tparam T            The C++ class type (must inherit from `virt_composer::object_t`).
 * @tparam member_ptr   Pointer to the member variable to register.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`)
 * @param member_name   The name of the member variable as it will be exposed in Lua.
 *
 * @see VC_REGISTER_MEMBER_OBJECT
 *
 * @example
 * // C++:
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_cmdpool)
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_host_free)
 *
 * struct cmdbuff_t : public vc::object_t {
 *     ref_t<cmdpool_t>    m_cmdpool;
 *     bool                m_host_free;
 * };
 *
 * // Lua:
 * vc = require("virt_composer")
 * vc.cmdbuff.m_host_free = false  -- Sets the member variable
 * vc.cmdbuff.m_cmdpool:do_something()  -- Accesses the member object
 *
 * @date 2026-09-08 06:54
 */
template <typename T, auto member_ptr>
void luaw_register_member_object(virt_state_t *vs, const char *member_name);

/*!
 * @brief Tells the framework that one registered object type is a base of another, so members
 * registered on the base become visible on the derived type too.
 *
 * Core:
 *   - By default every registered type is its own island for Lua and YAML member access: a member
 *     registered on `T` is only reachable through an object whose `type_id()` is exactly `T`.
 *     This links two of them.
 *   - The order of `T` and `U` does not matter. Whichever is the real C++ base is detected
 *     automatically, and its members are exposed on the derived type.
 *   - It mirrors an inheritance that must already exist in C++: `std::is_base_of_v<T, U>` or
 *     `std::is_base_of_v<U, T>` is checked at compile time. It cannot invent a relationship
 *     between unrelated types.
 *   - Call order matters against member registration, not only against other
 *     `register_inheritance` calls. Propagation happens once, when a member or operator is
 *     registered, so a member added to the base *before* this call will never reach the derived
 *     type. Register the pair first, then the members.
 *   - It is not transitive. For `A <- B <- C`, linking `A,B` and `B,C` does not make `A`'s members
 *     visible on `C`. Every pair that must be visible has to be registered, `A,C` included.
 *
 * Detail:
 *   - "Propagation happens once" means `set_lua_class_member()`, `set_class_member_setter()`,
 *     `set_trivial_copy_member()` and `set_class_operator()` each copy the registration into every
 *     type currently known to descend from the one being registered on. There is no live or lazy
 *     link to re-evaluate later.
 *   - `std::is_base_of_v` holds for any ancestor/descendant pair however many levels apart, so the
 *     type constraint never stands in the way of registering the non-adjacent pairs.
 *
 * @tparam T One of the two related types (must inherit from `virt_composer::object_t`).
 * @tparam U The other related type (must inherit from `virt_composer::object_t`).
 *
 * @param vs Pointer to the virtual state (`virt_state_t`).
 *
 * @see VC_REGISTER_MEMBER_FUNCTION, VC_REGISTER_MEMBER_OBJECT,
 *      VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER, set_class_operator
 *
 * @example
 * // C++:
 * struct base_t : public vc::object_t { int64_t base_val; ... };
 * struct derived_t : public base_t { int64_t derived_val; ... };
 *
 * register_inheritance<base_t, derived_t>(vs);   // must come before the next line
 * VC_REGISTER_MEMBER_OBJECT(vs, base_t, base_val);
 *
 * // Lua: an object whose real type is derived_t can now read base_val too, even though it was
 * // only ever registered against base_t:
 * vc.some_derived_instance.base_val
 *
 * @date 2026-09-08 06:54
 */
template <typename T, typename U>
requires std::is_base_of_v<vc::object_t, T> && std::is_base_of_v<vc::object_t, U>
void register_inheritance(virt_state_t *vs);

/*!
 * @brief Registers `fn` as the handler for a Lua operator on objects of a given type, e.g.
 * `VC_OPERATOR_ADD` for `a + b`.
 *
 * Core:
 *   - `fn` is a plain `lua_CFunction` and is not wrapped or generated the way
 *     @ref VC_REGISTER_MEMBER_FUNCTION wraps a C++ member function pointer. The two operands of a
 *     Lua operator can be any mix of vc objects and plain Lua values (`vc_obj + 5`, or two
 *     different vc types), so there is no single fixed C++ signature to template over; `fn` gets
 *     the raw stack and decides for itself, through `get_object_from_lua`, `lua_tonumber` and the
 *     like.
 *   - For a binary operator, both operands are checked in order for a registered handler, and `fn`
 *     is called with the stack `[operand1, operand2, which]`. `which` is `1` or `2`, saying which
 *     side carried the handler: Lua always passes the operands in left-to-right order either way,
 *     so a non-commutative operator such as SUB needs this to know which side dispatched.
 *   - For a unary operator there is no ambiguity and no `which` is pushed; `fn` is called with the
 *     stack `[operand1]`.
 *   - Whatever `fn` returns is passed back unmodified, following the normal `lua_CFunction`
 *     contract: push results, return their count. No manual stack cleanup is needed.
 *   - If neither operand has a handler for `op`, a Lua error is raised.
 *   - As with member registration, an operator registered on `type` also reaches every type
 *     already linked to it through `register_inheritance()`. Link the pair first, or a derived
 *     type registered afterwards will not pick the operator up.
 *
 * Detail:
 *   - The binary operators are ADD, SUB, MUL, DIV, MOD, POW, IDIV, BAND, BOR, BXOR, SHL, SHR,
 *     CONCAT, EQ, LT and LE. The unary ones are UNM, BNOT and LEN.
 *
 * @param vs   Virtual state context.
 * @param type The enumerated type of the C++ class (must be registered with
 *             @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param op   Which operator slot to bind (see @ref operator_e).
 * @param fn   Raw Lua C function implementing the operator for this type.
 *
 * @see operator_e, register_inheritance
 *
 * @date 2026-09-08 06:54
 */
void set_class_operator(virt_state_t *vs, object_type_e type, operator_e op, lua_CFunction fn);

/* TODO: add the functions to add the exception callbacks */

/*!
 * @brief Pushes a virt_composer object onto the Lua stack, as a value Lua's garbage collector
 * actually tracks.
 *
 * Core:
 *   - Repeated pushes of the same object return the same Lua value, so `==` between them holds in
 *     Lua.
 *   - The object is kept alive for exactly as long as Lua can still reach that value. Once nothing
 *     references it any more it is eligible for collection like any other Lua-owned object.
 *
 * @param L      Lua state.
 * @param object The virt_composer object to push.
 *
 * @return `0` always; this function currently has no failure path.
 *
 * @see get_object_from_lua, the inverse.
 *
 * @date 2026-09-08 06:54
 */
int push_vc_object(lua_State *L, ref_t<object_t> object);

/*!
 * @brief Retrieves the `vc::object_t*` a Lua stack value represents, or `nullptr` if it is not
 * one.
 *
 * Core:
 *   - The inverse of @ref push_vc_object: given a stack index, it hands back the underlying
 *     `object_t*` if the value there is a virt_composer object, and `nullptr` for anything else.
 *   - A non-object is not an error here, so a raw `lua_CFunction` - an operator handler registered
 *     through `set_class_operator()`, say - can use it to inspect arguments of mixed kinds.
 *
 * @param L    The Lua state.
 * @param idx  Stack index of the value to inspect.
 *
 * @return The object's `object_t*`, or `nullptr` if the value at `idx` is not a virt_composer
 *         object.
 *
 * @see push_vc_object
 *
 * @date 2026-09-08 06:54
 */
object_t *get_object_from_lua(lua_State *L, int idx);

/*!
 * @brief Calls a global Lua function by name and converts its result back to C++.
 *
 * Core:
 *   - `function_name` is looked up as a **global**, through `lua_getglobal`. A function nested in
 *     a table (`vc.foo`) or local to a script is not found this way, and the call then fails
 *     exactly as an unknown name would.
 *   - Failure is reported in the returned pair, not thrown.
 *
 * @tparam R            Return type to convert the Lua function's result into. Pass `void` if the
 *                      return value should be ignored; the pair's first element is then a
 *                      meaningless placeholder `int`, always `0`, not an actual return value.
 * @tparam Args         The types of the function parameters.
 *
 * @param vs            The virtual state that contains the function.
 * @param function_name The name of the global Lua function to call.
 * @param args...       The arguments to pass, pushed onto the Lua stack in order before the call.
 *
 * @return A pair: the converted return value (or the `void` placeholder above), and `VC_ERROR_OK`
 *         on success, or `VC_ERROR_FAILED_CALL` if `function_name` does not resolve to a callable
 *         value or the call itself errors.
 *
 * @date 2026-09-08 06:54
 */
template <typename R, typename ...Args>
std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
call_lua(virt_state_t *vs, const char *function_name,
        Args&& ...args);

/*! IMPLEMENTATION
 * 
 * 
 * 
 * ==============================================================================================
 * ==============================================================================================
 * ==============================================================================================
 * 
 * 
 * 
 * */

/* [INTERNAL] Both set by virt_composer_end.h, once every VIRT_COMPOSER_REGISTER_TYPE in this
translation unit has run: VIRT_TYPE_CNT becomes the total distinct type count (so every per-type
array in virt_state_t can be sized/indexed safely) and VIRT_TYPES_INITIALIZED flips to true.
create_state() checks VIRT_TYPES_INITIALIZED first and refuses to run if virt_composer_end.h was
never included. */
inline bool VIRT_TYPES_INITIALIZED;
inline size_t VIRT_TYPE_CNT;

/* [INTERNAL] Differentiates between a member function and a member object. */
enum luaw_member_e {
    LUAW_MEMBER_FUNCTION,
    LUAW_MEMBER_OBJECT,
};

/*!
 * [INTERNAL]
 * @brief Builds an object from a YAML node using the registered typed-builder callbacks.
 *
 * Core:
 *   - The node must be a mapping carrying `m_type`; anything else yields `nullptr` rather than an
 *     error.
 *   - `build_object_cbks` is walked for a callback matching that `m_type`, and construction is
 *     delegated to it. The coroutine suspends if the callback needs something not built yet.
 *   - For the parser and dependency-resolution system only.
 *
 * Detail:
 *   - Invalid nodes and unknown types are logged on the way through.
 *
 * @param vs   Virtual state context.
 * @param name Object name, used for registration and debugging.
 * @param node YAML node defining the object (must be a mapping with `m_type`).
 *
 * @return Coroutine task yielding a `vc::ref_t<vc::object_t>`, or `nullptr` if the node is not a
 *         mapping.
 *
 * @throws vc::except_t if no callback matches the object type.
 *
 * @date 2026-09-08 06:54
 */
co::task<vc::ref_t<vc::object_t>> build_object(virt_state_t *vs,
        const std::string& name, fkyaml::node& node);

/*!
 * [INTERNAL]
 * @brief Builds an object from a YAML node that carries no explicit type, by recognising its
 * shape.
 *
 * Core:
 *   - This is what lets a config skip the boilerplate: an integer node becomes an `integer_t`, a
 *     float a `float_t`, a string a `string_t`.
 *   - A node named exactly `"lua_script"` is loaded and executed as a Lua script - the same
 *     underlying mechanism as a `vc::lua_script_t`'s `m_source`/`m_source_path`, without needing
 *     the explicit `m_type` tag.
 *   - Specialized objects, such as SPIR-V shaders or GPU resources, do not come through here.
 *     They go through the registered `build_psudo_object_cbks` callbacks instead.
 *
 * @param vs   Virtual state context.
 * @param name Name of the object to build.
 * @param node YAML node defining the object.
 *
 * @return Coroutine task yielding `0` on success, the object built and registered, or `-1` on
 *         failure: an invalid node, or an unsupported type.
 *
 * @date 2026-09-08 06:54
 */
co::task_t build_pseudo_object(virt_state_t *vs, const std::string& name, fkyaml::node& node);

/*!
 * [INTERNAL]
 * @brief Generates a unique anonymous name for an untagged object.
 *
 * Core:
 *   - Names are of the form `"__<N>"` - `"__0"`, `"__1"`, and so on - where `N` comes from a
 *     counter held per `virt_state_t`.
 *   - The counter is advanced on every call, so this mutates `vs` and never returns the same name
 *     twice for one state.
 *
 * @param vs Virtual state context.
 *
 * @return The generated name.
 *
 * @date 2026-09-08 06:54
 */
std::string new_anon_name(virt_state_t *vs);

/*!
 * [INTERNAL]
 * @brief Raises a Lua error carrying a formatted message and the Lua-side stack trace.
 *
 * Core:
 *   - The Lua call stack is walked to gather context for each frame - source file, line number,
 *     and the line of code itself - and the result is concatenated with `err_str`, pushed, and
 *     raised through `lua_error`.
 *   - `lua_error` does not return, so nothing after a call to this runs.
 *
 * Detail:
 *   - Where the source file cannot be read or the line number is invalid, `"<unknown>"` stands in.
 *
 * @param L       Lua state.
 * @param err_str The message to report.
 * @param sloc    C++ source location of the call site, defaulted.
 *
 * @see lua_Debug, lua_getstack, lua_getinfo, lua_error
 *
 * @date 2026-09-08 06:54
 */
void luaw_push_error(lua_State *L, const std::string& err_str,
        const std::source_location sloc = std::source_location::current());

/*!
 * [INTERNAL]
 * @brief Catches C++ exceptions inside a Lua C function wrapper and turns them into Lua errors.
 *
 * Core:
 *   - Keeps exceptions from escaping into Lua, which would corrupt the Lua state. Meant to be
 *     called from the `catch` of a wrapper's `try`/`catch`.
 *   - `fkyaml::exception`, `vc::except_t` and `std::exception` are converted to Lua errors with
 *     descriptive messages. `vc::except_t` is caught ahead of the generic `std::exception`, so it
 *     is reported as `"Invalid call: <message>"` rather than `"std::exception: <message>"`.
 *   - Anything else is re-thrown, on the assumption that it is already a Lua error in flight.
 *
 * @param L The Lua state.
 *
 * @return Always `0`: the function either raises a Lua error or re-throws.
 *
 * @date 2026-09-08 06:54
 */
int luaw_catch_exception(lua_State *L);

/*!
 * [INTERNAL]
 * @brief Retrieves the `vc::virt_state_t` pointer stored in the Lua registry.
 *
 * Core:
 *   - `L` references `vs` and `vs` references `L`, so either can be recovered from the other; this
 *     is the direction from inside a Lua state.
 *
 * @param L The Lua state.
 *
 * @return Pointer to the `virt_state_t` object stored in the Lua registry.
 *
 * @date 2026-09-08 06:54
 */
virt_state_t *luaw_get_virt_state(lua_State *L);

/*!
 * [INTERNAL]
 * @brief Retrieves the `lua_State` pointer stored in a `vc::virt_state_t`.
 *
 * Core:
 *   - The other direction of @ref luaw_get_virt_state: `L` references `vs` and `vs` references
 *     `L`.
 *
 * @param vs The virt_state_t pointer.
 *
 * @return Pointer to the Lua state object stored in the virtual state.
 *
 * @date 2026-09-08 06:54
 */
lua_State *luaw_get_lua_state(virt_state_t *vs);

/*!
 * @brief Answers the coroutine pool a virt-state runs everything on.
 *
 * Core:
 *   - It is the one pool of the state: a config being parsed, the builders it wakes and any
 *     script that waits all run on it, so anything meant to run beside them is scheduled here.
 *   - It is valid for as long as the state is, and is cleared before the state's Lua state is
 *     closed.
 *
 * @param vs The virt_state_t pointer.
 *
 * @return The state's pool.
 *
 * @date 2026-09-22 03:52
 */
co::pool_p luaw_get_pool(virt_state_t *vs);

/*!
 * [INTERNAL]
 * @brief Registers a type-erased, memcpy-based accessor for a trivially-copyable member.
 *
 * Core:
 *   - The low-level primitive that `register_trivially_copyable_member<T, member_ptr>()` and
 *     @ref VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER are built on.
 *   - `copy_fn` is stored under `type` and `member_name`, to be found later by
 *     `resolve_memb_data()`, which is what `resolve_memb<T>()` and the `!copy` YAML tag use.
 *   - It takes part in the same base to derived propagation as `set_lua_class_member()` and
 *     `set_class_member_setter()`: it writes into every type currently in
 *     `inheritance_table[type]`. The same ordering rule therefore applies -
 *     `register_inheritance()` must have been called first for the member to reach a derived type.
 *
 * @param vs           Virtual state context.
 * @param type         The enumerated type of the C++ class the member belongs to (must be
 *                     registered with @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name  The name of the member, as referenced from a YAML `!copy` tag's `member`
 *                     field.
 * @param tid          `std::type_index` of the member's actual C++ type. It is checked against the
 *                     caller's requested `T` at `resolve_memb<T>()` time, so a mismatch is caught
 *                     rather than silently memcpy'd into a wrong-sized destination.
 * @param copy_fn      Type-erased copy function: given the source object and a destination buffer
 *                     with its size, copies the member's raw bytes into it.
 *
 * @date 2026-09-08 06:54
 */
void set_trivial_copy_member(virt_state_t *vs, object_type_e type, const char *member_name,
        std::type_index tid, std::function<void(vc::object_t *, void *, size_t)> copy_fn);

/*!
 * [INTERNAL]
 * @brief Registers a Lua-accessible member, function or object, for a C++ class type.
 *
 * Core:
 *   - Binds the member to `type` under `member_name`, which is what makes it callable or readable
 *     from a Lua script.
 *   - Propagates to every type currently linked to `type` by `register_inheritance()`, once, at
 *     the moment of this call.
 *
 * @param vs            Virtual state context.
 * @param type          The enumerated type of the C++ class (must be registered with
 *                      @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name   The name of the member as it will be exposed in Lua.
 * @param fn            The Lua C function wrapper for the member.
 * @param member_type   The kind of member (@ref luaw_member_e: function or object).
 *
 * @date 2026-09-08 06:54
 */
void set_lua_class_member(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn, luaw_member_e member_type);

/*!
 * [INTERNAL]
 * @brief Registers the setter for a Lua-accessible member object of a C++ class type.
 *
 * Core:
 *   - Binds the setter to `type` under `member_name`, which is what makes the member assignable
 *     from Lua rather than only readable.
 *   - Propagates to every type currently linked to `type` by `register_inheritance()`, once, at
 *     the moment of this call.
 *
 * @param vs            Virtual state context.
 * @param type          The enumerated type of the C++ class (must be registered with
 *                      @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name   The name of the member as it will be exposed in Lua.
 * @param fn            The Lua C function implementing the assignment.
 *
 * @date 2026-09-08 06:54
 */
void set_class_member_setter(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn);

/*!
 * [INTERNAL]
 * @brief Raw bookkeeping behind `register_inheritance<T,U>()`: records `derived` as inheriting
 * `base`'s registered members.
 *
 * Core:
 *   - No relationship check is performed here, so calling this directly can link two
 *     `object_type_e` values with no real C++ relationship at all.
 *   - `register_inheritance<T,U>()` is what enforces `std::is_base_of_v<T,U>` at compile time
 *     through its `requires` clause, works out which of `T` and `U` is genuinely the base, and
 *     calls this with the two in the right order.
 *   - The behaviour produced is otherwise identical, registration-order and non-transitivity
 *     caveats included. See @ref register_inheritance for those.
 *
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param base    The type whose members should also become visible on `derived`.
 * @param derived The type that should inherit `base`'s members.
 *
 * @date 2026-09-08 06:54
 */
void set_base_derived_relation(virt_state_t *vs, object_type_e base, object_type_e derived);

/*!
 * @brief Finds a previously-named object by name, without casting it to any particular type.
 *
 * Core:
 *   - Uses the same name-table lookup `get_ref<T>()` does, and hands back a plain
 *     `ref_t<object_t>` with no `to_related<T>()` cast applied.
 *   - Because there is no cast, this never throws on a type mismatch the way `get_ref<T>()` can.
 *     Use it when the concrete type is unknown or irrelevant, or when the check and cast should be
 *     done by hand.
 *
 * @param vs    Pointer to the virtual state (`virt_state_t`).
 * @param name  The name the object was registered under.
 *
 * @return A `ref_t<object_t>` to the object, or `nullptr` if no object is registered under `name`.
 *
 * @see get_ref
 *
 * @date 2026-09-08 06:54
 */
ref_t<vc::object_t> get_ref_base(virt_state_t *vs, const std::string& name);

/* See get_ref()'s declaration above for its doc comment. */
template <typename T>
ref_t<T> get_ref(virt_state_t *vs, const std::string& name) {
    auto base = get_ref_base(vs, name);
    return base ? base->to_related<T>() : nullptr;
}

/*!
 * [INTERNAL]
 * @brief Type-erased coroutine behind `resolve_memb<T>()`: does the dependency wait, the type
 * check and the memcpy.
 *
 * Core:
 *   - `resolve_memb<T>()` is a thin wrapper over this. It declares a `T ret;` and calls here with
 *     `&ret, sizeof(T), typeid(T)`.
 *   - This is the entry point for a caller that would rather work with a raw destination buffer,
 *     size and `type_index` than with a template parameter.
 *   - The user-facing behaviour - suspend and resume ordering, registration requirements, error
 *     conditions - is documented on @ref resolve_memb; this is where it is implemented.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`).
 * @param obj_name  Name of the source object to copy the member from. Waits for it to be built if
 *                  it has not been yet.
 * @param memb_name Name of the member to copy, as registered through `set_trivial_copy_member()`.
 * @param dst       Destination buffer to memcpy the member's bytes into.
 * @param sz        Size in bytes of `dst`, and of the copy.
 * @param tid       Expected `std::type_index` of the member. Must match what it was registered
 *                  with, or this throws.
 *
 * @throws vc::except_t if `memb_name` was never registered for `obj_name`'s type, or if `tid` does
 *         not match the registered member's type.
 *
 * @date 2026-09-08 06:54
 */
co::task_t resolve_memb_data(virt_state_t *vs, const std::string &obj_name,
        const std::string& memb_name, void *dst, size_t sz, std::type_index tid);

/*!
 * [INTERNAL]
 * @brief Non-templated core of the dependency resolver.
 *
 * Core:
 *   - Holds the low-level operations the parser needs to manage object dependencies while parsing
 *     a configuration: marking a wait, checking whether a dependency exists, and retrieving it.
 *   - Used by the templated `depend_resolver_t`, which supplies the typing on top. It is not meant
 *     to be used directly outside the parser and dependency-resolution system.
 *
 * @date 2026-09-08 06:54
 */
struct depend_resolver_internal_t {
    virt_state_t *vs;

    depend_resolver_internal_t(virt_state_t *vs) : vs(vs) {}

    void internal_mark_wait(const std::string &dep_name, co::state_t *state);
    bool internal_check_depend(const std::string &dep_name);
    vc::ref_t<vc::object_t> internal_get_dep_object(const std::string &dep_name);
    std::string internal_get_obj_type_name(const std::string &dep_name);
};

/*!
 * [INTERNAL]
 * @brief Awaitable that suspends the calling coroutine until an object named `required_depend`
 * has been built, then resolves it to a `ref_t<T>`.
 *
 * Core:
 *   - This is the low-level mechanism the `!ref` handling in `resolve_int()`, `resolve_float()`
 *     and `resolve_str()`, and all of `resolve_obj<T>()`, are built on.
 *   - `await_ready()` checks whether the dependency is already registered, through
 *     `depend_resolver_internal_t::internal_check_depend()`.
 *   - If it is not, `await_suspend()` parks the caller on the wait queue for that name
 *     (`internal_mark_wait()`) and yields to the next runnable coroutine. It is resumed later by
 *     `mark_dependency_solved()`, once an object with that name is registered.
 *   - `await_resume()` then looks the object up and casts it to `T` through `to_related<T>()`,
 *     throwing `vc::except_t` if the resolved object is not actually a `T`.
 *
 * @tparam T  The expected type of the resolved object.
 *
 * @see resolve_obj, mark_dependency_solved
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
struct depend_resolver_t : depend_resolver_internal_t {
    /* We save the searched dependency */
    depend_resolver_t(virt_state_t *vs, std::string required_depend)
    : depend_resolver_internal_t(vs), required_depend(required_depend) {}

    /* If we already have the dependency we can already retur */
    bool await_ready() noexcept { return internal_check_depend(required_depend); }

    /* Else we place ourselves on the waiting queue */
    template <typename P>
    co::handle<void> await_suspend(co::handle<P> caller) noexcept {
        auto state = co::external_on_suspend(caller);

        /* We place ourselves on the waiting queue: */
        internal_mark_wait(required_depend, state);

        /* Else we return the next work in line that can be done */
        return co::external_wait_next_task(state->pool);
    }

    /* The `if (!ret)` throw below only ever fires when `obj` itself is null (the dependency was
    never registered) - a genuine cast failure inside to_related<T>() throws std::runtime_error
    directly and never reaches here, despite the message's "maybe cast doesn't work?" phrasing. */
    vc::ref_t<T> await_resume() {
        auto obj = internal_get_dep_object(required_depend);
        auto ret = obj ? obj->to_related<T>() : nullptr;
        if (!ret) {
            DBG("Invalid ref...");
            throw vc::except_t(
                    sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    internal_get_obj_type_name(required_depend).c_str(),
                    demangle<T, 4>().c_str()));
        }
        return ret;
    }

    std::string required_depend;
};

/* See resolve_obj()'s declaration above for its doc comment. */
template <typename T>
co::task<vc::ref_t<T>> resolve_obj(virt_state_t *vs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref") {
        /* This is simply a reference to an object m_field: !ref tag_name*/
        co_return co_await vc::depend_resolver_t<T>(vs, node.as_str());
    }
    else if (node.is_mapping() && node.as_map().size() == 1
            && node.as_map().begin()->second.contains("m_type"))
    {
        /* This is in the form m_field: tag_name: m_type: "..." */
        std::string tag = node.as_map().begin()->first.as_str();
        auto ref = co_await vc::build_object(vs, tag, node.as_map().begin()->second);
        co_return ref->template to_related<T>();
    }
    else if (node.contains("m_type")) {
        /* This is in the form m_field: m_type: "...", ie, inlined object */
        std::string tag = node.contains("m_tag") ?
                node["m_tag"].as_str() : new_anon_name(vs);
        auto ref = co_await vc::build_object(vs, tag, node);
        co_return ref->template to_related<T>();
    }

    /* None of the above */
    throw vc::except_t{std::format("node:{} is invalid in this contex",
            fkyaml::node::serialize(node))};
}

/* See resolve_memb()'s declaration above for its doc comment. */
template <typename T>
co::task<T> resolve_memb(virt_state_t *vs, fkyaml::node& node) {
    T ret;
    if (!node.has_tag_name() || node.get_tag_name() != "!copy")
        throw vc::except_t{std::format("node: {} must have the tag !copy for this operation",
                node.as_str())};
    std::string obj_name = node["object"].as_str();
    std::string memb_name = node["member"].as_str();
    co_await resolve_memb_data(vs, obj_name, memb_name, &ret, sizeof(T), typeid(T));
    co_return ret;
}

template <bool B, typename T>
inline consteval void luaw_static_assert(const char *description) {
    if constexpr (!B)
        throw description; /* This throw forces the termination of compilation */
}

/* See add_lua_flag_mapping()'s declaration above for its doc comment. */
template <typename T>
err_e add_lua_flag_mapping(virt_state_t *vs, const std::unordered_map<std::string, T>& mapping) {
    std::vector<std::pair<lua_Integer, std::string>> aux;
    for (auto &[k, v] : mapping)
        aux.push_back({(lua_Integer)v, k});
    return add_lua_flag_mapping(vs, aux);
} 

/*!
 * [INTERNAL]
 * @brief Template for converting a Lua value at a given stack index into a C++ type.
 *
 * Core:
 *   - The conversions live in the specializations; the primary template exists to reject anything
 *     unsupported with a static assertion rather than silently misconverting it.
 *
 * @tparam Param The C++ type to convert to.
 * @tparam index The Lua stack index of the value to convert.
 *
 * @date 2026-09-08 06:54
 */
/*!
 * [INTERNAL]
 * @brief What every @ref luaw_param_t does when it cannot convert what it was handed.
 *
 * Core:
 *   - It raises by default, because the usual caller is a `lua_CFunction` converting its own
 *     arguments: that runs inside the Lua call which passed them, so raising is safe and is how a
 *     script is told what it got wrong.
 *   - A caller converting a *result* replaces it with something that throws. A result is
 *     converted after the Lua call that produced it has returned, and there no protected call is
 *     left for a raise to land in - `luaD_throw` ends the process instead of unwinding.
 *
 * Detail:
 *   - It lives on a base rather than on each specialization so that a conversion written later
 *     inherits the policy instead of quietly defaulting to raising, which is how the result path
 *     came to abort in the first place.
 *
 * @date 2026-09-22 07:30
 */
struct luaw_param_base_t {
    std::function<void (lua_State *, const std::string&, const std::source_location)> throw_error =
            luaw_push_error;
};

template <typename Param>
struct luaw_param_t : luaw_param_base_t {
    void luaw_single_param(lua_State *L, ssize_t index) {
        DBG("FAILURE at index: %zd", index);
        /* What a parameter can be:
        1. vc::ref_t of some object
        2. a std::string
        3. an integer bitmap
        4. an integer
        ... etc. see below */

        /* If this is resolved to a void it will error out, which is ok, because this case is either
        way an error */
        luaw_static_assert<false, Param>(" - Is not a valid parameter type");
    }
};

/* This resolves userdata(void *) received from lua to an vc parameter */
template <>
struct luaw_param_t<void *> : luaw_param_base_t {
    void *luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("void* at index: %zd", index);
        if (lua_isnil(L, index))
            return NULL;
        return lua_touserdata(L, index);
    }
};

/* This resolves userdata(vc::ref) received from lua to an vc parameter */
template <typename T>
struct luaw_param_t<vc::ref_t<T>> : luaw_param_base_t {
    vc::ref_t<T> luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("Ref at index: %zd", index);
        if constexpr (std::is_same_v<T, lua_object_t>) {
            if (auto obj = get_object_from_lua(L, index);
                    obj && obj->type_id() == lua_object_t::type_id_static())
            {
                return obj->to_related<lua_object_t>();
            }
            auto obj = lua_object_t::create();
            lua_object_t::capture_lua_object(L, obj, index);
            return obj;
        } else {
            if (lua_isnil(L, index))
                return vc::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
            auto obj = get_object_from_lua(L, index);
            if (!obj)
                throw_error(L, std::format("Expected userdata at index {}", index),
                        std::source_location::current());
            return obj->to_related<T>();
        }
    }
};

/* This resolves bitmasks received from lua to an vc parameter */
template <typename T>
struct luaw_param_t<bm_t<T>> : luaw_param_base_t {
    T luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("BitMap at index: %zd", index);
        /* There are 2 options here (maybe later we will also add numbers, but not for now):
            1. This is a string that converts to the respective type bitmask
            2. An integer, this will be converted to T
            3. This is an enum value, either like 1. or 2. */
        auto from_string = [this](lua_State *L, int idx) -> T {
            const char *val = lua_tostring(L, idx);
            if (!val) {
                throw_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vc-bitmask] "
                        "object is an invalid string: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))), std::source_location::current());
            }
            fkyaml::node str_enum_val{val};
            return get_enum_val<T>(str_enum_val);
        };
        auto from_integer = [this](lua_State *L, int idx) -> T {
            int valid = 0;
            auto val = lua_tointegerx(L, idx, &valid);
            if (!valid) {
                throw_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vc-bitmask] "
                        "object is an invalid integer: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))), std::source_location::current());
            }
            return (T)val;
        };
        if (lua_isinteger(L, index)) {
            return from_integer(L, (int)index);
        }
        else if (lua_isstring(L, index)) {
            return from_string(L, (int)index);
        } 
        else if (lua_istable(L, index)) {
            int len = lua_rawlen(L, index);
            T ret = (T)0;
            for (int i = 1; i <= len; i++) {
                lua_rawgeti(L, index, i);
                if (lua_isinteger(L, -1))
                    ret = (T)(ret | from_integer(L, -1));
                else if (lua_isstring(L, -1))
                    ret = (T)(ret | from_string(L, -1));
                else {
                    throw_error(L, std::format(
                            "Invalid parameter at index {}, failed conversion to [vc-bitmask] "
                            "object is an invalid string or integer: [{}]",
                            index, lua_typename(L, lua_type(L, index))), std::source_location::current());
                }
                lua_pop(L, 1);
            }
            return ret;
        }
        else {
            throw_error(L, std::format(
                    "Invalid parameter at index {}, failed conversion to [vc-bitmask] "
                    "object is neither table, integer or string: [{}]",
                    index, lua_typename(L, lua_type(L, index))), std::source_location::current());
            return (T)0;
        }
    }
};

/* This resolves bool received from lua to an vc parameter */
template <>
struct luaw_param_t<bool> : luaw_param_base_t {
    bool luaw_single_param(lua_State *L, ssize_t index) {
        return lua_toboolean(L, index);
    }
};

/* This resolves integers received from lua to an vc parameter */
template <std::integral Integer>
struct luaw_param_t<Integer> : luaw_param_base_t {
    Integer luaw_single_param(lua_State *L, ssize_t index) {
        return lua_tointeger(L, index);
    }
};

/* This resolves floats received from lua to an vc parameter */
template <std::floating_point Float>
struct luaw_param_t<Float> : luaw_param_base_t {
    Float luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("Float at index: %zd", index);
        int valid = 0;
        Float ret = lua_tonumberx(L, index, &valid);
        if (!valid) {
            throw_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to float from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))),
                    std::source_location::current());
        }
        return ret;
    }
};

/* This resolves strings received from lua to an vc parameter */
template <>
struct luaw_param_t<const char *> : luaw_param_base_t {
    const char *luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("char* at index: %zd", index);
        const char *ret = lua_tostring(L, index);
        if (!ret) {
            throw_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to string from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))),
                    std::source_location::current());
        }
        return ret;
    }
};

/* Nil at `index` produces an empty string rather than failing (matches the nil-as-default
convention used by vector/tuple above); any other value Lua can't convert to a string
(lua_tostring returns nullptr - tables, booleans, userdata, etc.) hard-fails via
luaw_push_error()/lua_error(), unlike the old inline std::string branch this replaces, which
silently degraded any such value to "". A number at `index` still converts via lua_tostring's own
number-to-string coercion, same as it always has. */
template <>
struct luaw_param_t<std::string> : luaw_param_base_t {
    std::string luaw_single_param(lua_State *L, ssize_t index) {
        if (lua_isnil(L, index))
            return {};
        const char *ret = lua_tostring(L, index);
        if (!ret) {
            throw_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to string from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))),
                    std::source_location::current());
        }
        return ret;
    }
};

/*!
 * [INTERNAL]
 * @brief Helper template that strips `bm_t` wrappers off a type.
 *
 * Core:
 *   - Used to normalize types for the tuple, pair and vector specializations of `luaw_param_t`,
 *     which need the underlying type rather than the parsing marker.
 *
 * @tparam T The type to process.
 *
 * @date 2026-09-08 06:54
 */
template <typename T>
struct de_bitmaptizize { using Type = T; }; 

template <typename T>
struct de_bitmaptizize<bm_t<T>> { using Type = T; };

template <typename ...Args>
struct de_bitmaptizize<std::tuple<Args...>> {
    using Type = std::tuple<typename de_bitmaptizize<Args>::Type...>;
};

template <typename T, typename U>
struct de_bitmaptizize<std::pair<T, U>> {
    using Type = std::pair<typename de_bitmaptizize<T>::Type, typename de_bitmaptizize<U>::Type>;
};

template <typename T>
struct de_bitmaptizize<std::vector<T>> {
    using Type = std::vector<typename de_bitmaptizize<T>::Type>;
};

/* Nil at `index` produces a default-constructed tuple rather than failing; any non-table, non-nil
value hard-fails via luaw_push_error() (lua_error()). A table doesn't have to match
sizeof...(Args) exactly - only min(len, sizeof...(Args)) elements are read, filling tuple slots
0..count-1 in order (element 0 first, matching Lua's own "extra values discarded, missing values
default" unpack convention); any slots beyond that stay default-constructed (a short table), and
any extra table elements past sizeof...(Args) are simply never read (a long table). Pushed in
reverse (count down to 1) so the stack top ends up holding element 0, matching the per-slot
negative-index reads below. */
template <typename ...Args>
struct luaw_param_t<std::tuple<Args...>> : luaw_param_base_t {
    template <size_t ...I>
    auto _luaw_single_param_impl(lua_State *L, ssize_t index, std::index_sequence<I...>) {
        using Ret = typename de_bitmaptizize<std::tuple<Args...>>::Type;
        Ret ret;
        if (lua_isnil(L, index))
            return ret;
        if (!lua_istable(L, index)) {
            throw_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index),
                    std::source_location::current());
        }
        int abs_idx = lua_absindex(L, index);
        int len = lua_rawlen(L, index);
        size_t count = (size_t)len < sizeof...(Args) ? (size_t)len : sizeof...(Args);
        for (int i = (int)count; i >= 1; i--)
            lua_rawgeti(L, abs_idx, i);
        ([&] {
            if (I < count)
                std::get<I>(ret) = luaw_param_t<Args>{}.luaw_single_param(L, -ssize_t(I) - 1);
        }(), ...);
        lua_pop(L, (int)count);
        return ret;
    }

    auto luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("Tuple at index: %zd", index);
        return _luaw_single_param_impl(L, index, std::index_sequence_for<Args...>{});
    }
};

/* Delegates to the tuple specialization above (reads the same 2-element table as
std::tuple<Arg1,Arg2>) and unpacks the result into a pair, rather than duplicating its stack
handling. */
template <typename Arg1, typename Arg2>
struct luaw_param_t<std::pair<Arg1, Arg2>> : luaw_param_base_t {
    auto luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("Pair at index: %zd", index);
        auto tuple = luaw_param_t<std::tuple<Arg1, Arg2>>{}.luaw_single_param(L, index);
        typename de_bitmaptizize<std::pair<Arg1, Arg2>>::Type ret =
                {std::get<0>(tuple), std::get<1>(tuple)};
        return ret;
    }
};

/* Nil at `index` produces an empty vector rather than failing. Otherwise expects a table, and
(unlike the tuple specialization above) converts one element at a time - push, convert, pop - since
the element count isn't known at compile time so there's no single pack-expansion construction to
build. */
template <typename T>
struct luaw_param_t<std::vector<T>> : luaw_param_base_t {
    auto luaw_single_param(lua_State *L, ssize_t index) {
        // DBG("Vector at index: %zd", index);
        using Ret = typename de_bitmaptizize<std::vector<T>>::Type;
        if (lua_isnil(L, index))
            return Ret{};
        if (!lua_istable(L, index)) {
            throw_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index),
                    std::source_location::current());
        }
        int len = lua_rawlen(L, index);
        Ret ret(len);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            ret[i-1] = luaw_param_t<T>{}.luaw_single_param(L, -1);
            lua_pop(L, 1);
        }
        return ret;
    }
};

// helper to detect if a type is vc::ref_t<...>
template <typename>
struct is_vc_ref_t : std::false_type {};

template <typename T>
struct is_vc_ref_t<vc::ref_t<T>> : std::true_type {};

template <typename T>
constexpr bool is_vc_ref = is_vc_ref_t<T>::value;

// helper to detect if a type is std::tuple<...>
template <typename T>
struct is_tupple_t : std::false_type {};

template <typename ...Args>
struct is_tupple_t<std::tuple<Args...>> : std::true_type {};

template <typename T>
constexpr bool is_tupple = is_tupple_t<T>::value;

// helper to detect if a type is std::pair<...>
template <typename T>
struct is_pair_t : std::false_type {};

template <typename A, typename B>
struct is_pair_t<std::pair<A, B>> : std::true_type {};

template <typename T>
constexpr bool is_pair = is_pair_t<T>::value;

// helper to detect if a type is std::vector<...>
template <typename T>
struct is_vector_t : std::false_type {};

template <typename T, typename Alloc>
struct is_vector_t<std::vector<T, Alloc>> : std::true_type {};

template <typename T>
constexpr bool is_vector = is_vector_t<T>::value;

// helper to detect if a type is a known enum
template <typename T>
concept is_vc_enum = requires(fkyaml::node n) {
    get_enum_val<T>(n);
};

/*!
 * [INTERNAL]
 * @brief Template for pushing a C++ return value of type `T` onto the Lua stack.
 *
 * Core:
 *   - The pushes live in the specializations; the primary template exists to reject anything
 *     unsupported with a static assertion.
 *
 * @tparam T The C++ type to push to Lua.
 *
 * @date 2026-09-08 06:54
 */
/* TODO: returners and parameters must be part of the interface because users need to be able to
add their types */
template <typename T>
struct luaw_returner_t {
    void luaw_ret_push(lua_State *L, T&& t) {
        (void)t;
        luaw_static_assert<false, T>(" - Is not a valid return type");
    }
};

template <>
struct luaw_returner_t<bool> {
    void luaw_ret_push(lua_State *L, bool x) {
        lua_pushboolean(L, x);
    }
};

template <std::integral Integer>
struct luaw_returner_t<Integer> {
    void luaw_ret_push(lua_State *L, Integer x) {
        lua_pushinteger(L, x);
    }
};

template <std::floating_point Floating>
struct luaw_returner_t<Floating> {
    void luaw_ret_push(lua_State *L, Floating x) {
        lua_pushnumber(L, x);
    }
};

template <>
struct luaw_returner_t<const char *> {
    void luaw_ret_push(lua_State *L, const char *x) {
        lua_pushstring(L, x);
    }
};

template <>
struct luaw_returner_t<std::string> {
    void luaw_ret_push(lua_State *L, const std::string& x) {
        lua_pushstring(L, x.c_str());
    }
};

template <>
struct luaw_returner_t<void *> {
    void luaw_ret_push(lua_State *L, void *rawptr) {
        lua_pushlightuserdata(L, rawptr);
    }
};

/* A null ref_t<T> pushes nil rather than erroring. push_vc_object() currently always returns
VC_ERROR_OK (see its own comment), so the except_t throw below can't actually trigger today - it's
defensive against push_vc_object() ever growing a real failure path. */
template <typename T>
struct luaw_returner_t<vc::ref_t<T>> {
    void luaw_ret_push(lua_State *L, vc::ref_t<T> ref) {
        if (!ref) {
            lua_pushnil(L);
            return;
        }
        if (push_vc_object(L, ref) != VC_ERROR_OK)
            throw except_t("Failed to push user object");
    }
};

/* Builds a Lua table, one element per tuple slot, each pushed via its own luaw_returner_t<Type>
(so heterogeneous tuple elements each get the right conversion) - luaw_returner_t<std::vector<T>>
below does the same thing for a single, uniform element type. Note this duplicates
luaw_push_cpp_object()'s own tuple/vector handling elsewhere in this file - two independent
C++->Lua conversion paths exist side by side (this one used by call_lua()/luaw_function_wrapper's
return-value pushing, that one used for member-getter/return conversion elsewhere). */
template <typename ...Args>
struct luaw_returner_t<std::tuple<Args...>> {
    void luaw_ret_push(lua_State *L, const std::tuple<Args...>& t) {
        lua_createtable(L, std::tuple_size_v<std::decay_t<decltype(t)>>, 0);

        int i = 1;
        auto fn = [&](auto &arg) {
            using Type = std::decay_t<decltype(arg)>;
            luaw_returner_t<Type>{}.luaw_ret_push(L, arg);
            lua_rawseti(L, -2, i++);
        };
        std::apply([&](auto&& ...args){
            (fn(args), ...);
        }, t);
    }
};

/* A pair is just a fixed 2-slot tuple - delegates to luaw_returner_t<std::tuple<Arg1,Arg2>>
rather than duplicating its table-building logic. */
template <typename Arg1, typename Arg2>
struct luaw_returner_t<std::pair<Arg1, Arg2>> {
    void luaw_ret_push(lua_State *L, const std::pair<Arg1, Arg2>& p) {
        luaw_returner_t<std::tuple<Arg1, Arg2>>{}.luaw_ret_push(L, std::tuple<Arg1, Arg2>(p.first, p.second));
    }
};

template <typename T>
struct luaw_returner_t<std::vector<T>> {
    void luaw_ret_push(lua_State *L, const std::vector<T>& v) {
        lua_createtable(L, v.size(), 0);
        for (size_t i = 0; i < v.size(); i++) {
            luaw_returner_t<std::decay_t<T>>{}.luaw_ret_push(L, v[i]);
            lua_rawseti(L, -2, i+1);
        }
    }
};

/* It would be better for the user to push a string or an array of strings, it makes more sense to
see the things, but sadly I don't know if that is possible, because the thing is that we can't
really get the signification of the bits. Once here we don't really know if Enum is a type of a
bitmap or we simply where told in it there is a bitmap. */
template <is_vc_enum Enum>
struct luaw_returner_t<Enum> {
    void luaw_ret_push(lua_State *L, Enum x) {
        lua_pushnumber(L, (int)x);
    }
};

/* [INTERNAL] Shared body behind luaw_function_wrapper<function,Params...>() - converts each Lua
argument via luaw_param_t<Params,I+1> (Params start at Lua stack index 1), calls `function`, and
(if it returns non-void) pushes the result via luaw_returner_t. Returns the count of Lua return
values (0 or 1), matching the lua_CFunction contract. */
template <auto function, typename ...Params, size_t ...I>
inline int luaw_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    using RetType = decltype(function(
            luaw_param_t<Params>{}.luaw_single_param(L, I + 1)...));

    // ([L]{
    //     DBG("Index: %zu -> (%s, %s)", I + 1, demangle<Params>().c_str(),
    //             lua_typename(L, lua_type(L, I + 1)));
    // }(), ...);

    if constexpr (std::is_void_v<RetType>) {
        function(luaw_param_t<Params>{}.luaw_single_param(L, I + 1)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, function(
                luaw_param_t<Params>{}.luaw_single_param(L, I + 1)...));
        return 1;
    }
}

/* [INTERNAL] Same as luaw_function_wrapper_impl() above but for a member function - Lua stack
index 1 is `self` (unboxed via get_object_from_lua), so Params start at I+2 instead of I+1. */
template <typename T, auto member_ptr, typename ...Params, size_t ...I>
int luaw_member_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    auto o = get_object_from_lua(L, 1);
    if (!o)
        luaw_push_error(L, "internal_error: Nil user object can't call member function!");
    auto obj = o->to_related<T>();

    using RetType = decltype((obj.get()->*member_ptr)(
            luaw_param_t<Params>{}.luaw_single_param(L, I + 2)...));

    if constexpr (std::is_void_v<RetType>) {
        (obj.get()->*member_ptr)(luaw_param_t<Params>{}.luaw_single_param(L, I + 2)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, (obj.get()->*member_ptr)(
                luaw_param_t<Params>{}.luaw_single_param(L, I + 2)...));
        return 1;
    }    
}

/* See luaw_function_wrapper()'s declaration above for its doc comment. */
template <auto Function, typename ...Params>
inline int luaw_function_wrapper(lua_State *L) {
    try {
        return luaw_function_wrapper_impl<Function, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

/*!
 * [INTERNAL]
 * @brief Wraps a C++ member function as a `lua_CFunction`, converting arguments and return values
 * automatically.
 *
 * Core:
 *   - The first Lua argument is expected to be a userdata representing the object instance.
 *   - Exceptions thrown by the wrapped member function are caught and turned into a Lua error.
 *
 * Detail:
 *   - The parameter and return conversion itself happens in
 *     `luaw_member_function_wrapper_impl()`; this is the try/catch around it.
 *
 * @tparam T          The type of the object instance.
 * @tparam member_ptr The member function pointer to wrap.
 * @tparam Params     The types of the parameters expected by the member function.
 *
 * @param L The Lua state.
 *
 * @return The number of values returned to Lua: 0 for void, 1 otherwise.
 *
 * @date 2026-09-08 06:54
 */
template <typename T, auto member_ptr, typename ...Params>
inline int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<T, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

/* consteval + throw forces a compile-time-only error when `test` is false, same trick as
luaw_static_assert() above. Unlike that one, the message parameter here is unnamed/unused in the
body - `ToDisplay` isn't read either, it just makes each instantiation distinct per type so the
compiler's error output points at the actual offending type. */
template <bool test, typename ToDisplay>
inline consteval void demangle_static_assert(const char *) {
    if constexpr (!test)
        throw;
}

/* [INTERNAL] The C++->Lua counterpart to luaw_lua_to_cpp_object() - pushes `object` onto `L` via
luaw_returner_t<Type>, which supplies the actual per-category push logic (string, bool, integral/
floating-point, vector/tuple/pair, enum, vc::ref_t<T> - each specialization documents its own
behavior at its own definition, above). Always returns 0; an unsupported type fails to compile via
luaw_returner_t<T>'s own primary-template static_assert rather than a check here. */
template <typename T>
int luaw_push_cpp_object(lua_State *L, const T &object) {
    using Type = std::decay_t<T>;
    luaw_returner_t<Type>{}.luaw_ret_push(L, object);
    return 0;
}

/* [INTERNAL] Lua-callable getter for a registered member object - invoked via __index (see the
__index lambda in virt_composer.cpp), stack layout [obj, key] so obj sits at -2. Pushes `member`
back to Lua via luaw_push_cpp_object() and returns it as the single result. */
template <typename T, auto member_ptr>
int luaw_member_object_wrapper(lua_State *L) {
    try {
        auto o = get_object_from_lua(L, -2);
        if (!o) {
            luaw_push_error(L, "internal_error: Nil user object can't get member!");
        }
        auto obj = o->to_related<T>();
        auto &member = obj.get()->*member_ptr;

        if (luaw_push_cpp_object(L, member) < 0) {
            luaw_push_error(L, "Couldn't construct the member object!");
        }

        return 1;
    }
    catch (...) { return luaw_catch_exception(L); }
}

/* Types excluded from luaw_lua_to_cpp_object()'s dispatch - conversions that are fine for
call-scoped use (an ordinary function parameter, via luaw_param_t directly) but not safe to store
long-term in a C++ object member. Recurses into vector/tuple/pair element types so e.g.
std::vector<const char*> or std::tuple<int, const char*> are caught too, not just a blacklisted
type used bare. */
template <typename T>
struct luaw_setter_blacklist_t : std::false_type {};

template <>
struct luaw_setter_blacklist_t<const char *> : std::true_type {};

template <typename T, typename Alloc>
struct luaw_setter_blacklist_t<std::vector<T, Alloc>> : luaw_setter_blacklist_t<T> {};

template <typename ...Args>
struct luaw_setter_blacklist_t<std::tuple<Args...>>
        : std::bool_constant<(luaw_setter_blacklist_t<Args>::value || ...)> {};

template <typename A, typename B>
struct luaw_setter_blacklist_t<std::pair<A, B>>
        : std::bool_constant<luaw_setter_blacklist_t<A>::value || luaw_setter_blacklist_t<B>::value> {};

/* [INTERNAL] The Lua->C++ counterpart to luaw_push_cpp_object() - converts the Lua value at
`index` into `object`. Checks luaw_setter_blacklist_t first - a blacklisted type must never reach
the generic fallback below, since that would otherwise convert it just fine. Enum is the one
remaining special case (goes through `bm_t<T>`'s single-value parsing path rather than a direct
luaw_param_t<Enum,...> specialization - a bare enum was never meant to be Lua-parseable without
that wrapper). Everything else - string, bool, integral/floating-point, vector/tuple/pair,
vc::ref_t<T> - falls back to luaw_param_t<Type,-1>, each specialization documenting its own nil/
error/shape-mismatch behavior at its own definition, above. Returns 0 on success, -1 on a shape
mismatch or a blacklist hit; an unsupported type fails to compile via luaw_param_t<T,index>'s own
primary-template static_assert. Used for member setters, call_lua()'s return conversion, and
vector/tuple/pair element conversion. */
template <typename T>
int luaw_lua_to_cpp_object(lua_State *L, int index, T &object) {
    using Type = std::decay_t<T>;

    if constexpr (luaw_setter_blacklist_t<Type>::value) {
        demangle_static_assert<false, decltype(object)>(
                " - Is blacklisted from member-object setters (see luaw_setter_blacklist_t)");
        return -1;
    }
    else {
        /* An enum is parsed through bm_t<T>'s single-value path; a bare enum was never meant to be
        Lua-parseable without that wrapper. Everything else converts as itself. 2026-09-22 07:30 */
        using ParamT = std::conditional_t<is_vc_enum<Type>, bm_t<Type>, Type>;
        luaw_param_t<ParamT> param{};

        /* This is the one caller that must report rather than raise, which is what this
        function's -1 has always claimed it does. A result is converted after the Lua call that
        produced it is over, so no protected call is left for a lua_error to land in and luaD_throw
        ends the process instead of unwinding. Raising stays the default everywhere else, because
        an argument is converted inside the call that passed it. 2026-09-22 07:30 */
        param.throw_error = [](lua_State *, const std::string& str,
                const std::source_location) -> void
        {
            throw std::runtime_error(str);
        };

        try {
            object = param.luaw_single_param(L, index);
        }
        catch (std::exception &e) {
            DBG("Failed to convert object at index %d: %s", index, e.what());
            return -1;
        }
        return 0;
    }
}

/* [INTERNAL] Lua-callable setter for a registered member object - invoked via __newindex (see the
__newindex lambda in virt_composer.cpp), stack layout [obj, key, value] so obj sits at -3. Converts
the value on top of the stack into `member` via luaw_lua_to_cpp_object() and assigns it in place. */
template <typename T, auto member_ptr>
int luaw_member_setter_object_wrapper(lua_State *L) {
    auto o = get_object_from_lua(L, -3);
    if (!o) {
        luaw_push_error(L, "Invalid userdata");
    }
    auto obj = o->to_related<T>();
    auto &member = obj.get()->*member_ptr;

    if (luaw_lua_to_cpp_object(L, -1, member) < 0) {
        luaw_push_error(L, "Couldn't convert from type from lua to cpp type");
    }
    return 0;
}

/* Backs VC_REGISTER_MEMBER_FUNCTION - see that macro's doc for the user-facing contract. Just
wraps luaw_member_function_wrapper<T,member_ptr,Params...> as the Lua-callable and hands it to
set_lua_class_member() (which is what actually propagates it across registered base/derived
types). */
template <typename T, auto member_ptr, typename ...Params>
void luaw_register_member_function(virt_state_t *vs, const char *function_name) {
    set_lua_class_member(vs, T::type_id_static(), function_name,
            &luaw_member_function_wrapper<T, member_ptr, Params...>, LUAW_MEMBER_FUNCTION);
}

/* Backs VC_REGISTER_MEMBER_OBJECT - see that macro's doc for the user-facing contract. Registers
both directions: luaw_member_object_wrapper as the getter (via set_lua_class_member) and
luaw_member_setter_object_wrapper as the setter (via set_class_member_setter) - a member object is
always both readable and writable from Lua, there's no read-only variant. */
template <typename T, auto member_ptr>
void luaw_register_member_object(virt_state_t *vs, const char *member_name) {
    set_lua_class_member(vs, T::type_id_static(), member_name,
            &luaw_member_object_wrapper<T, member_ptr>, LUAW_MEMBER_OBJECT);
    set_class_member_setter(vs, T::type_id_static(), member_name,
            &luaw_member_setter_object_wrapper<T, member_ptr>);
}

/* Backs VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER - see that macro's doc for the user-facing contract.
Records `member_ptr`'s type via typeid (so resolve_memb_data() can later reject a mismatched `T`)
and registers a small lambda that memcpy's straight out of the member, keyed on T's own type_id()
so the lookup happens by the source object's runtime type, not by the caller's `T`. */
template <typename T, auto member_ptr>
void register_trivially_copyable_member(virt_state_t *vs, const char *member_name) {
    using member_type = std::decay_t<decltype(((T *)NULL)->*member_ptr)>;
    auto typeid_of_member = std::type_index(typeid(member_type));
    static_assert(std::is_trivially_copyable_v<member_type>,
            "The member object must be trivially copiable to be registered");

    set_trivial_copy_member(vs, T::type_id_static(), member_name, typeid_of_member,
        [](object_t *obj, void *dst, size_t sz) {
            auto tobj = (T *)obj;
            memcpy(dst, &(tobj->*member_ptr), sz);
        }
    );
}

/* Detects which of T/U is the real base at compile time (is_base_of_v either way) and forwards to
set_base_derived_relation() with the base first - see register_inheritance()'s declaration above
for the full user-facing contract. The demangle_static_assert() below triggers a compile error if
neither is actually related to the other. */
template <typename T, typename U>
requires std::is_base_of_v<vc::object_t, T> && std::is_base_of_v<vc::object_t, U>
void register_inheritance(virt_state_t *vs) {
    if constexpr (std::is_base_of_v<U, T>)
        set_base_derived_relation(vs, U::type_id_static(), T::type_id_static());
    else if constexpr (std::is_base_of_v<T, U>)
        set_base_derived_relation(vs, T::type_id_static(), U::type_id_static());
    else {
        demangle_static_assert<
                std::is_base_of_v<U, T> || std::is_base_of_v<T, U>,
                std::pair<T, U>>
                ("ERROR: U is not related to T and T is not related to U");
    }   
}


/* Shared tail for call_lua()/lua_object_t::call<R>(): assumes the callee is already on top of
vs's Lua stack (pushed by the caller - lua_getglobal() for call_lua(), push(L) for
lua_object_t::call<R>()) - pushes each of `args`, pcalls, and converts the single result. Not a
public entry point on its own; it's the "push args, pcall, convert result" logic both of those
share, minus how the callee itself gets onto the stack. */
template <typename R, typename ...Args>
std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
call_on_stack(virt_state_t *vs, Args&& ...args)
{
    using RetT = std::conditional_t<!std::is_void_v<R>, R, int>;

    auto L = luaw_get_lua_state(vs);
    int pushcnt = 1; /* the callee, already on the stack before this call */
    try {
        ([&](auto &obj){
            if (luaw_push_cpp_object(L, obj) < 0){
                DBG("Failed to push cpp argument onto lua stack");
                throw "";
            }
            pushcnt++;
        }(args), ...);
    }
    catch (...) {
        lua_pop(L, pushcnt);
        return {RetT{}, VC_ERROR_FAILED_CALL};
    }
    int argc = std::tuple_size_v<std::tuple<Args...>>;
    if constexpr (std::is_void_v<R>) {
        if (lua_pcall(L, argc, 0, 0) != LUA_OK) {
            DBG("LUA call_on_stack([%d]) Failed: \n%s", argc, lua_tostring(L, -1));
            lua_pop(L, 1);
            return {0, VC_ERROR_FAILED_CALL};
        }
        return {0, VC_ERROR_OK};
    }
    else {
        R result = {};
        if (lua_pcall(L, argc, 1, 0) != LUA_OK) {
            DBG("LUA call_on_stack([%d]) Failed: \n%s", argc, lua_tostring(L, -1));
            lua_pop(L, 1);
            return {result, VC_ERROR_FAILED_CALL};
        }
        /* The conversion answers rather than raises, so a script that returned a shape this R
        cannot take is a failed call and not a dead process. Until 22-09-2026 this return was
        dropped on the floor and the raise underneath it ended the actor. 2026-09-22 06:50 */
        if (luaw_lua_to_cpp_object(L, -1, result) < 0) {
            DBG("LUA call_on_stack([%d]): the result does not convert to the type asked for",
                    argc);
            lua_pop(L, 1);
            return {R{}, VC_ERROR_FAILED_CALL};
        }
        lua_pop(L, 1);
        return {result, VC_ERROR_OK};
    }
}

/* See call_lua()'s declaration above for its doc comment. */
template <typename R, typename ...Args>
std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
call_lua(virt_state_t *vs, const char *function_name, Args&& ...args)
{
    lua_getglobal(luaw_get_lua_state(vs), function_name);
    return call_on_stack<R>(vs, std::forward<Args>(args)...);
}

/* See get_enum_val(node, enum_vals)'s declaration above for its doc comment. */
template <typename T>
inline T get_enum_val(fkyaml::node &node, const std::unordered_map<std::string, T>& enum_vals) {
    if (node.is_string()) {
        if (!::has(enum_vals, node.as_str()))
            throw vc::except_t(std::format("Unknown enum({}) value: {}",
                    demangle<T>(), node.as_str()));
        return enum_vals.find(node.as_str())->second;
    }
    if (node.is_integer()) {
        return T(node.as_int());
    }
    if (node.is_sequence()) {
        lua_Integer ret = 0;
        for (auto &val : node.as_seq())
            ret |= (lua_Integer)get_enum_val(val, enum_vals);
        return (T)ret;
    }
    throw vc::except_t{std::format("Node({}), can't be converted to an enum of type ({})",
            fkyaml::node::serialize(node), demangle<T>())};
}

/* See get_enum_val(node)'s declaration above for its doc comment. */
template <typename T>
inline T get_enum_val(fkyaml::node &n) = delete;

/* See err_e_from_str's declaration above for its doc comment. Every other enum that reaches Lua is
registered this way; the library's own never was, which is why a script that received one saw a
bare number and had nothing to compare it against. 2026-09-22 06:50 */
inline std::unordered_map<std::string, err_e> err_e_from_str = {
    {"VC_ERROR_OK",          VC_ERROR_OK},
    {"VC_ERROR_GENERIC",     VC_ERROR_GENERIC},
    {"VC_ERROR_PARSE_YAML",  VC_ERROR_PARSE_YAML},
    {"VC_ERROR_FAILED_CALL", VC_ERROR_FAILED_CALL},
    {"VC_ERROR_REDEFINED",   VC_ERROR_REDEFINED},
};

/* This is what makes is_vc_enum<err_e> true, so the enum returner already in this file carries an
err_e across and no specialization of its own is needed. 2026-09-22 06:50 */
template <> inline err_e get_enum_val<err_e>(fkyaml::node &n) {
    return get_enum_val(n, err_e_from_str);
}

/* Definitions - see the three to_string() overloads' declarations above for their doc comments. */
inline std::string to_string(object_type_e type) {
    return type.name();
}

template <typename T>
inline std::string to_string(ref_t<T> ref) {
    return "ref: " + ref->to_string();
}

inline std::string to_string(const object_t& ref) {
    return ref.to_string();
}

/* lua_object_t --------------------------------------------------------------------------------- */

/* Pushes a duplicate of the value at `idx`, then hands it to `capture_ref()` to actually capture
it (including nil-as-reset handling - see that function's own comment). `ref` must already exist
(via `create()`); the null check is defensive rather than expected to trigger at any current call
site. */
inline void lua_object_t::capture_lua_object(lua_State *L, vc::ref_t<lua_object_t> ref, int idx) {
    if (!ref)
        luaw_push_error(L, "internal_error: capture_lua_object() called with a null lua_object_t ref");
    lua_pushvalue(L, idx);
    ref->capture_ref(L);
}

inline void lua_object_t::capture(vc::ref_t<lua_object_t> oth) {
    release();
    /* oth may be non-null but hold nothing (e.g. a missing Lua argument produces a fresh,
    never-captured instance) - treat that the same as oth being null. */
    if (!oth || oth->ref == LUA_NOREF || !oth->L)
        return;
    oth->push(oth->L);
    capture_ref(oth->L);
}

/* Unrefs the held value from its registry sub-table, if anything was ever captured (no-op
otherwise). Leaves the object in the same "nothing captured" state as a fresh create(), so it's
safe to capture() into again afterward. */
inline void lua_object_t::release() {
    if (ref == LUA_NOREF || !L)
        return;
    lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);
    luaL_unref(L, -1, ref);
    lua_pop(L, 1);
    ref = LUA_NOREF;
    L = nullptr;
}

/* Just releases whatever's held - see release(). */
inline lua_object_t::~lua_object_t() {
    release();
}

inline void lua_object_t::push(lua_State *L) {
    if (ref == LUA_NOREF || !this->L) {
        lua_pushnil(L);
        return;
    }
    /* Compares Lua universes (shared registry), not raw thread pointers - L and this->L legitimately
    differ when called from a coroutine's own thread. */
    if (luaw_get_virt_state(L) != luaw_get_virt_state(this->L)) {
        luaw_push_error(L, "internal_error: lua_object_t used with a different lua_State than "
                "the one it was captured on");
        return;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, table_ref);
    lua_rawgeti(L, -1, ref);
    lua_remove(L, -2);
}

inline int lua_object_t::call(lua_State *L, int nargs) {
    if (ref == LUA_NOREF || !this->L) {
        luaw_push_error(L, "internal_error: lua_object_t has nothing captured (released, or never "
                "captured)");
        return 0;
    }
    /* See push()'s matching check for why this compares Lua universes, not raw thread pointers. */
    if (luaw_get_virt_state(L) != luaw_get_virt_state(this->L)) {
        luaw_push_error(L, "internal_error: lua_object_t used with a different lua_State than "
                "the one it was captured on");
        return 0;
    }
    int base = lua_gettop(L) - nargs;
    push(L);
    lua_insert(L, base + 1);
    if (lua_pcall(L, nargs, LUA_MULTRET, 0) != LUA_OK) {
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);
        luaw_push_error(L, err);
        return 0;
    }
    return lua_gettop(L) - base;
}

/* Needs call_on_stack() above, which its body calls. */
template <typename R, typename ...Args>
std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
lua_object_t::call(Args&& ...args)
{
    using RetT = std::conditional_t<!std::is_void_v<R>, R, int>;
    if (ref == LUA_NOREF || !L)
        return {RetT{}, VC_ERROR_FAILED_CALL};
    push(L);
    return call_on_stack<R>(luaw_get_virt_state(L), std::forward<Args>(args)...);
}

/* c_function_t --------------------------------------------------------------------------------- */

/* Builds a `c_function_t`, then calls `init()` immediately - see that function's doc for what
`source` needs to be for this to actually succeed. Throws if `init()` fails, so a `c_function_t`
never exists without a working `_fn` already bound. */
inline vc::ref_t<c_function_t> c_function_t::create(virt_state_t *vs, std::string name,
        std::string source)
{
    auto ret = std::make_shared<c_function_t>(vc::object_t::Private{type_id_static()});
    ret->m_name = name;
    ret->m_source = source;
    if (ret->init(vs) < 0)
        throw vc::except_t("Failed c_function_t init");
    DBG("Created Lua Function: name: %s src: %s", name.c_str(), source.c_str());
    return ret;
}

/* Invokes whatever `init()` bound to `_fn`. Returns `-1` without calling anything if `_fn` was
never set - normally unreachable since `create()` throws on a failed `init()`, but guarded here
anyway since nothing stops a caller from holding onto a `c_function_t` whose `init()` failed some
other way. */
inline int c_function_t::call(lua_State *L) {
    if (!_fn) {
        DBG("No function to call");
        return -1;
    }
    return _fn(L);
}

/* Resolves `_fn` from `m_source`/`m_name`. Only one source kind works today:
`m_source == "[INTERNAL]"` looks `m_name` up in `internal_funcs` (populated by
`add_internal_func()`). Any other `m_source` falls into the `else` and fails - DLL/shared-object
loading is planned (see the TODO below) but not implemented yet. */
inline vc::ret_t c_function_t::init(virt_state_t *vs) {
    auto *funcs = state_internal_funcs(vs);
    if (m_source == "[INTERNAL]" && has(*funcs, m_name)) {
        _fn = (*funcs)[m_name];
        return VC_ERROR_OK;
    }
    /* TODO: DLL/SO source */
    else {
        return VC_ERROR_GENERIC;
    }
}

}; /* namespace virt_composer */

#endif
