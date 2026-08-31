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
 * Users of the parser can instantiate a `virt-state`, which maintains a name-indexed lookup of
 * objects (`object_t` being the base class for all objects in the system, `vc` standing for
 * virt_composer) - obtained as `vc::ref_t<vc::object_t>` handles via accessors like `get_ref()`,
 * though the pool itself doesn't hold ownership that way. The object pool can be dynamically
 * enriched or modified by reading YAML config files. Additionally, users can execute Lua scripts to
 * interact with the object pool. A single Lua state is shared across all objects in the pool.
 *
 * Lua scripts are primarily used to define functions for later execution. While basic
 * initialization in scripts is acceptable, the order of script execution is not guaranteed.
 * Therefore, scripts should focus on defining functions rather than performing actions directly.
 *
 * User-defined composers must register object types, members, and functions to make them available
 * to the parser. Users can also attach custom Lua functions to objects during registration.
 */

/* TODO: all yaml nodes should be able to define dependencies, those dependencies would be
especially usefull for things like shaders, lua scripts, expressions, etc. This would in a sense
create a strict ordering that and I should check if it can create cycles (deadlocks). */

#include  <typeindex>

#include "virt_object.h"
#include "co_utils.h"
#include "yaml.h"
#include "minilua.h"
#include "demangle.h"


#if defined(_MSC_VER) && !defined(ssize_t)
using ssize_t = ptrdiff_t;
#endif

/*! TODO: this:
 * Something like: in .so you have a int register_meta(vc::virt_state_t *vs) function like usual,
 * but it will register it's types to the vs implementation, expanding it.
 * This needs to be done.
 */
/* This define is meant to let a dynamically-loaded library (.so) give its own registered types a
private ID range (e.g. lib-id << 24) instead of colliding with the host's, once the ".so type
registration" TODO right above this is actually implemented. Currently it has NO EFFECT: nothing
reads `virt_tag_t::off` (the value this macro feeds) - `compile_unique_id<virt_tag_t>()` always
starts counting from 0 regardless of what this is set to. */
#ifndef VIRT_COMPOSER_UID_START_OFFSET
# define VIRT_COMPOSER_UID_START_OFFSET 0
#endif

/*!
 * @def VIRT_COMPOSER_ENABLE_LUA_IO
 * @brief Exposes Lua's standard `io` library to every Lua script running in this virt_state_t.
 *
 * Disabled (`0`) by default: `luaw_init()` skips `luaL_requiref(L, LUA_IOLIBNAME, ...)`, so Lua
 * scripts have no `io.*` functions at all (no reading/writing arbitrary files from Lua).
 *
 * @note Must be `#define`d before including `virt_composer.h`, like any other `VIRT_COMPOSER_*`
 *       configuration macro.
 */
#ifndef VIRT_COMPOSER_ENABLE_LUA_IO
# define VIRT_COMPOSER_ENABLE_LUA_IO 0
#endif

/*!
 * @def VIRT_COMPOSER_ENABLE_LUA_OS
 * @brief Exposes Lua's standard `os` library to every Lua script running in this virt_state_t.
 *
 * Disabled (`0`) by default: `luaw_init()` skips `luaL_requiref(L, LUA_OSLIBNAME, ...)`, so Lua
 * scripts have no `os.*` functions at all (no `os.execute`, `os.remove`, `os.getenv`, etc.).
 *
 * @note Must be `#define`d before including `virt_composer.h`, like any other `VIRT_COMPOSER_*`
 *       configuration macro.
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
 * @brief Registers a new enumerator type for use with the virt_composer framework.
 *
 * This macro generates a unique enumerator value for the given `type` using
 * `virt_object::compile_unique_id<virt_composer::virt_tag_t>()` and associates it with the
 * stringified enum name (`#type`). It is primarily used to register custom types as enumerators
 * within the virt_composer system.
 *
 * @param type The enum name associated to a type to register as an enumerator.
 * 
 * @note Those enums should be returned by the respective objects inheriting `vc::object_t`
 *       from `type_id()` and `type_id_static()`.
 * @note To be more precise: you can use whatever method you wish for your virtual objects' type
 *       id, but at the end of the day `virt_composer::VIRT_TYPE_CNT` must be a number greater than
 *       all the type ids in use (arrays of this length get declared) and every type id must be
 *       different. Using this macro plus `virt_composer_end.h` (which computes `VIRT_TYPE_CNT` for
 *       you automatically, right before `create_state()`) already guarantees both - this note only
 *       matters if you roll your own type-id scheme instead.
 * 
 * @see object_type_e
 */
#define VIRT_COMPOSER_REGISTER_TYPE(type) \
        constexpr virt_composer::object_type_e type{\
        virt_object::compile_unique_id<virt_composer::virt_tag_t>(), #type}

/*!
 * @def VC_REGISTER_MEMBER_OBJECT(vs, obj_type, memb)
 * @brief Macro to simplify registering a member object for Lua scripting.
 *
 * This macro expands to a call to `luaw_register_member_object`, registering the specified member
 * variable of `obj_type` for Lua access.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param memb      The member variable to register.
 *
 * @see luaw_register_member_object
 * @note Only known object types can be used in those calls, usual data types: string, bool, int,
 *       double (compatible with Lua), vector, tuples, pairs, and `vc::bm_t<T>` (see its own doc -
 *       one-way, Lua->C++ only), `vc::ref_t<T>` objects. In rest, this library doesn't know how to
 *       convert them from Lua to their C++ counterpart and vice versa.
 * 
 * @example
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_cmdpool)
 * VC_REGISTER_MEMBER_OBJECT(vs, cmdbuff_t, m_host_free)
 */
#define VC_REGISTER_MEMBER_OBJECT(vs, obj_type, memb)   \
virt_composer::luaw_register_member_object<             \
/* self        */   obj_type,                           \
/* member      */   &obj_type::memb>                    \
/* member name */   (vs, #memb)

/*!
 * @def VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs, obj_type, memb)
 * @brief Registers a trivially-copyable member for `!copy`/`resolve_memb<T>()` access from YAML.
 *
 * This macro expands to a call to `register_trivially_copyable_member`, registering a memcpy-based
 * accessor for the specified member variable of `obj_type`. It's used from inside a builder
 * callback, via `resolve_memb<T>(vs, node)`, to let a YAML field tagged `!copy` (with
 * `object`/`member` sub-fields) pull a raw value straight out of another already-built object's
 * member, byte for byte.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param memb      The member variable to register. Must be trivially copyable
 *                  (`std::is_trivially_copyable_v`) - enforced by a `static_assert`.
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
 */
#define VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER(vs, obj_type, memb)   \
virt_composer::register_trivially_copyable_member<                  \
/* self        */   obj_type,                                       \
/* member      */   &obj_type::memb>                                \
/* member name */   (vs, #memb)

/*!
 * @def VC_REGISTER_MEMBER_FUNCTION(vs, obj_type, fn, ...)
 * @brief Macro to simplify registering a member function for Lua scripting.
 *
 * This macro expands to a call to `luaw_register_member_function`, registering the specified member
 * function of `obj_type` for Lua invocation.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`)
 * @param obj_type  The C++ class type (must inherit from `virt_composer::object_t`).
 * @param fn        The member function to register.
 * @param ...       Parameter types of the member function.
 *
 * @see luaw_register_member_function 
 * @note Only known object types can be used in those calls, usual data types: string, bool, int,
 *       double (compatible with Lua), vector, tuples, pairs, and `vc::bm_t<T>` (see its own doc -
 *       one-way, Lua->C++ only), `vc::ref_t<T>` objects. In rest, this library doesn't know how to
 *       convert them from Lua to their C++ counterpart and vice versa.
 *
 * @example
 * VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin_rpass, vc::ref_t<vku::framebuffs_t>, uint32_t);
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
 * Negative values follow this codebase's general convention (see e.g. `debug.h`'s `ASSERT_FN`) of
 * "negative means failure" - `VC_ERROR_OK` (`0`) is success, any other value is a failure.
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
};

/*!
 * Identifies a Lua arithmetic/relational/misc metamethod slot (`__add`, `__eq`, `__unm`, ...).
 *
 * One `lua_CFunction` can be registered per `object_type_e` per entry here via
 * @ref set_class_operator. Every virt_composer object shares a single Lua metatable, so these
 * slots are the only per-type customization point for operators - see @ref set_class_operator
 * for the exact dispatch rules.
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
 * @brief The exception type virt_composer itself throws for framework-level errors.
 *
 * Thrown throughout the parser/Lua bridge for conditions specific to virt_composer's own model -
 * an invalid/unknown object type, a duplicate or missing name, a malformed YAML node shape, an
 * unknown enum string value, and similar. (A failed `ref_t<T>` cast throws plain
 * `std::runtime_error` instead, not this - see the note below for how that's reported differently.)
 * The constructor takes just the error message; it automatically prepends a C++
 * backtrace (`cpp_backtrace()`) to `err_str`, so `what()` already includes call-stack context
 * (a real backtrace on Linux/Unix when boost::stacktrace or <backtrace.h> is available, a fixed
 * placeholder string on MSVC - see cpp_backtrace.h).
 *
 * @note `luaw_catch_exception()` catches `except_t` specifically, ahead of the generic
 *       `std::exception` fallback, so an `except_t` thrown from inside a Lua-callable wrapper is
 *       reported to Lua as `"Invalid call: <message>"` rather than the more generic
 *       `"std::exception: <message>"`.
 */
struct except_t : public std::exception {
    std::string err_str;

    except_t(const std::string& str);
    const char *what() const noexcept override { return err_str.c_str(); };
};

/*!
 * @brief Opaque handle for one independent virt_composer instance - one Lua state, one pool of
 * named objects, one set of registered types/members.
 *
 * `virt_state_t` is fully defined in `virt_composer.cpp`, not this header - from user code it's
 * only ever seen as a pointer (`virt_state_t *`), obtained from `create_state()` and passed to
 * essentially every other function in this file.
 *
 * @note Two `virt_state_t` instances are fully independent - separate Lua states, separate name
 *       tables, separate object pools; nothing built in one is visible from the other. The one
 *       exception: `c_function_t::internal_funcs` (what `add_internal_func()` registers into)
 *       is a process-wide `static`, not per-instance - registering an internal function once
 *       makes it visible to every `virt_state_t` in the process, not just the one you had in mind.
 *
 * @warning Every `vc::object_t` obtained from a `virt_state_t` (via `get_ref`, `call_lua`, a member
 *       getter, ...) is only valid as long as that `virt_state_t` is alive - none of them are safe
 *       to keep around past its destruction (which closes the underlying Lua state).
 *       The caller is responsible for not letting that occur; nothing here enforces it.
 *
 * @see create_state, get_ref, parse_config, call_lua
 */
/* TODO: nothing currently enforces the @warning above - a ref_t<T> (especially ref_t<lua_object_t>,
which holds a raw lua_State* directly) that outlives its virt_state_t is a live use-after-free, not
a caught error. Worth investigating a real fix: a weak_ptr<virt_state_t> stashed alongside the raw
lua_State* so a stale reference can be detected and turned into a thrown exception instead of UB,
or some cheaper "generation counter"/invalidation scheme checked at each entry point
(release()/capture()/push()/call() for lua_object_t; to_related<T>()/get_ref<T>() more generally). */
struct virt_state_t;
struct virt_tag_t { static constexpr int off = VIRT_COMPOSER_UID_START_OFFSET; };

/*!
 * @brief Enumeration type for all objects derived from `vc::object_t`.
 *
 * This is a common type enumeration used to uniquely identify object types in the virt_composer
 * framework.
 * New types should be registered using @ref VIRT_COMPOSER_REGISTER_TYPE(name_of_enum), which
 * generates a unique enumerator for tracking within the implementation (up to including
 * `virt_composer_end.h`).
 *
 * @see VIRT_COMPOSER_REGISTER_TYPE
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
 * @brief Base object type for virt_composer.
 *
 * This type represents the base class for all objects in the virt_composer framework - every
 * `vc::object_t`-derived type (`integer_t`, `float_t`, `string_t`, and any user-defined type) is
 * ultimately a `virt_object::object_t<object_type_e>` (see `virt_object.h`'s generic
 * `object_t<Id>`, parameterized here by this file's own `object_type_e` type-id enum).
 */
using object_t = vo::object_t<object_type_e>;

/*!
 * @brief Return type for object operations.
 *
 * Alias for `virt_object::ret_t` (`int64_t`) - used as the return type of `init()`/`uninit()`-style
 * functions on objects derived from @ref object_t.
 */
using ret_t = vo::ret_t;

/*!
 * @brief Reference type for virt_composer objects.
 *
 * Template alias for a reference to an object of type `T` (must be inherited from
 * virt_composer::object_t)
 *
 * @tparam T The object type that is held by this reference.
 */
template <typename T>
using ref_t = vo::ref_t<T>;

/*!
 * @brief Template struct for handling bitmask/enum parameters from Lua ("bm" = bitmap/bitmask).
 *
 * This struct enables Lua scripts to pass bitmask or enum values in multiple formats:
 * - As a **string** (e.g., `"vc.READ"`), which is converted to the corresponding enum value.
 * - As an **integer** (e.g., `1`), which is cast to the enum type.
 * - As a **table** of strings/integers (e.g., `{vc.READ, vc.WRITE}`), which are combined into a
 * bitmask.
 *
 * It is typically used with @ref VC_REGISTER_MEMBER_FUNCTION or @ref luaw_function_wrapper,
 * to register functions that accept bitmask/enum parameters, allowing flexible input from Lua.
 *
 * @tparam T The enum/bitmask type (e.g., `OpenFlagBits`).
 *
 * @note The enum/bitmask type `T` must be convertible from both strings (via `get_enum_val<T>`) and
 * integers. (!! You need to specialize get_enum_val in virt_composer namespace to make it work !!)
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
 * @note `bm_t<T>` only describes how an incoming Lua value gets parsed into a `T` - it has no
 * distinct existence beyond that (it's stripped back down to plain `T` right after parsing). A
 * `T`-typed member or return value is pushed back to Lua as a plain enum value, never re-wrapped
 * through `bm_t<T>`'s string/table forms - this is a one-way (Lua->C++) conversion helper.
 */
template <typename T>
struct bm_t {
    using type = T;
};

/*!
 * vc::integer_t
 * -------------
 *
 * Wraps a 64-bit integer for bookkeeping or parameter storage. Useful for tracking values in a
 * reference-managed system.
 *
 * Members:
 * - value: The stored 64-bit integer.
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial integer value.
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
 * vc::float_t
 * -----------
 *
 * Wraps a double-precision floating-point value for bookkeeping or parameter storage. Useful for
 * tracking values in a reference-managed system.
 *
 * Members:
 * - value: The stored double-precision floating-point number.
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial floating-point value.
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
 * vc::string_t
 * ------------
 *
 * Wraps a standard string for bookkeeping or parameter storage. Useful for managing text values
 * in a reference-managed system.
 *
 * Members:
 * - value: The stored string.
 *
 * Init: create(value)
 *   - Parameters:
 *     - value: Initial string content.
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
 * vc::lua_script_t
 * ----------------
 *
 * Holds a Lua script as a string. Can be loaded or executed from C++ code using a Lua state,
 * enabling scripting functionality.
 *
 * Members:
 * - content: The text of the Lua script.
 *
 * Init: create(content)
 *   - Parameters:
 *     - content: The Lua script source code as a string.
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
 * vc::c_function_t
 * -----------------
 *
 * Represents a C++ function exposed to Lua, callable from Lua scripts via a Lua state.
 *
 * Members:
 * - m_name:   Name of the function as seen in Lua.
 * - m_source: Where the function comes from. Currently only `"[INTERNAL]"` works - it looks
 *             `m_name` up in the table `add_internal_func()` registers into. Any other value
 *             fails `create()` right now (DLL/shared-object loading is planned but not yet
 *             implemented).
 *
 * Member functions:
 * - call(L): Invokes the underlying C++ callback with the given Lua state; returns `-1` if
 *   nothing was ever successfully bound.
 *
 * Init: create(name, source)
 *   - Parameters:
 *     - name:   Name of the function in Lua.
 *     - source: Must currently be `"[INTERNAL]"` - see above.
 *   - Throws `vc::except_t` if `init()` fails (wrong `source`, or `name` was never registered
 *     via `add_internal_func()`).
 */
struct c_function_t : public vc::object_t {
    std::string m_name;
    std::string m_source;

    c_function_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~c_function_t() { uninit(); }

    static vc::ref_t<c_function_t> create(std::string name, std::string source);

    virtual vc::object_type_e type_id() const override { return VC_TYPE_C_FUNCTION; }
    static vc::object_type_e type_id_static() { return VC_TYPE_C_FUNCTION; }

    int call(lua_State *L);

    inline std::string to_string() const override {
        return std::format("vc::c_function[{}]: m_name={} m_source={}",
                (void*)this, m_name, m_source);
    }

    static void add_internal_func(std::string name, std::function<int(lua_State *L)> fn) {
        c_function_t::internal_funcs[name] = fn;
    }

private:
    std::function<int(lua_State *L)> _fn;
    static std::map<std::string, std::function<int(lua_State *L)>> internal_funcs;

    /* TODO: */
    static std::map<std::string, void *> dll_handles;
    static std::map<std::string, std::function<int(lua_State *L)>> dll_funcs;

    vc::ret_t init();
    vc::ret_t uninit() { return VC_ERROR_OK; }
};

inline std::map<std::string, std::function<int(lua_State *L)>>  c_function_t::internal_funcs;
inline std::map<std::string, std::function<int(lua_State *L)>>  c_function_t::dll_funcs;
inline std::map<std::string, void *>                            c_function_t::dll_handles;

/*!
 * vc::lua_object_t
 * ----------------
 * Holds a strong reference to an arbitrary Lua value (function, table, string, ...) so C++ can
 * keep it alive past the call that handed it over, and invoke/push it again later - the reverse
 * of c_function_t (a C++ callback exposed to Lua).
 *
 * - `create()` - factory, makes an empty shell with nothing captured.
 * - `capture_ref(L)` - replaces the held value with whatever is on top of `L`'s stack.
 * - `capture(oth)` - Lua-visible "capture": takes its own independent reference to `oth`'s value
 *   rather than aliasing `oth`'s registry slot (`oth` may be a reference someone else still holds).
 * - `capture_lua_object(L, ref, idx)` - duplicates the value at stack index `idx` into an
 *   existing `ref`.
 * - `push(L)` - pushes the captured value back onto `L`; also the Lua-visible "push",
 *   registered as a raw `lua_CFunction` (not via `VC_REGISTER_MEMBER_FUNCTION`) so it always gets
 *   the real calling `L` rather than `this->L` - needed to stay correct when called from a
 *   coroutine running on a different thread than the one this value was captured on.
 * - `call(L, nargs)` / `call<R>(args...)` - invokes the captured value as a function.
 *
 * @warning `push(L)` and `call(L, nargs)` raise Lua errors via `luaw_push_error()` on failure -
 * only safe to call where a `lua_pcall` is already active up the call stack (as a registered
 * `lua_CFunction`, or inside `call_lua()`).
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
     * previously held first. */
    void capture_ref(lua_State *L);

    /*! Lua-visible "capture" (`self:capture(x)`). Takes its own independent reference to `oth`'s
     * value rather than mutating `oth`'s registry slot directly, since `oth` may be a reference
     * someone else still holds. A missing/nil argument is a release. */
    void capture(vc::ref_t<lua_object_t> oth);

    /*! Pushes the captured value back onto `L`. Given the correct ambient `L` explicitly, so it
     * stays correct from a different thread (e.g. a coroutine) than the one this value was
     * captured on - also the Lua-visible "push", registered as a raw `lua_CFunction` for that
     * reason (the member-function macro has no way to supply the real calling `L`). */
    void push(lua_State *L);

    /*! Raw primitive: `nargs` argument values are already on `L` - pushes the captured callee
     * below them and pcalls with `LUA_MULTRET`. */
    int call(lua_State *L, int nargs);

    /*! `call_lua<R>()`-shaped convenience: typed C++ args in, typed C++ result out. */
    template <typename R, typename ...Args>
    std::pair<std::conditional_t<!std::is_void_v<R>, R, int>, err_e>
    call(Args&& ...args);

    /*! Unrefs whatever is currently held, if anything. */
    void release();

    /*! Duplicates the value at stack index `idx` and captures it into `ref` (which must already
     * exist, via `create()`). */
    static void capture_lua_object(lua_State *L, vc::ref_t<lua_object_t> ref, int idx);
};

/*!
 * @brief Human-readable description of an `object_type_e` value - just its registered name
 * (`VIRT_COMPOSER_REGISTER_TYPE`'s stringified `#type`), e.g. `"VC_TYPE_INTEGER"`.
 */
inline std::string to_string(object_type_e type);

/* To string for own objects: */

/*!
 * @brief Human-readable description of a virt_composer object reference: `"ref: "` followed by
 * the object's own `to_string()` (see `object_t::to_string()`, pure virtual - every concrete type
 * provides its own).
 */
template <typename T>
inline std::string to_string(ref_t<T> ref);

/*!
 * @brief Human-readable description of a virt_composer object: equivalent to calling
 * `ref.to_string()` directly.
 *
 * Convenience overload for when you already have a plain reference (not a `ref_t<T>`) - e.g. from
 * inside a member function of the object itself (`*this`), or after dereferencing a `ref_t<T>`.
 */
inline std::string to_string(const object_t& ref);

/*!
 * @brief Creates and returns a new shared pointer to a virtual state object.
 *
 * @return A shared pointer to the newly created @c virt_state_t object, or `nullptr` if this
 *         translation unit never included `virt_composer_end.h` (so `VIRT_TYPE_CNT` was never
 *         finalized), or if the underlying Lua state failed to initialize.
 */
std::shared_ptr<virt_state_t> create_state();

/*!
 * @brief Finds a previously-named object by name and casts it to the requested type.
 *
 * Looks `name` up in the virt_state_t's name table (populated by `mark_dependency_solved()` -
 * every top-level YAML entry, plus anything explicitly named via `vc.create_object(name, ...)`)
 * and casts the result to `T` via `object_t::to_related<T>()`.
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
 * @see get_ref_base (returns the untyped `ref_t<object_t>`, if you don't want the cast)
 */
template <typename T>
ref_t<T> get_ref(virt_state_t *vs, const std::string& name);

/*!
 * @brief Converts a YAML/Lua node to an enum/bitmask value of type `T`, given an explicit
 * string-name -> value table.
 *
 * Handles three node shapes:
 * - A **string** (e.g. `"READ"`): looked up in `enum_vals`; throws `vc::except_t` if not found.
 * - An **integer**: cast directly to `T` via `T(node.as_int())`.
 * - A **sequence** (e.g. `[READ, WRITE]`): each element resolved recursively and OR'd together,
 *   for combining bitmask flags.
 *
 * This is the helper `get_enum_val<T>(node)` (the single-argument, per-type overload declared
 * right below) is meant to be implemented in terms of - write your own `enum_vals` table once per
 * enum type and forward to this function, rather than reimplementing the string/integer/sequence
 * handling yourself. This is exactly the pattern every enum specialization in `vulkan/
 * vulkan_composer.h` uses, e.g.:
 * @code
 * inline std::unordered_map<std::string, VkImageTiling> vk_image_tiling_from_str = { ... };
 * template <> inline VkImageTiling get_enum_val<VkImageTiling>(fkyaml::node &n) {
 *     return get_enum_val(n, vk_image_tiling_from_str);
 * }
 * @endcode
 *
 * @tparam T  The enum/bitmask type to produce.
 * @param node       The YAML/Lua-derived node to convert.
 * @param enum_vals  Table mapping each valid string name to its `T` value.
 *
 * @throws vc::except_t if `node` is a string not present in `enum_vals`, or isn't a
 *         string/integer/sequence at all.
 *
 * @see get_enum_val(fkyaml::node&), bm_t
 */
template <typename T>
inline T get_enum_val(fkyaml::node &node, const std::unordered_map<std::string, T>& enum_vals);

/*!
 * @brief Per-enum-type entry point for enum/bitmask conversion - deleted by default, meant to be
 * specialized once per enum type you want to use with `bm_t<T>`/YAML.
 *
 * This primary template is intentionally `= delete`d: it is never meant to be called for a
 * generic, unspecialized `T` directly. To make your own enum type `T` usable as `bm_t<T>` (in
 * `VC_REGISTER_MEMBER_FUNCTION`/`luaw_function_wrapper`) or as a plain YAML/Lua-convertible enum
 * value, provide an explicit specialization that forwards to `get_enum_val<T>(node, enum_vals)`
 * with your own string-name table - see that function's doc for a real example from
 * `vulkan_composer.h`.
 *
 * @note This deletion is also what `is_vc_enum<T>` (a `requires` expression checking whether
 *       `get_enum_val<T>(n)` is well-formed) relies on: for a `T` with no specialization, the
 *       expression resolves to this deleted primary, calling a deleted function is ill-formed,
 *       and the `requires` expression is SFINAE-friendly about that - it just evaluates to
 *       `false` rather than hard-erroring, so `is_vc_enum<T>` correctly reports "not a known
 *       enum" instead of breaking compilation everywhere it's checked.
 *
 * @tparam T  The enum/bitmask type - must have its own explicit specialization to be usable.
 * @see get_enum_val(fkyaml::node&, const std::unordered_map<std::string,T>&), is_vc_enum, bm_t
 */
template <typename T>
inline T get_enum_val(fkyaml::node &n);

/* Virt Composer - YAML Parser API
------------------------------------------------------------------------------------------------- */

/*!
 * @brief Parses a YAML configuration file and builds a schema for the given virtual state.
 *
 * This function reads a YAML configuration file and constructs the virtual objects described in
 * said file, storing the resulting object in the virt_state_t object. References to those objects
 * can be retrieved and same objects can also be referenced in various LUA scripts.
 *
 * @param vs    Pointer to the virtual state structure to be populated with the parsed configuration.
 * @param path  Path to the YAML configuration file to parse.
 *
 * @return virt_composer::err_e
 *         - @c VC_ERROR_OK on success. Note: an unresolved `!ref` (naming an object that never
 *           gets built) does not cause a failure here - it's silently left unresolved.
 *         - @c VC_ERROR_PARSE_YAML if the YAML file itself is malformed (fails to deserialize).
 *         - @c VC_ERROR_GENERIC for any other failure - schema construction failure, or an object
 *           with an unrecognized `m_type`.
 *
 */
err_e parse_config(virt_state_t *vs, const char *path);

/*!
 * @brief Registers a named builder callback for typed objects.
 *
 * Adds a callback function to an internal array that will be invoked when a YAML node
 * with a matching `m_type` field is encountered during parsing. The callback is responsible
 * for constructing the object from the provided node.
 *
 * @param vs      Pointer to the virtual state.
 * @param match   The `m_type` string to match against YAML nodes.
 * @param builder The callback coroutine function to invoke when a match is found.
 *                Parameters: virtual state, node name, and the YAML node itself
 *
 * @return `VC_ERROR_OK` (this function currently has no failure path).
 *
 * @note Only typed objects (with an `m_type` field) can be nested. Auto-identified-objects cannot.
 */
err_e add_named_builder_callback(virt_state_t *vs, const std::string& match,
        std::function<co::task<vc::ref_t<vc::object_t>> (
                virt_state_t *, const std::string&, fkyaml::node&)> builder);

/*!
 * @brief Registers an automatic builder callback for auto-identified-objects.
 *
 * Adds a callback pair to an internal array that will be invoked when a YAML node
 * matches the structure recognized by the analyser function. The analyser function should
 * return `true` if the node structure matches, and the builder function will then be called
 * to construct the object.
 *
 * @param vs       Pointer to the virtual state.
 * @param analyser Function that checks if a YAML node matches the expected structure.
 * @param builder  Coroutine function to construct the object if the analyser returns `true`.
 *                 Parameters: virtual state, node name, and the YAML node itself.
 *                 Return: `0` on success, or a negative value on error.
 *
 * @return `VC_ERROR_OK` (this function currently has no failure path).
 *
 * @note Only typed objects (with an `m_type` field) can be nested. Auto-identified-objects cannot.
 */
err_e add_auto_builder_callback(virt_state_t *vs,
        std::function<bool(const std::string&, fkyaml::node& node)> analyser,
        std::function<co::task_t(virt_state_t *, const std::string&, fkyaml::node&)> builder);

/*!
 * Marks a dependency as resolved and notifies all coroutines waiting for it. To be used inside
 * builder callbacks.
 *
 * This function registers a newly constructed object in the virtual state (`virt_state_t`),
 * exposes it to Lua as `vc.<depend_name>`, and resumes any coroutines that were suspended while
 * waiting for this dependency.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`), which manages objects and
 *                      dependencies.
 * @param depend_name   The name/identifier of the dependency being resolved.
 * @param depend        The object reference (`vc::ref_t<vc::object_t>`) to register.
 *
 * @throws vc::except_t If the object is null or if the dependency name is already taken.
 *
 * @example
 * // After constructing an object, mark it as resolved:
 * mark_dependency_solved(vs, "my_object", my_object_ref);
 */
void mark_dependency_solved(virt_state_t *vs, std::string depend_name, vc::ref_t<vc::object_t> dep);

/*!
 * Asynchronously resolves a YAML node to an integer value, supporting both direct values and
 * references. To be used inside the build_object callback.
 *
 * This coroutine function resolves a YAML node to an `int64_t` value. It handles three cases:
 * 1. **Reference nodes** (e.g., `!ref object_name`): Resolves the referenced integer object.
 * 2. **String nodes**: Evaluates the string as a mathematical expression (using `texpr`).
 * 3. **Direct integer nodes**: Returns the integer value directly.
 *
 * @param vs    Pointer to the virtual state (`virt_state_t`), providing parsing context and
 *              dependency management.
 * @param node  The YAML node to resolve. Can be a reference, a string expression, or a direct
 *              integer.
 *
 * @return A coroutine task that yields the resolved `int64_t` value.
 *
 * @note
 * - Rounds the result of evaluated expressions to the nearest integer.
 *
 * @example
 * // Resolve a reference or expression:
 * int64_t val = co_await resolve_int(vs, yaml_node);
 */
co::task<int64_t> resolve_int(virt_state_t *vs, fkyaml::node& node);

/*!
 * Asynchronously resolves a YAML node to a floating-point value, supporting both direct values and
 * references. To be used inside the build_object callback.
 *
 * This coroutine function resolves a YAML node to a `double` value. It handles four cases:
 * 1. **Reference nodes** (e.g., `!ref object_name`): Resolves the referenced float object.
 * 2. **String nodes**: Evaluates the string as a mathematical expression (using `texpr`).
 * 3. **Integer nodes**: Cast directly to `double`.
 * 4. **Direct float nodes**: Returns the floating-point value directly.
 *
 * @param vs Pointer to the virtual state (`virt_state_t`), providing parsing context and dependency
 * management.
 * @param node The YAML node to resolve. Can be a reference, a string expression, an integer, or a
 * direct float.
 *
 * @return A coroutine task that yields the resolved `double` value.
 *
 * @example
 * // Resolve a reference or direct float:
 * double val = co_await resolve_float(vs, yaml_node);
 */
co::task<double> resolve_float(virt_state_t *vs, fkyaml::node& node);

/*!
 * Asynchronously resolves a YAML node to a string value, supporting both direct values and
 * references. To be used inside the build_object callback.
 *
 * This coroutine function resolves a YAML node to a `std::string` value. It handles two cases:
 * 1. **Reference nodes** (e.g., `!ref object_name`): Resolves the referenced string object.
 * 2. **Direct string nodes**: Returns the string value directly.
 *
 * @param vs Pointer to the virtual state (`virt_state_t`), providing parsing context and dependency
 * management.
 * @param node The YAML node to resolve. Can be a reference or a direct string.
 *
 * @return A coroutine task that yields the resolved `std::string` value.
 *
 * @example
 * // Resolve a reference or direct string:
 * std::string val = co_await resolve_str(vs, yaml_node);
 */
co::task<std::string> resolve_str(virt_state_t *vs, fkyaml::node& node);

/*!
 * Asynchronously resolves a YAML node into an object reference, handling direct references, tagged
 * mappings, and inlined object definitions. To be used inside the build_object callback.
 *
 * This coroutine function is used during configuration parsing to resolve a YAML node into a
 * strongly-typed reference (`vc::ref_t<T>`). It supports three cases:
 * 1. **Reference nodes** (e.g., `m_field: !ref object_name`).
 * 2. **Tagged mapping nodes** (e.g., `m_field: tag_name: m_type: "..."`).
 * 3. **Inlined object nodes** (e.g., `m_field: m_type: "..."`).
 *
 * @tparam T The expected type of the resolved object.
 * @param vs Pointer to the virtual state (`virt_state_t`), providing parsing context and
 * dependency management.
 * @param node The YAML node to resolve. The node can be a reference, a tagged mapping, or an
 * inlined object.
 *
 * @return A coroutine task that yields a `vc::ref_t<T>`, a reference to the resolved object.
 *         The coroutine suspends if the object is not yet available and resumes when it is ready.
 *
 * @throws vc::except_t If the node format is invalid or unsupported in the current context.
 *
 *
 * @example
 * // Resolve a reference:
 * auto ref = co_await resolve_obj<my_type_t>(vs, yaml_node);
 *
 */
template <typename T>
co::task<vc::ref_t<T>> resolve_obj(virt_state_t *vs, fkyaml::node& node);

/*!
 * Reads a trivially-copyable value out of another, already-built object's member, by raw memcpy.
 * Uses the coroutine engine purely to let objects be declared in whatever order is convenient in
 * YAML, not to wait on any real I/O or external event: if the source object hasn't been built yet,
 * this pauses construction of the *current* object and lets other objects keep being built,
 * resuming once the source object becomes available. To be used inside a builder callback, on a
 * YAML node tagged `!copy` with `object`/`member` sub-fields (e.g.
 * `value: !copy\n  object: some_vec3\n  member: x`).
 *
 * The target member must have been registered ahead of time with
 * `VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER`/`register_trivially_copyable_member` on the *source*
 * object's type - this function looks that registration up by the source object's runtime
 * `type_id()`, not by `T`, so `T` must match the exact type the member was registered with.
 *
 * @tparam T  The C++ type to copy the member's bytes into - must match the registered member's
 *            type (checked at runtime via `std::type_index`, not enforced at compile time here).
 * @param vs    Pointer to the virtual state (`virt_state_t`).
 * @param node  The YAML node - must be tagged `!copy` and contain `object`/`member` string fields
 *              naming the source object and its member.
 *
 * @return A coroutine task that yields the copied `T` value.
 *
 * @throws vc::except_t if `node` isn't tagged `!copy`, if the source object's type never
 *         registered `member` for trivial-copy access, or if the registered member's type doesn't
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
 */
template <typename T>
co::task<T> resolve_memb(virt_state_t *vs, fkyaml::node& node);

/* Virt Composer - LUA API
------------------------------------------------------------------------------------------------- */

/*!
 * @brief Adds free (non-member) functions to the `vc` Lua module table, callable as
 * `vc.<name>(...)` from any script.
 *
 * `create_state()` itself uses this to register `vc.create_object` internally; user code calls it
 * the same way to add its own top-level functions. Different from
 * `VC_REGISTER_MEMBER_FUNCTION`/`luaw_register_member_function`, which register a function ON a
 * specific object type (`obj:fn(...)`) - these live directly on the `vc` table itself, with no
 * receiver object.
 *
 * @param vs           Pointer to the virtual state (`virt_state_t`).
 * @param vc_tab_funcs The functions to add, as `{name, lua_CFunction}` pairs (`luaL_Reg`). For a
 *                     C++ function with automatic argument/return conversion, wrap it with
 *                     `luaw_function_wrapper<...>` first (see that function's doc) rather than
 *                     writing a raw `lua_CFunction` by hand.
 *
 * @return `VC_ERROR_OK` (this function currently has no failure path).
 *
 * @note Safe to call more than once - each call appends its functions to whatever was already
 *       registered, it doesn't replace the previous set.
 *
 * @example
 * add_lua_tab_funcs(vs, {{"my_func", luaw_function_wrapper<&my_free_function, int, int>}});
 * // Lua: vc.my_func(1, 2)
 */
err_e add_lua_tab_funcs(virt_state_t *vs, const std::vector<luaL_Reg>& vc_tab_funcs);

/*!
 * @brief Adds integer constants directly onto the `vc` Lua module table, e.g. `vc.READ = 1`.
 *
 * Makes `vc.READ`/`vc.WRITE`-style names usable as plain Lua expressions in scripts - without this,
 * `vc.READ` simply doesn't exist as a field on the `vc` table. This is one of two independent ways
 * a script can supply an enum/flag value to a `bm_t<T>` parameter: as this kind of named integer
 * constant (`vc.READ`, evaluated by Lua itself before the call happens), or as a bare string
 * literal (`"READ"`), resolved separately via a `get_enum_val<T>` specialization - see that
 * function's doc. The `unordered_map<std::string, T>` overload just below forwards to this one.
 *
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param mapping The constants to add, as `{integer_value, name}` pairs.
 *
 * @return `VC_ERROR_OK` (this function currently has no failure path).
 *
 * @example
 * add_lua_flag_mapping(vs, {{1, "READ"}, {2, "WRITE"}});
 * // Lua: vc.READ == 1, vc.WRITE == 2
 */
err_e add_lua_flag_mapping(virt_state_t *vs,
        const std::vector<std::pair<lua_Integer, std::string>> &mapping);

/*!
 * @brief Adds integer constants onto the `vc` Lua module table from an existing enum name table -
 * convenience wrapper around the `vector<pair<lua_Integer,string>>` overload.
 *
 * Converts `mapping` into that overload's `{value, name}` pair form and forwards to it. The
 * intended pattern is to reuse the *same* table you already wrote for a `get_enum_val<T>`
 * specialization - one `std::unordered_map<std::string, T>` backing both the C++-side
 * string-to-enum lookup and the Lua-side `vc.<NAME>` constants, rather than keeping two separate
 * lists in sync. Confirmed as the actual pattern in `vulkan_composer.h`: `shader_stage_from_string`
 * backs both `get_enum_val<vku_shader_stage_e>`'s specialization and this function's own
 * registration.
 *
 * @tparam T  The enum/flag type - only its integer values matter here, they're cast to
 *            `lua_Integer`.
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param mapping The name -> value table to expose as `vc.<name>` constants.
 *
 * @return `VC_ERROR_OK` (this function currently has no failure path).
 *
 * @see add_lua_flag_mapping(virt_state_t*, const std::vector<std::pair<lua_Integer,std::string>>&),
 *      get_enum_val
 */
template <typename T>
err_e add_lua_flag_mapping(virt_state_t *vs, const std::unordered_map<std::string, T>& mapping);

/**
 * @brief A Lua C function wrapper for calling C++ functions from Lua.
 *
 * This template generates a Lua-compatible C function that wraps a C++ function,
 * automatically converting Lua arguments to C++ types and handling return values.
 * It can recognize `vc::ref_t<T>` references.
 *
 * @tparam function The C++ function to wrap. Must be callable with the provided `Params...`.
 * @tparam Params   The types of the parameters expected by the wrapped function.
 *
 * @param L The Lua state.
 * @return int The number of values returned to Lua (0 for void, 1 otherwise).
 *
 * @note Exceptions thrown by `function` are caught and turned into a Lua error - no need for your
 *       own try/catch around it.
 */
template <auto function, typename ...Params>
inline int luaw_function_wrapper(lua_State *L);

/*!
 * @brief Registers a member function of a C++ class for Lua scripting.
 *
 * This template function registers a member function of a C++ class (which must inherit from
 * `virt_composer::object_t`) so that it can be called from Lua. It bridges the C++ member function
 * to Lua, allowing Lua scripts to invoke the function on objects of the registered type.
 *
 * @tparam T            The C++ class type (must inherit from `virt_composer::object_t`).
 * @tparam member_ptr   Pointer to the member function to register.
 * @tparam Params       Parameter types of the member function.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`)
 * @param function_name The name of the function as it will be exposed in Lua.
 *
 * @note This is typically used with the @ref VC_REGISTER_MEMBER_FUNCTION macro for convenience.
 * @note Only known object types can be used in those calls, usual data types: string, bool, int,
 *       double (compatible with Lua), vector, tuples, pairs, and `vc::bm_t<T>` (see its own doc -
 *       one-way, Lua->C++ only), `vc::ref_t<T>` objects. In rest, this library doesn't know how to
 *       convert them from Lua to their C++ counterpart and vice versa.
 *
 * @see VC_REGISTER_MEMBER_FUNCTION
 *
 * @example
 * // C++:
 * VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin_rpass, vc::ref_t<vku::framebuffs_t>, uint32_t);
 *
 * struct cmdbuff_t : public vc::object_t {
 *     ref_t<cmdpool_t>    m_cmdpool;
 *     bool                m_host_free;
 *     void begin_rpass(ref_t<framebuffs_t> fbs, uint32_t img_idx);
 * };
 *
 * // Lua:
 * vc = require("virt_composer")
 * vc.cmdbuff:begin_rpass(vc.fb, 2)  -- Calls the registered member function
 *                                   -- cmdbuff is a reference (vc::ref_t<cmdbuff_t> in C++)
 */
template <typename T, auto member_ptr, typename ...Params>
void luaw_register_member_function(virt_state_t *vs, const char *function_name);

/*!
 * @brief Registers a member object (variable) of a C++ class for Lua scripting.
 *
 * This template function registers a member variable of a C++ class (which must inherit from
 * `virt_composer::object_t`) so that it can be accessed and modified from Lua scripts.
 *
 * @tparam T            The C++ class type (must inherit from `virt_composer::object_t`).
 * @tparam member_ptr   Pointer to the member variable to register.
 *
 * @param vs            Pointer to the virtual state (`virt_state_t`)
 * @param member_name   The name of the member variable as it will be exposed in Lua.
 *
 * @note This is typically used with the @ref VC_REGISTER_MEMBER_OBJECT macro for convenience.
 * @note Only known object types can be used in those calls, usual data types: string, bool, int,
 *       double (compatible with Lua), vector, tuples, pairs, and `vc::bm_t<T>` (see its own doc -
 *       one-way, Lua->C++ only), `vc::ref_t<T>` objects. In rest, this library doesn't know how to
 *       convert them from Lua to their C++ counterpart and vice versa.
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
 */
template <typename T, auto member_ptr>
void luaw_register_member_object(virt_state_t *vs, const char *member_name);

/*!
 * @brief Tells the framework that one registered object type is a base of another, so members
 * registered on the base become visible on the derived type too.
 *
 * By default, every registered type (`VIRT_COMPOSER_REGISTER_TYPE`) is its own island as far as
 * Lua/yaml member access is concerned: a member registered with `VC_REGISTER_MEMBER_FUNCTION`,
 * `VC_REGISTER_MEMBER_OBJECT`, or `VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER` on type `T` is only
 * reachable through an object whose `type_id()` is exactly `T`. `register_inheritance<T, U>(vs)`
 * teaches the framework that `T` and `U` are actually related - whichever one is the real C++
 * base (`T` or `U`, order doesn't matter, it's detected automatically) has its members exposed to
 * the derived type as well. This mirrors real C++ inheritance for the *scripting* surface, on top
 * of a C++ hierarchy you must already have: `T` and `U` are required (`std::is_base_of_v<T, U>`
 * or `std::is_base_of_v<U, T>`, checked at compile time) to be genuinely related in C++ - this
 * function does not, and cannot, invent an inheritance relationship between two unrelated types.
 *
 * @tparam T One of the two related types (must inherit from `virt_composer::object_t`).
 * @tparam U The other related type (must inherit from `virt_composer::object_t`).
 *
 * @param vs Pointer to the virtual state (`virt_state_t`).
 *
 * @note **Call order matters relative to member registration, not just relative to other
 *       `register_inheritance` calls.** Propagation happens once, at the moment a member/operator
 *       is registered (`set_lua_class_member()`/`set_class_member_setter()`/
 *       `set_trivial_copy_member()`/`set_class_operator()` all copy it into every type currently
 *       known to descend from it) - it is not a live/lazy link. A member added to the base *before*
 *       `register_inheritance()` establishes the relation will never retroactively reach the
 *       derived type. Always call `register_inheritance()` for a pair before registering members
 *       on the base you want the derived type to inherit.
 *
 * @note **This call is not transitive across a hierarchy deeper than the one pair you give it.**
 *       For a chain `A <- B <- C`, calling `register_inheritance<A,B>(vs)` then
 *       `register_inheritance<B,C>(vs)` does *not* also make `A`'s members visible on `C`, even
 *       though `C` genuinely is an `A` in C++ - each call only links the exact two types passed
 *       to it. Since `std::is_base_of_v` (and therefore this function's own type constraint) is
 *       satisfied for *any* ancestor/descendant pair regardless of how many levels separate them,
 *       the fix is simply to register every pair you actually need visible, not just the adjacent
 *       links: `register_inheritance<A,B>(vs); register_inheritance<B,C>(vs);
 *       register_inheritance<A,C>(vs);` for a 3-level hierarchy where `A`'s members must reach
 *       `C` too.
 *
 * @see VC_REGISTER_MEMBER_FUNCTION, VC_REGISTER_MEMBER_OBJECT, VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER,
 *      set_class_operator
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
 */
template <typename T, typename U>
requires std::is_base_of_v<vc::object_t, T> && std::is_base_of_v<vc::object_t, U>
void register_inheritance(virt_state_t *vs);

/*!
 * Registers `fn` as the handler for Lua operator `op` (e.g. `VC_OPERATOR_ADD` for `a + b`) on
 * objects of `type`.
 *
 * `fn` is a plain `lua_CFunction` - it is NOT wrapped/generated the way `VC_REGISTER_MEMBER_FUNCTION`
 * wraps a C++ member function pointer. This is intentional: the two operands of a Lua operator can
 * be any mix of vc objects and plain Lua values (e.g. `vc_obj + 5`, or two different vc types), so
 * there's no single fixed C++ signature to template over - `fn` gets the raw Lua stack and decides
 * for itself what to do with each operand (via `get_object_from_lua`, `lua_tonumber`, etc.).
 *
 * @par Dispatch (binary operators: ADD, SUB, MUL, DIV, MOD, POW, IDIV, BAND, BOR, BXOR, SHL, SHR,
 * CONCAT, EQ, LT, LE)
 * Both operands are checked in order (operand 1, then operand 2) for whichever one has an operator
 * registered for this `op`, then:
 *  - Pushes one extra argument, a 3rd stack slot: `1` if operand 1 was the one with the registered
 *    handler, `2` if operand 2 was. This tells `fn` which side triggered the call, since Lua always
 *    passes both operands in original left-to-right order either way (needed to get non-commutative
 *    operators like SUB right regardless of which side dispatched).
 *  - Calls `fn(L)` with stack `[operand1, operand2, which]`, and returns whatever `fn` returns,
 *    unmodified - `fn` follows the normal `lua_CFunction` return contract (push results, return
 *    their count); no manual stack cleanup needed.
 *  - If neither operand has a handler for `op`, raises a Lua error.
 *
 * @par Dispatch (unary operators: UNM, BNOT, LEN)
 * Only one operand exists, so there's no ambiguity and no `which` argument is pushed - `fn` is
 * called with stack `[operand1]`.
 *
 * @note Like `VC_REGISTER_MEMBER_FUNCTION`/`VC_REGISTER_MEMBER_OBJECT`, registering an operator on
 *       `type` also reaches every type already linked to it via `register_inheritance()` - call
 *       `register_inheritance()` for the pair first, or a derived type registered afterward won't
 *       pick up the operator.
 *
 * @param vs   Virtual state context.
 * @param type The enumerated type of the C++ class (must be registered with
 *             @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param op   Which operator slot to bind (see @ref operator_e).
 * @param fn   Raw Lua C function implementing the operator for this type.
 *
 * @see operator_e
 */
void set_class_operator(virt_state_t *vs, object_type_e type, operator_e op, lua_CFunction fn);

/* TODO: add the functions to add the exception callbacks */

/*!
 * @brief Pushes a virt_composer object onto the Lua stack, as a value Lua's garbage collector
 * actually tracks.
 *
 * Repeated pushes of the same object return the same Lua value (so `==` between them holds in
 * Lua), and the object is kept alive for exactly as long as Lua can still reach that value - once
 * nothing references it anymore, it's eligible for collection like any other Lua-owned object.
 *
 * @param L      Lua state.
 * @param object The virt_composer object to push.
 *
 * @return `0` always (this function currently has no failure path).
 *
 * @see get_object_from_lua (the inverse)
 */
int push_vc_object(lua_State *L, ref_t<object_t> object);

/*!
 * @brief Retrieves the `vc::object_t*` a Lua stack value represents, or `nullptr` if it isn't one.
 *
 * The inverse of `push_vc_object()`: given a Lua stack index, returns the underlying `object_t*` if
 * the value there is a virt_composer object, or `nullptr` for anything else. Useful when writing
 * your own raw `lua_CFunction` (e.g. an operator handler registered via `set_class_operator()`)
 * that needs to inspect its arguments.
 *
 * @param L    The Lua state.
 * @param idx  Stack index of the value to inspect.
 *
 * @return The object's `object_t*`, or `nullptr` if the value at `idx` isn't a virt_composer
 *         object.
 *
 * @see push_vc_object
 */
object_t *get_object_from_lua(lua_State *L, int idx);

/*!
 * @brief Calls a global Lua function by name and converts its result back to C++.
 *
 * Looks `function_name` up as a **global** (via `lua_getglobal`) - a function nested in a table
 * (e.g. `vc.foo`) or a script-local one won't be found this way, and the call fails the same as an
 * unknown name would.
 *
 * @tparam R            Return type to convert the Lua function's result into. Pass `void` if the
 *                       return value should be ignored - the returned pair's first element is then
 *                       a meaningless placeholder `int` (always `0`), not an actual return value.
 * @tparam Args         The types of the function parameters.
 *
 * @param vs            The virtual state that contains the function.
 * @param function_name The name of the global Lua function to call.
 * @param args...       The arguments to pass, pushed onto the Lua stack in order before the call.
 *
 * @return A pair: the converted return value (or the `void` placeholder above), and `VC_ERROR_OK`
 *         on success or `VC_ERROR_FAILED_CALL` if `function_name` doesn't resolve to a callable
 *         value, or the call itself errors.
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
 * [INTERNAL] Asynchronously builds an object from a YAML node using registered callbacks.
 *
 * @param vs Virtual state context.
 * @param name Object name (used for registration and debugging).
 * @param node YAML node defining the object (must be a mapping with `m_type`).
 *
 * @return Coroutine task yielding a `vc::ref_t<vc::object_t>`.
 *         Returns `nullptr` if the node is not a mapping.
 *
 * @throws vc::except_t if no callback matches the object type.
 *
 * @details
 * - Iterates through `build_object_cbks` to find a matching callback.
 * - Delegates construction to the callback and suspends if needed.
 * - Logs errors for invalid nodes or unknown types.
 *
 * @note
 * - For internal use only (parser/dependency resolution system).
 */
co::task<vc::ref_t<vc::object_t>> build_object(virt_state_t *vs,
        const std::string& name, fkyaml::node& node);

/*!
 * [INTERNAL] Asynchronously builds a pseudo-object from a YAML node without requiring an explicit type.
 *
 * Pseudo-objects are a simplified way to create objects without boilerplate, supporting:
 * - Integers (creates an `integer_t` object).
 * - Floats (creates a `float_t` object).
 * - Strings (creates a `string_t` object).
 * - A node named exactly `"lua_script"`: loaded and executed as a Lua script (same underlying
 *   mechanism as a `vc::lua_script_t`'s `m_source`/`m_source_path`, just without needing the
 *   explicit `m_type` tag).
 *
 * For specialized objects (e.g., SPIR-V shaders, GPU resources), callbacks (`build_psudo_object_cbks`)
 * are used instead.
 *
 * @param vs Virtual state context.
 * @param name Name of the object to build.
 * @param node YAML node defining the object.
 *
 * @return Coroutine task yielding:
 *         - `0` on success (object built and registered).
 *         - `-1` on failure (invalid node or unsupported type).
 */
co::task_t build_pseudo_object(virt_state_t *vs, const std::string& name, fkyaml::node& node);

/*!
 * [INTERNAL] Generates a unique anonymous name for untagged objects.
 *
 * @param vs Virtual state context - mutates `vs`'s anonymous-name counter each call.
 * @return A unique name of the form `"__<N>"` (e.g. `"__0"`, `"__1"`, ...), where `N` is a
 *         per-virt_state_t counter incremented on every call.
 */
std::string new_anon_name(virt_state_t *vs);

/*!
 * [INTERNAL] Pushes a formatted error message with stack trace context to Lua and raises a Lua
 * error.
 *
 * This function constructs a detailed error message by capturing the Lua call stack,
 * including source file names, line numbers, and the relevant line of code (if available).
 * The error message is then pushed to the Lua stack and raised as a Lua error.
 *
 * @param L       The Lua state.
 * @param err_str The error message to include in the error output.
 *
 * @details
 * The function walks the Lua call stack to gather context information for each stack frame,
 * such as the source file, line number, and the actual line of code where the error occurred.
 * The resulting error message is a concatenation of the stack trace and the provided error string.
 * The error is then pushed to the Lua stack and raised using `lua_error`.
 *
 * @note
 * If the source file cannot be read or the line number is invalid, "<unknown>" is used as a placeholder.
 *
 * @see lua_Debug, lua_getstack, lua_getinfo, lua_error
 */
void luaw_push_error(lua_State *L, const std::string& err_str,
        const std::source_location sloc = std::source_location::current());

/*!
 * [INTERNAL] Catches C++ exceptions and propagates them as Lua errors.
 *
 * This function is designed to be called from Lua C function wrappers to handle
 * exceptions thrown during the execution of wrapped C++ functions. It catches
 * exceptions of various types and converts them into Lua errors, ensuring that
 * exceptions do not escape into Lua and break the Lua state.
 *
 * @param L The Lua state.
 * @return int Always returns 0, as the function either propagates a Lua error or re-throws.
 *
 * @details
 * - TODO: callback can be called here, to check if the error can be handled by the user
 * - Catches `fkyaml::exception`, `std::exception`, vc::except_t, converting them to Lua errors with
 * descriptive messages.
 * - Re-throws any other exceptions, which are assumed to be Lua errors already.
 *
 * @note
 * This function is intended to be used in `try`/`catch` blocks within Lua C function wrappers.
 * It ensures that C++ exceptions are safely converted to Lua errors, preventing Lua state corruption.
 *
 */
int luaw_catch_exception(lua_State *L);

/*!
 * [INTERNAL] Retrieves the `vc::virt_state_t` pointer stored in the Lua registry from inside the
 * LUA State. L references vs and vice-versa.
 *
 * @param L The Lua state.
 * @return vc::virt_state_t* Pointer to the `virt_state_t` object stored in the Lua registry.
 *
 */
virt_state_t *luaw_get_virt_state(lua_State *L);

/*!
 * [INTERNAL] Retrieves the `lua_state` pointer stored in the vc::virt_state_t object. L
 * references vs and vice-versa.
 *
 * @param vs The virt_state_t pointer.
 * @return lua_State* Pointer to the Lua state object stored in the virtual state.
 *
 */
lua_State *luaw_get_lua_state(virt_state_t *vs);

/*!
 * [INTERNAL] Registers a type-erased, memcpy-based accessor for a trivially-copyable member -
 * the low-level primitive `register_trivially_copyable_member<T, member_ptr>()`/
 * `VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER` build on top of.
 *
 * Stores `copy_fn` (expected to memcpy the member's bytes into the caller-supplied destination
 * buffer) under `type`/`member_name`, for later lookup by `resolve_memb_data()` (which
 * `resolve_memb<T>()`/the `!copy` YAML tag use). Participates in the same base->derived member
 * propagation as `set_lua_class_member()`/`set_class_member_setter()`: it writes into every type
 * currently in `inheritance_table[type]`, so the same registration-order rule applies -
 * `register_inheritance()` must be called before this for the member to reach a derived type too.
 *
 * @param vs           Virtual state context.
 * @param type         The enumerated type of the C++ class the member belongs to (must be
 *                      registered with @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name  The name of the member, as referenced from a YAML `!copy` tag's `member`
 *                      field.
 * @param tid          `std::type_index` of the member's actual C++ type - checked against the
 *                      caller's requested `T` at `resolve_memb<T>()` time, so a mismatch is
 *                      caught rather than silently memcpy'd into the wrong-sized destination.
 * @param copy_fn      Type-erased copy function: given the source object and a destination
 *                      buffer/size, copies the member's raw bytes into it.
 */
void set_trivial_copy_member(virt_state_t *vs, object_type_e type, const char *member_name,
        std::type_index tid, std::function<void(vc::object_t *, void *, size_t)> copy_fn);

/*!
 * [INTERNAL] Registers a Lua-accessible member (function or object) for a C++ class type.
 *
 * This function binds a C++ class member (function or variable) to Lua, making it callable or
 * accessible from Lua scripts. The member is associated with the specified `type` and `member_name`.
 *
 * @param vs            Virtual state context.
 * @param type          The enumerated type of the C++ class (must be registered with 
 *                      @refVIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name   The name of the member as it will be exposed in Lua.
 * @param fn            The Lua C function wrapper for the member.
 * @param member_type   The type of member (@ref luaw_member_e: function or object).
 */
void set_lua_class_member(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn, luaw_member_e member_type);

/*!
 * [INTERNAL] Registers a setter function for a Lua-accessible member object of a C++ class type.
 *
 * This function binds a setter for a C++ class member variable, allowing its value to be modified
 * from Lua. The setter is associated with the specified `type` and `member_name`.
 *
 * @param vs            Virtual state context.
 * @param type          The enumerated type of the C++ class (must be registered with 
 *                      @ref VIRT_COMPOSER_REGISTER_TYPE).
 * @param member_name   The name of the member as it will be exposed in Lua.
 * @param fn            The Lua C function wrapper for setting the member's value.
 */
void set_class_member_setter(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn);

/*!
 * [INTERNAL] Raw bookkeeping behind `register_inheritance<T,U>()` - records `derived` as
 * inheriting `base`'s registered members, with no compile-time relationship check.
 *
 * `register_inheritance<T,U>()` is what actually enforces `std::is_base_of_v<T,U>` (at compile
 * time, via its `requires` clause) before figuring out which of `T`/`U` is genuinely the base and
 * calling this function with `base`/`derived` in the right order - this function itself performs
 * no such check, so calling it directly can link two `object_type_e` values that have no real C++
 * relationship at all. See `register_inheritance()`'s doc for the full behavior this produces
 * (including the registration-order and non-transitivity caveats) - this function is the same
 * mechanism, just without the type safety.
 *
 * @param vs      Pointer to the virtual state (`virt_state_t`).
 * @param base    The type whose members should also become visible on `derived`.
 * @param derived The type that should inherit `base`'s members.
 */
void set_base_derived_relation(virt_state_t *vs, object_type_e base, object_type_e derived);

/*!
 * @brief Finds a previously-named object by name, without casting it to any particular type.
 *
 * Looks `name` up in the virt_state_t's name table (the same lookup `get_ref<T>()` uses
 * internally) and returns it as a plain `ref_t<object_t>` - no `to_related<T>()` cast applied, so
 * this never throws on a type mismatch the way `get_ref<T>()` can. Useful when you don't know (or
 * don't care about) the object's concrete type, or want to do your own type check/cast.
 *
 * @param vs    Pointer to the virtual state (`virt_state_t`).
 * @param name  The name the object was registered under.
 *
 * @return A `ref_t<object_t>` to the object, or `nullptr` if no object is registered under `name`.
 *
 * @see get_ref
 */
ref_t<vc::object_t> get_ref_base(virt_state_t *vs, const std::string& name);

/* See get_ref()'s declaration above for its doc comment. */
template <typename T>
ref_t<T> get_ref(virt_state_t *vs, const std::string& name) {
    auto base = get_ref_base(vs, name);
    return base ? base->to_related<T>() : nullptr;
}

/*!
 * [INTERNAL] Type-erased coroutine behind `resolve_memb<T>()` - does the actual dependency wait,
 * type check, and memcpy.
 *
 * `resolve_memb<T>()` is a thin wrapper around this: it declares `T ret;` and calls this with
 * `&ret, sizeof(T), typeid(T)`. See `resolve_memb<T>()`'s doc for the full user-facing behavior
 * (suspend/resume ordering, registration requirements, error conditions) - this function is where
 * that's actually implemented, for callers that want to work with a raw destination
 * buffer/size/type_index instead of a template parameter.
 *
 * @param vs        Pointer to the virtual state (`virt_state_t`).
 * @param obj_name  Name of the source object to copy the member from (waits for it to be built if
 *                  it isn't yet).
 * @param memb_name Name of the member to copy, as registered via `set_trivial_copy_member()`.
 * @param dst       Destination buffer to memcpy the member's bytes into.
 * @param sz        Size, in bytes, of `dst` (and of the copy).
 * @param tid       Expected `std::type_index` of the member - must match what it was registered
 *                  with, or this throws.
 *
 * @throws vc::except_t if `memb_name` was never registered for `obj_name`'s type, or if `tid`
 *         doesn't match the registered member's type.
 */
co::task_t resolve_memb_data(virt_state_t *vs, const std::string &obj_name,
        const std::string& memb_name, void *dst, size_t sz, std::type_index tid);

/*!
 * [INTERNAL] Non-templated core of the dependency resolver.
 *
 * This struct provides the low-level functionality required by the parser to manage
 * object dependencies during configuration parsing. It is used internally by the
 * templated `depend_resolver_t` to handle waiting, checking, and retrieving dependencies.
 *
 * @note
 * This is an internal utility and should not be used directly outside the parser or
 * dependency resolution system.
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
 * [INTERNAL] Awaitable that suspends the calling coroutine until an object named `required_depend`
 * has been built, then resolves it to a `ref_t<T>`.
 *
 * The low-level mechanism `resolve_int()`/`resolve_float()`/`resolve_str()`'s `!ref` handling and
 * `resolve_obj<T>()` all build on: `await_ready()` checks whether the dependency is already
 * registered (via `depend_resolver_internal_t::internal_check_depend()`); if not, `await_suspend()`
 * parks the caller on the wait queue for that name (`internal_mark_wait()`) and yields to the next
 * runnable coroutine, to be resumed later by `mark_dependency_solved()` once an object with that
 * name is registered; `await_resume()` then looks the object up and casts it to `T` via
 * `to_related<T>()`, throwing `vc::except_t` if the resolved object isn't actually a `T`.
 *
 * @tparam T  The expected type of the resolved object.
 * @see resolve_obj, mark_dependency_solved
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
 * [INTERNAL] Template for converting Lua values to C++ types.
 *
 * Specializations of this template handle conversion of Lua values (at a given stack index)
 * to C++ types. Unsupported types will trigger a static assertion.
 *
 * @tparam Param The C++ type to convert to.
 * @tparam index The Lua stack index of the value to convert.
 */
template <typename Param, ssize_t index>
struct luaw_param_t{
    void luaw_single_param(lua_State *L) {
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
template <ssize_t index>
struct luaw_param_t<void *, index> {
    void *luaw_single_param(lua_State *L) {
        // DBG("void* at index: %zd", index);
        if (lua_isnil(L, index))
            return NULL;
        return lua_touserdata(L, index);
    }
};

/* This resolves userdata(vc::ref) received from lua to an vc parameter */
template <typename T, ssize_t index>
struct luaw_param_t<vc::ref_t<T>, index> {
    vc::ref_t<T> luaw_single_param(lua_State *L) {
        // DBG("Ref at index: %zd", index);
        if constexpr (std::is_same_v<T, lua_object_t>) {
            if (auto obj = get_object_from_lua(L, index);
                    obj && obj->type_id() == lua_object_t::type_id_static())
                return obj->to_related<lua_object_t>();
            auto obj = lua_object_t::create();
            lua_object_t::capture_lua_object(L, obj, index);
            return obj;
        } else {
            if (lua_isnil(L, index))
                return vc::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
            auto obj = get_object_from_lua(L, index);
            if (!obj)
                luaw_push_error(L, std::format("Expected userdata at index {}", index));
            return obj->to_related<T>();
        }
    }
};

/* This resolves bitmasks received from lua to an vc parameter */
template <typename T, ssize_t index>
struct luaw_param_t<bm_t<T>, index> {
    std::function<void (lua_State *, const std::string&, const std::source_location)> throw_error =
            luaw_push_error;

    T luaw_single_param(lua_State *L) {
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
template <ssize_t index>
struct luaw_param_t<bool, index> {
    bool luaw_single_param(lua_State *L) {
        return lua_toboolean(L, index);
    }
};

/* This resolves integers received from lua to an vc parameter */
template <std::integral Integer, ssize_t index>
struct luaw_param_t<Integer, index> {
    Integer luaw_single_param(lua_State *L) {
        return lua_tointeger(L, index);
    }
};

/* This resolves floats received from lua to an vc parameter */
template <std::floating_point Float, ssize_t index>
struct luaw_param_t<Float, index> {
    Float luaw_single_param(lua_State *L) {
        // DBG("Float at index: %zd", index);
        int valid = 0;
        Float ret = lua_tonumberx(L, index, &valid);
        if (!valid) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to float from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
        }
        return ret;
    }
};

/* This resolves strings received from lua to an vc parameter */
template <ssize_t index>
struct luaw_param_t<const char *, index> {
    const char *luaw_single_param(lua_State *L) {
        // DBG("char* at index: %zd", index);
        const char *ret = lua_tostring(L, index);
        if (!ret) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to string from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
        }
        return ret;
    }
};

/*!
 * [INTERNAL] Helper template to remove `bm_t` wrappers from types.
 *
 * Used to normalize types for tuple/pair/vector specializations of `luaw_param_t`.
 *
 * @tparam T The type to process.
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

/* Nil at `index` produces a default-constructed (empty) tuple rather than failing. Otherwise
expects a table of exactly `sizeof...(Args)` elements: pushes them in reverse (len down to 1), so
after the loop the stack top is element 0, next is element 1, etc. - that's why the pack expansion
below reads each one via a negative index (-I-1), letting the whole tuple be constructed in one
expression instead of building it element by element. */
template <typename ...Args, ssize_t index>
struct luaw_param_t<std::tuple<Args...>, index> {
    template <size_t ...I>
    auto _luaw_single_param_impl(lua_State *L, std::index_sequence<I...>) {
        typename de_bitmaptizize<std::tuple<Args...>>::Type ret;
        if (lua_isnil(L, index))
            return ret;
        if (!lua_istable(L, index)) {
            luaw_push_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index));
        }
        int abs_idx = lua_absindex(L, index);
        int len = lua_rawlen(L, index);
        for (int i = len; i >= 1; i--)
            lua_rawgeti(L, abs_idx, i);
        ret = typename de_bitmaptizize<std::tuple<Args...>>::Type{
                luaw_param_t<Args, -ssize_t(I)-1>{}.luaw_single_param(L)...};
        lua_pop(L, len);
        return ret;
    }

    auto luaw_single_param(lua_State *L) {
        // DBG("Tuple at index: %zd", index);
        return _luaw_single_param_impl(L, std::index_sequence_for<Args...>{});
    }
};

/* Delegates to the tuple specialization above (reads the same 2-element table as
std::tuple<Arg1,Arg2>) and unpacks the result into a pair, rather than duplicating its stack
handling. */
template <typename Arg1, typename Arg2, ssize_t index>
struct luaw_param_t<std::pair<Arg1, Arg2>, index> {
    auto luaw_single_param(lua_State *L) {
        // DBG("Pair at index: %zd", index);
        auto tuple = luaw_param_t<std::tuple<Arg1, Arg2>, index>{}.luaw_single_param(L);
        typename de_bitmaptizize<std::pair<Arg1, Arg2>>::Type ret =
                {std::get<0>(tuple), std::get<1>(tuple)};
        return ret;
    }
};

/* Nil at `index` produces an empty vector rather than failing. Otherwise expects a table, and
(unlike the tuple specialization above) converts one element at a time - push, convert, pop - since
the element count isn't known at compile time so there's no single pack-expansion construction to
build. */
template <typename T, ssize_t index>
struct luaw_param_t<std::vector<T>, index> {
    auto luaw_single_param(lua_State *L) {
        // DBG("Vector at index: %zd", index);
        typename de_bitmaptizize<std::vector<T>>::Type ret;
        if (lua_isnil(L, index))
            return ret;
        if (!lua_istable(L, index)) {
            luaw_push_error(L, std::format("Invalid object of type: {} at index {}",
                    lua_typename(L, lua_type(L, index)), index));
        }
        int len = lua_rawlen(L, index);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            ret.push_back(luaw_param_t<T, -1>{}.luaw_single_param(L));
            lua_pop(L, 1);
        }
        return ret;
    }
};

/*!
 * [INTERNAL] Template for pushing C++ return values to Lua.
 *
 * Specializations of this template handle pushing C++ values of type `T` to the Lua stack.
 * Unsupported types will trigger a static assertion.
 *
 * @tparam T The C++ type to push to Lua.
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

template <typename T>
struct luaw_returner_t<std::vector<T>> {
    void luaw_ret_push(lua_State *L, const std::vector<T>& v) {
        lua_createtable(L, v.size(), 0);
        for (int i = 0; i < v.size(); i++) {
            luaw_returner_t<std::decay_t<T>>{}.luaw_ret_push(L, v[i]);
            lua_rawseti(L, -2, i+1);
        }
    }
};

/* [INTERNAL] Shared body behind luaw_function_wrapper<function,Params...>() - converts each Lua
argument via luaw_param_t<Params,I+1> (Params start at Lua stack index 1), calls `function`, and
(if it returns non-void) pushes the result via luaw_returner_t. Returns the count of Lua return
values (0 or 1), matching the lua_CFunction contract. */
template <auto function, typename ...Params, size_t ...I>
inline int luaw_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    using RetType = decltype(function(
            luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...));

    // ([L]{
    //     DBG("Index: %zu -> (%s, %s)", I + 1, demangle<Params>().c_str(),
    //             lua_typename(L, lua_type(L, I + 1)));
    // }(), ...);

    if constexpr (std::is_void_v<RetType>) {
        function(luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, function(
                luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...));
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
            luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));

    if constexpr (std::is_void_v<RetType>) {
        (obj.get()->*member_ptr)(luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...);
        return 0;
    }
    else {
        luaw_returner_t<RetType>{}.luaw_ret_push(L, (obj.get()->*member_ptr)(
                luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));
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
 * [INTERNAL] A Lua C function wrapper for calling C++ member functions from Lua.
 *
 * This template generates a Lua-compatible C function that wraps a C++ member function,
 * automatically converting Lua arguments to C++ types and handling return values.
 * The first Lua argument is expected to be a userdata representing the object instance.
 *
 * @tparam T          The type of the object instance.
 * @tparam member_ptr The member function pointer to wrap.
 * @tparam Params     The types of the parameters expected by the member function.
 *
 * @param L     The Lua state.
 * @return int  The number of values returned to Lua (0 for void, 1 otherwise).
 *
 * @note Exceptions thrown by the wrapped member function are caught and turned into a Lua error -
 *       see luaw_member_function_wrapper_impl() for the actual parameter/return conversion.
 */
template <typename T, auto member_ptr, typename ...Params>
inline int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<T, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

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

/* consteval + throw forces a compile-time-only error when `test` is false, same trick as
luaw_static_assert() above. Unlike that one, the message parameter here is unnamed/unused in the
body - `ToDisplay` isn't read either, it just makes each instantiation distinct per type so the
compiler's error output points at the actual offending type. */
template <bool test, typename ToDisplay>
inline consteval void demangle_static_assert(const char *) {
    if constexpr (!test)
        throw;
}

/* [INTERNAL] The C++->Lua counterpart to luaw_lua_to_cpp_object() - pushes `object` onto `L`,
dispatching on T's (decayed) category: string, bool, integral/floating-point, vector/tuple/pair
(recursively, per element, as a Lua table), enum (as a plain number - see the comment on that
branch for why not a string), and `vc::ref_t<T>` (nil for a null ref, otherwise via
push_vc_object()). Always returns 0; unsupported types fail at compile time instead (see the
final `else` below). */
template <typename T>
int luaw_push_cpp_object(lua_State *L, const T &object) {
    using Type = std::decay_t<T>;

    if constexpr (std::is_same_v<Type, std::string>) {
        lua_pushstring(L, object.c_str());
        return 0;
    }
    else if constexpr(std::is_same_v<Type, bool>) {
        lua_pushboolean(L, object);
        return 0;
    }
    else if constexpr (std::is_integral_v<Type>) {
        lua_pushinteger(L, object);
        return 0;
    }
    else if constexpr (std::is_floating_point_v<Type>) {
        lua_pushnumber(L, object);
        return 0;
    }
    else if constexpr (is_vector<Type>) {
        /* pushes a table */
        lua_createtable(L, object.size(), 0);
        for (size_t i = 0; i < object.size(); i++) {
            luaw_push_cpp_object(L, object[i]);
            lua_rawseti(L, -2, i+1);
        }
        return 0;
    }
    else if constexpr (is_tupple<Type>) {
        lua_createtable(L, std::tuple_size_v<Type>, 0);
        [&]<size_t... I>(std::index_sequence<I...>) {
            ([&](auto &item, size_t i) {
                luaw_push_cpp_object(L, item);
                lua_rawseti(L, -2, i+1);
            }(std::get<I>(object), I), ...);
        }(std::make_index_sequence<std::tuple_size_v<Type>>{});
        return 0;
    }
    else if constexpr (is_pair<Type>) {
        lua_createtable(L, 2, 0);
        luaw_push_cpp_object(L, object.first);
        lua_rawseti(L, -2, 1);
        luaw_push_cpp_object(L, object.second);
        lua_rawseti(L, -2, 2);
        return 0;
    }
    else if constexpr (is_vc_enum<Type>) {
        /* It would be better for the user to push a string or an array of strings,
        it makes more sense to see the things, but sadly I don't know if that is possible, because
        the thing is that we can't really get the signification of the bits. Once here we don't
        really know if Type is a type of a bitmap or we simply where told in it there is a bitmap.
        */
        lua_pushnumber(L, (int)object);
        return 0;
    }
    else if constexpr (is_vc_ref<Type>) {
        if (!object) {
            lua_pushnil(L);
            return 0;
        }
        ASSERT_FN(push_vc_object(L, object));
        return 0;
    }
    else {
        demangle_static_assert<false, decltype(object)>(" - Is not a valid member type");
        return -1;
    }
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

/* [INTERNAL] The Lua->C++ counterpart to luaw_push_cpp_object() - converts the Lua value at
`index` into `object`, dispatching on T's (decayed) category: string, bool, integral/floating-
point, vector/tuple/pair (recursively, per element), enum (via the `bm_t<T>` single-value parsing
path), and `vc::ref_t<T>` (a nil `lua_object_t` resets to an empty instance instead of failing,
unlike every other `ref_t<T>` here, which fails on nil). Returns 0 on success, -1 on a shape
mismatch (e.g. a table-shaped type given a non-table value). Used for member setters, call_lua()'s
return conversion, and vector/tuple/pair element conversion. */
template <typename T>
int luaw_lua_to_cpp_object(lua_State *L, int index, T &object) {
    using Type = std::decay_t<T>;

    if constexpr (std::is_same_v<Type, std::string>) {
        const char *str = lua_tostring(L, index);
        object = str ? str : "";
        return 0;
    }
    else if constexpr (std::is_same_v<Type, bool>) {
        object = lua_toboolean(L, index);
        return 0;
    }
    else if constexpr (std::is_integral_v<Type>) {
        uint64_t val = lua_tointeger(L, index);
        object = (Type)val;
        return 0;
    }
    else if constexpr (std::is_floating_point_v<Type>) {
        double val = lua_tonumber(L, index);
        object = (Type)val;
        return 0;
    }
    else if constexpr (is_vector<Type>) {
        if (!lua_istable(L, index)) {
            DBG("Expected table here");
            return -1;
        }
        int len = lua_rawlen(L, index);
        std::vector<typename Type::value_type> to_asign(len);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            luaw_lua_to_cpp_object(L, -1, to_asign[i-1]);
            lua_pop(L, 1);
        }
        object = to_asign;
        return 0;
    }
    else if constexpr (is_tupple<Type>) {
        if (!lua_istable(L, index)) {
            DBG("Expected table here");
            return -1;
        }
        int len = lua_rawlen(L, index);
        if (len != std::tuple_size_v<Type>) {
            DBG("Tuple and table sizes mismatch");
            return -1;
        }
        [&]<size_t... I>(std::index_sequence<I...>) {
            ([&](auto &item, size_t i) {
                lua_rawgeti(L, index, i+1);
                luaw_lua_to_cpp_object(L, -1, item);
                lua_pop(L, 1);
            }(std::get<I>(object), I), ...);
        }(std::make_index_sequence<std::tuple_size_v<Type>>{});
        return 0;
    }
    else if constexpr (is_pair<Type>) {
        if (!lua_istable(L, index)) {
            DBG("Expected table here");
            return -1;
        }
        int len = lua_rawlen(L, index);
        if (len != 2) {
            DBG("Tuple and table sizes mismatch");
            return -1;
        }
        lua_rawgeti(L, index, 1);
        luaw_lua_to_cpp_object(L, -1, object.first);
        lua_pop(L, 1);
        lua_rawgeti(L, index, 2);
        luaw_lua_to_cpp_object(L, -1, object.second);
        lua_pop(L, 1);
        return 0;
    }
    else if constexpr (is_vc_enum<Type>) {
        try {
            object = luaw_param_t<bm_t<T>, -1>{
                .throw_error = [](lua_State *, const std::string& str,
                        const std::source_location) -> void
                {
                    throw std::runtime_error(str);
                }
            }.luaw_single_param(L);
        }
        catch(std::exception &e) {
            DBG("Failed to parse enum object: %s", e.what());
            return -1;
        }
        return 0;
    }
    else if constexpr (is_vc_ref_t<Type>::value) {
        if constexpr (std::is_same_v<typename Type::element_type, lua_object_t>) {
            if (auto obj = get_object_from_lua(L, index);
                    obj && obj->type_id() == lua_object_t::type_id_static()) {
                object = obj->to_related<lua_object_t>();
                return 0;
            }
            object = lua_object_t::create();
            lua_object_t::capture_lua_object(L, object, index);
            return 0;
        } else {
            auto obj = get_object_from_lua(L, index);
            if (!obj) {
                DBG("Invalid user object");
                return -1;
            }
            object = obj->to_related<Type::element_type>();
            return 0;
        }
    }
    else {
        demangle_static_assert<false, decltype(object)>(" - Is not a valid object type");
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
        luaw_lua_to_cpp_object(L, -1, result);
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
inline vc::ref_t<c_function_t> c_function_t::create(std::string name, std::string source) {
    auto ret = std::make_shared<c_function_t>(vc::object_t::Private{type_id_static()});
    ret->m_name = name;
    ret->m_source = source;
    if (ret->init() < 0)
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
inline vc::ret_t c_function_t::init() {
    if (m_source == "[INTERNAL]" && has(internal_funcs, m_name)) {
        _fn = internal_funcs[m_name];
        return VC_ERROR_OK;
    }
    /* TODO: DLL/SO source */
    else {
        return VC_ERROR_GENERIC;
    }
}

}; /* namespace virt_composer */

#endif
