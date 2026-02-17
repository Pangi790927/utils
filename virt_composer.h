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
#include "co_utils.h"
#include "yaml.h"
#include "minilua.h"
#include "demangle.h"

/*! Use this to register a new type. This is only used to register the enumerator. */
#define VIRT_COMPOSER_REGISTER_TYPE(type) \
        constexpr virt_composer::object_type_e type{\
        virt_object::compile_unique_id<virt_composer::virt_tag_t>(), #type}

namespace virt_composer {

namespace vc = virt_composer; 
namespace vo = virt_object;

enum err_e : int32_t {
    VC_ERROR_NONE = 0,
    VC_ERROR_GENERIC = -1,
    VC_ERROR_PARSE_YAML = -2
};

struct except_t : public std::exception {
    std::string err_str;

    except_t(const std::string& str);
    const char *what() const noexcept override { return err_str.c_str(); };
};

struct virt_state_t;
struct virt_tag_t {};

/*! This is a common type enumeration for all the types that can be derived from vc::object_t
 * The intended way to use this is with VIRT_COMPOSER_REGISTER_TYPE(name_of_enum), this will
 * create a new enumeration type that is tracked by this implementation (tracking ends at
 * virt_composer_end.h) */
using object_type_e = vo::EnumClass<virt_tag_t>;

/*! This is needed to create our special virtual objects */
struct virt_traits_t {
    using ret_t = int64_t; /* it annoys me to no end that I don't seem to find a way to make this
                              return whatever (ex: VkResult), but whatever */
    using type_t = object_type_e;
};

/*! The object that can  */
using object_t = vo::object_t<virt_traits_t>;

/*! The return type of the init/uninit and other default functions inside objects */
using ret_t = virt_traits_t::ret_t;


/*! The type of reference */
template <typename T>
using ref_t = vo::ref_t<T, virt_traits_t>;

/*!
 * @brief Creates and returns a new shared pointer to a virtual state object.
 *
 * @return std::shared_ptr<virt_state_t>
 *         A shared pointer to the newly created @c virt_state_t object.
 */
std::shared_ptr<virt_state_t> create_state();

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
 *         - @c VC_ERROR_OK on success.
 *         - @c VC_ERROR_PARSE_YAML if the YAML file is malformed or contains unknown objects.
 *         - @c VC_ERROR_GENERIC for other errors (e.g., schema construction failure).
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
 * @return err_e `VC_ERROR_OK` on success, or an error code on failure.
 *
 * @see build_object_cbks 
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
 * @return err_e `VC_ERROR_OK` on success, or an error code on failure.
 * 
 * @see build_psudo_object_cbks
 * @note Only typed objects (with an `m_type` field) can be nested. Auto-identified-objects cannot.
 */
err_e add_auto_builder_callback(virt_state_t *vs,
        std::function<bool(const std::string&, fkyaml::node& node)> analyser,
        std::function<co::task_t(virt_state_t *, const std::string&, fkyaml::node&)> builder);

/*!
 * Marks a dependency as resolved and notifies all coroutines waiting for it. To be used inside
 * builder callbacks.
 *
 * This function registers a newly constructed object in the virtual state (`virt_state_t`)
 * and resumes any coroutines that were suspended while waiting for this dependency.
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
 * This coroutine function resolves a YAML node to a `double` value. It handles two cases:
 * 1. **Reference nodes** (e.g., `!ref object_name`): Resolves the referenced float object.
 * 2. **String nodes**: Evaluates the string as a mathematical expression (using `texpr`).
 * 3. **Direct float nodes**: Returns the floating-point value directly.
 *
 * @param vs Pointer to the virtual state (`virt_state_t`), providing parsing context and dependency
 * management.
 * @param node The YAML node to resolve. Can be a reference or a direct float.
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
 */co::task<std::string> resolve_str(virt_state_t *vs, fkyaml::node& node);

/*!
 * Asynchronously resolves a YAML node into an object reference, handling both direct references
 * and inlined object definitions. To be used inside the build_object callback.
 *
 * This coroutine function is used during configuration parsing to resolve a YAML node into a
 * strongly-typed reference (`vc::ref_t<T>`). It supports three cases:
 * 1. **Reference nodes** (e.g., `m_field: !ref object_name`).
 * 2. **Tagged mapping nodes** (e.e., `m_field: tag_name: m_type: "..."`).
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

/* Virt Composer - LUA API
------------------------------------------------------------------------------------------------- */


err_e add_lua_tab_funcs(virt_state_t *vs, const std::vector<luaL_Reg>& vku_tab_funcs);

/* adds a new constant value for lua usage */
err_e add_lua_flag_mapping(virt_state_t *vs, const std::map<lua_Integer, std::string> &mapping);

/**
 * @brief A Lua C function wrapper for calling C++ functions from Lua.
 *
 * This template generates a Lua-compatible C function that wraps a C++ function,
 * automatically converting Lua arguments to C++ types and handling return values.
 * It is designed to be used with `luaL_Reg` for registering C++ functions in Lua.
 *
 * @tparam function The C++ function to wrap. Must be callable with the provided `Params...`.
 * @tparam Params   The types of the parameters expected by the wrapped function.
 *
 * @param L The Lua state.
 * @return int The number of values returned to Lua (0 for void, 1 otherwise).
 * 
 * @details
 * The actual parameter conversion and function invocation is delegated to
 * `luaw_function_wrapper_impl`, which uses `luaw_param_t` to convert Lua values
 * to C++ types and `luaw_returner_t` to push C++ return values back to Lua.
 * This wrapper catches all exceptions and forwards them to `luaw_catch_exception`.
 * 
 * @see luaw_function_wrapper_impl, luaw_param_t, luaw_returner_t
 */
template <auto function, typename ...Params>
inline int luaw_function_wrapper(lua_State *L);

/**
 * @brief A Lua C function wrapper for calling C++ member functions from Lua.
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
 * @details
 * This wrapper catches all exceptions and forwards them to `luaw_catch_exception`.
 * The actual parameter conversion and member function invocation is delegated to
 * `luaw_member_function_wrapper_impl`, which uses `luaw_param_t` to convert Lua values
 * to C++ types and `luaw_returner_t` to push C++ return values back to Lua.
 *
 * @see luaw_member_function_wrapper_impl, luaw_param_t, luaw_returner_t
 */
template <typename T, auto member_ptr, typename ...Params>
inline int luaw_member_function_wrapper(lua_State *L);

/* TODO: add the functions to add the exception callbacks */

/*! IMPLEMENTATION
 * ==============================================================================================
 * ==============================================================================================
 * ============================================================================================== */

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
 * - Strings (creates a `string_t` object).
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
 * @param vs Virtual state context (unused, reserved for future use).
 * @return Unique name in the format "anonymous_<incremented_id>".
 */
std::string new_anon_name(virt_state_t *vs);

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
 * [INTERNAL] Coroutine for resolving object dependencies during parsing.
 *
 * This function is used within parser callbacks to asynchronously resolve a field that may depend
 * on another object not yet initialized. It suspends the current coroutine until the target object
 * (specified by the YAML node) is fully constructed and marked as initialized.
 *
 * @tparam T The type of the object to resolve (e.g., `my_struct_t`).
 * @param vs Pointer to the virtual state (`vc::virt_state_t`), used to manage parsing context.
 * @param node The YAML node representing the field or object to resolve.
 *
 * @return A coroutine task that yields a `vc::ref_t<T>`, a reference to the resolved object.
 *         The coroutine resumes only when the object is ready.
 *
 * @note This function is designed to work with the dependency resolver system. If the object
 *       referenced by `node` is not yet initialized, the coroutine will suspend and automatically
 *       resume when the object becomes available.
 *
 * @example
 * // Usage in a parser callback:
 * auto resolved_field = co_await resolve_obj<my_struct_t>(vs, yaml_node);
 * // `resolved_field` is now safe to use.
 */
template <typename T>
struct depend_resolver_t : depend_resolver_internal_t {
    /* We save the searched dependency */
    depend_resolver_t(virt_state_t *vs, std::string required_depend)
    : required_depend(required_depend), depend_resolver_internal_t(vs) {}

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

    vc::ref_t<T> await_resume() {
        auto ret = internal_get_dep_object(required_depend).to_related<T>();
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

/* See above for a description */
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
        co_return ref.template to_related<T>();
    }
    else if (node.contains("m_type")) {
        /* This is in the form m_field: m_type: "...", ie, inlined object */
        std::string tag = node.contains("m_tag") ?
                node["m_tag"].as_str() : new_anon_name(vs);
        auto ref = co_await vc::build_object(vs, tag, node);
        co_return ref.template to_related<T>();
    }

    /* None of the above */
    throw vc::except_t{std::format("node:{} is invalid in this contex",
            fkyaml::node::serialize(node))};
}

template <bool B, typename T>
inline consteval void luaw_static_assert(const char *description) {
    if constexpr (!B)
        throw description; /* This throw forces the termination of compilation */
}

inline void* luaw_to_user_data(int index) { return (void*)(intptr_t)(index); }
inline int luaw_from_user_data(void *val) { return (int)(intptr_t)(val); }

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
void luaw_push_error(lua_State *L, const std::string& err_str);

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

/**
 * @brief Gets the object at the given index from the virtual state.
 * @param vs    Virtual state containing the object list.
 * @param index Index of the object to retrieve.
 * @return vc::ref_t<vc::object_t> Object reference, contains `nullptr` if the index is invalid.
 */
vc::ref_t<vc::object_t> luaw_get_object_at_index(virt_state_t *vs, ssize_t index);

/*! This is here just to hold the diverse bitmaps */
template <typename T>
struct bm_t {
    using type = T;
};

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

/* This resolves userdata(void *) received from lua to an vku parameter */
template <ssize_t index>
struct luaw_param_t<void *, index> {
    void *luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return NULL;
        return lua_touserdata(L, index);
    }
};

/* This resolves userdata(vku::ref) received from lua to an vku parameter */
template <typename T, ssize_t index>
struct luaw_param_t<vc::ref_t<T>, index> {
    vc::ref_t<T> luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return vc::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
        int obj_index = luaw_from_user_data(lua_touserdata(L, index));
        if (obj_index == 0) {
            luaw_push_error(L, std::format("Invalid parameter at index {} of expected type {} but "
                    "got [{}] instead",
                    index, demangle<T>(), lua_typename(L, lua_type(L, index))));
        }
        auto vs = luaw_get_virt_state(L);
        return luaw_get_object_at_index(vs, obj_index).to_related<T>();
    }
};

/* This resolves bitmasks received from lua to an vku parameter */
template <typename T, ssize_t index>
struct luaw_param_t<bm_t<T>, index> {
    T luaw_single_param(lua_State *L) {
        /* There are 2 options here (maybe later we will also add numbers, but not for now):
            1. This is a string that converts to the respective type bitmask
            2. An integer, this will be converted to T
            3. This is an enum value, either like 1. or 2. */
        auto from_string = [](lua_State *L, int idx) -> T {
            const char *val = lua_tostring(L, idx);
            if (!val) {
                luaw_push_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                        "object is an invalid string: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))));
            }
            fkyaml::node str_enum_val{val};
            return get_enum_val<T>(str_enum_val);
        };
        auto from_integer = [](lua_State *L, int idx) -> T {
            int valid = 0;
            uint32_t val = lua_tointegerx(L, idx, &valid);
            if (!valid) {
                luaw_push_error(L, std::format(
                        "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                        "object is an invalid integer: [{}]",
                        idx, lua_typename(L, lua_type(L, idx))));
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
                    luaw_push_error(L, std::format(
                            "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                            "object is an invalid string or integer: [{}]",
                            index, lua_typename(L, lua_type(L, index))));
                }
                lua_pop(L, 1);
            }
            return ret;
        }
        else {
            luaw_push_error(L, std::format(
                    "Invalid parameter at index {}, failed conversion to [vku-bitmask] "
                    "object is neither table, integer or string: [{}]",
                    index, lua_typename(L, lua_type(L, index))));
            return (T)0;
        }
    }
};

/* This resolves integers received from lua to an vku parameter */
template <std::integral Integer, ssize_t index>
struct luaw_param_t<Integer, index> {
    Integer luaw_single_param(lua_State *L) {
        int valid = 0;
        Integer ret = lua_tointegerx(L, index, &valid);
        if (!valid) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to integer from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
        }
        return ret;
    }
};

/* This resolves floats received from lua to an vku parameter */
template <std::floating_point Float, ssize_t index>
struct luaw_param_t<Float, index> {
    Float luaw_single_param(lua_State *L) {
        int valid = 0;
        Float ret = lua_tonumberx(L, index, &valid);
        if (!valid) {
            luaw_push_error(L,
                    std::format("Invalid parameter at index {}, failed conversion to integer from "
                    "[{}]",
                    index, lua_typename(L, lua_type(L, index))));
        }
        return ret;
    }
};

/* This resolves strings received from lua to an vku parameter */
template <ssize_t index>
struct luaw_param_t<char *, index> {
    char *luaw_single_param(lua_State *L) {
        char *ret = lua_tostring(L, index);
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
        return _luaw_single_param_impl(L, std::index_sequence_for<Args...>{});
    }
};

template <typename Arg1, typename Arg2, ssize_t index>
struct luaw_param_t<std::pair<Arg1, Arg2>, index> {
    auto luaw_single_param(lua_State *L) {
        auto tuple = luaw_param_t<std::tuple<Arg1, Arg2>, index>{}
                .luaw_single_param(L);
        typename de_bitmaptizize<std::pair<Arg1, Arg2>>::Type ret =
                {std::get<0>(tuple), std::get<1>(tuple)};
        return ret;
    }
};

template <typename T, ssize_t index>
struct luaw_param_t<std::vector<T>, index> {
    auto luaw_single_param(lua_State *L) {
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
template <typename T>
struct luaw_returner_t {
    void luaw_ret_push(lua_State *L, T&& t) {
        (void)t;
        luaw_static_assert<false, T>(" - Is not a valid return type");
    }
};

template <std::integral Integer>
struct luaw_returner_t<Integer> {
    void luaw_ret_push(lua_State *L, Integer&& x) {
        lua_pushinteger(L, x);
    }
};

template <>
struct luaw_returner_t<void *> {
    void luaw_ret_push(lua_State *L, void *rawptr) {
        lua_pushlightuserdata(L, rawptr);
    }
};

template <auto function, typename ...Params, size_t ...I>
inline int luaw_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    using RetType = decltype(function(
            luaw_param_t<Params, I + 1>{}.luaw_single_param(L)...));

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

template <typename T, auto member_ptr, typename ...Params, size_t ...I>
int luaw_member_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    int index = luaw_from_user_data(lua_touserdata(L, 1));
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't call member function! (check if : used)");
    }
    auto vs = luaw_get_virt_state(L);
    auto o = luaw_get_object_at_index(vs, index);
    if (!o)
        luaw_push_error(L, "internal_error: Nil user object can't call member function!");
    auto obj = o.to_related<T>();

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

template <auto Function, typename ...Params>
inline int luaw_function_wrapper(lua_State *L) {
    try {
        return luaw_function_wrapper_impl<Function, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename T, auto member_ptr, typename ...Params>
inline int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<T, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

}; /* namespace virt_composer */

#endif
