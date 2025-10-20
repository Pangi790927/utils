#ifndef VULKAN_COMPOSER_H
#define VULKAN_COMPOSER_H

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

#define LUA_IMPL

#include "vulkan_utils.h"
#include "yaml.h"
#include "tinyexpr.h"
#include "minilua.h"
#include "demangle.h"

#include <coroutine>
#include <filesystem>

/* TODO: find a better name for this file */

/* What do I really care about? So I can make a format that covers all the things I want:
    1. I want to be able to have buffers with:
        a. matrices of different dimensionalities, dimensions and object sizes:
            - one cell for a simple struct buffer
            - vectors of floats, ints, complex, etc.
            - vectors of more complex objects
            - matrices of the above with 1 dimension, 2, 3, etc.
        b. images (specifically rgb data), those will also be (I think) framebuffers and the sorts
        c. not a category, but different types of buffers, ubo's and normal in/outs
    2. Semaphores and Fences with clear linkages with objects (buffer - shaders, etc)
    3. Shaders, here, I need to have a way to describe what inputs I can have maybe? Or maybe a way
    to describe a computation pipeline
    4. User defined steps, for example moments the user needs to fill in data to be drawn, or moment
    the user sets ubos
    5. End point of drawing step

    (I should create a json in the way I want it to look, maybe in this way I figure the thing
    better)
 */

// struct vulkan_instance_t {
//     std::string filename;
//     vku_opts_t vku_opts;    /*! json object: opts */  
// };

#define ASSERT_VKC_CO(fn_call)     \
do {                               \
    auto ret = (fn_call);          \
    if (ret != VKC_ERROR_OK) {     \
        DBG("Failed " #fn_call);   \
        co_return -1;              \
    }                              \
} while (0)

#define ASSERT_BOOL_CO(bool_expr)  \
do {                               \
    auto ret = (bool_expr);        \
    if (!ret) {                    \
        DBG("Failed " #bool_expr); \
        co_return -1;              \
    }                              \
} while (0)

namespace vkc {

enum vkc_error_e : int32_t {
    VKC_ERROR_OK = 0,
    VKC_ERROR_GENERIC = -1, 
    VKC_ERROR_PARSE_YAML = -2,
    VKC_ERROR_LUA_CALL = -3,

    VKC_ERROR_FAILED_CALL = -4,
    VKC_ERROR_FAILED_LOAD = -5,
    VKC_ERROR_FAILED_LUA_INIT = -6,
    VKC_ERROR_FAILED_LUA_LOAD = -7,
    VKC_ERROR_FAILED_LUA_EXEC = -8,
};

using namespace vku_utils;          /* This is temporary here */

inline std::string app_path = std::filesystem::weakly_canonical("./");

inline std::map<std::string, vku_ref_p<vku_object_t>> objects;
inline std::map<std::string, std::vector<std::coroutine_handle<void>>> wanted_objects;
inline std::deque<std::coroutine_handle<void>> work;

struct vkc_lua_var_t : public vku_object_t {
    std::string name;
    static vku_ref_p<vkc_lua_var_t> create(std::string name) {
        auto ret = vku_ref_t<vkc_lua_var_t>::create_obj_ref(std::make_unique<vkc_lua_var_t>(), {});
        ret->get()->name = name;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct vkc_lua_script_t : public vku_object_t {
    std::string content;
    static vku_ref_p<vkc_lua_script_t> create(std::string content) {
        auto ret = vku_ref_t<vkc_lua_script_t>::create_obj_ref(std::make_unique<vkc_lua_script_t>(), {});
        ret->get()->content = content;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};


struct vkc_integer_t : public vku_object_t {
    int64_t value = 0;
    static vku_ref_p<vkc_integer_t> create(int64_t value) {
        auto ret = vku_ref_t<vkc_integer_t>::create_obj_ref(std::make_unique<vkc_integer_t>(), {});
        ret->get()->value = value;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct vkc_string_t : public vku_object_t {
    std::string value;
    static vku_ref_p<vkc_string_t> create(const std::string& value) {
        auto ret = vku_ref_t<vkc_string_t>::create_obj_ref(std::make_unique<vkc_string_t>(), {});
        ret->get()->value = value;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct vkc_spirv_t : public vku_object_t {
    vku_spirv_t spirv;
    static vku_ref_p<vkc_spirv_t> create(const vku_spirv_t& spirv) {
        auto ret = vku_ref_t<vkc_spirv_t>::create_obj_ref(std::make_unique<vkc_spirv_t>(), {});
        ret->get()->spirv = spirv;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

inline vkc_error_e vkc_error_g = VKC_ERROR_OK;
inline int code_incr = 0;
inline std::exception_ptr vkc_exception_g = nullptr;
struct co_t {
    struct co_state_t {
        struct final_awaiter_t {
            bool await_ready() noexcept  { return false; }
            void await_resume() noexcept {}
            
            std::coroutine_handle<void> await_suspend(
                    std::coroutine_handle<co_state_t> ending_task) noexcept
            {
                /* If we where called by someone, we continue him */
                if (ending_task.promise().caller)
                    return ending_task.promise().caller;

                /* The task ended because of an exception */
                if (ending_task.promise().exception) {
                    /* TODO: Unwind old errors */
                    vkc_exception_g = ending_task.promise().exception;
                    return std::noop_coroutine();
                }

                /* The task that ended is a root task and it errored out, so we return an error */
                if (ending_task.promise().retval != VKC_ERROR_OK) {
                    vkc_error_g = VKC_ERROR_GENERIC;
                    return std::noop_coroutine();
                }

                /* Else we check if there is more work to be done, if not we exit the coro-link */
                if (work.empty())
                    return std::noop_coroutine();

                /* More work to do, we do that: */
                auto ret = work.front();
                work.pop_front();
                return ret;
            }
        };
        co_t get_return_object() {
            return co_t{std::coroutine_handle<co_state_t>::from_promise(*this)};
        }

        std::suspend_always initial_suspend() noexcept    { return {}; }
        final_awaiter_t     final_suspend() noexcept      { return {}; }
        void                unhandled_exception()         { exception = std::current_exception(); }
        void                return_value(int ret)         { return_value((vkc_error_e)ret); }

        void return_value(vkc_error_e ret) {
            /* TODO: fix this */
            if (!caller && ret != VKC_ERROR_OK) {
                throw std::runtime_error("Failed dangling coro");
            }
            this->retval = ret;
        }

        std::coroutine_handle<co_state_t> caller = nullptr;
        std::exception_ptr exception = nullptr;
        vkc_error_e retval = VKC_ERROR_GENERIC;
    };

    using handle_t = std::coroutine_handle<co_state_t>;
    using promise_type = co_state_t;

    co_t() {}
    co_t(handle_t h) : h(h) {}

    /* This is for calling  */
    bool await_ready() noexcept { return false; }
    handle_t await_suspend(handle_t caller) noexcept {
        caller.promise().caller = h;    /* we first mark the coroutine we will return to and */
        return h;                       /* we return this coroutine as the continuation */
    }
    vkc_error_e await_resume() {
        /* We save the exception pointer(even if no exception) and the return value */
        std::exception_ptr exc_ptr = h.promise().exception;
        auto ret = h.promise().retval;

        /* We now destroy the coroutine as it returned and is no longer usefull */
        h.destroy();

        /* finally we check if we return or except(if we have an exception to propagate): */        
        if (exc_ptr)
            std::rethrow_exception(exc_ptr);
        return ret;
    }

    handle_t h;
};

/*! What this thing does:
 *      - If the object with the name required_depend exists, it returns it as the requested
 *          type (that is, if the dynamic cast works, else it throws)
 *      - Else if it doesn't exists, it waits until it becomes available
 */
template <typename VkuT>
struct depend_resolver_t {
    /* We save the searched dependency */
    depend_resolver_t(std::string required_depend) : required_depend(required_depend) {}

    /* If we already have the dependency we can already retur */
    bool await_ready() noexcept { return has(objects, required_depend); }

    /* Else we place ourselves on the waiting queue */
    std::coroutine_handle<void> await_suspend(co_t::handle_t caller) noexcept {
        /* We place ourselves on the waiting queue: */
        wanted_objects[required_depend].push_back(caller);

        /* If we don't have anymore work that can be done we must return back to the initial
        and there we will see that depends can't be resolved and error out */
        if (work.empty())
            return std::noop_coroutine();

        /* Else we return the next work in line that can be done */
        auto ret = work.front();
        work.pop_front();
        return ret;
    }

    vku_ref_p<VkuT> await_resume() {
        if (!has(objects, required_depend)) {
            DBG("Object not found");
            throw vku_err_t(std::format("Object not found, {}", required_depend));
        }
        if (!objects[required_depend]) {
            DBG("For some reason this object now holds a nullptr...");
            throw vku_err_t("nullptr object");
        }
        auto ret = objects[required_depend]->to_derived<VkuT>();
        if (!ret) {
            DBG("Invalid ref...");
            throw vku_err_t(sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    demangle<4>(typeid(objects[required_depend]->get()).name()).c_str(),
                    demangle<VkuT, 4>().c_str()));
        }
        return ret;
    }

    std::string required_depend;
};

void mark_dependency_solved(std::string depend_name, vku_ref_p<vku_object_t> depend) {
    /* First remember the dependency: */
    if (!depend) {
        DBG("Object into nullptr");
        throw vku_err_t{std::format("Object turned into nullptr: {}", depend_name)};
    }
    objects[depend_name] = depend;

    /* Second, awake all the ones waiting for the respective dependency */
    if (has(wanted_objects, depend_name)) {
        work.insert(work.begin(),
                wanted_objects[depend_name].begin(), wanted_objects[depend_name].end());
        wanted_objects.erase(depend_name);
    }
}


inline void builder_sched(co_t new_work) {
    /* work that is to be resumed by ending coroutines, each coroutine that ends enqueues the next
    available work to be done via the final awaiter */
    work.push_back(new_work.h);
}

co_t build_object(const std::string& name, fkyaml::node& node);

inline bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

inline std::unordered_map<std::string, vku_shader_stage_t> shader_stage_from_string = {
    {"VKU_SPIRV_VERTEX",    VKU_SPIRV_VERTEX},
    {"VKU_SPIRV_FRAGMENT",  VKU_SPIRV_FRAGMENT},
    {"VKU_SPIRV_COMPUTE",   VKU_SPIRV_COMPUTE},
    {"VKU_SPIRV_GEOMETRY",  VKU_SPIRV_GEOMETRY},
    {"VKU_SPIRV_TESS_CTRL", VKU_SPIRV_TESS_CTRL},
    {"VKU_SPIRV_TESS_EVAL", VKU_SPIRV_TESS_EVAL},
};

inline auto get_from_map(auto &m, const std::string& str) {
    if (!has(m, str))
        throw std::runtime_error(std::format("Failed to get object: {} from: {}",
                str, demangle<decltype(m), 2>()));
    return m[str];
}

inline std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::weakly_canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
        DBG("The path is restricted to the application main directory");
        throw std::runtime_error("File_error");
    }

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

co_t build_pseudo_object(const std::string& name, fkyaml::node& node) {
    if (node.is_integer()) {
        auto obj = vkc_integer_t::create(node.as_int());
        mark_dependency_solved(name, obj->to_base<vku_object_t>());
        co_return 0;
    }

    if (node.is_string()) {
        auto obj = vkc_string_t::create(node.as_str());
        mark_dependency_solved(name, obj->to_base<vku_object_t>());
        co_return 0;
    }

    if (node.is_mapping() && node.contains("shader_type")) {
        vku_spirv_t spirv;

        if (node.contains("source")) {
            spirv = vku_spirv_compile(
                    get_from_map(shader_stage_from_string, node["shader_type"].as_str()),
                    node["source"].as_str().c_str());
        }

        if (node.contains("source-path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }
            spirv = vku_spirv_compile(
                    get_from_map(shader_stage_from_string, node["shader_type"].as_str()),
                    get_file_string_content(node["source-path"].as_str()).c_str());
        }

        if (node.contains("spirv-path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }

            spirv.type = get_from_map(shader_stage_from_string, node["shader_type"].as_str());
            std::string file_path = std::filesystem::weakly_canonical(node["spirv-path"].as_str());

            if (!starts_with(file_path, app_path)) {
                DBG("The path is restricted to the application main directory");
                co_return 0;
            }

            std::ifstream file(file_path.c_str(), std::ios::binary | std::ios::ate);
            std::streamsize size = file.tellg();

            if (size % sizeof(uint32_t) != 0) {
                DBG("File must be a shader, so it must have it's data multiple of %zu",
                        sizeof(uint32_t));
                co_return 0;
            }

            file.seekg(0, std::ios::beg);
            spirv.content.resize(size / sizeof(uint32_t));
            if (!file.read((char *)spirv.content.data(), size)) {
                DBG("Failed to read shader data");
                co_return -1;
            }
        }
        
        if (!spirv.content.size()) {
            DBG("Spirv shader can't be empty!")
            co_return -1;
        }

        auto obj = vkc_spirv_t::create(spirv);
        mark_dependency_solved(name, obj->to_base<vku_object_t>());

        co_return 0;
    }

    if (name == "lua-script") {
        if (!(node.contains("source") || node.contains("source-path"))) {
            DBG("lua-script must be a node that has either source or source-path")
            co_return -1;
        }

        if (node.contains("source") && node.contains("source-path")) {
            DBG("lua-script can be either loaded from inline script or from a specified path, not"
                    "from both!");
            co_return -1;
        }

        if (node.contains("source")) {
            auto obj = vkc_lua_script_t::create(node["source"].as_str());
            mark_dependency_solved(name, obj->to_base<vku_object_t>());
            co_return 0;
        }

        if (node.contains("source-path")) {
            std::string source = get_file_string_content(node["source-path"].as_str());

            auto obj = vkc_lua_script_t::create(source);
            mark_dependency_solved(name, obj->to_base<vku_object_t>());
            co_return 0;
        }
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

#define VKC_RESOLVE_INT(node) \
    (node).has_tag_name() && (node).get_tag_name() == "!ref" ? \
              (co_await depend_resolver_t<vkc_integer_t>((node).as_str()))->get()->value \
            : (node).as_int();

#define VKC_RESOLVE_STR(node) \
    (node).has_tag_name() && (node).get_tag_name() == "!ref" ? \
              (co_await depend_resolver_t<vkc_string_t>((node).as_str()))->get()->value \
            : (node).as_str();

co_t build_object(const std::string& name, fkyaml::node& node) {
    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return -1;
    }

    if (false);
    else if (node["type"] == "vku_instance_t") {
        auto obj = vku_instance_t::create();
        mark_dependency_solved(name, obj->to_base<vku_object_t>());
    }
    else if (node["type"] == "lua_var_t") {
        auto obj = vkc_lua_var_t::create(name);
        mark_dependency_solved(name, obj->to_base<vku_object_t>());
    }
    else if (node["type"] == "vku_window_t") {
        if (node["name"].has_tag_name()) {
            DBG("Has tag name");
            DBG("serialize: %s", fkyaml::node::serialize(node["width"]).c_str());
            DBG("content: %s", node["width"].as_str().c_str());
            DBG("Tag name: %s", (node["width"]).get_tag_name().c_str());
        }
        else {
            DBG("only serialize: %s", fkyaml::node::serialize(node["name"]).c_str());
        }
        auto w = VKC_RESOLVE_INT(node["width"]);
        auto h = VKC_RESOLVE_INT(node["height"]);
        auto name = VKC_RESOLVE_STR(node["name"]);
        auto obj = vku_window_t::create(w, h, name);
        mark_dependency_solved(name, obj->to_base<vku_object_t>());
    }
    else if (node["type"] == "vku_surface_t") { /* TODO: */ }
    else if (node["type"] == "vku_device_t") { /* TODO: */ }
    else if (node["type"] == "vku_cmdpool_t") { /* TODO: */ }
    else if (node["type"] == "vku_image_t") { /* TODO: */ }
    else if (node["type"] == "vku_img_view_t") { /* TODO: */ }
    else if (node["type"] == "vku_img_sampl_t") { /* TODO: */ }
    else if (node["type"] == "vku_buffer_t") { /* TODO: */ }
    else if (node["type"] == "vku_binding_desc_t") { /* TODO: */ }
    else if (node["type"] == "vku_binding_desc_t::buff_binding_t") { /* TODO: */ }
    else if (node["type"] == "vku_binding_desc_t::sampl_binding_t") { /* TODO: */ }
    else if (node["type"] == "vku_shader_t") { /* TODO: */ }
    else if (node["type"] == "vku_swapchain_t") { /* TODO: */ }
    else if (node["type"] == "vku_renderpass_t") { /* TODO: */ }
    else if (node["type"] == "vku_pipeline_t") { /* TODO: */ }
    else if (node["type"] == "vku_framebuffs_t") { /* TODO: */ }
    else if (node["type"] == "vku_sem_t") { /* TODO: */ }
    else if (node["type"] == "vku_fence_t") { /* TODO: */ }
    else if (node["type"] == "vku_cmdbuff_t") { /* TODO: */ }
    else if (node["type"] == "vku_desc_pool_t") { /* TODO: */ }
    else if (node["type"] == "vku_desc_set_t") { /* TODO: */ }

    co_return 0;
}


co_t build_schema(fkyaml::node& root) {
    ASSERT_BOOL_CO(root.is_mapping());

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("type")) {
            builder_sched(build_pseudo_object(name.as_str(), node));
        }
        else {
            builder_sched(build_object(name.as_str(), node));
        }
    }

    co_return 0;
}

inline vkc_error_e parse_config(const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        auto coro = build_schema(config).h;
        coro.resume();

        coro.destroy();
        if (vkc_exception_g) {
            DBG("Failed because of an exception");
            std::rethrow_exception(vkc_exception_g);
        }
        if (vkc_error_g != VKC_ERROR_OK) {
            DBG("Failed parse global");
            return VKC_ERROR_GENERIC;
        }

        if (wanted_objects.size()) {
            for (auto &[k, v]: wanted_objects) {
                DBG("Unknown Object: %s", k.c_str());
            }
            return VKC_ERROR_PARSE_YAML;
        }
    }
    catch (fkyaml::exception &e) {
        DBG("fkyaml::exception: %s", e.what());
        return VKC_ERROR_PARSE_YAML;
    }
    catch (std::exception &e) {
        DBG("Exception: %s", e.what());
        return VKC_ERROR_GENERIC;
    }

    return VKC_ERROR_OK;
}


inline std::string lua_example_str = R"___(
vku = require("vulkan_utils")

function on_loop_run()
    vku.glfw_pool_events() -- needed for keys to be available
    if vku.get_key(vku.window, vku.GLFW_KEY_ESCAPE) == vku.GLFW_PRESS then
        vku.signal_close() -- this will close the loop
    end

    img_idx = vku.aquire_next_img(vku.swc, vku.img_sem)

    -- do mvp update somehow?

    vku.cbuff.begin()
    vku.cbuff.begin_rpass(vku.fbs, img_idx)
    vku.cbuff.bind_vert_buffs(0, {{vku.vbuff, 0}})
    vku.cbuff.bind_idx_buff(vku.ibuff, 0, vku.VK_INDEX_TYPE_UINT16)
    vku.cbuff.bind_desc_set(vku.VK_PIPELINE_BIND_POINT_GRAPHICS, vku.pl, vku.desc_set)
    vku.cbuff.draw_idx(vku.pl, indices.size())
    vku.cbuff.end_rpass()
    vku.cbuff.end_begin()

    vku.submit_cmdbuff({{vku.img_sem, vku.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
        vku.cbuff, vku.fence, {vku.draw_sem})
    vku.present(vku.swc, {vku.draw_sem}, img_idx)

    vku.wait_fences({vku.fence})
    vku.reset_fences({vku.fence})
end

function on_window_resize(w, h)
    vku.device_wait_handle(vku.dev)

    vku.window.set_width(w)
    vku.window.set_height(h)

    -- this will recurse and rebuild all dependees on window
    vku.window.rebuild()
end
)___";

inline int impl_glfw_pool_events(lua_State *L) {
    DBG_SCOPE();
    (void)L;
    return 0;
}

inline int impl_get_key(lua_State *L) {
    DBG_SCOPE();
    (void)L;
    return 0;
}

inline int impl_TODO(lua_State *L) {
    DBG_SCOPE();
    (void)L;
    return 0;
}

inline const luaL_Reg vku_tab_funcs[] = {
    {"glfw_pool_events", impl_TODO},
    {"get_key", impl_TODO},
    {"signal_close", impl_TODO},
    {"aquire_next_img", impl_TODO},
    {NULL, NULL}
};

inline int luaopen_vku (lua_State *L) {
    luaL_newlib(L, vku_tab_funcs);

    /* This is also a great example of how to register all the types:
        - all the nmes will be registered and depending of their types
        - all their members and functions will also be registered
        - all the numbers will be also registered as integers
        - each type that is not a constant will have a _t that holds the user ptr
        */

    /* this makes vulkan_utils.cbuff = {} */
    lua_createtable(L, 0, 0);
    lua_setfield(L, -2, "cbuff");

    /* This pushes the reference of vulkan_utils.cbuff on the stack */
    lua_getfield(L, -1, "cbuff");

    /* this makes vulkan_utils.cbuff.begin = begin_fn */
    lua_pushcfunction(L, impl_TODO);
    lua_setfield(L, -2, "begin");

    /* this makes vulkan_utils.cbuff.begin_rpass = begin_rpass_fn */
    lua_pushcfunction(L, impl_TODO);
    lua_setfield(L, -2, "begin_rpass");

    /* this makes vulkan_utils.cbuff.bind_vert_buffs = bind_vert_buffs_fn */
    lua_pushcfunction(L, impl_TODO);
    lua_setfield(L, -2, "bind_vert_buffs");

    /* this makes vulkan_utils.cbuff._this = &false_user_data */
    char false_user_data;
    lua_pushlightuserdata(L, &false_user_data);
    lua_setfield(L, -2, "_t");

    /* Poping the reference of cbuff from the stack */
    lua_pop(L, 1);

    return 1;
}

inline lua_State *lua_state;
inline vkc_error_e luaw_init() {
    lua_state = luaL_newstate();
    if (lua_state == NULL) {
        DBG("Failed to init lua");
        return VKC_ERROR_FAILED_LUA_INIT;
    }

    luaL_requiref(lua_state, "vulkan_utils", luaopen_vku, 1);       lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_GNAME, luaopen_base, 1);           lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_LOADLIBNAME, luaopen_package, 1);  lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_COLIBNAME, luaopen_coroutine, 1);  lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_TABLIBNAME, luaopen_table, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_STRLIBNAME, luaopen_string, 1);    lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_MATHLIBNAME, luaopen_math, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_UTF8LIBNAME, luaopen_utf8, 1);     lua_pop(lua_state, 1);
    luaL_requiref(lua_state, LUA_DBLIBNAME, luaopen_debug, 1);      lua_pop(lua_state, 1);

    /* We don't want lua to access our system, so we intentionally don't include those */
    // luaL_requiref(L, LUA_IOLIBNAME, luaopen_io, 1); lua_pop(L, 1);
    // luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1); lua_pop(L, 1);

    if (luaL_loadstring(lua_state, lua_example_str.c_str()) != LUA_OK) {
        DBG("LUA Load Failed: \n%s", lua_tostring(lua_state, -1));
        lua_close(lua_state);
        return VKC_ERROR_FAILED_LUA_LOAD;
    }
    if (lua_pcall(lua_state, 0, 0, 0) != LUA_OK) {
        DBG("LUA Exec Failed: \n%s", lua_tostring(lua_state, -1));
        lua_close(lua_state);
        return VKC_ERROR_FAILED_LUA_EXEC;
    }
 
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_uninit() {
    lua_close(lua_state);
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_execute_loop_run() {
    lua_getglobal(lua_state, "on_loop_run");
    if (lua_pcall(lua_state, 0, 0, 0) != LUA_OK) {
        DBG("LUA luaw_execute_loop_run Failed: \n%s", lua_tostring(lua_state, -1));
        return VKC_ERROR_FAILED_CALL;
    }
    return VKC_ERROR_OK;
}

inline vkc_error_e luaw_execute_window_resize(int width, int height) {
    lua_getglobal(lua_state, "on_window_resize");
    lua_pushinteger(lua_state, width);
    lua_pushinteger(lua_state, height);
    if (lua_pcall(lua_state, 2, 0, 0) != LUA_OK) {
        DBG("LUA luaw_execute_window_resize Failed: \n%s", lua_tostring(lua_state, -1));
        return VKC_ERROR_FAILED_CALL;
    }
    return VKC_ERROR_OK;
}

}; /* namespace vkc */

#endif
