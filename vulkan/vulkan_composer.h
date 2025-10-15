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

enum vkc_error_e : int32_t {
    VKC_ERROR_OK = 0,
    VKC_ERROR_FAILED_CALL = -1,
    VKC_ERROR_FAILED_LOAD = -2,
    VKC_ERROR_FAILED_LUA_INIT = -3,
    VKC_ERROR_FAILED_LUA_LOAD = -4,
    VKC_ERROR_FAILED_LUA_EXEC = -5,
};

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

#endif
