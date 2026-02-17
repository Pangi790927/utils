
#include <filesystem>

#define LUA_IMPL

#include "virt_composer.h"

#include "co_utils.h"
#include "yaml.h"
#include "tinyexpr.h"

namespace virt_composer
{

namespace vo = virt_object;
namespace vc = virt_composer;

/* Max number of named references objects */
static constexpr const int MAX_NUMBER_OF_OBJECTS = 16384;
// enum luaw_member_e {
//     LUAW_MEMBER_FUNCTION,
//     LUAW_MEMBER_OBJECT,
// };

// struct luaw_member_t {
//     lua_CFunction fn;
//     luaw_member_e member_type;
// };

/*! This holds the objects reference such that lua can use them */
struct object_ref_t {
    /*! The actual virtual object that this reference points to is held by this vc::ref_t. */
    vc::ref_t<vc::object_t> obj;

    /*! this is required such that when this object gets removed it also gets removed from
    objects_map */
    std::string name;
};

/*!
 * This object holds all the cpp-backed objects with their associated names. This object also holds
 * the currently required objects by other coroutine object builders. It practically holds the
 * required state to split the parsing process, if needed, in multiple steps. It also is the main
 * holder of objects after parsing is done and can be used to further append objects via lua for
 * example.
 */
struct parser_state_t {
    /*! This contains a table with the objects, for implementation reasons, it was best to create
     * a table with the maximum number of user objects (if you don't want this to be slow, keep the
     * lua side objects in a resonable number) As such, inside lua, the light user data objects
     * stored are actually indexes in this array.
     */
    std::vector<object_ref_t> objects;

    /*! This holds an name-index map for all the objects above. It is used to find the objects by
     * their name.
     */
    std::map<std::string, int> objects_map;

    /*! Parser object: during parsing multiple coroutine will want an named object. This map
     * stores those coroutines states. Once the object is resolved the coroutines will be moved
     * back into the running queue. If after parsing this map is not empty we will know that there
     * where unresolved references and error out.
     */
    std::map<std::string, std::vector<co::state_t *>> wanted_objects;

    /*! A stack of free indexes inside the objects table, this is used to keep track of what slots
     * can be used to store objects.
     */
    std::vector<int> free_objects;

    parser_state_t(int table_size) : objects(table_size) {
        for (int i = table_size-1; i >= 1; i--)
            ps.free_objects.push_back(i);
    }

    /* TODO: move this function from here, it doesn't belong here it is usefull only when
    the 'oth' state is made by copying this state into another object and after that adding to it.
    This function will not make sense or work in many other cases */
    /* TODO: this is stupid slow, must be made faster (create a clear interface of adding and
    removing objects and make get_new and append return the internal held update list) */
    std::vector<int> get_new(const parser_state_t &oth) {
        std::set<int> free_objects_this(ps.free_objects.begin(), ps.free_objects.end());
        std::set<int> free_objects_other(oth.ps.free_objects.begin(), oth.ps.free_objects.end());

        std::vector<int> new_other;
        std::set_difference(
            free_objects_this.begin(), free_objects_this.end(),
            free_objects_other.begin(), free_objects_other.end(),
            std::back_inserter(new_other)
        );

        std::vector<int> new_this;
        std::set_difference(
            free_objects_other.begin(), free_objects_other.end(),
            free_objects_this.begin(), free_objects_this.end(),
            std::back_inserter(new_this)
        );

        if (new_this.size())
            throw vc::except_t(
                    "How could the state change while we where creating a new object? huh?");
        return new_other;
    }

    /* TODO: as for above: move it and make it faster */
    void append(const parser_state_t &oth) {
        std::set<int> free_objects_this(ps.free_objects.begin(), ps.free_objects.end());
        std::set<int> free_objects_other(oth.ps.free_objects.begin(), oth.ps.free_objects.end());

        std::vector<int> new_other;
        std::set_difference(
            free_objects_this.begin(), free_objects_this.end(),
            free_objects_other.begin(), free_objects_other.end(),
            std::back_inserter(new_other)
        );

        std::vector<int> new_this;
        std::set_difference(
            free_objects_other.begin(), free_objects_other.end(),
            free_objects_this.begin(), free_objects_this.end(),
            std::back_inserter(new_this)
        );

        for (int idx : new_other) {
            this->ps.objects[idx] = oth.ps.objects[idx];
            this->ps.objects_map[oth.ps.objects[idx].name] = idx;
        }
        this->ps.free_objects = std::vector<int>(free_objects_other.begin(), free_objects_other.end());
    }
};

/*! This holds the state of the  */
struct virt_state_t {
    /*! The Lua state associated with this virt state */
    lua_State *L = nullptr;

    /*! This is the index inside LUA_REGISTRYINDEX of the "virt_composer" Lua library, you must do
     * something like 'vc = require("virt_composer")' to use the objects/functions from inside Lua
     */
    int lua_table_idx;

    /*! This is the index inside LUA_REGISTRYINDEX of the virtual state associated to the lua_State
     */
    int lua_vs_index;

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

    /*! Holds the objects as named references and parser info, @see parser_state_t */
    parser_state_t ps = parser_state_t(MAX_NUMBER_OF_OBJECTS);

    /*! Holds a list of constants that can be used inside  */
    std::map<std::string, double> constants = {
        {"SIZEOF_INT16", sizeof(int16_t)},
        {"SIZEOF_INT32", sizeof(int32_t)},
        {"SIZEOF_INT64", sizeof(int64_t)},
        {"SIZEOF_UINT16", sizeof(uint16_t)},
        {"SIZEOF_UINT32", sizeof(int32_t)},
        {"SIZEOF_UINT64", sizeof(int64_t)},
        {"SIZEOF_FLOAT", sizeof(float)},
        {"SIZEOF_DOUBLE", sizeof(double)},
        {"SIZEOF_VEC_2F", sizeof(float)*2},
        {"SIZEOF_VEC_3F", sizeof(float)*3},
        {"SIZEOF_VEC_4F", sizeof(float)*4},
        {"SIZEOF_VEC_2D", sizeof(double)*2},
        {"SIZEOF_VEC_3D", sizeof(double)*3},
        {"SIZEOF_VEC_4D", sizeof(double)*4},
        {"SIZEOF_MAT_2x2F", sizeof(float)*2*2},
        {"SIZEOF_MAT_3x3F", sizeof(float)*3*3},
        {"SIZEOF_MAT_4x4F", sizeof(float)*4*4},
        {"SIZEOF_MAT_2x2D", sizeof(double)*2*2},
        {"SIZEOF_MAT_3x3D", sizeof(double)*3*3},
        {"SIZEOF_MAT_4x4D", sizeof(double)*4*4},
    };

    std::vector<std::vector<luaL_Reg>> tab_funcs;

    ~virt_state_t() {
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }
};

/* This is the path the application was run from */
static std::string app_path = std::filesystem::canonical("./");


template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

static int internal_create_object(lua_State *L);
static lua_State *luaw_init(vc::virt_state_t *vs);
static int luaopen_vku(lua_State *L);
 
except_t::except_t(const std::string& str) {
    err_str = std::format(
            "\n------BACKTRACE------\n"
            "{}"
            "EXCEPTION: {}"
            "\n---------------------",
            cpp_backtrace(),
            str);
}


std::shared_ptr<virt_state_t> create_state() {
    auto vs = std::make_shared<virt_state_t>();

    ASSERT_RET(nullptr, CHK_PTR(vs->L = luaw_init(vs.get())));
    ASSERT_RET(nullptr, add_lua_tab_funcs(vs.get(), {{"create_object", internal_create_object}}));
    return vs;
}


bool depend_resolver_internal_t::internal_check_depend(const std::string &dep_name) {
     return has(vs->ps.objects_map, dep_name);
}

void depend_resolver_internal_t::internal_mark_wait(const std::string &dep_name, co::state_t *state) {
    vs->ps.wanted_objects[dep_name].push_back(state);
}

vc::ref_t<vc::object_t> depend_resolver_internal_t::internal_get_dep_object(
        const std::string &dep_name)
{
    if (!has(vs->ps.objects_map, dep_name)) {
        DBG("Object not found");
        throw vc::except_t(std::format("Object not found, {}", dep_name));
    }
    if (!vs->ps.objects[vs->ps.objects_map[dep_name]].obj) {
        DBG("For some reason this object now holds a nullptr...");
        throw vc::except_t("nullptr object");
    }
    return vs->ps.objects[vs->ps.objects_map[dep_name]].obj;
}

std::string depend_resolver_internal_t::internal_get_obj_type_name(const std::string &dep_name) {
    return demangle<4>(typeid(vs->ps.objects[vs->ps.objects_map[dep_name]].obj.get()).name()).c_str();
}


err_e add_named_builder_callback(vc::virt_state_t *vs, const std::string& match,
        std::function<co::task<vc::ref_t<vc::object_t>>(
                vc::virt_state_t *, const std::string&, fkyaml::node&)> builder)
{
    vs->build_object_cbks.push_back({match, builder});
    return VC_ERROR_OK;
}

err_e add_auto_builder_callback(vc::virt_state_t *vs,
        std::function<bool(const std::string&, fkyaml::node& node)> analyser,
        std::function<co::task_t(vc::virt_state_t *, const std::string&, fkyaml::node&)> builder)
{
    vs->object_cbks_t.push_back({analyser, builder});
    return VC_ERROR_OK;
}

err_e add_lua_tab_funcs(virt_state_t *vs, const std::vector<luaL_Reg>& vku_tab_funcs) {
    return VC_ERROR_OK;
}

err_e add_lua_flag_mapping(virt_state_t *vs, const std::map<lua_Integer, std::string> &mapping) {
    auto L = vs->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);
    for (auto& [k, v] : mapping) {
        lua_pushinteger(L, v);
        lua_setfield(L, -2, k.c_str());
    }
    lua_pop(L, 1);
    return VC_ERROR_OK;
}

void mark_dependency_solved(virt_state_t *vs, std::string depend_name, vc::ref_t<vc::object_t> depend) {
    /* First remember the dependency: */
    if (!depend) {
        DBG("Object into nullptr");
        throw vc::except_t{std::format("Object turned into nullptr: {}", depend_name)};
    }
    if (has(vs->ps.objects_map, depend_name)) {
        DBG("Name taken");
        throw vc::except_t{std::format("Tag name already exists: {}", depend_name)};
    }
    int new_id = vs->ps.free_objects.back();
    vs->ps.free_objects.pop_back();

    DBG("Adding object: %s [%d]", depend->to_string().c_str(), new_id);
    vs->ps.objects_map[depend_name] = new_id;
    depend->cbks = std::make_shared<vo::object_cbks_t<vc::virt_traits_t>>();
    depend->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});
    vs->objects[new_id].obj = depend;
    vs->objects[new_id].name = depend_name;

    /* Second, awake all the ones waiting for the respective dependency */
    if (vc::has(vs->ps.wanted_objects, depend_name)) {
        for (auto s : vs->ps.wanted_objects[depend_name])
            co::external_sched_resume(s);
        vs->ps.wanted_objects.erase(depend_name);
    }
}

static std::pair<double, int> resolve_string_as_expression(std::string expr_str,
        vc::virt_state_t *vs)
{
    /* TODO: Try to resolve user-defined vars as well (may be bit hard to do, as I think I need to
    modify tinyexpr to make something usefull, because I may want to wait for that respective value
    to exist) */
    std::vector<texpr::te_variable> vars;
    for (auto &[name, value] : vs->constants)
        vars.push_back(texpr::te_variable{
            .name = name.c_str(),
            .address = (void *)&value,
            .type = texpr::TE_VARIABLE,
            .context = nullptr,
        });

    int err = 0;
    texpr::te_expr *expr = texpr::te_compile(expr_str.c_str(), vars.data(), vars.size(), &err);

    if (!expr)
        throw vc::except_t{std::format("Failed to parse expr: [{}] error: {}", expr_str, err)};

    double expr_result = texpr::te_eval(expr);
    texpr::te_free(expr);
    return expr_result;
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<int64_t> resolve_int(vc::virt_state_t *vs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await vc::depend_resolver_t<integer_t>(vs, node.as_str()))->value;
    if (node.is_string()) {
        /* Try to resolve an expression resulting in an integer: */
        return std::round(resolve_string_as_expression(node.as_str(), vs));
    }
    else
        co_return node.as_int();
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<double> resolve_float(vc::virt_state_t *vs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await vc::depend_resolver_t<float_t>(vs, node.as_str()))->value;
    if (node.is_string()) {
        /* Try to resolve an expression resulting in an double: */
        return resolve_string_as_expression(node.as_str(), vs);
    }
    else
        co_return node.as_float();
}

/*! This either follows a reference to a string or it returns the direct value if available */
co::task<std::string> resolve_str(vc::virt_state_t *vs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await vc::depend_resolver_t<string_t>(vs, node.as_str()))->value;
    co_return node.as_str();
}

co::task<vc::ref_t<vc::object_t>> build_object(vc::virt_state_t *vs,
        const std::string& name, fkyaml::node& node)
{
    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return nullptr;
    }
    for (auto &[match, cbk] : vs->build_object_cbks)
        if (match == node["m_type"].as_str())
            co_return co_await cbk(vs, name, node);

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vc::except_t{std::format("Invalid object type: {}", node["m_type"].as_str())};
}

static std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
        /* TODO: sure about this? */
        DBG("The path is restricted to the application main directory");
        throw vc::except_t(std::format("File_error [{} vs {}]", file_path, app_path));
    }

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

co::task_t build_pseudo_object(vc::virt_state_t *vs, const std::string& name, fkyaml::node& node) {
    for (auto &[match, cbk] : vs->build_psudo_object_cbks)
        if (match(name, node)) {
            int ret = co_await cbk(vs, name, node)
            ASSERT_ECOFN(ret);
            co_return 0;
        }

    /* builtin integer resolution */
    if (node.is_integer()) {
        auto obj = integer_t::create(node.as_int());
        mark_dependency_solved(vs, name, obj.to_related<vc::object_t>());
        co_return 0;
    }

    /* builtin */
    if (node.is_string()) {
        auto obj = string_t::create(node.as_str());
        mark_dependency_solved(vs, name, obj.to_related<vc::object_t>());
        co_return 0;
    }

    if (name == "lua_script") {
        if (!(node.contains("m_source") || node.contains("source_path"))) {
            DBG("lua-script must be a node that has either source or source-path")
            co_return -1;
        }

        if (node.contains("m_source") && node.contains("m_source_path")) {
            DBG("lua-script can be either loaded from inline script or from a specified path, not"
                    "from both!");
            co_return -1;
        }

        if (node.contains("m_source")) {
            auto obj = lua_script_t::create(node["m_source"].as_str());
            mark_dependency_solved(vs, name, obj.to_related<vku::object_t>());
            co_return 0;
        }

        if (node.contains("m_source_path")) {
            std::string source = get_file_string_content(node["m_source_path"].as_str());

            auto obj = lua_script_t::create(source);
            mark_dependency_solved(vs, name, obj.to_related<vku::object_t>());
            co_return 0;
        }
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

std::string new_anon_name(virt_state_t *vs) {
    return "anonymous_" + std::to_string(vs->anonymous_increment++);
}

static co::task_t build_schema(vc::virt_state_t *vs, fkyaml::node& root) {
    ASSERT_COFN(CHK_BOOL(root.is_mapping()));

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("m_type")) {
            co_await co::sched(vkc::build_pseudo_object(vs, name.as_str(), node));
        }
        else {
            co_await co::sched(vkc::build_object(vs, name.as_str(), node));
        }
    }

    co_return 0;
}

err_e parse_config(vc::virt_state_t *vs, const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        auto pool = co::create_pool();
        pool->sched(build_schema(vs, config));

        if (pool->run() != co::RUN_OK) {
            DBG("Failed to create the schema");
            return VC_ERROR_GENERIC;
        }

        if (vs->ps.wanted_objects.size()) {
            for (auto &[k, v]: vs->ps.wanted_objects) {
                DBG("Unknown Object: %s", k.c_str());
            }
            return VC_ERROR_PARSE_YAML;
        }
    }
    catch (fkyaml::exception &e) {
        DBG("fkyaml::exception: %s", e.what());
        return VC_ERROR_PARSE_YAML;
    }
    catch (std::exception &e) {
        DBG("Exception: %s", e.what());
        return VC_ERROR_GENERIC;
    }

    return VC_ERROR_NONE;
}

static int luaopen_vku(lua_State *L) {
    int top = lua_gettop(L);
    auto vs = luaw_get_virt_state(L);

    /* TODO: when a vc name is not found, maybe the solution is not to despair and error out, but
    instead of that, stop the execution via a coroutine and wait for it's
    OBS: SEE The other todo */

    {
        /* This metatable describes a generic vkc/vku object inside lua. Practically, it expososes
        member objects and functions to lua. */
        luaL_newmetatable(L, "__vku_metatable");

        lua_pushcfunction(L, [](lua_State *L) {
            int id = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
            auto vs = luaw_get_virt_state(L);
            auto &o = vs->ps.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
            }
            lua_pushstring(L, o.obj->to_string().c_str());
            return 1;
        });
        lua_setfield(L, -2, "__tostring");

        /* params: 1.usrptr, 2.key -> returns: 1.value */
        lua_pushcfunction(L, [](lua_State *L) {
            // DBG("__index: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -1); /* an const char *, ok on unwind */

            // DBG("usr_id: %d", id);
            // DBG("member_name: %s", member_name);

            auto vs = luaw_get_virt_state(L);
            auto &o = vs->ps.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
            }
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VIRT_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
            }
            if (!has(lua_class_members[class_id], member_name)) {
                if (strcmp(member_name, "rebuild") == 0) try {
                    o.obj.rebuild();
                    return 0;
                } catch (...) { return luaw_catch_exception(L); }
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
            }
            auto &member = lua_class_members[class_id][member_name];
            if (member.member_type == LUAW_MEMBER_FUNCTION) {
                lua_pushcfunction(L, member.fn);
                return 1;
            }
            else if (member.member_type == LUAW_MEMBER_OBJECT) {
                return member.fn(L);
            } else {
                luaw_push_error(L, std::format("NOT IMPLEMENTED YET: non-function member access"));
            }
            luaw_push_error(L, std::format("INTERNAL ERROR: shouldn't reach here"));
            return 0;
        });
        lua_setfield(L, -2, "__index");

        /* params: 1.usrptr, 2.key, 3.value  */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__newindex: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -3)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -2); /* an const char *, ok on unwind */

            DBG("usr_id: %d", id);
            DBG("member_name: %s", member_name);

            auto vs = luaw_get_virt_state(L);
            auto &o = vs->ps.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
            }
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VIRT_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
            }
            if (!has(lua_class_member_setters[class_id], member_name)) {
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
            }
            auto &member = lua_class_member_setters[class_id][member_name];
            return member(L);
        });
        lua_setfield(L, -2, "__newindex");

        /* params: 1.usrptr [... rest of params] */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__call: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, 1)); /* an int, ok on unwind */

            DBG("usr_id: %d", id);

            auto vs = luaw_get_virt_state(L);
            auto &o = vs->ps.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
            }
            DBG("tostr: %s", o.obj->to_string().c_str());
            vku::object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id != VKC_TYPE_LUA_FUNCTION) {
                luaw_push_error(L, std::format("invalid class id: {} is not VKC_TYPE_LUA_FUNCTION",
                        vku::to_string(class_id)));
            }
            return o.obj.to_related<lua_function_t>()->call(L);
        });
        lua_setfield(L, -2, "__call");

        /* params: 1.usrptr */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__gc");

            int id = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
            auto vs = luaw_get_virt_state(L);
            auto &o = vs->ps.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                return 0;
            }

            /* The object is no longer known to lua, as such we also delete it's slot. Obs: It may
            still be alive, meaning, it is known by the c++ side, just not by the lua side.
            !!! It will also loose it's name with this operation (Is that really ok?) */
            o.obj->cbks->usr_ptr = nullptr;

            /* we clean it's name mapping, it's reference and free it's id */
            vs->ps.objects_map.erase(o.name);
            o = vc::object_ref_t{};
            vs->ps.free_objects.push_back(id);
            return 0;
        });
        lua_setfield(L, -2, "__gc");

        /* vku::window_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::window_t, m_name);
        VKC_REG_MEMB(vku::window_t, m_width);
        VKC_REG_MEMB(vku::window_t, m_height);

        /* vku::instance_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::instance_t, m_app_name);
        VKC_REG_MEMB(vku::instance_t, m_engine_name);
        VKC_REG_MEMB(vku::instance_t, m_extensions);
        VKC_REG_MEMB(vku::instance_t, m_layers);

        /* vku::cmdbuff_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_FN(vku::cmdbuff_t, begin, bm_t<VkCommandBufferUsageFlagBits>);
        VKC_REG_FN(vku::cmdbuff_t, begin_rpass, vku::ref_t<vku::framebuffs_t>, uint32_t);
        VKC_REG_FN(vku::cmdbuff_t, bind_vert_buffs,
                uint32_t, std::vector<std::pair<vku::ref_t<vku::buffer_t>, VkDeviceSize>>);
        VKC_REG_FN(vku::cmdbuff_t, bind_desc_set,
                bm_t<VkPipelineBindPoint>, vku::ref_t<vku::pipeline_t>, vku::ref_t<vku::desc_set_t>);
        VKC_REG_FN(vku::cmdbuff_t, bind_idx_buff,
                vku::ref_t<vku::buffer_t>, uint64_t, bm_t<VkIndexType>);
        VKC_REG_FN(vku::cmdbuff_t, draw, vku::ref_t<vku::pipeline_t>, uint64_t);
        VKC_REG_FN(vku::cmdbuff_t, draw_idx, vku::ref_t<vku::pipeline_t>, uint64_t);
        VKC_REG_FN(vku::cmdbuff_t, end_rpass);
        VKC_REG_FN(vku::cmdbuff_t, end);
        luaw_register_member_function<vku::cmdbuff_t, &vku::cmdbuff_t::end>("end_begin");
        VKC_REG_FN(vku::cmdbuff_t, reset);
        VKC_REG_FN(vku::cmdbuff_t, bind_compute, vku::ref_t<vku::compute_pipeline_t>);
        VKC_REG_FN(vku::cmdbuff_t, dispatch_compute, uint32_t, uint32_t, uint32_t);

        /* vkc::cpu_buffer_t
        ----------------------------------------------------------------------------------------- */
        VKC_REG_FN(vkc::cpu_buffer_t, data);
        VKC_REG_FN(vkc::cpu_buffer_t, size);

        /* all others:
        ----------------------------------------------------------------------------------------- */
        for (auto &cbk : cbk_register_members)
            cbk();

        /* Done objects
        ----------------------------------------------------------------------------------------- */

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");

        lua_pop(L, 1); /* pop luaL_newmetatable */
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top == lua_gettop(L))); /* sanity check */

    {
        /* Registers the vulkan_utils library and some standalone functions from vku(vulkan utils)
        or vkc(vulkan composer) */
        vku_tab_funcs.push_back({NULL, NULL});
        luaL_checkversion(L);
        lua_createtable(L, 0, vku_tab_funcs.size() - 1);
        luaL_setfuncs(L, vku_tab_funcs.data(), 0);

        /* Registers this lua table for later use */
        vs->lua_table_idx = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);

        /* Registers glfw enum in this library */
        luaw_set_glfw_fields(L); /* This adds all glfw enums tokens */

        /* Registers objects loaded from the yaml confing as objects in the library */
        auto vs = luaw_get_virt_state(L);
        for (auto &[k, id] : vs->ps.objects_map) {
            if (!vs->ps.objects[id].obj) {
                DBG("Null user object?");
            }
            DBG("Registering object: %s with id: %d", k.c_str(), id);
            /* this makes vulkan_utils.key = object_id and sets it's metadata */
            lua_pushlightuserdata(L, luaw_to_user_data(id));
            luaL_setmetatable(L, "__vku_metatable");
            lua_setfield(L, -2, k.c_str());
        }

        /* Registers vulkan enums in the lua library */
        register_flag_mapping(L, vk_format_from_str);
        register_flag_mapping(L, vk_vertex_input_rate_from_str);
        register_flag_mapping(L, vk_shader_stage_flag_bits_from_str);
        register_flag_mapping(L, vk_descriptor_type_from_str);
        register_flag_mapping(L, vk_pipeline_stage_flag_bits_from_str);
        register_flag_mapping(L, vk_index_type_from_str);
        register_flag_mapping(L, vk_pipeline_bind_point_from_str);
        register_flag_mapping(L, vk_command_buffer_usage_flag_bits_from_str);
        register_flag_mapping(L, vk_image_aspect_flag_bits_from_str);
        register_flag_mapping(L, vk_primitive_topology_from_str);
        register_flag_mapping(L, vk_memory_property_flag_bits_from_str);
        register_flag_mapping(L, vk_sharing_mode_from_str);
        register_flag_mapping(L, vk_buffer_usage_flag_bits_from_str);
        register_flag_mapping(L, shader_stage_from_string);
        register_flag_mapping(L, vkc_constant_sizes_from_string);

        for (auto &cbk : cbk_register_mapping)
            cbk(L);
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top + 1 == lua_gettop(L))); /* sanity check */

    return 1;
}

static lua_State *luaw_init(vc::virt_state_t *vs) {
    lua_State *L = luaL_newstate();
    if (L == NULL) {
        DBG("Failed to init lua");
        return nullptr;
    }

    lua_pushstring(L, "virt_state");
    lua_pushlightuserdata(L, vs);
    lua_settable(L, LUA_REGISTRYINDEX);

    luaL_requiref(L, "vulkan_utils", luaopen_vku, 1);      lua_pop(L, 1);
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);          lua_pop(L, 1);
    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);   lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);     lua_pop(L, 1);

    /* TODO: configure if we want or don't want to be able to access the system */
    /* We don't want lua to access our system, so we intentionally don't include those */
    // luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1); lua_pop(L, 1);
    // luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1); lua_pop(L, 1);

    // if (!has(vs->objects_map, "lua_script")) {
    //     DBG("Config didn't provide a starter script to execute");
    //     return nullptr;
    // }
    // auto start_script = vs->objects[vs->objects_map["lua_script"]].obj.to_related<lua_script_t>();
    // if (luaL_loadstring(L, start_script->content.c_str()) != LUA_OK) {
    //     DBG("LUA Load Failed: \n%s", lua_tostring(L, -1));
    //     lua_close(L);
    //     return nullptr;
    // }
    // if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
    //     DBG("LUA Exec Failed: \n%s", lua_tostring(L, -1));
    //     lua_close(L);
    //     return nullptr;
    // }
 
    return L;
}

int luaw_catch_exception(lua_State *L) {
    /* We don't let errors get out of the call because we don't want to break lua. As such, we catch
    any error and propagate it as a lua error. */
    try {
        throw ; // re-throw the current exception
    }
    catch (vc::except_t &err) {
        /* TODO: add a callback here */
        // if (vkerr.vk_err == VK_SUBOPTIMAL_KHR) {
        //     DBG("TODO: resize? Somehow...");
        //     ASSERT_FN(luaw_execute_window_resize(800, 600));
        // }
        // else {
        // }
        luaw_push_error(L, std::format("Invalid call: {}", err.what()));
    }
    catch (fkyaml::exception &e) {
        luaw_push_error(L, std::format("fkyaml::exception: {}", e.what()));
    }
    catch (std::exception &e) {
        luaw_push_error(L, std::format("std::exception: {}", e.what()));
    }
    catch (...) {
        throw ; /* most probably the lua string */
    }

    return 0;
}

vc::virt_state_t *luaw_get_virt_state(lua_State *L) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_vs_idx);
    auto ptr = (vc::virt_state_t *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ptr;
}

vc::ref_t<vc::object_t> luaw_get_object_at_index(vc::virt_state_t *vs, ssize_t index) {
    if (index <= 0 && index >= vs->ps.objects.size()) {
        return nullptr; /* 0 is also invalid from our point of view */
    }
    return vs->ps.objects[obj_index].obj;
}

void luaw_push_error(lua_State *L, const std::string& err_str) {
    lua_Debug ar;
    std::string context;
    int i = 2;
    auto line_source = [](const char *src, int N) -> std::string {
        if (!src)
            return "<unknown>";
        std::istringstream stream(src);
        std::string line;
        int current = 1;

        while (std::getline(stream, line)) {
            if (current == N)
                return line;
            current++;
        }
        return "<unknown>";
    };

    while (lua_getstack(L, i, &ar)) {
        lua_getinfo(L, "nSl", &ar);
        context = std::format("      at {:>20}:{:<4}, in '{}':\n",
                ar.short_src, ar.currentline, line_source(ar.source, ar.currentline)) + context;
        i++;
    }
    if (lua_getstack(L, 1, &ar)) {
        lua_getinfo(L, "nSl", &ar);
        context += std::format("Error at {:>20}:{:<4}, in '{}':\n",
                ar.short_src, ar.currentline, line_source(ar.source, ar.currentline));
    }
    context += err_str;
    lua_pushstring(L, context.c_str());
    lua_error(L);
}

/* Lua is interesting... It seems that I can use next(#t) or next(nil) to check if the table is an
array or a dictionary + lua_rawlen to check for both. yadayada, I need to write it in code */
fkyaml::node create_yaml_from_lua_object(lua_State *L, int index) {
    index = lua_absindex(L, index);
    if (lua_isboolean(L, index)) {
        fkyaml::node ret(fkyaml::node_type::BOOLEAN);
        ret.as_bool() = lua_toboolean(L, index);
        return ret;
    }
    else if (lua_isinteger(L, index)) {
        fkyaml::node ret(fkyaml::node_type::INTEGER);
        ret.as_int() = lua_tointeger(L, index);
        return ret;
    }
    else if (lua_isnumber(L, index)) {
        fkyaml::node ret(fkyaml::node_type::FLOAT);
        ret.as_float() = lua_tonumber(L, index);
        return ret;
    }
    else if (lua_isstring(L, index)) {
        fkyaml::node ret(fkyaml::node_type::STRING);
        ret.as_str() = lua_tostring(L, index) ? lua_tostring(L, index) : "";
        return ret;
    }
    else if (lua_isnil(L, index)) {
        fkyaml::node ret(fkyaml::node_type::NULL_OBJECT);
        return ret;
    }
    else if (lua_istable(L, index)) {
        ; /* we continue bellow */
    }
    else {
        luaw_push_error(L, std::format("Unknown conversion from type: {} to yaml object",
                lua_typename(L, lua_type(L, index))));
    }

    bool array_detected = false;
    bool dict_detected = false;
    int array_len;

    /* AFAIK only arrays have a rawlen */
    if ((array_len = lua_rawlen(L, index)) != 0)
        array_detected = true;

    /* Assuming that lua_next is continuous for arrays (next(t, k) -> k+1), we must do two things:
    First check if the first key is in the array, if not, than this table also has dict keys, else
    any potential dictionary key will be placed after the array. (continued bellow...) */
    lua_pushnil(L);
    if (lua_next(L, index) != 0) {
        if ((lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) < 1 ||
                lua_tointeger(L, -2) >= array_len))
        {
            dict_detected = true;
        }
        lua_pop(L, 2);
    }
    else return fkyaml::node{fkyaml::node_type::MAPPING}; /* If empty we return an empty table */

    /* (...continuation from above) As such, second we now check if any dictionary key exists after
    the array part. */
    if (array_detected) {
        lua_pushinteger(L, array_len);
        if (lua_next(L, index) != 0) {
            if ((lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) < 1 ||
                    lua_tointeger(L, -2) > array_len))
            {
                dict_detected = true;
            }
            lua_pop(L, 2);
        }
    }

    if (array_detected && dict_detected) {
        luaw_push_error(L, "Create object doesn't support tables with both a hash part and "
                "an array part");
    }

    if (array_detected) {
        int len = lua_rawlen(L, index);
        fkyaml::node to_ret(fkyaml::node_type::SEQUENCE);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            auto to_add = create_yaml_from_lua_object(L, -1);
            to_ret.as_seq().push_back(to_add);
            lua_pop(L, 1);
        }
        return to_ret;
    }

    if (dict_detected) {
        lua_pushnil(L);
        fkyaml::node to_ret(fkyaml::node_type::MAPPING);
        while (lua_next(L, index) != 0) {
            const char *key = lua_tostring(L, -2);
            if (key) {
                auto to_add = create_yaml_from_lua_object(L, -1);
                to_ret[key] = to_add;
            }
            lua_pop(L, 1);
        }
        return to_ret;
    }

    luaw_push_error(L, "internal_error: shouldn't reach here");
    return fkyaml::node{};
}

inline int internal_create_object(lua_State *L) {
    const char *name = lua_tostring(L, 1);
    if (!name) {
        luaw_push_error(L, "Error at index 1: first parameter must be a string, the tag of the "
                "object");
        lua_error(L);
    }
    auto object_description = create_yaml_from_lua_object(L, 2);

    /* We copy the whole objects ref state, such that for now we have an exact copy of the global
    vku namespace and we can reference it's objects. If we error out, the only references that will
    remain alive are those that where backed up by g_rs and if we don't error out, at the end we
    append the differences to g_rs. */
    auto vs = luaw_get_virt_state(L);
    vc::virt_state_t ref_state = *vs;

    DBG("create_object: %s", fkyaml::node::serialize(object_description).c_str());
    auto pool = co::create_pool();

    if (!object_description.contains("m_type")) {
        pool->sched(vkc::build_pseudo_object(&ref_state, name, object_description));
    }
    else {
        pool->sched(build_object(&ref_state, name, object_description));
    }

    if (pool->run() != co::RUN_OK) {
        luaw_push_error(L, "CO_OJECT_CREATOR: Failed to create the object");
        lua_error(L);
    }

    if (ref_state.wanted_objects.size()) {
        std::string unknown_objects = "[";
        for (auto &[k, v]: ref_state.wanted_objects) {
            unknown_objects += std::format("{}, ", k);
        }
        unknown_objects += "]";
        luaw_push_error(L, std::format("unknown objects: {}", unknown_objects));
        lua_error(L);
    }

    if (!has(ref_state.objects_map, name)) {
        luaw_push_error(L, "internal_error: Object is not found after creation");
        lua_error(L);
    }

    auto new_idx = vs->get_new(ref_state);

    DBG("Getting lua table...");

    /* Get back the vulkan_utils table */
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);

    for (int id : new_idx) {
        if (!ref_state.objects[id].obj) {
            DBG("Null user object?");
        }
        DBG("Registering object: %s with id: %d", ref_state.objects[id].name.c_str(), id);
        /* this makes vulkan_utils.key = object_id and sets it's metadata */
        lua_pushlightuserdata(L, luaw_to_user_data(id));
        luaL_setmetatable(L, "__vku_metatable");
        lua_setfield(L, -2, ref_state.objects[id].name.c_str());
    }

    lua_getfield(L, -1, name);
    lua_remove(L, -2); /* pops vulkan_utils table */

    /* actualize the global state */
    vs->append(ref_state);

    /* Eventual errors are catched outside of this function */
    return 1;
}


// // helper to detect if a type is vku::ref_t<...>
// template <typename>
// struct is_vku_ref_t : std::false_type {};

// template <typename T>
// struct is_vku_ref_t<vku::ref_t<T>> : std::true_type {};

// // helper to detect if a type is vku::ref_t<...>
// template <typename T>
// concept is_vku_enum = requires(fkyaml::node n) {
//     get_enum_val<T>(n);
// };

// /* getter */
// template <typename VkuT, auto member_ptr>
// int luaw_member_object_wrapper(lua_State *L) {
//     try {
//         int index = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
//         if (index == 0) {
//             luaw_push_error(L, "Nil user object can't get member!");
//         }
//         auto vs = luaw_get_virt_state(L);
//         auto &o = vs->ps.objects[index];
//         if (!o.obj) {
//             luaw_push_error(L, "internal_error: Nil user object can't get member!");
//         }
//         auto obj = o.obj.to_related<VkuT>();
//         auto &member = obj.get()->*member_ptr;

//         using member_type = std::decay_t<decltype(member)>;

//         if constexpr (std::is_same_v<member_type, std::string>) {
//             lua_pushstring(L, member.c_str());
//             return 1;
//         }
//         else if constexpr (std::is_integral_v<member_type>) {
//             lua_pushinteger(L, member);
//             return 1;
//         }
//         else if constexpr (std::is_floating_point_v<member_type>) {
//             lua_pushnumber(L, member);
//             return 1;
//         }
//         else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
//             lua_createtable(L, member.size(), 0);
//             for (size_t i = 1; auto &str : member) {
//                 lua_pushstring(L, str.c_str());
//                 lua_rawseti(L, -2, i++);
//             }
//             return 1;
//         }
//         else if constexpr (is_vku_enum<member_type>) {
//             lua_pushnumber(L, (int)member);
//             return 1;
//         }
//         else if constexpr (is_vku_ref_t<member_type>::value) {
//             if (!member) {
//                 lua_pushnil(L);
//                 return 1;
//             }
//             if (!member->cbks) {
//                 luaw_push_error(L, "internal_error: How did this object get known to lua ?!");
//             }
//             if (!member->cbks->usr_ptr) {
//                 /* So this object was no longer known by the lua side, we must resurect it */

//                 /* We first get it a new id */
//                 int new_id = vs->ps.free_objects.back();
//                 vs->ps.free_objects.pop_back();

//                 /* make it reference it's own id */
//                 member->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});

//                 /* add it's lua-name-mapping and it's lua-id-mapping */
//                 std::string name = new_anon_name();
//                 vs->ps.objects_map[name] = new_id;
//                 vs->ps.objects[new_id].obj = member;
//                 vs->ps.objects[new_id].name = name;
//             }
//             int member_id = (intptr_t)member->cbks->usr_ptr.get();
//             if (member_id >= vs->ps.objects.size() || member_id < 0) {
//                 luaw_push_error(L, "internal_error: Integrity check failed");
//             }
//             lua_pushlightuserdata(L, luaw_to_user_data(member_id));
//             luaL_setmetatable(L, "__vku_metatable");
//             return 1;
//         }
//         else {
//             demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
//             return 0;
//         }
//     }
//     catch (...) { return luaw_catch_exception(L); }
// }

// template <typename VkuT, auto member_ptr>
// int luaw_member_setter_object_wrapper(lua_State *L) {
//     int index = luaw_from_user_data(lua_touserdata(L, -3)); /* an int, ok on unwind */
//     if (index == 0) {
//         luaw_push_error(L, "Nil user object can't set member!");
//     }
//     auto vs = luaw_get_virt_state(L);
//     auto &o = vs->ps.objects[index];
//     if (!o.obj) {
//         luaw_push_error(L, "internal_error: Nil user object can't set member!");
//     }
//     auto obj = o.obj.to_related<VkuT>();
//     auto &member = obj.get()->*member_ptr;

//     using member_type = std::decay_t<decltype(member)>;

//     if constexpr (std::is_same_v<member_type, std::string>) {
//         const char *str = lua_tostring(L, -1);
//         member = str ? str : "";
//         return 0;
//     }
//     else if constexpr (std::is_integral_v<member_type>) {
//         uint64_t val = lua_tointeger(L, -1);
//         member = (member_type)val;
//         return 0;
//     }
//     else if constexpr (std::is_floating_point_v<member_type>) {
//         double val = lua_tonumber(L, -1);
//         member = (member_type)val;
//         return 0;
//     }
//     else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
//         if (!lua_istable(L, -1)) {
//             luaw_push_error(L, "You need a table for this assignment!");
//         }
//         int len = lua_rawlen(L, -1);
//         std::vector<std::string> to_asign;
//         for (int i = 1; i <= len; i++) {
//             lua_rawgeti(L, -1, i);
//             const char *str = lua_tostring(L, -1);
//             to_asign.push_back(str ? str : "");
//             lua_pop(L, 1);
//         }
//         member = to_asign;
//         return 0;
//     }
//     else if constexpr (is_vku_enum<member_type>) {
//         uint64_t val = lua_tointeger(L, -1);
//         member = (member_type)val;
//         return 0;
//     }
//     else if constexpr (is_vku_ref_t<member_type>::value) {
//         int index = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
//         if (index == 0) {
//             member = nullptr;
//             return 0;
//         }
//         member = vs->ps.objects[index].obj;
//         return 0;
//     }
//     else {
//         demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
//         return 0;
//     }
// }

// template <typename VkuT, auto member_ptr, typename ...Params>
// void luaw_register_member_function(const char *function_name) {
//     lua_class_members[VkuT::type_id_static()][function_name] = luaw_member_t{
//         .fn = &luaw_member_function_wrapper<VkuT, member_ptr, Params...>,
//         .member_type = LUAW_MEMBER_FUNCTION
//     };
// }

// template <typename VkuT, auto member_ptr>
// void luaw_register_member_object(const char *member_name) {
//     lua_class_members[VkuT::type_id_static()][member_name] = luaw_member_t{
//         .fn = &luaw_member_object_wrapper<VkuT, member_ptr>,
//         .member_type = LUAW_MEMBER_OBJECT
//     };

//     lua_class_member_setters[VkuT::type_id_static()][member_name] =
//             &luaw_member_setter_object_wrapper<VkuT, member_ptr>;
// }

// #define VKC_REG_MEMB(obj_type, memb)            \
// vulkan_composer::luaw_register_member_object<   \
// /* self        */   obj_type,                   \
// /* member      */   &obj_type::memb>            \
// /* member name */   (#memb)

// #define VKC_REG_FN(obj_type, fn, ...)           \
// vulkan_composer::luaw_register_member_function< \
// /* self     */  obj_type,                       \
// /* function */  &obj_type::fn,                  \
// /* params   */  ##__VA_ARGS__>                  \
// /* fn name  */  (#fn)

// inline void glfw_pool_events() {
//     glfwPollEvents();
// }

// inline uint32_t glfw_get_key(vku::ref_t<vku::window_t> window, uint32_t key) {
//     if (!window)
//         throw vc::except_t("Window parameter can't be null");
//     return glfwGetKey(window->get_window(), key);
// }

// inline void internal_signal_close() {
//     /* TODO: set loop closed */
//     DBG("TODO: set loop closed");
// }

// inline uint32_t internal_aquire_next_img(
//         vku::ref_t<vku::swapchain_t> swc, vku::ref_t<vku::sem_t> sem)
// {
//     uint32_t ret;
//     vku::aquire_next_img(swc, sem, &ret);
//     return ret;
// }

// inline uint32_t internal_device_wait_handle(vku::ref_t<vku::device_t> dev) {
//     return vkDeviceWaitIdle(dev->vk_dev);
// }

// inline int copy_from_cpu_to_gpu(vku::ref_t<vku::buffer_t> dst, void *src,
//         size_t len, size_t off)
// {
//     (void)dst, void(src), void(len), void(off);
//     DBG("TODO: transfer cpu->gpu");
//     return 0;
// }

// inline int copy_from_gpu_to_cpu(void *dst, vku::ref_t<vku::buffer_t> src,
//         size_t len, size_t off)
// {
//     (void)dst, void(src), void(len), void(off);
//     DBG("TODO: transfer gpu->cpu");
//     return 0;
// }

// static std::vector<luaL_Reg> vku_tab_funcs = {
//     {"glfw_pool_events",    luaw_function_wrapper<glfw_pool_events>},
//     {"get_key",             luaw_function_wrapper<glfw_get_key,
//             vku::ref_t<vku::window_t>, uint32_t>},
//     {"signal_close",        luaw_function_wrapper<internal_signal_close>},
//     {"aquire_next_img",     luaw_function_wrapper<internal_aquire_next_img,
//             vku::ref_t<vku::swapchain_t>, vku::ref_t<vku::sem_t>>},
//     {"submit_cmdbuff",      luaw_function_wrapper<vku::submit_cmdbuff,
//             std::vector<std::pair<vku::ref_t<vku::sem_t>, bm_t<VkPipelineStageFlagBits>>>,
//             vku::ref_t<vku::cmdbuff_t>, vku::ref_t<vku::fence_t>,
//             std::vector<vku::ref_t<vku::sem_t>>>},
//     {"present",             luaw_function_wrapper<vku::present,
//             vku::ref_t<vku::swapchain_t>,
//             std::vector<vku::ref_t<vku::sem_t>>,
//             uint32_t>},
//     {"wait_fences",         luaw_function_wrapper<vku::wait_fences,
//             std::vector<vku::ref_t<vku::fence_t>>>},
//     {"reset_fences",        luaw_function_wrapper<vku::reset_fences,
//             std::vector<vku::ref_t<vku::fence_t>>>},
//     {"device_wait_handle",  luaw_function_wrapper<internal_device_wait_handle,
//             vku::ref_t<vku::device_t>>},
//     {"copy_from_cpu_to_gpu",luaw_function_wrapper<copy_from_cpu_to_gpu,
//             vku::ref_t<vku::buffer_t>, void *, size_t, size_t>},
//     {"copy_from_gpu_to_cpu",luaw_function_wrapper<copy_from_gpu_to_cpu,
//             void *, vku::ref_t<vku::buffer_t>, size_t, size_t>},
// };

// inline void luaw_set_glfw_fields(lua_State *L);

// void register_flag_mapping(lua_State *L, auto &mapping) {
//     for (auto& [k, v] : mapping) {
//         lua_pushinteger(L, (uint32_t)v);
//         lua_setfield(L, -2, k.c_str());
//     }
// };

// static std::vector<std::function<void(lua_State *L)>> cbk_register_mapping;
// static std::vector<std::function<void(void)>> cbk_register_members;


// inline vkc_error_e luaw_execute_loop_run(lua_State *L) {
//     lua_getglobal(L, "on_loop_run");
//     if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
//         DBG("LUA luaw_execute_loop_run Failed: \n%s", lua_tostring(L, -1));
//         return VKC_ERROR_FAILED_CALL;
//     }
//     return VKC_ERROR_OK;
// }

// inline vkc_error_e luaw_execute_window_resize(lua_State *L, int width, int height) {
//     lua_getglobal(L, "on_window_resize");
//     lua_pushinteger(L, width);
//     lua_pushinteger(L, height);
//     if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
//         DBG("LUA luaw_execute_window_resize Failed: \n%s", lua_tostring(L, -1));
//         return VKC_ERROR_FAILED_CALL;
//     }
//     return VKC_ERROR_OK;
// }




/*! TODO: When executing a lua script, we can use the follwing trick to resolve unknown variables
 * from inside the __index callback:
 */
// co::task_t co_interrupted() {
//     DBG_SCOPE();
//     auto state = co_await co::get_state();
//     auto pool = co_await co::get_pool();
//     auto callback = [state, pool](){
//         DBG("Entered the callback");
//         auto continuation = [&]() -> co::task_t {
//             struct stop_awaitable_t : public std::suspend_always {
//                 std::coroutine_handle<void> await_suspend(std::coroutine_handle<>) {
//                     return std::noop_coroutine();
//                 }
//             };

//             co_await sem->wait();
//             co_await stop_awaitable_t{};
//             co_return 0;
//         }();
//         DBG("Starting a continuation coroutine");
//         colib::external_init_task(state, pool);
//         continuation.h.resume(); // Manually start the coroutine
//         continuation.h.destroy();
//         DBG("Done the callback");
//     };

//     DBG("Before the callback");
//     callback(); // This would be the place where we call Lua and it calls us back
//     DBG("After the callback");
//     co_return 0;
// }

} /* namespace virt_composer */
