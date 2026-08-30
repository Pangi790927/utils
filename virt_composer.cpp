#ifdef _MSC_VER
# define NOMINMAX
#endif

#include <array>
#include <filesystem>
#include <fstream>

#define LUA_IMPL

#include "virt_composer.h"

#include "co_utils.h"
#include "yaml.h"
#include "tinyexpr.h"

/* TODO: - add object dependencyes, ie objects that need to load before loading the curent one
         - in the same idea, make a vector of variables, such that those vars will be available in
           both lua and tinyexpr resolver */

namespace virt_composer
{

namespace vo = virt_object;
namespace vc = virt_composer;

/* Max number of named references objects */
static constexpr const int MAX_NUMBER_OF_OBJECTS = 16384;

/*! Holds information of a member, either a member funtion or a member object */
struct luaw_member_t {
    lua_CFunction fn;
    luaw_member_e member_type;
};

/*! Holds a function that will copy from that respective member, also holds the type_index of the
 * member to check at runtime that the copy doesn't come from  */
struct trivial_copy_member_t {
    std::type_index tid{typeid(void)};
    std::function<void(vc::object_t *, void *, size_t)> copy_fn;
};

/*! This holds the state of the  */
struct virt_state_t {
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

    ~virt_state_t() {
        DBG_SCOPE();
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }
};

struct box_t {
    vc::ref_t<vc::object_t> self_obj;   // the strong ref - Lua's actual claim on the object
};

/* This is the path the application was run from */
static std::string app_path = std::filesystem::canonical("./").string();


static lua_State *luaw_init(vc::virt_state_t *vs);
static int internal_create_object(lua_State *L);
static int luaopen_vc(lua_State *L);
 
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
    /* You are not supposed to create state globaly, if you did that and it breaks, then that's on
    you */
    ASSERT_RET(nullptr, CHK_BOOL(VIRT_TYPES_INITIALIZED));

    auto vs = std::make_shared<virt_state_t>();
    vs->pool = co::create_pool();

    ASSERT_RET(nullptr, CHK_PTR(vs->L = luaw_init(vs.get())));
    ASSERT_RET(nullptr, add_lua_tab_funcs(vs.get(), {{"create_object", internal_create_object}}));

    VC_REGISTER_MEMBER_OBJECT(vs.get(), integer_t, value);
    VC_REGISTER_MEMBER_OBJECT(vs.get(), float_t, value);
    VC_REGISTER_MEMBER_OBJECT(vs.get(), string_t, value);

    /* lua_object_t::push() mirrored as a Lua-visible member function - the explicit way to get
    the raw captured value back out of its boxed vc reference (see lua_object_t's own doc
    comment). Registered directly against the raw lua_CFunction shape (not via
    VC_REGISTER_MEMBER_FUNCTION) since push() operates on the stack directly rather than
    returning a typed value luaw_returner_t could convert. */
    set_lua_class_member(vs.get(), lua_object_t::type_id_static(), "push",
            [](lua_State *L) -> int {
                auto obj = get_object_from_lua(L, 1);
                if (!obj)
                    luaw_push_error(L, "internal_error: Nil user object can't push!");
                obj->to_related<lua_object_t>()->push(L);
                return 1;
            }, LUAW_MEMBER_FUNCTION);

    /* lua_object_t::capture(L) mirrored as a Lua-visible member function - `self:capture(x)`
    pushes exactly [self, x], so x is already on top of the stack when capture(L) runs, matching
    its "operate on whatever's on top" contract with no repositioning needed.

    - I (the human) aprove of this comment */
    set_lua_class_member(vs.get(), lua_object_t::type_id_static(), "capture",
            [](lua_State *L) -> int {
                auto obj = get_object_from_lua(L, 1);
                if (!obj)
                    luaw_push_error(L, "internal_error: Nil user object can't capture!");
                obj->to_related<lua_object_t>()->capture(L);
                return 0;
            }, LUAW_MEMBER_FUNCTION);

    /* lua_object_t::release() mirrored as a Lua-visible member function - takes no args and
    returns void, so the normal member-function macro handles it directly. */
    VC_REGISTER_MEMBER_FUNCTION(vs.get(), lua_object_t, release);

    return vs;
}

ref_t<vc::object_t> get_ref_base(virt_state_t *vs, const std::string& name) {
    if (!has(vs->name_to_object, name))
        return nullptr;
    return vs->name_to_object[name]->shared_this();
}

bool depend_resolver_internal_t::internal_check_depend(const std::string &dep_name) {
     return has(vs->name_to_object, dep_name);
}

void depend_resolver_internal_t::internal_mark_wait(const std::string &dep_name, co::state_t *state) {
    vs->wanted_objects[dep_name].push_back(state);
}

vc::ref_t<vc::object_t> depend_resolver_internal_t::internal_get_dep_object(
        const std::string &dep_name)
{
    if (!has(vs->name_to_object, dep_name)) {
        DBG("Object not found");
        throw vc::except_t(std::format("Object not found, {}", dep_name));
    }
    return vs->name_to_object[dep_name]->shared_this();
}

std::string depend_resolver_internal_t::internal_get_obj_type_name(const std::string &dep_name) {
    return demangle<4>(typeid(vs->name_to_object[dep_name]).name()).c_str();
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
    vs->build_psudo_object_cbks.push_back({analyser, builder});
    return VC_ERROR_OK;
}

err_e add_lua_tab_funcs(virt_state_t *vs, const std::vector<luaL_Reg>& vc_tab_funcs) {
    vs->tab_funcs.pop_back();
    vs->tab_funcs.insert(vs->tab_funcs.end(), vc_tab_funcs.begin(), vc_tab_funcs.end());
    vs->tab_funcs.push_back({NULL, NULL});

    lua_rawgeti(vs->L, LUA_REGISTRYINDEX, vs->lua_table_idx);
    luaL_setfuncs(vs->L, vs->tab_funcs.data(), 0);
    return VC_ERROR_OK;
}

err_e add_lua_flag_mapping(virt_state_t *vs,
        const std::vector<std::pair<lua_Integer, std::string>> &mapping)
{
    auto L = vs->L;
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);
    for (auto& [v, k] : mapping) {
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
    if (has(vs->name_to_object, depend_name)) {
        DBG("Name taken");
        throw vc::except_t{std::format("Tag name already exists: {}", depend_name)};
    }

    vs->name_to_object[depend_name] = depend.get();
    vs->object_to_name[depend.get()] = depend_name;

    lua_rawgeti(vs->L, LUA_REGISTRYINDEX, vs->lua_table_idx);
    push_vc_object(vs->L, depend);
    lua_setfield(vs->L, -2, depend_name.c_str());
    lua_pop(vs->L, 1);

    /* Second, awake all the ones waiting for the respective dependency */
    if (::has(vs->wanted_objects, depend_name)) {
        for (auto s : vs->wanted_objects[depend_name])
            co::external_sched_resume(s);
        vs->wanted_objects.erase(depend_name);
    }
}

static double resolve_string_as_expression(std::string expr_str,
        vc::virt_state_t *vs)
{
    std::vector<texpr::te_variable> vars;
    for (auto &[name, value] : vs->constants)
        vars.push_back(texpr::te_variable{
            .name = name.c_str(),
            .address = (void *)&value,
            .type = texpr::TE_VARIABLE,
            .context = nullptr,
        });

    int err = 0;
    texpr::te_expr *expr = texpr::te_compile(expr_str.c_str(), vars.data(), vars.size(), &err, nullptr);

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
        co_return (int64_t)std::round(resolve_string_as_expression(node.as_str(), vs));
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
        co_return resolve_string_as_expression(node.as_str(), vs);
    }
    else if (node.is_integer()) {
        co_return (double)node.as_int();
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

/*! used inside resolve_memb */
co::task_t resolve_memb_data(virt_state_t *vs, const std::string &obj_name,
        const std::string& memb_name, void *dst, size_t sz, std::type_index tid)
{

    auto obj = co_await vc::depend_resolver_t<object_t>(vs, obj_name);
    auto type = obj->type_id();

    if (!has(vs->trivial_copy_member[type], memb_name))
        throw vc::except_t(std::format(
                "This member[{}] from object[{}] was not registered for copy typeid[{}]",
                memb_name, obj_name, (int64_t)type));

    auto &copy_info = vs->trivial_copy_member[type][memb_name];
    if (tid != copy_info.tid)
        throw vc::except_t(std::format("For member[{}] from object[{}] type mismatch {} vs {}",
                memb_name, obj_name, tid.name(), copy_info.tid.name()));

    copy_info.copy_fn(obj.get(), dst, sz);
    co_return 0;
}

static bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

static std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative).string();

    /* This restriction only makes sense while Lua scripts have no other way to touch the
    filesystem - once VIRT_COMPOSER_ENABLE_LUA_IO/_OS exposes Lua's io/os standard libraries to
    scripts directly, a script can already read/write anything the process can, so enforcing it
    here would just be a false sense of security rather than an actual boundary. */
#if !(VIRT_COMPOSER_ENABLE_LUA_IO || VIRT_COMPOSER_ENABLE_LUA_OS)
    if (!starts_with(file_path, app_path)) {
        DBG("The path is restricted to the application main directory");
        throw vc::except_t(std::format("File_error [{} vs {}]", file_path, app_path));
    }
#endif

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

static co::task<vc::ref_t<vc::object_t>> init_lua_script(vc::virt_state_t *vs,
        const std::string& name, fkyaml::node& node)
{
    if (!(node.contains("m_source") || node.contains("m_source_path"))) {
        DBG("lua-script must be a node that has either m_source or m_source_path")
        co_return nullptr;
    }

    if (node.contains("m_source") && node.contains("m_source_path")) {
        DBG("lua-script can be either loaded from inline script or from a specified path, not"
                "from both!");
        co_return nullptr;
    }

    auto exec_lua_src = [](auto vs, const std::string& str){
        if (luaL_dostring(vs->L, str.c_str()) != LUA_OK) {
            DBG("LUA exec string Failed: \n%s", lua_tostring(vs->L, -1));
            lua_pop(vs->L, 1);
            return VC_ERROR_FAILED_CALL;
        }
        return VC_ERROR_OK;
    };

    if (node.contains("m_source")) {
        auto obj = lua_script_t::create(node["m_source"].as_str());
        if (exec_lua_src(vs, obj->content) != VC_ERROR_OK) {
            throw vc::except_t{"Failed to execute loaded script"};
        }
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj;
    }

    if (node.contains("m_source_path")) {
        std::string source = get_file_string_content(node["m_source_path"].as_str());
        auto obj = lua_script_t::create(source);
        if (exec_lua_src(vs, obj->content) != VC_ERROR_OK) {
            throw vc::except_t{"Failed to execute loaded script"};
        }
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj;
    }

    /* shouldn't reach here either way */
    co_return nullptr;
}

co::task<vc::ref_t<vc::object_t>> build_object(vc::virt_state_t *vs,
        const std::string& name, fkyaml::node& node)
{
    DBG("Building: %s", name.c_str());

    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return nullptr;
    }

    if (node["m_type"] == "vc::c_function_t") {
        /* lua_function has the same tag_name as the function name */
        auto src = co_await resolve_str(vs, node["m_source"]);
        auto obj = vc::c_function_t::create(name, src);
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj->to_related<vc::object_t>();
    }

    if (node["m_type"] == "vc::lua_script_t")
        co_return co_await init_lua_script(vs, name, node);

    if (node["m_type"] == "vc::integer_t") {
        auto obj = integer_t::create(co_await resolve_int(vs, node["value"]));
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj->to_related<vc::object_t>();
    }

    if (node["m_type"] == "vc::float_t") {
        auto obj = float_t::create(co_await resolve_float(vs, node["value"]));
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj->to_related<vc::object_t>();
    }

    if (node["m_type"] == "vc::string_t") {
        auto obj = string_t::create(co_await resolve_str(vs, node["value"]));
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return obj->to_related<vc::object_t>();
    }

    for (auto &[match, cbk] : vs->build_object_cbks)
        if (match == node["m_type"].as_str()) try {
            co_return co_await cbk(vs, name, node);
        }
        catch (...) {
            DBG("Excepted IN {Name %s Type: %s}", name.c_str(), match.c_str());
            throw;
        }

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vc::except_t{std::format("Invalid object type: {}", node["m_type"].as_str())};
}

co::task_t build_pseudo_object(vc::virt_state_t *vs, const std::string& name, fkyaml::node& node) {
    DBG("PseBuilding: %s", name.c_str());

    for (auto &[match, cbk] : vs->build_psudo_object_cbks)
        if (match(name, node)) try {
            int ret = co_await cbk(vs, name, node);
            /* TODO: this ecofn here seems suspicious */
            ASSERT_ECOFN(ret);
            co_return 0;
        }
        catch(...) {
            DBG("Excepted for %s", name.c_str());
            throw;
        }

    /* builtin integer resolution */
    if (node.is_integer()) {
        auto obj = integer_t::create(node.as_int());
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return 0;
    }

    if (node.is_float_number()) {
        auto obj = float_t::create(node.as_float());
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return 0;
    }

    /* builtin */
    if (node.is_string()) {
        auto obj = string_t::create(node.as_str());
        mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
        co_return 0;
    }

    if (name == "lua_script") {
        co_return (co_await init_lua_script(vs, name, node)) != nullptr ? 0 : -1;
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

std::string new_anon_name(virt_state_t *vs) {
    return "__" + std::to_string(vs->anonymous_increment++);
}

static co::task_t build_schema(vc::virt_state_t *vs, fkyaml::node& root) {
    ASSERT_COFN(CHK_BOOL(root.is_mapping()));

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("m_type")) {
            co_await co::sched(build_pseudo_object(vs, name.as_str(), node));
        }
        else {
            co_await co::sched(build_object(vs, name.as_str(), node));
        }
    }

    co_return 0;
}

err_e parse_config(vc::virt_state_t *vs, const char *path) {
    DBG_SCOPE();
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);
        vs->pool->sched(build_schema(vs, config));

        if (vs->pool->run() != co::RUN_OK) {
            DBG("Failed to create the schema");
            return VC_ERROR_GENERIC;
        }

        if (vs->wanted_objects.size()) {
            for (auto &[k, v]: vs->wanted_objects) {
                DBG("WARNING: Unknown Object: %s", k.c_str());
            }
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

    return VC_ERROR_OK;
}

/*! [INTERNAL] Shared `__add`/`__sub`/.../`__le` dispatcher for binary operators.
 *
 * Every vc object shares one metatable, so Lua invokes this same function whenever either operand
 * of the binary op is a vc object, regardless of whether that operand's specific type actually
 * registered a handler for `Op`. This checks operand 1 first, then operand 2, for a registered
 * handler; whichever one has it becomes "self" and its slot number (1 or 2) is pushed as a 3rd
 * stack argument so the handler can tell which side triggered the call (needed for non-commutative
 * ops). The handler is then called directly and its return value forwarded as-is - see
 * @ref set_class_operator for the full contract. */
template <operator_e Op>
static int luaw_binary_operator_dispatch(lua_State *L) {
    auto vs = luaw_get_virt_state(L);

    lua_CFunction fn = nullptr;
    int which = 0;

    for (int idx = 1; idx <= 2 && !fn; idx++) {
        auto obj = get_object_from_lua(L, idx);
        if (!obj)
            continue;
        vc::object_type_e class_id = obj->type_id();
        if (class_id < 0 || class_id >= (int)VIRT_TYPE_CNT) {
            luaw_push_error(L, std::format("invalid class id: {}", vc::to_string(class_id)));
            return 0;
        }
        if (auto candidate = vs->lua_class_operators[class_id][(size_t)Op]; candidate) {
            fn = candidate;
            which = idx;
        }
    }

    if (!fn) {
        luaw_push_error(L, "attempt to perform operation on incompatible vc objects");
        return 0;
    }

    lua_pushinteger(L, which);
    return fn(L);
}

/*! [INTERNAL] Shared `__unm`/`__bnot`/`__len` dispatcher for unary operators. Only one operand
 * exists, so there's no ambiguity and no "which" argument is pushed. */
template <operator_e Op>
static int luaw_unary_operator_dispatch(lua_State *L) {
    auto obj = get_object_from_lua(L, 1);
    if (!obj) {
        luaw_push_error(L, "internal_error: Nil user object can't perform unary operation!");
        return 0;
    }

    auto vs = luaw_get_virt_state(L);
    vc::object_type_e class_id = obj->type_id();
    if (class_id < 0 || class_id >= (int)VIRT_TYPE_CNT) {
        luaw_push_error(L, std::format("invalid class id: {}", vc::to_string(class_id)));
        return 0;
    }

    auto fn = vs->lua_class_operators[class_id][(size_t)Op];
    if (!fn) {
        luaw_push_error(L, std::format("class id {} has no operator registered",
                vc::to_string(class_id)));
        return 0;
    }

    return fn(L);
}

static int luaopen_vc(lua_State *L) {
    int top = lua_gettop(L);
    auto vs = luaw_get_virt_state(L);

    /* Table used to track objects inside lua */
    lua_newtable(L);
    lua_newtable(L);
    lua_pushstring(L, "v");
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
    vs->weak_cache_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* Table lua_object_t captures live in - plain (no __mode weak table), since here we
    deliberately want a strong hold on whatever gets captured. */
    lua_newtable(L);
    vs->lua_object_ref_table = luaL_ref(L, LUA_REGISTRYINDEX);

    {
        /* This metatable describes a generic vc object inside lua. Practically, it expososes
        member objects and functions to lua. */
        luaL_newmetatable(L, "__vc_metatable");

        lua_pushcfunction(L, [](lua_State *L) {
            auto obj = get_object_from_lua(L, -1);
            lua_pushstring(L, obj->to_string().c_str());
            return 1;
        });
        lua_setfield(L, -2, "__tostring");

        /* params: 1.usrptr, 2.key -> returns: 1.value */
        lua_pushcfunction(L, [](lua_State *L) {
            // DBG("__index: %d", lua_gettop(L));
            auto obj = get_object_from_lua(L, -2);
            const char *member_name = lua_tostring(L, -1); /* an const char *, ok on unwind */

            auto vs = luaw_get_virt_state(L);
            vc::object_type_e class_id = obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= (int)VIRT_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vc::to_string(class_id)));
            }
            if (!has(vs->lua_class_members[class_id], member_name)) {
                DBG("On ERROR Settrs: class_id[%s[%d]]", class_id.name(), class_id.value());
                for (auto &[k, m] : vs->lua_class_members[class_id]) {
                    DBG("key: %s", k.c_str());
                }
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vc::to_string(class_id), member_name));
            }
            auto &member = vs->lua_class_members[class_id][member_name];
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
            auto obj = get_object_from_lua(L, -3);
            const char *member_name = lua_tostring(L, -2); /* an const char *, ok on unwind */

            auto vs = luaw_get_virt_state(L);
            vc::object_type_e class_id = obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= (int)VIRT_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vc::to_string(class_id)));
            }
            if (!has(vs->lua_class_member_setters[class_id], member_name)) {
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vc::to_string(class_id), member_name));
            }
            auto &member = vs->lua_class_member_setters[class_id][member_name];
            return member(L);
        });
        lua_setfield(L, -2, "__newindex");

        /* params: 1.usrptr [... rest of params] */
        lua_pushcfunction(L, [](lua_State *L) {
            // DBG("__call: %d", lua_gettop(L));
            auto obj = get_object_from_lua(L, 1);
            if (!obj) {
                luaw_push_error(L, "invalid null object, ie called on nil");
            }
            vc::object_type_e class_id = obj->type_id(); /* an int, still ok on unwind */
            if (class_id != VC_TYPE_C_FUNCTION) {
                luaw_push_error(L, std::format("invalid class id: {} is not VC_TYPE_C_FUNCTION",
                        vc::to_string(class_id)));
            }
            lua_remove(L, 1); /* We don't want the function itself as an parameter */
            return obj->to_related<c_function_t>()->call(L);
        });
        lua_setfield(L, -2, "__call");

        /* params: 1.usrptr */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__gc");

            auto *box = (box_t *)luaL_testudata(L, -1, "__vc_metatable");
            if (!box) {
                DBG("Invalid: garbage colector called on invalid object");
                return 0;
            }

            auto &obj = box->self_obj;
            auto vs = luaw_get_virt_state(L);

            /* The object is no longer known to lua, as such we also delete it's slot. Obs: It may
            still be alive, meaning, it is known by the c++ side, just not by the lua side.
            !!! It will also loose it's name with this operation (Is that really ok?) */
            if (has(vs->object_to_name, obj.get())) {
                vs->name_to_object.erase(vs->object_to_name[obj.get()]);
                vs->object_to_name.erase(obj.get());
            }

            box->~box_t();
            return 0;
        });
        lua_setfield(L, -2, "__gc");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_ADD>);
        lua_setfield(L, -2, "__add");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_SUB>);
        lua_setfield(L, -2, "__sub");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_MUL>);
        lua_setfield(L, -2, "__mul");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_DIV>);
        lua_setfield(L, -2, "__div");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_MOD>);
        lua_setfield(L, -2, "__mod");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_POW>);
        lua_setfield(L, -2, "__pow");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_IDIV>);
        lua_setfield(L, -2, "__idiv");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_BAND>);
        lua_setfield(L, -2, "__band");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_BOR>);
        lua_setfield(L, -2, "__bor");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_BXOR>);
        lua_setfield(L, -2, "__bxor");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_SHL>);
        lua_setfield(L, -2, "__shl");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_SHR>);
        lua_setfield(L, -2, "__shr");

        lua_pushcfunction(L, &luaw_unary_operator_dispatch<VC_OPERATOR_UNM>);
        lua_setfield(L, -2, "__unm");

        lua_pushcfunction(L, &luaw_unary_operator_dispatch<VC_OPERATOR_BNOT>);
        lua_setfield(L, -2, "__bnot");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_CONCAT>);
        lua_setfield(L, -2, "__concat");

        lua_pushcfunction(L, &luaw_unary_operator_dispatch<VC_OPERATOR_LEN>);
        lua_setfield(L, -2, "__len");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_EQ>);
        lua_setfield(L, -2, "__eq");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_LT>);
        lua_setfield(L, -2, "__lt");

        lua_pushcfunction(L, &luaw_binary_operator_dispatch<VC_OPERATOR_LE>);
        lua_setfield(L, -2, "__le");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");

        lua_pop(L, 1); /* pop luaL_newmetatable */
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top == lua_gettop(L))); /* sanity check */

    {
        vs->tab_funcs.push_back({NULL, NULL});
        luaL_checkversion(L);
        lua_createtable(L, 0, vs->tab_funcs.size() - 1);
        luaL_setfuncs(L, vs->tab_funcs.data(), 0);

        /* Registers this lua table for later use */
        vs->lua_table_idx = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);
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

    luaL_requiref(L, "virt_composer", luaopen_vc, 1);      lua_pop(L, 1);
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);          lua_pop(L, 1);
    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);   lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);    lua_pop(L, 1);
    luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);     lua_pop(L, 1);

    /* We don't want lua to access our system, so we intentionally don't include those */

#if VIRT_COMPOSER_ENABLE_LUA_IO
    luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1); lua_pop(L, 1);
#endif
#if VIRT_COMPOSER_ENABLE_LUA_OS
    luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1); lua_pop(L, 1);
#endif

    return L;
}

int luaw_catch_exception(lua_State *L) {
    /* We don't let errors get out of the call because we don't want to break lua. As such, we catch
    any error and propagate it as a lua error. */
    try {
        DBG("Rethrow");
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
    lua_pushstring(L, "virt_state");
    lua_gettable(L, LUA_REGISTRYINDEX);
    auto ptr = (vc::virt_state_t *)lua_touserdata(L, -1);
    lua_pop(L, 1);
    return ptr;
}

lua_State *luaw_get_lua_state(vc::virt_state_t *vs) {
    return vs->L;
}

void set_trivial_copy_member(virt_state_t *vs, object_type_e type, const char *member_name,
        std::type_index tid, std::function<void(vc::object_t *, void *, size_t)> copy_fn)
{
    DBG("set_pod_member_read: %s type: %s[%d] memb_type: %s vs[%p] member_name[%s]",
            member_name, type.name(), type.value(),
            tid.name(), vs, member_name);
    vs->inheritance_table[type].insert(type);
    for (auto &d : vs->inheritance_table[type])
        vs->trivial_copy_member[d][member_name] = { .tid = tid, .copy_fn = copy_fn };
}

void set_lua_class_member(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn, luaw_member_e member_type)
{
    DBG("set_lua_class_member: %s type: %s[%d] memb_type: %s vs[%p] fn[%p]",
            member_name, type.name(), type.value(),
            member_type == LUAW_MEMBER_FUNCTION ? "'function'" : "'object'", vs, fn);
    /* So here is what must happen: all the derived objects must also get this member */
    vs->inheritance_table[type].insert(type);
    for (auto &d : vs->inheritance_table[type])
        vs->lua_class_members[d][member_name] = luaw_member_t{
            .fn = fn,
            .member_type = member_type
        };
}

void set_class_member_setter(virt_state_t *vs, object_type_e type, const char *member_name,
        lua_CFunction fn)
{
    DBG("set_class_member_setter: %s type: %s[%d] vs[%p] fn[%p]",
            member_name, type.name(), type.value(), vs, fn);
    /* So here is what must happen: all the derived objects must also get this member */
    vs->inheritance_table[type].insert(type);
    for (auto &d : vs->inheritance_table[type])
        vs->lua_class_member_setters[d][member_name] = fn;
}

void set_class_operator(virt_state_t *vs, object_type_e type, operator_e op, lua_CFunction fn) {
    DBG("set_class_operator: type: %s[%d] op[%d] vs[%p] fn[%p]",
            type.name(), type.value(), (int)op, vs, fn);
    /* So here is what must happen: all the derived objects must also get this operator */
    vs->inheritance_table[type].insert(type);
    for (auto &d : vs->inheritance_table[type])
        vs->lua_class_operators[d][(size_t)op] = fn;
}

void set_base_derived_relation(virt_state_t *vs, object_type_e base, object_type_e derived) {
    DBG("set_base_derived_relation set %s as base of %s", base.name(), derived.name());
    vs->inheritance_table[base].insert(base);
    vs->inheritance_table[derived].insert(derived);
    vs->inheritance_table[base].insert(derived);
}

int push_vc_object(lua_State *L, ref_t<object_t> object) {
    auto vs = luaw_get_virt_state(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->weak_cache_ref);     // get the object cache
    lua_pushlightuserdata(L, object.get());                    // the object ptr is the key
    lua_rawget(L, -2);                                         // get the lua object associated
    if (!lua_isnil(L, -1)) {                                   // if we got it we remove the table
        lua_remove(L, -2);                                     // and let the object on the stack
        return 0;
    }
    lua_pop(L, 1);                                             // else we remove the cache table

    auto *box = (box_t *)lua_newuserdatauv(L, sizeof(box_t), 0);  // we now create a new object
                                                                  // on the stack
    new (box) box_t{ object }; // initiate the object's reference
    luaL_setmetatable(L, "__vc_metatable"); // set our known metadata

    lua_pushlightuserdata(L, object.get());  // finally, we add it on the stack (first the key)
    lua_pushvalue(L, -2);                    // ordering, key must be before the value so we push it again
    lua_rawset(L, -4);                       // we set the key into the table
    lua_remove(L, -2);                       // we now remove the remaining copy of the box
    return 0;
}

object_t *get_object_from_lua(lua_State *L, int idx) {
    auto *box = (box_t *)luaL_testudata(L, idx, "__vc_metatable");
    return box ? box->self_obj.get() : nullptr;
}

void lua_object_t::capture(lua_State *L) {
    /* Release whatever this instance previously held, if anything - capture() replaces it. */
    release();

    /* Capturing nil is just releasing - without this, luaL_ref would hand back LUA_REFNIL (a
    valid-looking ref distinct from LUA_NOREF), leaving this instance in a state push()/call()'s
    "nothing captured" guards (ref == LUA_NOREF) wouldn't recognize. */
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    auto vs = luaw_get_virt_state(L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_object_ref_table);     // push the sub-table
    lua_insert(L, -2);                                              // [sub-table, value]
    int new_ref = luaL_ref(L, -2);                                  // pops value, refs into sub-table
    lua_pop(L, 1);                                                  // pop sub-table

    this->L = L;
    table_ref = vs->lua_object_ref_table;
    ref = new_ref;
}

void luaw_push_error(lua_State *L, const std::string& err_str, const std::source_location sloc) {
    DBG("Throwing error: %s SRC_LOC[%s:%d %s]",
            err_str.c_str(), sloc.file_name(), sloc.line(), sloc.function_name());
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
static fkyaml::node create_yaml_from_lua_object(lua_State *L, int index) {
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
    else if (auto *obj = get_object_from_lua(L, index)) {
        /* TODO: I don't find it ok that an object needs a name to be included in the object
        building mechanism, as such we may need a way to resolve it
        OPTIONS: - make two build_object one that depends on yaml and one that depends on lua
                 - swithc to only-lua */
        auto vs = luaw_get_virt_state(L);
        std::string name;
        if (has(vs->object_to_name, obj))
            name = vs->object_to_name[obj];
        else {
            name = new_anon_name(vs);
            vs->name_to_object[name] = obj;
            vs->object_to_name[obj] = name;
        }
        fkyaml::node ret(fkyaml::node_type::STRING);
        ret.as_str() = name;
        ret.add_tag_name("!ref");
        return ret;
    }
    else {
        luaw_push_error(L, std::format("Unknown conversion from type: {} to yaml object",
                lua_typename(L, lua_type(L, index))));
    }

    bool array_detected = false;
    bool dict_detected = false;
    int array_len;

    /* AFAIK only arrays have a rawlen */
    if ((array_len = lua_rawlen(L, index)) != 0) {
        array_detected = true;
    }

    /* Assuming that lua_next is continuous for arrays (next(t, k) -> k+1), we must do two things:
    First check if the first key is in the array, if not, than this table also has dict keys, else
    any potential dictionary key will be placed after the array. (continued bellow...) */
    lua_pushnil(L);
    if (lua_next(L, index) != 0) {
        if ((lua_type(L, -2) != LUA_TNUMBER || lua_tointeger(L, -2) < 1 ||
                lua_tointeger(L, -2) > array_len))
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

static int internal_create_object(lua_State *L) {
    auto vs = luaw_get_virt_state(L);
    const char *cname = lua_tostring(L, 1);
    std::string name;
    if (!cname)
        name = new_anon_name(vs);
    else
        name = cname;
    auto object_description = create_yaml_from_lua_object(L, 2);

    if (!object_description.contains("m_type")) {
        vs->pool->sched(vc::build_pseudo_object(vs, name, object_description));
    }
    else {
        vs->pool->sched(build_object(vs, name, object_description));
    }

    co::run_e ret;
    try {
        ret = vs->pool->run();
    }
    catch (fkyaml::exception &e) {
        luaw_push_error(L, sformat("fkyaml::exception: %s", e.what()));
        lua_error(L);
    }
    catch (std::exception &e) {
        luaw_push_error(L, sformat("Exception: %s", e.what()));
        lua_error(L);
    }
    if (ret != co::RUN_OK) {
        luaw_push_error(L, "CO_OJECT_CREATOR: Failed to create the object");
        lua_error(L);
    }

    if (vs->wanted_objects.size()) {
        for (auto &[k, v]: vs->wanted_objects) {
            DBG("WARNING: Unknown Object: %s", k.c_str());
        }
    }

    if (!has(vs->name_to_object, name)) {
        luaw_push_error(L, "internal_error: Object is not found after creation");
        lua_error(L);
    }

    /* Get back the virt_composer table */
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->lua_table_idx);
    lua_getfield(L, -1, name.c_str()); /* get the object with the respective name */
    lua_remove(L, -2); /* pops vulkan_utils table */

    return 1;
}

} /* namespace virt_composer */

