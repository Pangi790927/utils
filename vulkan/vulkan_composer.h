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

#include <coroutine>

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

using namespace vku_utils; /* This is temporary here */

inline std::map<std::string, vku_ref_p<vku_object_t>> objects;
inline std::map<std::string, std::vector<std::coroutine_handle<void>>> wanted_objects;
inline std::deque<std::coroutine_handle<void>> work;

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

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
        void                return_value(vkc_error_e ret) { this->ret = ret; }
        void                return_value(int ret)         { this->ret = (vkc_error_e)ret; }

        std::coroutine_handle<co_state_t> caller;
        std::exception_ptr exception;
        vkc_error_e ret;
    };

    using handle_t = std::coroutine_handle<co_state_t>;
    using promise_type = co_state_t;

    co_t() {}
    co_t(handle_t h) : h(h) {}

    /* This is for calling  */
    bool        await_ready() noexcept { return false; }
    handle_t    await_suspend(handle_t caller) noexcept {
        caller.promise().caller = h;    /* we first mark the coroutine we will return to and */
        return h;                       /* we return this coroutine as the continuation */
    }
    vkc_error_e await_resume() {
        /* We save the exception pointer(even if no exception) and the return value */
        std::exception_ptr exc_ptr = h.promise().exception;
        auto ret = h.promise().ret;

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

    std::string required_depend;
};

void mark_dependency_solved(std::string depend_name, vku_ref_p<vku_object_t> depend) {
    /* First remember the dependency: */
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


co_t build_object(fkyaml::node& node) {
    if (false);
    else if (node["type"] == "vku_instance_t") { /* TODO: mark_dependency_solved...*/ }
    else if (node["type"] == "vku_window_t") { /* TODO: */ }
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
    ASSERT_BOOL_CO(root.is_sequence());

    for (auto &node : root) {
        if (!node.is_mapping()) {
            DBG("warning node: %s not a mapping", fkyaml::node::serialize(node).c_str());
            continue;
        }
        if (!node.contains("type")) {
            DBG("warning node: %s does not have a type", fkyaml::node::serialize(node).c_str());
            continue;
        }

        builder_sched(build_object(node));
    }

    co_return 0;
}

inline vkc_error_e parse_config(const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        build_schema(config).h.resume();

        if (wanted_objects.size()) {
            for (auto &[k, v]: wanted_objects) {
                DBG("Unknown Object: %s", k.c_str());
                return VKC_ERROR_PARSE_YAML;
            }
        }
    }
    catch (fkyaml::exception &e) {
        DBG("fkyaml::exception: %s", e.what());
        return VKC_ERROR_PARSE_YAML;
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
