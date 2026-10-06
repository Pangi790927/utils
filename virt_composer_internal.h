#ifndef VIRT_COMPOSER_INTERNAL_H
#define VIRT_COMPOSER_INTERNAL_H

/*!
 * @file
 * @brief What the library's own .cpp files share and nothing else may see: virt_state_t whole, the
 * structs its members are made of, and may_claim_name.
 *
 * Core:
 *   - Only a .cpp file of virt_composer includes it, having defined VIRT_COMPOSER_LIBRARY first:
 *     virt_composer.cpp, and the components' .cpp files as they come to need it. A host or a
 *     plugin includes virt_composer.h and the components' headers, never this.
 *   - A component reads a state's members directly, as virt_composer.cpp does, instead of through
 *     accessors published in virt_composer.h for it alone.
 *
 * Detail:
 *   - virt_state_t's layout crosses modules: a plugin's own copy of the library runs on a state
 *     its host made. So this file is in the ABI hash, as virt_composer.cpp was while the struct
 *     lived there.
 *   - Inline code in virt_composer.h, which hosts and plugins compile, cannot see this file; what
 *     it needs of a state, state_internal_funcs() among it, stays declared there.
 *
 * @date 06-10-2026-00:40
 */

#ifndef VIRT_COMPOSER_H
# error "virt_composer_internal.h is included right after virt_composer.h - see the file comment."
#endif
#ifndef VIRT_COMPOSER_LIBRARY
# error "virt_composer_internal.h is the library's own: only virt_composer's .cpp files include \
it, defining VIRT_COMPOSER_LIBRARY first - see the file comment."
#endif

#include <functional>
#include <map>
#include <set>
#include <string>
#include <typeindex>
#include <vector>

namespace virt_composer
{

/* Moved here from virt_composer.cpp, unchanged but for max_type_cnt, which is inline now that more
than one file reads it. 06-10-2026-00:40 */
/*! Holds information of a member, either a member function or a member object */
struct luaw_member_t {
    lua_CFunction fn;
    luaw_member_e member_type;
};

/*! Holds a function that will copy from that respective member, also holds the type_index of the
 * member to check at runtime that the copy doesn't come from a mismatched-type source */
struct trivial_copy_member_t {
    std::type_index tid{typeid(void)};
    std::function<void(vc::object_t *, void *, size_t)> copy_fn;
};

/*! Holds one independent virt_composer instance's entire state - see virt_state_t's own doc
comment in virt_composer.h for the user-facing summary; this is the actual definition. */
struct virt_state_t {
    /* Coroutine pool build_object()/build_pseudo_object()/build_schema() get scheduled onto - see
    parse_config()/internal_create_object() for where it's actually run(). */
    co::pool_p pool;

    /*! The Lua state associated with this virt state */
    lua_State *L = nullptr;

    /*! This is the index inside LUA_REGISTRYINDEX of the "virt_composer" Lua library, you must do
     * something like 'vc = require("virt_composer")' to use the objects/functions from inside Lua
     */
    int lua_table_idx;

    /*! Used to name anonymous objects for inside this state instance. */
    int64_t anonymous_increment = 0;

    /*!
     * @name Object Construction Callbacks
     * @brief Callbacks for constructing objects from YAML nodes.
     * @{
     *
     * Each object is constructed from a YAML node (nested or not). The composer determines
     * which function to call using two methods:
     * - **By `m_type` field**: If the node contains a known `m_type`, the associated builder
     *   function is called with the object name and node contents. Only typed objects can be nested.
     *   Example:
     *   ```
     *   my_typed_object_name:
     *       m_type: object_type_t
     *       other_field: 15
     *   ```
     *
     * - **By structure**: An analyser function checks if the node matches an expected structure.
     *   If so, the associated function constructs the object. Examples:
     *   ```
     *   my_integer_value: 15  // Constructs a builtin `vc::integer_t`
     *   my_object_type: inlined_script: lua_function_call("Print me") // Calls user defined
     *                                                                 // matcher-constructer
     *   ```
     *   The analyser returns an integer:
     *   - Negative: Parsing stops with an error.
     *   - Positive: Reserved for future use.
     *   - Zero: No object constructed.
     *
     * @note
     * Typed objects (`build_object_cbks`) can be nested; pseudo-objects (`build_psudo_object_cbks`)
     * cannot.
     */
    std::vector<
        std::pair<
            std::string,
            std::function<co::task<vc::ref_t<vc::object_t>> (vc::virt_state_t *,
                    const std::string&, fkyaml::node&)>
        >
    > build_object_cbks; ///< Callbacks for typed objects (nested, `m_type`-based).

    std::vector<
        std::pair<
            std::function<bool(const std::string&, fkyaml::node& node)>,
            std::function<co::task_t(vc::virt_state_t *, const std::string&, fkyaml::node&)>
        >
    > build_psudo_object_cbks; ///< Callbacks for pseudo-objects (structure-based).
    /*! @} */

    /*! This holds an name-index map for some of the objects above. It is used to find the objects by
     * their name.
     */
    std::map<std::string, vc::object_t *> name_to_object;
    std::map<vc::object_t *, std::string> object_to_name;

    /*! Parser object: during parsing multiple coroutine will want an named object. This map
     * stores those coroutines states. Once the object is resolved the coroutines will be moved
     * back into the running queue. If after parsing this map is not empty we will know that there
     * where unresolved references and error out.
     */
    std::map<std::string, std::vector<co::state_t *>> wanted_objects;

    /*! Used to track objects from lua, those are needed to reference the constructed lua objects
     * afferent to vc::object_t */
    int weak_cache_ref = LUA_NOREF;

    /*! Registry ref to the dedicated (non-weak) table lua_object_t captures live in. Kept
     * separate from weak_cache_ref/the main registry so per-capture churn never touches the same
     * table luaw_get_virt_state()'s hot "virt_state" string lookup runs against on every single
     * dispatcher call. Unlike weak_cache_ref, this table is deliberately NOT weak-mode - the whole
     * point of capturing is to keep the value alive even after Lua itself drops every reference
     * to it. */
    int lua_object_ref_table = LUA_NOREF;

    /*! Holds a list of constants that can be used inside  */
    std::map<std::string, double> constants = {
        {"SIZEOF_INT16", (double)sizeof(int16_t)},
        {"SIZEOF_INT32", (double)sizeof(int32_t)},
        {"SIZEOF_INT64", (double)sizeof(int64_t)},
        {"SIZEOF_UINT16", (double)sizeof(uint16_t)},
        {"SIZEOF_UINT32", (double)sizeof(uint32_t)},
        {"SIZEOF_UINT64", (double)sizeof(uint64_t)},
        {"SIZEOF_FLOAT", (double)sizeof(float)},
        {"SIZEOF_DOUBLE", (double)sizeof(double)},
        {"SIZEOF_VEC_2F", (double)sizeof(float)*2},
        {"SIZEOF_VEC_3F", (double)sizeof(float)*3},
        {"SIZEOF_VEC_4F", (double)sizeof(float)*4},
        {"SIZEOF_VEC_2D", (double)sizeof(double)*2},
        {"SIZEOF_VEC_3D", (double)sizeof(double)*3},
        {"SIZEOF_VEC_4D", (double)sizeof(double)*4},
        {"SIZEOF_MAT_2x2F", (double)sizeof(float)*2*2},
        {"SIZEOF_MAT_3x3F", (double)sizeof(float)*3*3},
        {"SIZEOF_MAT_4x4F", (double)sizeof(float)*4*4},
        {"SIZEOF_MAT_2x2D", (double)sizeof(double)*2*2},
        {"SIZEOF_MAT_3x3D", (double)sizeof(double)*3*3},
        {"SIZEOF_MAT_4x4D", (double)sizeof(double)*4*4},
    };

    /*! Holds free functions */
    std::vector<luaL_Reg> tab_funcs;

    /*! Holds functions to memcpy from member objects, helps when needing to transfer exact data
     * inside yaml config files, member must be a trivially copiable type */
    std::vector<std::unordered_map<std::string, vc::trivial_copy_member_t>> trivial_copy_member =
            std::vector<std::unordered_map<std::string, vc::trivial_copy_member_t>> {VIRT_TYPE_CNT};

    /*! This holds member functions and member objects getters */
    std::vector<std::unordered_map<std::string, vc::luaw_member_t>> lua_class_members =
            std::vector<std::unordered_map<std::string, vc::luaw_member_t>> {VIRT_TYPE_CNT};

    /*! This holds member objects setters */
    std::vector<std::unordered_map<std::string, lua_CFunction>> lua_class_member_setters =
            std::vector<std::unordered_map<std::string, lua_CFunction>> {VIRT_TYPE_CNT};

    /*! This holds, per class id, the registered Lua operator (arithmetic/relational/misc
     * metamethod) handlers, indexed by `operator_e`. A null entry means no handler is registered. */
    std::vector<std::array<lua_CFunction, (size_t)vc::VC_OPERATOR_CNT>> lua_class_operators =
            std::vector<std::array<lua_CFunction, (size_t)vc::VC_OPERATOR_CNT>> {VIRT_TYPE_CNT};

    /*! This holds for every base_type all the derived types, including itself, used for setting
     * member functions and object to all the derived also */
    std::vector<std::unordered_set<int>> inheritance_table =
            std::vector<std::unordered_set<int>>{VIRT_TYPE_CNT};

    /*! The plugins already registered into this state, by real path.
     *
     * It lives here rather than in a table beside the plugins because a state is the thing it
     * describes: a rebuilt state is a new object with an empty set, where anything keyed on a
     * state's address would still be answering for a state that no longer exists.
     * 2026-09-20 18:55 */
    std::set<std::string> loaded_plugins;

    /*! The internal-function table this state binds its `[INTERNAL]` names from.
     *
     * A pointer and not a table, because a plugin carrying its own copy of this library has an
     * internal_funcs of its own. What matters is that the plugin and its host write into and read
     * from the same one, and that one is whichever module made the state. 2026-09-20 19:10 */
    std::map<std::string, std::function<int(lua_State *L)>> *internal_funcs =
            c_function_t::own_internal_funcs();

    /*! Who is registering right now, as a plugin's real path, or null when it is the host.
     *
     * register_plugin() sets it around plugin_register_meta() and clears it after, so a
     * registration can name the owner of what it is about to write without being handed it.
     * 2026-09-20 19:30, 05-10-2026-22:33 */
    const std::string *registering_plugin = nullptr;

    /*! Set when a registration was refused for a name already owned, and read by
     * register_plugin() once the plugin has finished. It exists because a registration has
     * nowhere else to report this - nothing checks what those functions answer.
     * 2026-09-20 19:30 */
    bool name_conflict = false;

    /* Closes the Lua state - see virt_state_t's own doc comment in virt_composer.h for why nothing
    obtained from this virt_state_t is safe to keep alive past this point. */
    ~virt_state_t() {
        DBG_SCOPE();
        /* The pool goes first. A task suspended on a semaphore or on I/O holds whatever it was
        awaiting, and a Lua coroutine's resumer holds a thread of the state below; clear() destroys
        every one of them, so none is left to resume a thread that lua_close has already taken
        away. 2026-09-22 03:32 */
        /* It is moved out of the member first, so luaw_get_pool() answers null while lua_close
        runs the finalizers: an object dying there can tell the pool has ended and do no pool work.
        `ending` keeps the pool object itself alive until after them, so nothing holding its raw
        pointer is left dangling. 2026-09-23 00:37 */
        auto ending = std::move(pool);
        if (ending)
            ending->clear();
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }
};

/* [INTERNAL] How far this state's per-type tables reach, which is one past the largest type id it
can answer for. Not a count of the types it knows: a plugin's range sits at an offset fixed for the
process, so a state that skipped an earlier plugin's range still reaches past it and carries unused
rows where that plugin's types would have been. 2026-09-20 17:45 */
inline size_t max_type_cnt(vc::virt_state_t *vs) { return vs->lua_class_members.size(); }

/*! [INTERNAL] Answers whether `name` may be registered into the state by whoever is registering
 * now, the host or the plugin register_plugin() names, and records the claim when it may. A name
 * another already owns is refused, said in the log, and marks the plugin's registration refused.
 * @date 06-10-2026-02:45 */
bool may_claim_name(virt_state_t *vs, const std::string& name);

} /* namespace virt_composer */

#endif /* VIRT_COMPOSER_INTERNAL_H */
