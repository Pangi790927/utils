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
#include "colib.h"

#include <coroutine>
#include <filesystem>

namespace co = colib;
namespace vku = vku_utils;

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

// struct vulkan_instance_t binding_desc_t{
//     std::string filename;
//     vku::opts_t vku_opts;    /*! json object: opts */  
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

namespace vkc {


inline std::string app_path = std::filesystem::canonical("./");

inline std::map<std::string, vku::ref_t<vku::object_t>> objects;
inline std::map<std::string, std::vector<co::state_t *>> wanted_objects;
inline std::deque<std::coroutine_handle<void>> work;

struct lua_var_t : public vku::object_t {
    std::string name;
    static vku::ref_t<lua_var_t> create(std::string name) {
        auto ret = vku::ref_t<lua_var_t>::create_obj_ref(std::make_unique<lua_var_t>(), {});
        ret->name = name;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct lua_script_t : public vku::object_t {
    std::string content;
    static vku::ref_t<lua_script_t> create(std::string content) {
        auto ret = vku::ref_t<lua_script_t>::create_obj_ref(std::make_unique<lua_script_t>(), {});
        ret->content = content;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};


struct integer_t : public vku::object_t {
    int64_t value = 0;
    static vku::ref_t<integer_t> create(int64_t value) {
        auto ret = vku::ref_t<integer_t>::create_obj_ref(std::make_unique<integer_t>(), {});
        ret->value = value;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct float_t : public vku::object_t {
    double value = 0;
    static vku::ref_t<float_t> create(double value) {
        auto ret = vku::ref_t<float_t>::create_obj_ref(std::make_unique<float_t>(), {});
        ret->value = value;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct string_t : public vku::object_t {
    std::string value;
    static vku::ref_t<string_t> create(const std::string& value) {
        auto ret = vku::ref_t<string_t>::create_obj_ref(std::make_unique<string_t>(), {});
        ret->value = value;
        return ret;
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct spirv_t : public vku::object_t {
    vku::spirv_t spirv;
    static vku::ref_t<spirv_t> create(const vku::spirv_t& spirv) {
        auto ret = vku::ref_t<spirv_t>::create_obj_ref(std::make_unique<spirv_t>(), {});
        ret->spirv = spirv;
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

template <typename VkuT>
struct depend_resolver_t {
    /* We save the searched dependency */
    depend_resolver_t(std::string required_depend) : required_depend(required_depend) {}

    /* If we already have the dependency we can already retur */
    bool await_ready() noexcept { return has(objects, required_depend); }

    /* Else we place ourselves on the waiting queue */
    template <typename P>
    co::handle<void> await_suspend(co::handle<P> caller) noexcept {
        auto state = co::external_on_suspend(caller);
        /* We place ourselves on the waiting queue: */
        wanted_objects[required_depend].push_back(state);

        /* Else we return the next work in line that can be done */
        return co::external_wait_next_task(state->pool);
    }

    vku::ref_t<VkuT> await_resume() {
        if (!has(objects, required_depend)) {
            DBG("Object not found");
            throw vku::err_t(std::format("Object not found, {}", required_depend));
        }
        if (!objects[required_depend]) {
            DBG("For some reason this object now holds a nullptr...");
            throw vku::err_t("nullptr object");
        }
        auto ret = objects[required_depend].to_derived<VkuT>();
        if (!ret) {
            DBG("Invalid ref...");
            throw vku::err_t(sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    demangle<4>(typeid(objects[required_depend].get()).name()).c_str(),
                    demangle<VkuT, 4>().c_str()));
        }
        return ret;
    }

    std::string required_depend;
};

void mark_dependency_solved(std::string depend_name, vku::ref_t<vku::object_t> depend) {
    /* First remember the dependency: */
    if (!depend) {
        DBG("Object into nullptr");
        throw vku::err_t{std::format("Object turned into nullptr: {}", depend_name)};
    }
    objects[depend_name] = depend;

    /* Second, awake all the ones waiting for the respective dependency */
    if (vkc::has(wanted_objects, depend_name)) {
        for (auto s : wanted_objects[depend_name])
            co::external_sched_resume(s);
        wanted_objects.erase(depend_name);
    }
}

inline bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

inline std::unordered_map<std::string, vku_shader_stage_e> shader_stage_from_string = {
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
    std::string file_path = std::filesystem::canonical(file_path_relative);

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

co::task_t build_pseudo_object(const std::string& name, fkyaml::node& node) {
    if (node.is_integer()) {
        auto obj = integer_t::create(node.as_int());
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return 0;
    }

    if (node.is_string()) {
        auto obj = string_t::create(node.as_str());
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return 0;
    }

    if (node.is_mapping() && node.contains("m_shader_type")) {
        vku::spirv_t spirv;

        if (node.contains("m_source")) {
            spirv = vku::spirv_compile(
                    get_from_map(shader_stage_from_string, node["m_shader_type"].as_str()),
                    node["m_source"].as_str().c_str());
        }

        if (node.contains("m_source_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }
            spirv = vku::spirv_compile(
                    get_from_map(shader_stage_from_string, node["m_shader_type"].as_str()),
                    get_file_string_content(node["m_source_path"].as_str()).c_str());
        }

        if (node.contains("m_spirv_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }

            spirv.type = get_from_map(shader_stage_from_string, node["m_shader_type"].as_str());
            std::string file_path = std::filesystem::canonical(node["m_spirv_path"].as_str());

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

        auto obj = spirv_t::create(spirv);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());

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
            mark_dependency_solved(name, obj.to_base<vku::object_t>());
            co_return 0;
        }

        if (node.contains("m_source_path")) {
            std::string source = get_file_string_content(node["m_source_path"].as_str());

            auto obj = lua_script_t::create(source);
            mark_dependency_solved(name, obj.to_base<vku::object_t>());
            co_return 0;
        }
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<int64_t> resolve_int(fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<integer_t>(node.as_str()))->value;
    co_return node.as_int();
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<double> resolve_float(fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<float_t>(node.as_str()))->value;
    co_return node.as_float();
}

/*! This either follows a reference to a string or it returns the direct value if available */
co::task<std::string> resolve_str(fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<string_t>(node.as_str()))->value;
    co_return node.as_str();
}

co::task<vku::ref_t<vku::object_t>> build_object(const std::string& name, fkyaml::node& node);

inline int64_t anonymous_increment = 0;
template <typename VkuT>
co::task<vku::ref_t<VkuT>> resolve_obj(fkyaml::node& node) {
    /* Check -- How objects work in the configuration file -- */

    if (node.has_tag_name() && node.get_tag_name() == "!ref") {
        /* This is simply a reference to an object m_field: !ref tag_name*/
        co_return co_await depend_resolver_t<VkuT>(node.as_str());
    }
    else if (node.is_mapping() && node.as_map().size() == 1
            && node.as_map().begin()->second.contains("m_type"))
    {
        /* This is in the form m_field: tag_name: m_type: "..." */
        std::string tag = node.as_map().begin()->first.as_str();
        auto ref = co_await build_object(tag, node.as_map().begin()->second);
        co_return ref.template to_derived<VkuT>();
    }
    else if (node.contains("m_type")) {
        /* This is in the form m_field: m_type: "...", ie, inlined object */
        std::string tag = node.contains("m_tag") ?
                node["m_tag"].as_str() : "anonymous_" + std::to_string(anonymous_increment++);
        auto ref = co_await build_object(tag, node);
        co_return ref.template to_derived<VkuT>();
    }

    /* None of the above */
    co_return nullptr;
    // if (node.contains("m_type"))
    //     co_await build_object
    // co_return co_await depend_resolver_t<Vkut>();
}

co::task<vku::ref_t<vku::object_t>> build_object(const std::string& name, fkyaml::node& node) {
    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return nullptr;
    }
    if (false);
    else if (node["m_type"] == "vku::instance_t") {
        auto obj = vku::instance_t::create();
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::lua_var_t") {
        /* lua_var has the same tag_name as the var name */
        auto obj = lua_var_t::create(name);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::window_t") {
        auto w = co_await resolve_int(node["m_width"]);
        auto h = co_await resolve_int(node["m_height"]);
        auto window_name = co_await resolve_str(node["m_name"]);
        auto obj = vku::window_t::create(w, h, window_name);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::surface_t") {
        auto window = co_await resolve_obj<vku::window_t>(node["m_window"]);
        auto instance = co_await resolve_obj<vku::window_t>(node["m_instance"]);
        auto obj = vku::surface_t::create(window, instance);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::device_t") {
        auto surf = co_await resolve_obj<vku::surface_t>(node["m_surface"]);
        auto obj = vku::device_t::create(surf);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdpool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto obj = vku::cmdpool_t::create(dev);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::image_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_view_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_sampl_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::buffer_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t") {
        std::vector<vku::ref_t<vku::binding_desc_set_t::binding_desc_t>> bindings;
        for (auto& subnode : node["m_descriptors"])
            bindings.push_back(
                    co_await resolve_obj<vku::binding_desc_set_t::binding_desc_t>(subnode));
        auto obj = vku::binding_desc_set_t::create(bindings);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::buff_binding_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::sampl_binding_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::shader_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::swapchain_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::renderpass_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::pipeline_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::framebuffs_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::sem_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::fence_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdbuff_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_pool_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_set_t") {
        /* TODO: */
        // mark_dependency_solved(name, obj.to_base<vku::object_t>());
        // co_return obj.to_base<vku::object_t>();
    }

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vku::err_t{"Invalid object type"};
}


co::task_t build_schema(fkyaml::node& root) {
    ASSERT_BOOL_CO(root.is_mapping());

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("m_type")) {
            co_await co::sched(build_pseudo_object(name.as_str(), node));
        }
        else {
            co_await co::sched(build_object(name.as_str(), node));
        }
    }

    co_return 0;
}

inline vkc_error_e parse_config(const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        auto pool = co::create_pool();
        pool->sched(build_schema(config));

        if (pool->run() != co::RUN_OK) {
            DBG("Failed to create the schema");
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
