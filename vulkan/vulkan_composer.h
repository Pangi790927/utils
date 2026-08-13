#ifndef VULKAN_COMPOSER_H
#define VULKAN_COMPOSER_H

/*!
 * TODO: this description must be moved in virt_composer
 * 
 * LUA API: Vulkan Utils
 * =====================
 * 
 * The Vulkan Utils (vulkan_utils or vku) Lua bindings provide a scripting interface for interacting
 * with Vulkan objects managed by the C++ backend. The bindings are designed to give users a
 * flexible way to control GPU resources, rendering loops, and engine logic directly from Lua
 * scripts, without exposing the raw Vulkan API.
 * 
 * Key Concepts:
 * -------------
 *
 * 1. Utility functions:
 *    - Examples: glfw_pool_events, get_key, signal_close, etc.
 *    - Used to manage input, events, and window state.
 *
 * 2. References to objects:
 *    - Objects created in YAML or in C++ are accessible as named references in Lua:
 *      vku.object_name
 *    - These are ref_t-managed Vulkan objects, allowing safe manipulation and
 *      automatic dependency propagation.
 *
 * 3. Constants and defines:
 *    - Vulkan and framework-specific constants are exposed in Lua for convenience:
 *      vku.VK_PIPELINE_STAGE_HOST_BIT
 *
 * 4. Object methods:
 *    - Named objects in Lua can call internal methods implemented in C++:
 *      - vku.cbuff:begin(...)
 *      - vku.window:rebuild()
 *    - Methods operate directly on GPU resources or manage Vulkan object lifetimes.
 *
 * How it works:
 * -------------
 * 1. YAML is loaded -> objects created -> Lua can be executed.
 * 2. C++ backend creates objects wrapped in ref_t handles for safe reference counting
 *    and dependency tracking.
 * 3. Lua scripts interact with objects, submit GPU commands, and respond to events.
 * 4. Users can expose C++ plugins to Lua (int function(lua_State *)) for advanced
 *    GPU operations such as filling triangle data or analyzing compute shader results.
 * 5. Lua can create internal objects dynamically, similar to YAML object creation.
 *
 * Example Usage:
 * --------------
 * vku = require("vulkan_utils")
 *
 * -- example filling buffers:
 * vku.fill_buffer_with_triangles_vertices(vku.staging_buffer.data(), vku.staging_buffer.size())
 * vku.copy_from_cpu_to_gpu(vku.staging_buffer, vku.vbuff, vku.staging_buffer.size(), 0)
 *
 * vku.fill_buffer_with_triangles_indexes(vku.staging_buffer.data(), vku.staging_buffer.size())
 * vku.copy_from_cpu_to_gpu(vku.staging_buffer, vku.ibuff, vku.SIZEOF_INT*3, 0)
 * 
 * -- TODO: example creating a vku object (vkc::lua_var_t is a dummy object)
 * t = {
 *     m_type = "vkc::lua_var_t",
 *     var1 = 1,
 *     var2 = 2,
 *     var3 = { var4 = "str" }
 * }
 * to = vku.create_object("tag_name", t)
 *
 * function on_loop_run()
 *     vku.glfw_pool_events()
 *     if vku.get_key(vku.window, vku.GLFW_KEY_ESCAPE) == vku.GLFW_PRESS then
 *         vku.signal_close()
 *     end
 *
 *     img_idx = vku.aquire_next_img(vku.swc, vku.img_sem)
 *
 *     vku.cbuff:begin(vku.VK_COMMAND_BUFFER_USAGE_NONE)
 *     vku.cbuff:begin_rpass(vku.fbs, img_idx)
 *     vku.cbuff:bind_vert_buffs(0, {{vku.vbuff, 0}})
 *     vku.cbuff:bind_idx_buff(vku.ibuff, 0, vku.VK_INDEX_TYPE_UINT16)
 *     vku.cbuff:bind_desc_set(vku.VK_PIPELINE_BIND_POINT_GRAPHICS, vku.pl, vku.desc_set)
 *     vku.cbuff:draw_idx(vku.pl, 3)
 *     vku.cbuff:end_rpass()
 *     vku.cbuff:end()
 *
 *     vku.submit_cmdbuff({{vku.img_sem, vku.VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
 *         vku.cbuff, vku.fence, {vku.draw_sem})
 *     vku.present(vku.swc, {vku.draw_sem}, img_idx)
 *
 *     vku.wait_fences({vku.fence}, true, -1)
 *     vku.reset_fences({vku.fence})
 * end
 *
 * function on_window_resize(w, h)
 *     vku.device_wait_handle(vku.dev)
 *     vku.window.m_width = w
 *     vku.window.m_height = h
 *     vku.window.rebuild()
 * end
 *
 * Key Points:
 * -----------
 * - Lua scripts operate on top of C++ objects; everything is managed safely via ref_t.
 * - Both static objects from YAML and dynamic objects from Lua can be manipulated similarly.
 * - Lua functions can drive the render loop, event handling, GPU resource updates, and
 *   compute shader analysis.
 * - Provides rapid prototyping and scripting capabilities while keeping Vulkan resource
 *   management safe and automatic.
 * 
 * 
 * How objects work in the configuration(YAML) file:
 * =================================================
 *
 * 1. Declaring a standalone object:
 *    An object can be defined on its own, using a tag name as its identifier:
 *
 *    ```yaml
 *     tag-name:
 *         m_type: object_type
 *         ...
 *    ```
 *
 *    In this form:
 *
 *    * The tag name (e.g., `tag-name`) is automatically treated as the object's type identifier.
 *    * The field `m_type` is required — it marks the entry as an object.
 *    * Other properties or fields may follow.
 *
 * 2. Declaring an object within another object:
 *    An object can also appear as a nested field inside another object:
 *
 *    ```yaml
 *     another-tag-name:
 *         ...
 *         m_field:
 *             tag-name:
 *                 m_type: object_type
 *                 ...
 *    ```
 *
 *    In this case:
 *
 *    * `m_field` contains an object named `tag-name`.
 *    * `tag-name` again includes an `m_type` to specify its type.
 *
 * 3. Declaring an inline (anonymous) object:
 *    You can define an object directly within a field without giving it an outer tag.
 *    However, you may optionally include an `m_tag` if you plan to reference it later:
 *
 *    ```yaml
 *     another-tag-name:
 *         ...
 *         m_field:
 *             m_type: object_type
 *             m_tag: optional-tag-name
 *    ```
 *
 *    Here:
 *
 *    * `m_type` identifies the object type.
 *    * `m_tag` (optional) assigns a reference name so this object can be reused elsewhere.
 */

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

#include "vulkan_utils.h"
#include "yaml.h"
#include "tinyexpr.h"
#include "minilua.h"
#include "demangle.h"
#include "co_utils.h"

#include <filesystem>

/* TODO: Thsi is almost done now, I must think what I want to further expose, and some tweaks:
    - I still need to create matrices and vectors
    - I want to have includes for shaders
    - Add functions to manipulate buffers, matrices and vectors
    ~ We need to fix the stupidity that is descriptors?
 */

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

#define VC_LUA_ASSERT(call) do {\
    auto [ret, err] = (call);\
    if (intptr_t(ret) < 0) {\
        DBGE("FAILED[ret]: " #call);\
        return -1;\
    }\
    if (intptr_t(err) < 0) {\
        DBGE("FAILED[err]: " #call);\
        return -1;\
    }\
} while (0)

namespace virt_composer {

extern inline std::unordered_map<std::string, VkResult>
        vk_result_from_str;

extern inline std::unordered_map<std::string, VkBufferUsageFlagBits>
        vk_buffer_usage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkImageUsageFlagBits>
        vk_image_usage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkImageTiling>
        vk_image_tiling_from_str;

extern inline std::unordered_map<std::string, VkSharingMode>
        vk_sharing_mode_from_str;

extern inline std::unordered_map<std::string, VkMemoryPropertyFlagBits>
        vk_memory_property_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkPrimitiveTopology>
        vk_primitive_topology_from_str;

extern inline std::unordered_map<std::string, VkImageAspectFlagBits>
        vk_image_aspect_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkCommandBufferUsageFlagBits>
        vk_command_buffer_usage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkPipelineBindPoint>
        vk_pipeline_bind_point_from_str;

extern inline std::unordered_map<std::string, VkIndexType>
        vk_index_type_from_str;

extern inline std::unordered_map<std::string, VkPipelineStageFlagBits>
        vk_pipeline_stage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkFormat>
        vk_format_from_str;

extern inline std::unordered_map<std::string, VkVertexInputRate>
        vk_vertex_input_rate_from_str;

extern inline std::unordered_map<std::string, VkShaderStageFlagBits>
        vk_shader_stage_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkDescriptorType>
        vk_descriptor_type_from_str;

extern inline std::unordered_map<std::string, vku_shader_stage_e>
        shader_stage_from_string;

extern inline std::unordered_map<std::string, VkSemaphoreType>
        vk_semaphore_type_from_str;

// extern inline std::unordered_map<std::string, VkPipelineStageFlagBits2>
//         vk_pipeline_stage_flag_bits2_from_str;

// extern inline std::unordered_map<std::string, VkAccessFlagBits2>
//         vk_access_flag_bits2_from_str;

extern inline std::unordered_map<std::string, VkImageLayout>
        vk_image_layout_from_str;

extern inline std::unordered_map<std::string, VkAccessFlagBits>
        vk_access_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkDependencyFlagBits>
        vk_dependency_flag_bits_from_str;

extern inline std::unordered_map<std::string, VkFenceCreateFlagBits>
        vk_fence_create_flag_bits_from_str;

template <> inline VkResult get_enum_val<VkResult>(fkyaml::node &n);
template <> inline VkBufferUsageFlagBits get_enum_val<VkBufferUsageFlagBits>(fkyaml::node &n);
template <> inline VkImageUsageFlagBits get_enum_val<VkImageUsageFlagBits>(fkyaml::node &n);
template <> inline VkImageTiling get_enum_val<VkImageTiling>(fkyaml::node &n);
template <> inline VkSharingMode get_enum_val<VkSharingMode>(fkyaml::node &n);
template <> inline VkMemoryPropertyFlagBits get_enum_val<VkMemoryPropertyFlagBits>(fkyaml::node &n);
template <> inline VkPrimitiveTopology get_enum_val<VkPrimitiveTopology>(fkyaml::node &n);
template <> inline VkImageAspectFlagBits get_enum_val<VkImageAspectFlagBits>(fkyaml::node &n);
template <> inline VkCommandBufferUsageFlagBits get_enum_val<VkCommandBufferUsageFlagBits>(
        fkyaml::node &n);
template <> inline VkPipelineBindPoint get_enum_val<VkPipelineBindPoint>(fkyaml::node &n);
template <> inline VkIndexType get_enum_val<VkIndexType>(fkyaml::node &n);
template <> inline VkPipelineStageFlagBits get_enum_val<VkPipelineStageFlagBits>(fkyaml::node &n);
template <> inline VkFormat get_enum_val<VkFormat>(fkyaml::node &n);
template <> inline VkVertexInputRate get_enum_val<VkVertexInputRate>(fkyaml::node &n);
template <> inline VkShaderStageFlagBits get_enum_val<VkShaderStageFlagBits>(fkyaml::node &n);
template <> inline VkDescriptorType get_enum_val<VkDescriptorType>(fkyaml::node &n);
template <> inline vku_shader_stage_e get_enum_val<vku_shader_stage_e>(fkyaml::node &n);
template <> inline VkSemaphoreType get_enum_val<VkSemaphoreType>(fkyaml::node &n);
// template <> inline VkPipelineStageFlagBits2 get_enum_val<VkPipelineStageFlagBits2>(fkyaml::node &n);
// template <> inline VkAccessFlagBits2 get_enum_val<VkAccessFlagBits2>(fkyaml::node &n);
template <> inline VkImageLayout get_enum_val<VkImageLayout>(fkyaml::node &n);
template <> inline VkPipelineStageFlagBits get_enum_val<VkPipelineStageFlagBits>(fkyaml::node &n);
template <> inline VkAccessFlagBits get_enum_val<VkAccessFlagBits>(fkyaml::node &n);
template <> inline VkDependencyFlagBits get_enum_val<VkDependencyFlagBits>(fkyaml::node &n);
template <> inline VkFenceCreateFlagBits get_enum_val<VkFenceCreateFlagBits>(fkyaml::node &n);


template <>
struct luaw_returner_t<VkResult> {
    void luaw_ret_push(lua_State *L, VkResult x) { lua_pushinteger(L, x); }
};

/* This is not really usefull, but an example on how to add new parameters: (obs: longer ones should
be implemented bellow, like the functions above) */
template <ssize_t index>
struct luaw_param_t<VkResult, index> {
    VkResult luaw_single_param(lua_State *L) {
        int valid;
        return (VkResult)lua_tointegerx(L, index, &valid);
    }
};


} /* virt_composer */

namespace vulkan_composer {

namespace vo = virt_object;
namespace vku = vulkan_utils;
namespace vc = virt_composer;
namespace vkc = vulkan_composer;

VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_SPIRV);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_CPU_BUFFER);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_LUA_VARIABLE);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_VERTEX_INPUT_DESC);
VIRT_COMPOSER_REGISTER_TYPE(VKC_TYPE_BINDING_DESC);

inline std::string app_path = std::filesystem::canonical("./");

/*!
 * 
 * vkc::cpu_buffer_t
 * -----------------
 *
 * Description: Represents a CPU-side memory buffer. Can be used for passing data between Lua
 * scripts and C++ callbacks, or for staging data to  and from Vulkan buffers. Essentially,
 * this object wraps a byte array.
 *
 * Member functions:
 * - data(): Returns a pointer to the buffer data.
 * - size(): Returns the size of the buffer.
 *
 * Init: create(sz)
 *   - Parameters:
 *     - sz: Initial size of the buffer in bytes
 * 
 */
struct cpu_buffer_t : public vku::object_t {
    cpu_buffer_t(object_t::Private priv) : object_t(priv) {}
    virtual ~cpu_buffer_t() {}

    static vku::object_type_e type_id_static() { return VKC_TYPE_CPU_BUFFER; }
    virtual vku::object_type_e type_id() const override { return VKC_TYPE_CPU_BUFFER; }

    static vku::ref_t<cpu_buffer_t> create(size_t sz) {
        auto ret = std::make_shared<cpu_buffer_t>(vku::object_t::Private{type_id_static()});
        ret->_data.resize(sz);
        return ret;
    }

    inline std::string to_string() const override {
        return std::format("vkc::cpu_buffer_t[{}]: size={} ", (void*)this, size());
    }

    void *data() { return (void *)_data.data(); }
    size_t size() const { return _data.size(); }

private:
    std::vector<uint8_t> _data;
};

/*!
 * 
 * vkc::spirv_t
 * ------------
 * 
 * Description: Wraps a SPIR-V shader module representation. Stores the SPIR-V bytecode 
 * along with metadata (type, stage, etc.) and allows it to be passed around in the Vulkan 
 * framework or used for shader module creation.
 *
 * Members:
 * - spirv: The SPIR-V representation, including type and bytecode content.
 *
 * Member functions:
 * - to_string(): Returns a formatted string showing the SPIR-V type and a hex dump of the content.
 *
 * TODO: fix: Init: create(spirv)
 *   - Parameters:
 *     - spirv: The SPIR-V object containing bytecode and type information.
 * 
 */
struct spirv_t : public vku::object_t {
    vku::spirv_t spirv;

    spirv_t(object_t::Private priv) : object_t(priv) {}
    virtual ~spirv_t() {}

    virtual vku::object_type_e type_id() const override { return VKC_TYPE_SPIRV; }
    static vku::object_type_e type_id_static() { return VKC_TYPE_SPIRV; }

    static vku::ref_t<spirv_t> create(const vku::spirv_t& spirv) {
        auto ret = std::make_shared<spirv_t>(vku::object_t::Private{type_id_static()});
        ret->spirv = spirv;
        return ret;
    }

    inline std::string to_string() const override {
        return std::format("vkc::spirv[{}]: spirv-type={} spirv-content=\n{}", (void*)this,
                vulkan_utils::to_string(spirv.type),
                hexdump_str((void *)spirv.content.data(), spirv.content.size() * sizeof(uint32_t)));
    }
};

/*!
 * 
 * vkc::vertex_input_desc_t
 * ------------------------
 *
 * Description: Represents a Vulkan vertex input description, including the binding 
 * and attribute layouts. Wraps a vku::vertex_input_desc_t structure containing the 
 * binding description (stride, input rate) and attribute descriptions (format, 
 * location, offset, etc.).
 *
 * Member functions:
 * - to_string(): Returns a formatted string showing the binding, stride, input rate, 
 *   and attribute descriptions.
 *
 * TODO: more clear: Init: create(vid)
 *   - Parameters:
 *     - vid: A vku::vertex_input_desc_t object describing the vertex input layout.
 *
 * Notes:
 * - Serves as a reusable object to describe vertex buffer layout for pipeline creation.
 * 
 */
struct vertex_input_desc_t : public vku::object_t {
    vku::vertex_input_desc_t vid;

    vertex_input_desc_t(object_t::Private priv) : object_t(priv) {}
    virtual ~vertex_input_desc_t() {}

    static vku::object_type_e type_id_static() { return VKC_TYPE_VERTEX_INPUT_DESC; }
    virtual vku::object_type_e type_id() const override { return VKC_TYPE_VERTEX_INPUT_DESC; }

    static vku::ref_t<vertex_input_desc_t> create(const vku::vertex_input_desc_t& vid) {
        auto ret = std::make_shared<vertex_input_desc_t>(vku::object_t::Private{type_id_static()});
        ret->vid = vid;
        return ret;
    }

    inline std::string to_string() const override {
        std::string ret = std::format("[binding={}, stride={}, in_rate={}]{{",
                vid.bind_desc.binding, vid.bind_desc.stride,
                vku::to_string(vid.bind_desc.inputRate));
        for (auto &attr : vid.attr_desc)
            ret += std::format("[loc={} bind={} fmt={} off=],", attr.location, attr.binding,
                    vku::to_string(attr.format), attr.offset);
        ret += "}}";
        return ret;
    }
};

/*! IMPLEMENTATION
 ***************************************************************************************************
 ***************************************************************************************************
 ***************************************************************************************************
 */

inline bool starts_with(const std::string& a, const std::string& b) {
    return a.size() >= b.size() && a.compare(0, b.size(), b) == 0;
}

template <typename T, typename K>
constexpr auto has(T&& data_struct, K&& key) {
    return std::forward<T>(data_struct).find(std::forward<K>(key))
            != std::forward<T>(data_struct).end();
}

inline auto get_from_map(auto &m, const std::string& str) {
    if (!vkc::has(m, str))
        throw vc::except_t(std::format("Failed to get object: {} from: {}",
                str, demangle<decltype(m), 2>()));
    return m[str];
}

inline std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
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

inline bool build_pseudo_object_match(const std::string&, fkyaml::node& node) {
    if (node.is_mapping() && node.contains("m_shader_type")) {
        return true;
    }
    return false;
}

inline co::task_t build_pseudo_object_cbk(vc::virt_state_t *vs, const std::string& name,
        fkyaml::node& node)
{
    if (node.is_mapping() && node.contains("m_shader_type")) {
        vku::spirv_t spirv;

        if (node.contains("m_source")) {
            spirv = vku::spirv_compile(
                    get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str()),
                    node["m_source"].as_str().c_str());
        }

        if (node.contains("m_source_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }
            spirv = vku::spirv_compile(
                    get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str()),
                    get_file_string_content(node["m_source_path"].as_str()).c_str());
        }

        if (node.contains("m_spirv_path")) {
            if (spirv.content.size()) {
                DBG("Trying to initialize spirv from 2 sources (only one of source, "
                        "source-path, or spirv-path allowed)");
                co_return -1;
            }

            spirv.type = get_from_map(vc::shader_stage_from_string, node["m_shader_type"].as_str());
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
        mark_dependency_solved(vs, name, obj->to_related<vku::object_t>());

        co_return 0;
    }
    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

inline void glfw_pool_events() {
    glfwPollEvents();
}

inline uint32_t glfw_get_key(vku::ref_t<vku::window_t> window, uint32_t key) {
    if (!window)
        throw vc::except_t("Window parameter can't be null");
    return glfwGetKey(window->get_window(), key);
}

inline void internal_signal_close() {
    /* TODO: do I really care about this? */
    /* TODO: set loop closed */
    DBG("TODO: set loop closed");
}

inline uint32_t internal_aquire_next_img(
        vku::ref_t<vku::swapchain_t> swc, vku::ref_t<vku::sem_t> sem)
{
    uint32_t ret;
    vku::aquire_next_img(swc, sem, &ret);
    return ret;
}

inline uint32_t internal_device_wait_handle(vku::ref_t<vku::device_t> dev) {
    return vkDeviceWaitIdle(dev->vk_dev);
}

inline int copy_from_cpu_to_gpu(vku::ref_t<vku::cmdpool_t> cp, vku::ref_t<vku::buffer_t> dst,
        void *src, size_t len, size_t off)
{
    (void)off;
    auto staging_vbuff = vku::buffer_t::create(
        cp->m_device,
        len,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_vbuff->map_data(0, len), src, len);
    staging_vbuff->unmap_data();

    auto vbuff = vku::buffer_t::create(
        cp->m_device,
        len,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    /* TODO: add offsets to copy_buff */
    vku::copy_buff(cp, dst, staging_vbuff, len, nullptr);
    return 0;
}

inline int copy_from_gpu_to_cpu(void *dst, vku::ref_t<vku::buffer_t> src,
        size_t len, size_t off)
{
    (void)dst, void(src), void(len), void(off);
    DBG("TODO: transfer gpu->cpu");
    return 0;
}

inline void luaw_set_glfw_fields(vc::virt_state_t *vs);

inline int register_meta(vc::virt_state_t *vs) {
    DBG_SCOPE();
    std::vector<luaL_Reg> vku_tab_funcs = {
        {"glfw_pool_events", vc::luaw_function_wrapper<
                /* FN:    */ glfw_pool_events
        >},
        {"get_key", vc::luaw_function_wrapper<
                /* FN:    */ glfw_get_key,
                /* PARAMS:*/ vc::ref_t<vku::window_t>,
                             uint32_t
        >},
        {"signal_close", vc::luaw_function_wrapper<
                /* FN:    */ internal_signal_close
        >},
        {"aquire_next_img", vc::luaw_function_wrapper<
                /* FN:    */ internal_aquire_next_img,
                /* PARAMS:*/ vc::ref_t<vku::swapchain_t>,
                             vc::ref_t<vku::sem_t>
        >},
        {"submit_cmdbuff", vc::luaw_function_wrapper<
                /* FN:    */ vku::submit_cmdbuff,
                /* PARAMS:*/ std::vector<std::pair<vc::ref_t<vku::sem_t>,
                             vc::bm_t<VkPipelineStageFlagBits>>>,
                             vc::ref_t<vku::cmdbuff_t>,
                             vc::ref_t<vku::fence_t>,
                             std::vector<vc::ref_t<vku::sem_t>>,
                             uint32_t
        >},
        {"submit_cmdbuff_tl", vc::luaw_function_wrapper<
                /* FN:    */ vku::submit_cmdbuff_tl,
                /* PARAMS:*/ std::vector<std::tuple<vc::ref_t<vku::sem_t>,
                                    vc::bm_t<VkPipelineStageFlagBits>, uint64_t>>,
                             vc::ref_t<vku::cmdbuff_t>,
                             vc::ref_t<vku::fence_t>,
                             std::vector<std::tuple<vc::ref_t<vku::sem_t>, uint64_t>>,
                             uint32_t
        >},
        {"present", vc::luaw_function_wrapper<
                /* FN:    */ vku::present,
                /* PARAMS:*/ vc::ref_t<vku::swapchain_t>,
                             std::vector<vc::ref_t<vku::sem_t>>,
                             uint32_t
        >},
        {"wait_fences", vc::luaw_function_wrapper<
                /* FN:    */ vku::wait_fences,
                /* PARAMS:*/ std::vector<vc::ref_t<vku::fence_t>>,
                             bool,
                             uint64_t
        >},
        {"wait_semaphores", vc::luaw_function_wrapper<
                /* FN:    */ vku::wait_semaphores,
                /* PARAMS:*/ std::vector<vc::ref_t<vku::sem_t>>,
                             std::vector<uint64_t>,
                             bool,
                             uint64_t
        >},
        {"reset_fences", vc::luaw_function_wrapper<
                /* FN:    */ vku::reset_fences,
                /* PARAMS:*/ std::vector<vc::ref_t<vku::fence_t>>
        >},
        {"device_wait_handle", vc::luaw_function_wrapper<
                /* FN:    */ internal_device_wait_handle,
                /* PARAMS:*/ vc::ref_t<vku::device_t>
        >},
        {"copy_from_cpu_to_gpu", vc::luaw_function_wrapper<
                /* FN:    */ copy_from_cpu_to_gpu,
                /* PARAMS:*/ vku::ref_t<vku::cmdpool_t>,
                             vc::ref_t<vku::buffer_t>,
                             void *,
                             size_t,
                             size_t
        >},
        {"copy_from_gpu_to_cpu", vc::luaw_function_wrapper<
                /* FN:    */ copy_from_gpu_to_cpu,
                /* PARAMS:*/ void *,
                             vc::ref_t<vku::buffer_t>,
                             size_t,
                             size_t
        >},
    };
    ASSERT_FN(add_lua_tab_funcs(vs, vku_tab_funcs));

    // /* vku::window_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_width);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::window_t, m_height);

    // /* vku::instance_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_app_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_engine_name);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_extensions);
    VC_REGISTER_MEMBER_OBJECT(vs, vku::instance_t, m_layers);

    // /* vku::device_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::device_t, get_graphics_queue_cnt);

    // /* vku::cmdbuff_t
    // ----------------------------------------------------------------------------------------- */

    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin, vc::bm_t<VkCommandBufferUsageFlagBits>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, begin_rpass,
            vku::ref_t<vku::framebuffs_t>, uint32_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_vert_buffs,
            uint32_t, std::vector<std::pair<vku::ref_t<vku::buffer_t>, VkDeviceSize>>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_desc_set,
            vc::bm_t<VkPipelineBindPoint>, vku::ref_t<vku::pipeline_layout_t>,
            vku::ref_t<vku::desc_set_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_idx_buff,
            vc::ref_t<vku::buffer_t>, uint64_t, vc::bm_t<VkIndexType>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, draw, vku::ref_t<vku::pipeline_t>, uint64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, draw_idx, vku::ref_t<vku::pipeline_t>, uint64_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, end_rpass);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, end);
    vc::luaw_register_member_function<vku::cmdbuff_t, &vku::cmdbuff_t::end>(vs, "end_begin");
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, reset);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, bind_compute,
            vku::ref_t<vku::compute_pipeline_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, dispatch_compute, uint32_t, uint32_t, uint32_t);

    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, set_event, vku::ref_t<vku::event_t>,
            vc::bm_t<VkPipelineStageFlagBits>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, reset_event, vku::ref_t<vku::event_t>,
            vc::bm_t<VkPipelineStageFlagBits>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, wait_events, std::vector<vku::ref_t<vku::event_t>>,
            vku::ref_t<vku::dependency_info_t>);

    VC_REGISTER_MEMBER_FUNCTION(vs, vku::cmdbuff_t, pipeline_barrier,
            vku::ref_t<vku::dependency_info_t>);

    // /* vkc::buffer_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::swapchain_t, img_count);

    // /* vkc::buffer_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::buffer_t, map_data, VkDeviceSize, VkDeviceSize);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::buffer_t, unmap_data);

    // /* vkc::cpu_buffer_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vkc::cpu_buffer_t, data);
    VC_REGISTER_MEMBER_FUNCTION(vs, vkc::cpu_buffer_t, size);

    // /* vku::fence_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::fence_t, get_status);

    // /* vku::fence_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::sem_t, get_counter);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::sem_t, signal, uint64_t);

    // /* vku::event_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::event_t, get_status);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::event_t, set_event);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::event_t, reset_event);

    // /* vku::desc_set_initializer_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t, get_binding, uint32_t);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t, update_set,
            vku::ref_t<vku::desc_set_t>);

    // /* vku::desc_set_initializer_t::buff_binding_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t::buff_binding_t, set_buffer,
            vku::ref_t<vku::buffer_t>, size_t, size_t);

    // /* vku::desc_set_initializer_t::sampl_binding_t
    // ----------------------------------------------------------------------------------------- */
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t::sampl_binding_t, set_view,
            vku::ref_t<vku::img_view_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t::sampl_binding_t, set_sampler,
            vku::ref_t<vku::img_sampl_t>);
    VC_REGISTER_MEMBER_FUNCTION(vs, vku::desc_set_initializer_t::sampl_binding_t, set_layout,
            vc::bm_t<VkImageLayout>);

    /* Done objects
    ----------------------------------------------------------------------------------------- */

    vc::add_lua_flag_mapping(vs, vc::vk_format_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_vertex_input_rate_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_shader_stage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_descriptor_type_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_pipeline_stage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_index_type_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_pipeline_bind_point_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_command_buffer_usage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_image_aspect_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_primitive_topology_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_memory_property_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_sharing_mode_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_buffer_usage_flag_bits_from_str);
    vc::add_lua_flag_mapping(vs, vc::vk_result_from_str);
    vc::add_lua_flag_mapping(vs, vc::shader_stage_from_string);
    // vc::add_lua_flag_mapping(vs, vc::vk_pipeline_stage_flag_bits2_from_str);
    // vc::add_lua_flag_mapping(vs, vc::vk_access_flag_bits2_from_str);
    luaw_set_glfw_fields(vs);

    ASSERT_FN(add_auto_builder_callback(vs, build_pseudo_object_match, build_pseudo_object_cbk));

    auto ret = add_named_builder_callback(vs,
        "vkc::vertex_input_desc_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            std::vector<VkVertexInputAttributeDescription> attrs;
            for (auto attr : node["m_attrs"].as_seq()) {
                auto m_location = co_await resolve_int(vs, attr["m_location"]);
                auto m_binding = co_await resolve_int(vs, attr["m_binding"]);
                auto m_format = vc::get_enum_val<VkFormat>(attr["m_format"]);
                auto m_offset = co_await resolve_int(vs, attr["m_offset"]);
                attrs.push_back(VkVertexInputAttributeDescription{
                    .location = (uint32_t)m_location,
                    .binding = (uint32_t)m_binding,
                    .format = m_format,
                    .offset = (uint32_t)m_offset
                });
            }
            auto m_binding = co_await resolve_int(vs, node["m_binding"]);
            auto m_stride = co_await resolve_int(vs, node["m_stride"]);
            auto m_in_rate = vc::get_enum_val<VkVertexInputRate>(node["m_in_rate"]);
            auto obj = vertex_input_desc_t::create(vku::vertex_input_desc_t{
                .bind_desc = {
                    .binding = (uint32_t)m_binding,
                    .stride = (uint32_t)m_stride,
                    .inputRate = m_in_rate,
                },
                .attr_desc = attrs,
            });
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto m_binding = co_await resolve_int(vs, node["m_binding"]);
            auto m_stage = vc::get_enum_val<VkShaderStageFlagBits>(node["m_stage"]);
            auto m_desc_type = vc::get_enum_val<VkDescriptorType>(node["m_desc_type"]);
            auto obj = vku::binding_t::create(VkDescriptorSetLayoutBinding{
                .binding = (uint32_t)m_binding,
                .descriptorType = m_desc_type,
                .descriptorCount = 1,
                .stageFlags = m_stage,
                .pImmutableSamplers = nullptr
            });
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::image_subresource_range_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto obj = vku::image_subresource_range_t::create(VkImageSubresourceRange {
                .aspectMask = node["m_aspect_mask"].is_null()
                        ? VK_IMAGE_ASPECT_COLOR_BIT
                        : vc::get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]),
                .baseMipLevel = node["m_base_mip_level"].is_null()
                        ? 0
                        : (uint32_t)co_await resolve_int(vs, node["m_base_mip_level"]),
                .levelCount = node["m_level_count"].is_null()
                        ? 1
                        : (uint32_t)co_await resolve_int(vs, node["m_level_count"]),
                .baseArrayLayer = node["m_base_array_layer"].is_null()
                        ? 0
                        : (uint32_t)co_await resolve_int(vs, node["m_base_array_layer"]),
                .layerCount = node["m_layer_count"].is_null()
                        ? 1
                        : (uint32_t)co_await resolve_int(vs, node["m_layer_count"]),
            });
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);
    
    ret = add_named_builder_callback(vs,
        "vku::dependency_info_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto src_stage = vc::get_enum_val<VkPipelineStageFlagBits>(node["m_src_stage_mask"]);
            auto dst_stage = vc::get_enum_val<VkPipelineStageFlagBits>(node["m_dst_stage_mask"]);
            VkDependencyFlags dep_flags = node["m_flags"].is_null()
                    ? 0
                    : vc::get_enum_val<VkDependencyFlagBits>(node["m_dep_flags"]);
            std::vector<VkMemoryBarrier> mem_bars;
            if (!node["m_mem_bars"].is_null()) {
                if (!node["m_mem_bars"].is_sequence())
                    throw vc::except_t("m_mem_bars must be an array");
                for (auto& subnode : node["m_mem_bars"]) {
                    VkMemoryBarrier bar {
                        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
                        .pNext = nullptr,
                        .srcAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["m_src_access_mask"]),
                        .dstAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["m_dst_access_mask"]),
                    };
                    mem_bars.push_back(bar);
                }
            }
            std::vector<VkBufferMemoryBarrier> buff_mem_bars;
            if (!node["m_buff_mem_bars"].is_null()) {
                if (!node["m_buff_mem_bars"].is_sequence())
                    throw vc::except_t("m_buff_mem_bars must be an array");
                for (auto& subnode : node["m_buff_mem_bars"]) {
                    auto buff = co_await resolve_obj<vku::buffer_t>(vs, subnode["m_buffer"]);
                    VkBufferMemoryBarrier bar {
                        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                        .pNext = nullptr,
                        .srcAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["m_src_access_mask"]),
                        .dstAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["m_dst_access_mask"]),
                        .srcQueueFamilyIndex = subnode["m_src_queue_family_index"].is_null()
                                ? VK_QUEUE_FAMILY_IGNORED
                                : (uint32_t)co_await resolve_int(vs,
                                        subnode["m_src_queue_family_index"]),
                        .dstQueueFamilyIndex = subnode["m_dst_queue_family_index"].is_null()
                                ? VK_QUEUE_FAMILY_IGNORED
                                : (uint32_t)co_await resolve_int(vs,
                                        subnode["m_dst_queue_family_index"]),
                        .buffer = buff->vk_buff,
                        .offset = subnode["m_offset"].is_null()
                                ? 0
                                : (VkDeviceSize)co_await resolve_int(vs, subnode["m_offset"]),
                        .size = subnode["m_size"].is_null()
                                ? buff->m_size
                                : (VkDeviceSize)co_await resolve_int(vs, subnode["m_size"]),
                    };
                    buff_mem_bars.push_back(bar);
                }
            }
            std::vector<VkImageMemoryBarrier> img_mem_bars;
            if (!node["m_img_mem_bars"].is_null()) {
                if (!node["m_img_mem_bars"].is_sequence())
                    throw vc::except_t("m_img_mem_bars must be an array");
                for (auto& subnode : node["m_img_mem_bars"]) {
                    auto img = co_await resolve_obj<vku::image_t>(vs, subnode["m_image"]);
                    auto img_subrange = subnode["m_subrange"].is_null()
                            ? VkImageSubresourceRange {
                                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                .baseMipLevel = 0,
                                .levelCount = 1,
                                .baseArrayLayer = 0,
                                .layerCount = 1,
                            }
                            : (co_await resolve_obj<vku::image_subresource_range_t>(
                                    vs, subnode["m_subrange"]))->m_img_subrange;
                    VkImageMemoryBarrier bar {
                        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                        .pNext = nullptr,
                        .srcAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["src_access_mask"]),
                        .dstAccessMask = vc::get_enum_val<VkAccessFlagBits>(
                                subnode["dst_access_mask"]),
                        .oldLayout = vc::get_enum_val<VkImageLayout>(subnode["m_old_layout"]),
                        .newLayout = vc::get_enum_val<VkImageLayout>(subnode["m_new_layout"]),
                        .srcQueueFamilyIndex = subnode["m_src_queue_family_index"].is_null()
                                ? VK_QUEUE_FAMILY_IGNORED
                                : (uint32_t)co_await resolve_int(vs,
                                        subnode["m_src_queue_family_index"]),
                        .dstQueueFamilyIndex = subnode["m_dst_queue_family_index"].is_null()
                                ? VK_QUEUE_FAMILY_IGNORED
                                : (uint32_t)co_await resolve_int(vs,
                                        subnode["m_dst_queue_family_index"]),
                        .image = img->vk_img,
                        .subresourceRange = img_subrange,
                    };
                    img_mem_bars.push_back(bar);
                }
            }
            auto obj = vku::dependency_info_t::create(src_stage, dst_stage,
                    mem_bars, buff_mem_bars, img_mem_bars, dep_flags);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vkc::cpu_buffer_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto m_size = co_await resolve_int(vs, node["m_size"]);
            auto obj = cpu_buffer_t::create(m_size);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);


    ret = add_named_builder_callback(vs,
        "vku::instance_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            std::string name = node["m_app_name"].is_null()
                    ? "vku::app_name_placeholder"
                    : co_await resolve_str(vs, node["m_app_name"]);
            std::string engine_name = node["m_engine_name"].is_null()
                    ? "vku::engine_name_placeholder"
                    : co_await resolve_str(vs, node["m_engine_name"]);
            std::vector<std::string> exts;
            if (!node["m_extensions"].is_null()) {
                if (!node["m_extensions"].is_sequence())
                    throw vc::except_t("m_extensions must be an array");
                for (auto &ext : node["m_extensions"])
                    exts.push_back(ext.as_str());
            }
            std::vector<std::string> layers;
            if (!node["m_layers"].is_null()) {
                if (!node["m_layers"].is_sequence())
                    throw vc::except_t("m_layers must be an array");
                for (auto &layer : node["m_layers"])
                    layers.push_back(layer.as_str());
            }
            auto obj = vku::instance_t::create(name, engine_name, exts, layers);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::window_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto w = co_await resolve_int(vs, node["m_width"]);
            auto h = co_await resolve_int(vs, node["m_height"]);
            auto window_name = co_await resolve_str(vs, node["m_name"]);
            auto obj = vku::window_t::create(w, h, window_name);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::surface_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto window = co_await resolve_obj<vku::window_t>(vs, node["m_window"]);
            auto instance = co_await resolve_obj<vku::instance_t>(vs, node["m_instance"]);
            auto obj = vku::surface_t::create(window, instance);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::image_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            if (node["m_path"].is_null()) {
                auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
                auto w = co_await resolve_int(vs, node["m_width"]);
                auto h = co_await resolve_int(vs, node["m_height"]);
                auto fmt = node["m_format"].is_null()
                        ? VK_FORMAT_R8G8B8A8_SRGB
                        : vc::get_enum_val<VkFormat>(node["m_format"]);
                auto usage = node["m_usage"].is_null()
                        ? VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                        : vc::get_enum_val<VkImageUsageFlagBits>(node["m_usage"]);
                auto tiling = node["m_tiling"].is_null()
                        ? VK_IMAGE_TILING_OPTIMAL
                        : vc::get_enum_val<VkImageTiling>(node["m_tiling"]);
                auto obj = vku::image_t::create(dev, w, h, fmt, usage, tiling);
                mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
                co_return obj->to_related<vku::object_t>();
            }
            else {
                auto cp = co_await resolve_obj<vku::cmdpool_t>(vs, node["m_cmdpool"]);
                auto path = co_await resolve_str(vs, node["m_path"]);
                auto obj = vku::load_image(cp, path);
                mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
                co_return obj->to_related<vku::object_t>();
            }
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::cmdpool_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::cmdpool_t::create(dev);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::device_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto inst = co_await resolve_obj<vku::instance_t>(vs, node["m_instance"]);
            auto surf = co_await resolve_obj<vku::surface_t>(vs, node["m_surface"]);
            std::vector<std::string> exts;
            if (!node["m_extensions"].is_null()) {
                if (!node["m_extensions"].is_sequence())
                    throw vc::except_t("m_extensions must be an array");
                for (auto &ext : node["m_extensions"])
                    exts.push_back(ext.as_str());
            }
            std::vector<std::string> layers;
            if (!node["m_layers"].is_null()) {
                if (!node["m_layers"].is_sequence())
                    throw vc::except_t("m_layers must be an array");
                for (auto &lay : node["m_layers"])
                    layers.push_back(lay.as_str());
            }
            auto obj = vku::device_t::create(inst, surf, exts, layers);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::img_sampl_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::img_sampl_t::create(dev);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::img_view_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto img = co_await resolve_obj<vku::image_t>(vs, node["m_image"]);
            auto aspect_mask = vc::get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]);
            auto obj = vku::img_view_t::create(img, aspect_mask);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_set_initializer_t::sampl_binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto view = node["m_view"].is_null()
                    ? nullptr
                    : co_await resolve_obj<vku::img_view_t>(vs, node["m_view"]);
            auto sampler = node["m_sampler"].is_null()
                    ? nullptr
                    : co_await resolve_obj<vku::img_sampl_t>(vs, node["m_sampler"]);
            auto layout = node["m_layout"].is_null()
                    ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                    : vc::get_enum_val<VkImageLayout>(node["m_layout"]);
            auto desc = co_await resolve_obj<vku::binding_t>(vs, node["m_desc"]);
            auto obj = vku::desc_set_initializer_t::sampl_binding_t::create(
                    desc->bd, view, sampler, layout);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_set_initializer_t::buff_binding_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto buff = node["m_buffer"].is_null()
                    ? nullptr
                    : co_await resolve_obj<vku::buffer_t>(vs, node["m_buffer"]);
            auto offset = node["m_offset"].is_null()
                    ? 0
                    : co_await resolve_int(vs, node["m_offset"]);
            auto size = node["m_size"].is_null()
                    ? 0
                    : co_await resolve_int(vs, node["m_size"]);
            auto desc = co_await resolve_obj<vku::binding_t>(vs, node["m_desc"]);
            auto obj = vku::desc_set_initializer_t::buff_binding_t::create(desc->bd, buff, offset, size);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_set_initializer_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            std::vector<vku::ref_t<vku::desc_set_initializer_t::binding_desc_t>> bindings;
            if (!node["m_descriptors"].is_null()) {
                if (!node["m_descriptors"].is_sequence())
                    throw vc::except_t("m_descriptors must be an array");
                for (auto& subnode : node["m_descriptors"]) {
                    bindings.push_back(
                            co_await resolve_obj<vku::desc_set_initializer_t::binding_desc_t>(vs,
                                    subnode));
                }
            }
            auto obj = vku::desc_set_initializer_t::create(bindings);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::buffer_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            size_t sz = co_await resolve_int(vs, node["m_size"]);
            auto usage_flags = vc::get_enum_val<VkBufferUsageFlagBits>(node["m_usage_flags"]);
            auto share_mode = vc::get_enum_val<VkSharingMode>(node["m_sharing_mode"]);
            auto memory_flags = vc::get_enum_val<VkMemoryPropertyFlagBits>(node["m_memory_flags"]);
            auto obj = vku::buffer_t::create(dev, sz, usage_flags, share_mode, memory_flags);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::pipeline_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto w = co_await resolve_int(vs, node["m_width"]);
            auto h = co_await resolve_int(vs, node["m_height"]);
            auto rp = co_await resolve_obj<vku::renderpass_t>(vs, node["m_renderpass"]);
            std::vector<vku::ref_t<vku::shader_t>> shaders;
            if (!node["m_shaders"].is_null()) {
                if (!node["m_shaders"].is_sequence())
                    throw vc::except_t("m_shaders must be an array");
                for (auto& sh : node["m_shaders"])
                    shaders.push_back(co_await resolve_obj<vku::shader_t>(vs, sh));
            }
            auto topol = vc::get_enum_val<VkPrimitiveTopology>(node["m_topology"]);
            auto indesc = co_await resolve_obj<vkc::vertex_input_desc_t>(vs, node["m_input_desc"]);
            auto pl = co_await resolve_obj<vku::pipeline_layout_t>(vs, node["m_pipeline_layout"]);
            auto obj = vku::pipeline_t::create(w, h, rp, shaders, topol, indesc->vid, pl);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::compute_pipeline_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto shader = co_await resolve_obj<vku::shader_t>(vs, node["m_shader"]);
            auto pl = co_await resolve_obj<vku::pipeline_layout_t>(vs, node["m_pipeline_layout"]);
            auto obj = vku::compute_pipeline_t::create(dev, shader, pl);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::renderpass_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto swc = co_await resolve_obj<vku::swapchain_t>(vs, node["m_swapchain"]);
            auto obj = vku::renderpass_t::create(swc);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::swapchain_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto surf = co_await resolve_obj<vku::surface_t>(vs, node["m_surface"]);
            auto obj = vku::swapchain_t::create(dev, surf);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::shader_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto spirv = co_await resolve_obj<spirv_t>(vs, node["m_spirv"]);
            auto obj = vku::shader_t::create(dev, spirv->spirv);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::fence_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto flags = node["m_flags"].is_null()
                    ? 0
                    : vc::get_enum_val<VkFenceCreateFlagBits>(node["m_flags"]);
            auto obj = vku::fence_t::create(dev, flags);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::sem_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto sem_type = node["m_sem_type"].is_null()
                    ? VK_SEMAPHORE_TYPE_BINARY
                    : vc::get_enum_val<VkSemaphoreType>(node["m_sem_type"]);
            auto initial = node["m_initial"].is_null()
                    ? 0
                    : co_await resolve_int(vs, node["m_initial"]);
            auto obj = vku::sem_t::create(dev, sem_type, initial);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::event_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto obj = vku::event_t::create(dev);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::framebuffs_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto rp = co_await resolve_obj<vku::renderpass_t>(vs, node["m_renderpass"]);
            auto obj = vku::framebuffs_t::create(rp);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_set_layout_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto bindings_initer = co_await resolve_obj<vku::desc_set_initializer_t>(
                    vs, node["m_bindings_initer"]);
            auto obj = vku::desc_set_layout_t::create(dev, bindings_initer);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::pipeline_layout_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            /* In the future this will not be a simple redirect, but will contain multiple layouts */
            auto desc_set_layout = co_await resolve_obj<vku::desc_set_layout_t>(
                    vs, node["m_desc_set_layout"]);
            auto obj = vku::pipeline_layout_t::create(desc_set_layout);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);


    ret = add_named_builder_callback(vs,
        "vku::desc_set_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto descriptor_pool = co_await resolve_obj<vku::desc_pool_t>(vs, node["m_descriptor_pool"]);
            auto desc_layout = co_await resolve_obj<vku::desc_set_layout_t>(vs, node["m_desc_set_layout"]);
            auto bindings = co_await resolve_obj<vku::desc_set_initializer_t>(vs, node["m_bindings_initer"]);
            auto obj = vku::desc_set_t::create(descriptor_pool, desc_layout, bindings);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::desc_pool_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto dev = co_await resolve_obj<vku::device_t>(vs, node["m_device"]);
            auto binds = co_await resolve_obj<vku::desc_set_initializer_t>(vs, node["m_bindings_initer"]);
            int cnt = co_await resolve_int(vs, node["m_cnt"]);
            auto obj = vku::desc_pool_t::create(dev, binds, cnt);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    ret = add_named_builder_callback(vs,
        "vku::cmdbuff_t",
        [](vc::virt_state_t *vs, const std::string& node_name, fkyaml::node& node)
            -> co::task<vc::ref_t<vc::object_t>>
        {
            auto cp = co_await resolve_obj<vku::cmdpool_t>(vs, node["m_cmdpool"]);
            auto obj = vku::cmdbuff_t::create(cp);
            mark_dependency_solved(vs, node_name, obj->to_related<vku::object_t>());
            co_return obj->to_related<vku::object_t>();
        }
    );
    ASSERT_FN(ret);

    return 0;
}

inline void luaw_set_glfw_fields(vc::virt_state_t *vs) {
    std::vector<std::pair<lua_Integer, std::string>> glfw_mapping = {
        {GLFW_VERSION_MAJOR, "GLFW_VERSION_MAJOR"},
        {GLFW_VERSION_MINOR, "GLFW_VERSION_MINOR"},
        {GLFW_VERSION_REVISION, "GLFW_VERSION_REVISION"},
        {GLFW_TRUE, "GLFW_TRUE"},
        {GLFW_FALSE, "GLFW_FALSE"},
        {GLFW_RELEASE, "GLFW_RELEASE"},
        {GLFW_PRESS, "GLFW_PRESS"},
        {GLFW_REPEAT, "GLFW_REPEAT"},
        {GLFW_HAT_CENTERED, "GLFW_HAT_CENTERED"},
        {GLFW_HAT_UP, "GLFW_HAT_UP"},
        {GLFW_HAT_RIGHT, "GLFW_HAT_RIGHT"},
        {GLFW_HAT_DOWN, "GLFW_HAT_DOWN"},
        {GLFW_HAT_LEFT, "GLFW_HAT_LEFT"},
        {GLFW_HAT_RIGHT_UP, "GLFW_HAT_RIGHT_UP"},
        {GLFW_HAT_RIGHT_DOWN, "GLFW_HAT_RIGHT_DOWN"},
        {GLFW_HAT_LEFT_UP, "GLFW_HAT_LEFT_UP"},
        {GLFW_HAT_LEFT_DOWN, "GLFW_HAT_LEFT_DOWN"},
        {GLFW_KEY_UNKNOWN, "GLFW_KEY_UNKNOWN"},
        {GLFW_KEY_SPACE, "GLFW_KEY_SPACE"},
        {GLFW_KEY_APOSTROPHE, "GLFW_KEY_APOSTROPHE"},
        {GLFW_KEY_COMMA, "GLFW_KEY_COMMA"},
        {GLFW_KEY_MINUS, "GLFW_KEY_MINUS"},
        {GLFW_KEY_PERIOD, "GLFW_KEY_PERIOD"},
        {GLFW_KEY_SLASH, "GLFW_KEY_SLASH"},
        {GLFW_KEY_0, "GLFW_KEY_0"},
        {GLFW_KEY_1, "GLFW_KEY_1"},
        {GLFW_KEY_2, "GLFW_KEY_2"},
        {GLFW_KEY_3, "GLFW_KEY_3"},
        {GLFW_KEY_4, "GLFW_KEY_4"},
        {GLFW_KEY_5, "GLFW_KEY_5"},
        {GLFW_KEY_6, "GLFW_KEY_6"},
        {GLFW_KEY_7, "GLFW_KEY_7"},
        {GLFW_KEY_8, "GLFW_KEY_8"},
        {GLFW_KEY_9, "GLFW_KEY_9"},
        {GLFW_KEY_SEMICOLON, "GLFW_KEY_SEMICOLON"},
        {GLFW_KEY_EQUAL, "GLFW_KEY_EQUAL"},
        {GLFW_KEY_A, "GLFW_KEY_A"},
        {GLFW_KEY_B, "GLFW_KEY_B"},
        {GLFW_KEY_C, "GLFW_KEY_C"},
        {GLFW_KEY_D, "GLFW_KEY_D"},
        {GLFW_KEY_E, "GLFW_KEY_E"},
        {GLFW_KEY_F, "GLFW_KEY_F"},
        {GLFW_KEY_G, "GLFW_KEY_G"},
        {GLFW_KEY_H, "GLFW_KEY_H"},
        {GLFW_KEY_I, "GLFW_KEY_I"},
        {GLFW_KEY_J, "GLFW_KEY_J"},
        {GLFW_KEY_K, "GLFW_KEY_K"},
        {GLFW_KEY_L, "GLFW_KEY_L"},
        {GLFW_KEY_M, "GLFW_KEY_M"},
        {GLFW_KEY_N, "GLFW_KEY_N"},
        {GLFW_KEY_O, "GLFW_KEY_O"},
        {GLFW_KEY_P, "GLFW_KEY_P"},
        {GLFW_KEY_Q, "GLFW_KEY_Q"},
        {GLFW_KEY_R, "GLFW_KEY_R"},
        {GLFW_KEY_S, "GLFW_KEY_S"},
        {GLFW_KEY_T, "GLFW_KEY_T"},
        {GLFW_KEY_U, "GLFW_KEY_U"},
        {GLFW_KEY_V, "GLFW_KEY_V"},
        {GLFW_KEY_W, "GLFW_KEY_W"},
        {GLFW_KEY_X, "GLFW_KEY_X"},
        {GLFW_KEY_Y, "GLFW_KEY_Y"},
        {GLFW_KEY_Z, "GLFW_KEY_Z"},
        {GLFW_KEY_LEFT_BRACKET, "GLFW_KEY_LEFT_BRACKET"},
        {GLFW_KEY_BACKSLASH, "GLFW_KEY_BACKSLASH"},
        {GLFW_KEY_RIGHT_BRACKET, "GLFW_KEY_RIGHT_BRACKET"},
        {GLFW_KEY_GRAVE_ACCENT, "GLFW_KEY_GRAVE_ACCENT"},
        {GLFW_KEY_WORLD_1, "GLFW_KEY_WORLD_1"},
        {GLFW_KEY_WORLD_2, "GLFW_KEY_WORLD_2"},
        {GLFW_KEY_ESCAPE, "GLFW_KEY_ESCAPE"},
        {GLFW_KEY_ENTER, "GLFW_KEY_ENTER"},
        {GLFW_KEY_TAB, "GLFW_KEY_TAB"},
        {GLFW_KEY_BACKSPACE, "GLFW_KEY_BACKSPACE"},
        {GLFW_KEY_INSERT, "GLFW_KEY_INSERT"},
        {GLFW_KEY_DELETE, "GLFW_KEY_DELETE"},
        {GLFW_KEY_RIGHT, "GLFW_KEY_RIGHT"},
        {GLFW_KEY_LEFT, "GLFW_KEY_LEFT"},
        {GLFW_KEY_DOWN, "GLFW_KEY_DOWN"},
        {GLFW_KEY_UP, "GLFW_KEY_UP"},
        {GLFW_KEY_PAGE_UP, "GLFW_KEY_PAGE_UP"},
        {GLFW_KEY_PAGE_DOWN, "GLFW_KEY_PAGE_DOWN"},
        {GLFW_KEY_HOME, "GLFW_KEY_HOME"},
        {GLFW_KEY_END, "GLFW_KEY_END"},
        {GLFW_KEY_CAPS_LOCK, "GLFW_KEY_CAPS_LOCK"},
        {GLFW_KEY_SCROLL_LOCK, "GLFW_KEY_SCROLL_LOCK"},
        {GLFW_KEY_NUM_LOCK, "GLFW_KEY_NUM_LOCK"},
        {GLFW_KEY_PRINT_SCREEN, "GLFW_KEY_PRINT_SCREEN"},
        {GLFW_KEY_PAUSE, "GLFW_KEY_PAUSE"},
        {GLFW_KEY_F1, "GLFW_KEY_F1"},
        {GLFW_KEY_F2, "GLFW_KEY_F2"},
        {GLFW_KEY_F3, "GLFW_KEY_F3"},
        {GLFW_KEY_F4, "GLFW_KEY_F4"},
        {GLFW_KEY_F5, "GLFW_KEY_F5"},
        {GLFW_KEY_F6, "GLFW_KEY_F6"},
        {GLFW_KEY_F7, "GLFW_KEY_F7"},
        {GLFW_KEY_F8, "GLFW_KEY_F8"},
        {GLFW_KEY_F9, "GLFW_KEY_F9"},
        {GLFW_KEY_F10, "GLFW_KEY_F10"},
        {GLFW_KEY_F11, "GLFW_KEY_F11"},
        {GLFW_KEY_F12, "GLFW_KEY_F12"},
        {GLFW_KEY_F13, "GLFW_KEY_F13"},
        {GLFW_KEY_F14, "GLFW_KEY_F14"},
        {GLFW_KEY_F15, "GLFW_KEY_F15"},
        {GLFW_KEY_F16, "GLFW_KEY_F16"},
        {GLFW_KEY_F17, "GLFW_KEY_F17"},
        {GLFW_KEY_F18, "GLFW_KEY_F18"},
        {GLFW_KEY_F19, "GLFW_KEY_F19"},
        {GLFW_KEY_F20, "GLFW_KEY_F20"},
        {GLFW_KEY_F21, "GLFW_KEY_F21"},
        {GLFW_KEY_F22, "GLFW_KEY_F22"},
        {GLFW_KEY_F23, "GLFW_KEY_F23"},
        {GLFW_KEY_F24, "GLFW_KEY_F24"},
        {GLFW_KEY_F25, "GLFW_KEY_F25"},
        {GLFW_KEY_KP_0, "GLFW_KEY_KP_0"},
        {GLFW_KEY_KP_1, "GLFW_KEY_KP_1"},
        {GLFW_KEY_KP_2, "GLFW_KEY_KP_2"},
        {GLFW_KEY_KP_3, "GLFW_KEY_KP_3"},
        {GLFW_KEY_KP_4, "GLFW_KEY_KP_4"},
        {GLFW_KEY_KP_5, "GLFW_KEY_KP_5"},
        {GLFW_KEY_KP_6, "GLFW_KEY_KP_6"},
        {GLFW_KEY_KP_7, "GLFW_KEY_KP_7"},
        {GLFW_KEY_KP_8, "GLFW_KEY_KP_8"},
        {GLFW_KEY_KP_9, "GLFW_KEY_KP_9"},
        {GLFW_KEY_KP_DECIMAL, "GLFW_KEY_KP_DECIMAL"},
        {GLFW_KEY_KP_DIVIDE, "GLFW_KEY_KP_DIVIDE"},
        {GLFW_KEY_KP_MULTIPLY, "GLFW_KEY_KP_MULTIPLY"},
        {GLFW_KEY_KP_SUBTRACT, "GLFW_KEY_KP_SUBTRACT"},
        {GLFW_KEY_KP_ADD, "GLFW_KEY_KP_ADD"},
        {GLFW_KEY_KP_ENTER, "GLFW_KEY_KP_ENTER"},
        {GLFW_KEY_KP_EQUAL, "GLFW_KEY_KP_EQUAL"},
        {GLFW_KEY_LEFT_SHIFT, "GLFW_KEY_LEFT_SHIFT"},
        {GLFW_KEY_LEFT_CONTROL, "GLFW_KEY_LEFT_CONTROL"},
        {GLFW_KEY_LEFT_ALT, "GLFW_KEY_LEFT_ALT"},
        {GLFW_KEY_LEFT_SUPER, "GLFW_KEY_LEFT_SUPER"},
        {GLFW_KEY_RIGHT_SHIFT, "GLFW_KEY_RIGHT_SHIFT"},
        {GLFW_KEY_RIGHT_CONTROL, "GLFW_KEY_RIGHT_CONTROL"},
        {GLFW_KEY_RIGHT_ALT, "GLFW_KEY_RIGHT_ALT"},
        {GLFW_KEY_RIGHT_SUPER, "GLFW_KEY_RIGHT_SUPER"},
        {GLFW_KEY_MENU, "GLFW_KEY_MENU"},
        {GLFW_KEY_LAST, "GLFW_KEY_LAST"},
        {GLFW_MOD_SHIFT, "GLFW_MOD_SHIFT"},
        {GLFW_MOD_CONTROL, "GLFW_MOD_CONTROL"},
        {GLFW_MOD_ALT, "GLFW_MOD_ALT"},
        {GLFW_MOD_SUPER, "GLFW_MOD_SUPER"},
        {GLFW_MOD_CAPS_LOCK, "GLFW_MOD_CAPS_LOCK"},
        {GLFW_MOD_NUM_LOCK, "GLFW_MOD_NUM_LOCK"},
        {GLFW_MOUSE_BUTTON_1, "GLFW_MOUSE_BUTTON_1"},
        {GLFW_MOUSE_BUTTON_2, "GLFW_MOUSE_BUTTON_2"},
        {GLFW_MOUSE_BUTTON_3, "GLFW_MOUSE_BUTTON_3"},
        {GLFW_MOUSE_BUTTON_4, "GLFW_MOUSE_BUTTON_4"},
        {GLFW_MOUSE_BUTTON_5, "GLFW_MOUSE_BUTTON_5"},
        {GLFW_MOUSE_BUTTON_6, "GLFW_MOUSE_BUTTON_6"},
        {GLFW_MOUSE_BUTTON_7, "GLFW_MOUSE_BUTTON_7"},
        {GLFW_MOUSE_BUTTON_8, "GLFW_MOUSE_BUTTON_8"},
        {GLFW_MOUSE_BUTTON_LAST, "GLFW_MOUSE_BUTTON_LAST"},
        {GLFW_MOUSE_BUTTON_LEFT, "GLFW_MOUSE_BUTTON_LEFT"},
        {GLFW_MOUSE_BUTTON_RIGHT, "GLFW_MOUSE_BUTTON_RIGHT"},
        {GLFW_MOUSE_BUTTON_MIDDLE, "GLFW_MOUSE_BUTTON_MIDDLE"},
        {GLFW_JOYSTICK_1, "GLFW_JOYSTICK_1"},
        {GLFW_JOYSTICK_2, "GLFW_JOYSTICK_2"},
        {GLFW_JOYSTICK_3, "GLFW_JOYSTICK_3"},
        {GLFW_JOYSTICK_4, "GLFW_JOYSTICK_4"},
        {GLFW_JOYSTICK_5, "GLFW_JOYSTICK_5"},
        {GLFW_JOYSTICK_6, "GLFW_JOYSTICK_6"},
        {GLFW_JOYSTICK_7, "GLFW_JOYSTICK_7"},
        {GLFW_JOYSTICK_8, "GLFW_JOYSTICK_8"},
        {GLFW_JOYSTICK_9, "GLFW_JOYSTICK_9"},
        {GLFW_JOYSTICK_10, "GLFW_JOYSTICK_10"},
        {GLFW_JOYSTICK_11, "GLFW_JOYSTICK_11"},
        {GLFW_JOYSTICK_12, "GLFW_JOYSTICK_12"},
        {GLFW_JOYSTICK_13, "GLFW_JOYSTICK_13"},
        {GLFW_JOYSTICK_14, "GLFW_JOYSTICK_14"},
        {GLFW_JOYSTICK_15, "GLFW_JOYSTICK_15"},
        {GLFW_JOYSTICK_16, "GLFW_JOYSTICK_16"},
        {GLFW_JOYSTICK_LAST, "GLFW_JOYSTICK_LAST"},
        {GLFW_GAMEPAD_BUTTON_A, "GLFW_GAMEPAD_BUTTON_A"},
        {GLFW_GAMEPAD_BUTTON_B, "GLFW_GAMEPAD_BUTTON_B"},
        {GLFW_GAMEPAD_BUTTON_X, "GLFW_GAMEPAD_BUTTON_X"},
        {GLFW_GAMEPAD_BUTTON_Y, "GLFW_GAMEPAD_BUTTON_Y"},
        {GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, "GLFW_GAMEPAD_BUTTON_LEFT_BUMPER"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, "GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER"},
        {GLFW_GAMEPAD_BUTTON_BACK, "GLFW_GAMEPAD_BUTTON_BACK"},
        {GLFW_GAMEPAD_BUTTON_START, "GLFW_GAMEPAD_BUTTON_START"},
        {GLFW_GAMEPAD_BUTTON_GUIDE, "GLFW_GAMEPAD_BUTTON_GUIDE"},
        {GLFW_GAMEPAD_BUTTON_LEFT_THUMB, "GLFW_GAMEPAD_BUTTON_LEFT_THUMB"},
        {GLFW_GAMEPAD_BUTTON_RIGHT_THUMB, "GLFW_GAMEPAD_BUTTON_RIGHT_THUMB"},
        {GLFW_GAMEPAD_BUTTON_DPAD_UP, "GLFW_GAMEPAD_BUTTON_DPAD_UP"},
        {GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, "GLFW_GAMEPAD_BUTTON_DPAD_RIGHT"},
        {GLFW_GAMEPAD_BUTTON_DPAD_DOWN, "GLFW_GAMEPAD_BUTTON_DPAD_DOWN"},
        {GLFW_GAMEPAD_BUTTON_DPAD_LEFT, "GLFW_GAMEPAD_BUTTON_DPAD_LEFT"},
        {GLFW_GAMEPAD_BUTTON_LAST, "GLFW_GAMEPAD_BUTTON_LAST"},
        {GLFW_GAMEPAD_BUTTON_CROSS, "GLFW_GAMEPAD_BUTTON_CROSS"},
        {GLFW_GAMEPAD_BUTTON_CIRCLE, "GLFW_GAMEPAD_BUTTON_CIRCLE"},
        {GLFW_GAMEPAD_BUTTON_SQUARE, "GLFW_GAMEPAD_BUTTON_SQUARE"},
        {GLFW_GAMEPAD_BUTTON_TRIANGLE, "GLFW_GAMEPAD_BUTTON_TRIANGLE"},
        {GLFW_GAMEPAD_AXIS_LEFT_X, "GLFW_GAMEPAD_AXIS_LEFT_X"},
        {GLFW_GAMEPAD_AXIS_LEFT_Y, "GLFW_GAMEPAD_AXIS_LEFT_Y"},
        {GLFW_GAMEPAD_AXIS_RIGHT_X, "GLFW_GAMEPAD_AXIS_RIGHT_X"},
        {GLFW_GAMEPAD_AXIS_RIGHT_Y, "GLFW_GAMEPAD_AXIS_RIGHT_Y"},
        {GLFW_GAMEPAD_AXIS_LEFT_TRIGGER, "GLFW_GAMEPAD_AXIS_LEFT_TRIGGER"},
        {GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, "GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER"},
        {GLFW_GAMEPAD_AXIS_LAST, "GLFW_GAMEPAD_AXIS_LAST"},
        {GLFW_NO_ERROR, "GLFW_NO_ERROR"},
        {GLFW_NOT_INITIALIZED, "GLFW_NOT_INITIALIZED"},
        {GLFW_NO_CURRENT_CONTEXT, "GLFW_NO_CURRENT_CONTEXT"},
        {GLFW_INVALID_ENUM, "GLFW_INVALID_ENUM"},
        {GLFW_INVALID_VALUE, "GLFW_INVALID_VALUE"},
        {GLFW_OUT_OF_MEMORY, "GLFW_OUT_OF_MEMORY"},
        {GLFW_API_UNAVAILABLE, "GLFW_API_UNAVAILABLE"},
        {GLFW_VERSION_UNAVAILABLE, "GLFW_VERSION_UNAVAILABLE"},
        {GLFW_PLATFORM_ERROR, "GLFW_PLATFORM_ERROR"},
        {GLFW_FORMAT_UNAVAILABLE, "GLFW_FORMAT_UNAVAILABLE"},
        {GLFW_NO_WINDOW_CONTEXT, "GLFW_NO_WINDOW_CONTEXT"},
        {GLFW_FOCUSED, "GLFW_FOCUSED"},
        {GLFW_ICONIFIED, "GLFW_ICONIFIED"},
        {GLFW_RESIZABLE, "GLFW_RESIZABLE"},
        {GLFW_VISIBLE, "GLFW_VISIBLE"},
        {GLFW_DECORATED, "GLFW_DECORATED"},
        {GLFW_AUTO_ICONIFY, "GLFW_AUTO_ICONIFY"},
        {GLFW_FLOATING, "GLFW_FLOATING"},
        {GLFW_MAXIMIZED, "GLFW_MAXIMIZED"},
        {GLFW_CENTER_CURSOR, "GLFW_CENTER_CURSOR"},
        {GLFW_TRANSPARENT_FRAMEBUFFER, "GLFW_TRANSPARENT_FRAMEBUFFER"},
        {GLFW_HOVERED, "GLFW_HOVERED"},
        {GLFW_FOCUS_ON_SHOW, "GLFW_FOCUS_ON_SHOW"},
        {GLFW_RED_BITS, "GLFW_RED_BITS"},
        {GLFW_GREEN_BITS, "GLFW_GREEN_BITS"},
        {GLFW_BLUE_BITS, "GLFW_BLUE_BITS"},
        {GLFW_ALPHA_BITS, "GLFW_ALPHA_BITS"},
        {GLFW_DEPTH_BITS, "GLFW_DEPTH_BITS"},
        {GLFW_STENCIL_BITS, "GLFW_STENCIL_BITS"},
        {GLFW_ACCUM_RED_BITS, "GLFW_ACCUM_RED_BITS"},
        {GLFW_ACCUM_GREEN_BITS, "GLFW_ACCUM_GREEN_BITS"},
        {GLFW_ACCUM_BLUE_BITS, "GLFW_ACCUM_BLUE_BITS"},
        {GLFW_ACCUM_ALPHA_BITS, "GLFW_ACCUM_ALPHA_BITS"},
        {GLFW_AUX_BUFFERS, "GLFW_AUX_BUFFERS"},
        {GLFW_STEREO, "GLFW_STEREO"},
        {GLFW_SAMPLES, "GLFW_SAMPLES"},
        {GLFW_SRGB_CAPABLE, "GLFW_SRGB_CAPABLE"},
        {GLFW_REFRESH_RATE, "GLFW_REFRESH_RATE"},
        {GLFW_DOUBLEBUFFER, "GLFW_DOUBLEBUFFER"},
        {GLFW_CLIENT_API, "GLFW_CLIENT_API"},
        {GLFW_CONTEXT_VERSION_MAJOR, "GLFW_CONTEXT_VERSION_MAJOR"},
        {GLFW_CONTEXT_VERSION_MINOR, "GLFW_CONTEXT_VERSION_MINOR"},
        {GLFW_CONTEXT_REVISION, "GLFW_CONTEXT_REVISION"},
        {GLFW_CONTEXT_ROBUSTNESS, "GLFW_CONTEXT_ROBUSTNESS"},
        {GLFW_OPENGL_FORWARD_COMPAT, "GLFW_OPENGL_FORWARD_COMPAT"},
        {GLFW_OPENGL_DEBUG_CONTEXT, "GLFW_OPENGL_DEBUG_CONTEXT"},
        {GLFW_OPENGL_PROFILE, "GLFW_OPENGL_PROFILE"},
        {GLFW_CONTEXT_RELEASE_BEHAVIOR, "GLFW_CONTEXT_RELEASE_BEHAVIOR"},
        {GLFW_CONTEXT_NO_ERROR, "GLFW_CONTEXT_NO_ERROR"},
        {GLFW_CONTEXT_CREATION_API, "GLFW_CONTEXT_CREATION_API"},
        {GLFW_SCALE_TO_MONITOR, "GLFW_SCALE_TO_MONITOR"},
        {GLFW_COCOA_RETINA_FRAMEBUFFER, "GLFW_COCOA_RETINA_FRAMEBUFFER"},
        {GLFW_COCOA_FRAME_NAME, "GLFW_COCOA_FRAME_NAME"},
        {GLFW_COCOA_GRAPHICS_SWITCHING, "GLFW_COCOA_GRAPHICS_SWITCHING"},
        {GLFW_X11_CLASS_NAME, "GLFW_X11_CLASS_NAME"},
        {GLFW_X11_INSTANCE_NAME, "GLFW_X11_INSTANCE_NAME"},
        {GLFW_NO_API, "GLFW_NO_API"},
        {GLFW_OPENGL_API, "GLFW_OPENGL_API"},
        {GLFW_OPENGL_ES_API, "GLFW_OPENGL_ES_API"},
        {GLFW_NO_ROBUSTNESS, "GLFW_NO_ROBUSTNESS"},
        {GLFW_NO_RESET_NOTIFICATION, "GLFW_NO_RESET_NOTIFICATION"},
        {GLFW_LOSE_CONTEXT_ON_RESET, "GLFW_LOSE_CONTEXT_ON_RESET"},
        {GLFW_OPENGL_ANY_PROFILE, "GLFW_OPENGL_ANY_PROFILE"},
        {GLFW_OPENGL_CORE_PROFILE, "GLFW_OPENGL_CORE_PROFILE"},
        {GLFW_OPENGL_COMPAT_PROFILE, "GLFW_OPENGL_COMPAT_PROFILE"},
        {GLFW_CURSOR, "GLFW_CURSOR"},
        {GLFW_STICKY_KEYS, "GLFW_STICKY_KEYS"},
        {GLFW_STICKY_MOUSE_BUTTONS, "GLFW_STICKY_MOUSE_BUTTONS"},
        {GLFW_LOCK_KEY_MODS, "GLFW_LOCK_KEY_MODS"},
        {GLFW_RAW_MOUSE_MOTION, "GLFW_RAW_MOUSE_MOTION"},
        {GLFW_CURSOR_NORMAL, "GLFW_CURSOR_NORMAL"},
        {GLFW_CURSOR_HIDDEN, "GLFW_CURSOR_HIDDEN"},
        {GLFW_CURSOR_DISABLED, "GLFW_CURSOR_DISABLED"},
        {GLFW_ANY_RELEASE_BEHAVIOR, "GLFW_ANY_RELEASE_BEHAVIOR"},
        {GLFW_RELEASE_BEHAVIOR_FLUSH, "GLFW_RELEASE_BEHAVIOR_FLUSH"},
        {GLFW_RELEASE_BEHAVIOR_NONE, "GLFW_RELEASE_BEHAVIOR_NONE"},
        {GLFW_NATIVE_CONTEXT_API, "GLFW_NATIVE_CONTEXT_API"},
        {GLFW_EGL_CONTEXT_API, "GLFW_EGL_CONTEXT_API"},
        {GLFW_OSMESA_CONTEXT_API, "GLFW_OSMESA_CONTEXT_API"},
        {GLFW_ARROW_CURSOR, "GLFW_ARROW_CURSOR"},
        {GLFW_IBEAM_CURSOR, "GLFW_IBEAM_CURSOR"},
        {GLFW_CROSSHAIR_CURSOR, "GLFW_CROSSHAIR_CURSOR"},
        {GLFW_HRESIZE_CURSOR, "GLFW_HRESIZE_CURSOR"},
        {GLFW_VRESIZE_CURSOR, "GLFW_VRESIZE_CURSOR"},
        {GLFW_HAND_CURSOR, "GLFW_HAND_CURSOR"},
        {GLFW_CONNECTED, "GLFW_CONNECTED"},
        {GLFW_DISCONNECTED, "GLFW_DISCONNECTED"},
        {GLFW_JOYSTICK_HAT_BUTTONS, "GLFW_JOYSTICK_HAT_BUTTONS"},
        {GLFW_COCOA_CHDIR_RESOURCES, "GLFW_COCOA_CHDIR_RESOURCES"},
        {GLFW_COCOA_MENUBAR, "GLFW_COCOA_MENUBAR"}
    };

    vc::add_lua_flag_mapping(vs, glfw_mapping);
}

}; /* namespace vkc */

namespace virt_composer {

inline std::unordered_map<std::string, VkResult> vk_result_from_str = {
    {"VK_SUCCESS",
            VK_SUCCESS},
    {"VK_NOT_READY",
            VK_NOT_READY},
    {"VK_TIMEOUT",
            VK_TIMEOUT},
    {"VK_EVENT_SET",
            VK_EVENT_SET},
    {"VK_EVENT_RESET",
            VK_EVENT_RESET},
    {"VK_INCOMPLETE",
            VK_INCOMPLETE},
    {"VK_ERROR_OUT_OF_HOST_MEMORY",
            VK_ERROR_OUT_OF_HOST_MEMORY},
    {"VK_ERROR_OUT_OF_DEVICE_MEMORY",
            VK_ERROR_OUT_OF_DEVICE_MEMORY},
    {"VK_ERROR_INITIALIZATION_FAILED",
            VK_ERROR_INITIALIZATION_FAILED},
    {"VK_ERROR_DEVICE_LOST",
            VK_ERROR_DEVICE_LOST},
    {"VK_ERROR_MEMORY_MAP_FAILED",
            VK_ERROR_MEMORY_MAP_FAILED},
    {"VK_ERROR_LAYER_NOT_PRESENT",
            VK_ERROR_LAYER_NOT_PRESENT},
    {"VK_ERROR_EXTENSION_NOT_PRESENT",
            VK_ERROR_EXTENSION_NOT_PRESENT},
    {"VK_ERROR_FEATURE_NOT_PRESENT",
            VK_ERROR_FEATURE_NOT_PRESENT},
    {"VK_ERROR_INCOMPATIBLE_DRIVER",
            VK_ERROR_INCOMPATIBLE_DRIVER},
    {"VK_ERROR_TOO_MANY_OBJECTS",
            VK_ERROR_TOO_MANY_OBJECTS},
    {"VK_ERROR_FORMAT_NOT_SUPPORTED",
            VK_ERROR_FORMAT_NOT_SUPPORTED},
    {"VK_ERROR_FRAGMENTED_POOL",
            VK_ERROR_FRAGMENTED_POOL},
    {"VK_ERROR_UNKNOWN",
            VK_ERROR_UNKNOWN},
    // {"VK_ERROR_VALIDATION_FAILED",
    //         VK_ERROR_VALIDATION_FAILED},
    {"VK_ERROR_OUT_OF_POOL_MEMORY",
            VK_ERROR_OUT_OF_POOL_MEMORY},
    {"VK_ERROR_INVALID_EXTERNAL_HANDLE",
            VK_ERROR_INVALID_EXTERNAL_HANDLE},
    {"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS",
            VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS},
    {"VK_ERROR_FRAGMENTATION",
            VK_ERROR_FRAGMENTATION},
    // {"VK_PIPELINE_COMPILE_REQUIRED",
    //         VK_PIPELINE_COMPILE_REQUIRED},
    // {"VK_ERROR_NOT_PERMITTED",
    //         VK_ERROR_NOT_PERMITTED},
    {"VK_ERROR_SURFACE_LOST_KHR",
            VK_ERROR_SURFACE_LOST_KHR},
    {"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR",
            VK_ERROR_NATIVE_WINDOW_IN_USE_KHR},
    {"VK_SUBOPTIMAL_KHR",
            VK_SUBOPTIMAL_KHR},
    {"VK_ERROR_OUT_OF_DATE_KHR",
            VK_ERROR_OUT_OF_DATE_KHR},
    {"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR",
            VK_ERROR_INCOMPATIBLE_DISPLAY_KHR},
    {"VK_ERROR_INVALID_SHADER_NV",
            VK_ERROR_INVALID_SHADER_NV},
    // {"VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR",
    //         VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR},
    // {"VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR",
    //         VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR},
    // {"VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR",
    //         VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR},
    // {"VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR",
    //         VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR},
    // {"VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR",
    //         VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR},
    // {"VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR",
    //         VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR},
    {"VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT",
            VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT},
    // {"VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT",
    //         VK_ERROR_PRESENT_TIMING_QUEUE_FULL_EXT},
    {"VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT",
            VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT},
    // {"VK_THREAD_IDLE_KHR",
    //         VK_THREAD_IDLE_KHR},
    // {"VK_THREAD_DONE_KHR",
    //         VK_THREAD_DONE_KHR},
    // {"VK_OPERATION_DEFERRED_KHR",
    //         VK_OPERATION_DEFERRED_KHR},
    // {"VK_OPERATION_NOT_DEFERRED_KHR",
    //         VK_OPERATION_NOT_DEFERRED_KHR},
    // {"VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR",
    //         VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR},
    // {"VK_ERROR_COMPRESSION_EXHAUSTED_EXT",
    //         VK_ERROR_COMPRESSION_EXHAUSTED_EXT},
    // {"VK_INCOMPATIBLE_SHADER_BINARY_EXT",
    //         VK_INCOMPATIBLE_SHADER_BINARY_EXT},
    // {"VK_PIPELINE_BINARY_MISSING_KHR",
    //         VK_PIPELINE_BINARY_MISSING_KHR},
    // {"VK_ERROR_NOT_ENOUGH_SPACE_KHR",
    //         VK_ERROR_NOT_ENOUGH_SPACE_KHR},
};

template <> inline VkResult get_enum_val<VkResult>(fkyaml::node &n) {
    return get_enum_val(n, vk_result_from_str);
}

inline std::unordered_map<std::string, VkImageTiling> vk_image_tiling_from_str = {
    {"VK_IMAGE_TILING_OPTIMAL", VK_IMAGE_TILING_OPTIMAL},
    {"VK_IMAGE_TILING_LINEAR", VK_IMAGE_TILING_LINEAR},
    {"VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT", VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT},
};

template <> inline VkImageTiling get_enum_val<VkImageTiling>(fkyaml::node &n) {
    return get_enum_val(n, vk_image_tiling_from_str);
}

inline std::unordered_map<std::string, VkImageUsageFlagBits> vk_image_usage_flag_bits_from_str = {
    {"VK_IMAGE_USAGE_TRANSFER_SRC_BIT",
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT},
    {"VK_IMAGE_USAGE_TRANSFER_DST_BIT",
            VK_IMAGE_USAGE_TRANSFER_DST_BIT},
    {"VK_IMAGE_USAGE_SAMPLED_BIT",
            VK_IMAGE_USAGE_SAMPLED_BIT},
    {"VK_IMAGE_USAGE_STORAGE_BIT",
            VK_IMAGE_USAGE_STORAGE_BIT},
    {"VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT",
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
    {"VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT",
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {"VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT",
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT},
    {"VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT",
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT},
    {"VK_IMAGE_USAGE_HOST_TRANSFER_BIT",
            VK_IMAGE_USAGE_HOST_TRANSFER_BIT},
    {"VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR},
    {"VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT",
            VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT},
    {"VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR",
            VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR},
    {"VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT",
            VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT},
    {"VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI",
            VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI},
    {"VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM",
            VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM},
    {"VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM",
            VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM},
    {"VK_IMAGE_USAGE_TENSOR_ALIASING_BIT_ARM",
            VK_IMAGE_USAGE_TENSOR_ALIASING_BIT_ARM},
    {"VK_IMAGE_USAGE_TILE_MEMORY_BIT_QCOM",
            VK_IMAGE_USAGE_TILE_MEMORY_BIT_QCOM},
    {"VK_IMAGE_USAGE_VIDEO_ENCODE_QUANTIZATION_DELTA_MAP_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_ENCODE_QUANTIZATION_DELTA_MAP_BIT_KHR},
    {"VK_IMAGE_USAGE_VIDEO_ENCODE_EMPHASIS_MAP_BIT_KHR",
            VK_IMAGE_USAGE_VIDEO_ENCODE_EMPHASIS_MAP_BIT_KHR},
};

template <> inline VkImageUsageFlagBits get_enum_val<VkImageUsageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_image_usage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkBufferUsageFlagBits> vk_buffer_usage_flag_bits_from_str = {
    {"VK_BUFFER_USAGE_NONE", (VkBufferUsageFlagBits)0},
    {"VK_BUFFER_USAGE_TRANSFER_SRC_BIT",
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT},
    {"VK_BUFFER_USAGE_TRANSFER_DST_BIT",
            VK_BUFFER_USAGE_TRANSFER_DST_BIT},
    {"VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT",
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT},
    {"VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT",
            VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT},
    {"VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT",
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
    {"VK_BUFFER_USAGE_STORAGE_BUFFER_BIT",
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
    {"VK_BUFFER_USAGE_INDEX_BUFFER_BIT",
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
    {"VK_BUFFER_USAGE_VERTEX_BUFFER_BIT",
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
    {"VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT",
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT},
    // {"VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR},
    {"VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT",
            VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT},
    {"VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT",
            VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT},
    {"VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT",
            VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT},
#ifdef VK_ENABLE_BETA_EXTENSIONS
    {"VK_BUFFER_USAGE_EXECUTION_GRAPH_SCRATCH_BIT_AMDX",
            VK_BUFFER_USAGE_EXECUTION_GRAPH_SCRATCH_BIT_AMDX},
#endif
    // {"VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR",
    //         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR},
    // {"VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR",
    //         VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR},
    // {"VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR",
    //         VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_ENCODE_DST_BIT_KHR},
    // {"VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR",
    //         VK_BUFFER_USAGE_VIDEO_ENCODE_SRC_BIT_KHR},
    // {"VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT",
    //         VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT},
    // {"VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT",
    //         VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT},
    // {"VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT",
    //         VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT},
    // {"VK_BUFFER_USAGE_TILE_MEMORY_BIT_QCOM",
    //         VK_BUFFER_USAGE_TILE_MEMORY_BIT_QCOM},
    {"VK_BUFFER_USAGE_RAY_TRACING_BIT_NV",
            VK_BUFFER_USAGE_RAY_TRACING_BIT_NV},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT},
    {"VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR",
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR},
};

template <> inline VkBufferUsageFlagBits get_enum_val<VkBufferUsageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_buffer_usage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkSharingMode> vk_sharing_mode_from_str = {
    {"VK_SHARING_MODE_EXCLUSIVE", VK_SHARING_MODE_EXCLUSIVE},
    {"VK_SHARING_MODE_CONCURRENT", VK_SHARING_MODE_CONCURRENT},
};

template <> inline VkSharingMode get_enum_val<VkSharingMode>(fkyaml::node &n) {
    return get_enum_val(n, vk_sharing_mode_from_str);
}

inline std::unordered_map<std::string, VkMemoryPropertyFlagBits>
        vk_memory_property_flag_bits_from_str =
{
    {"VK_MEMORY_PROPERTY_NONE", (VkMemoryPropertyFlagBits)0},
    {"VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT", VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT},
    {"VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT", VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT},
    {"VK_MEMORY_PROPERTY_HOST_COHERENT_BIT", VK_MEMORY_PROPERTY_HOST_COHERENT_BIT},
    {"VK_MEMORY_PROPERTY_HOST_CACHED_BIT", VK_MEMORY_PROPERTY_HOST_CACHED_BIT},
    {"VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT", VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT},
    {"VK_MEMORY_PROPERTY_PROTECTED_BIT", VK_MEMORY_PROPERTY_PROTECTED_BIT},
    {"VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD", VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD},
    {"VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD", VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD},
    // {"VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV", VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV},
};

template <> inline VkMemoryPropertyFlagBits get_enum_val<VkMemoryPropertyFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_memory_property_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkPrimitiveTopology> vk_primitive_topology_from_str = {
    {"VK_PRIMITIVE_TOPOLOGY_POINT_LIST",
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_LIST",
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_STRIP",
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY",
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY},
    {"VK_PRIMITIVE_TOPOLOGY_PATCH_LIST",
            VK_PRIMITIVE_TOPOLOGY_PATCH_LIST},
};

template <> inline VkPrimitiveTopology get_enum_val<VkPrimitiveTopology>(fkyaml::node &n) {
    return get_enum_val(n, vk_primitive_topology_from_str);
}

inline std::unordered_map<std::string, VkImageAspectFlagBits> vk_image_aspect_flag_bits_from_str = {
    {"VK_IMAGE_ASPECT_NONE", (VkImageAspectFlagBits)0},
    {"VK_IMAGE_ASPECT_COLOR_BIT", VK_IMAGE_ASPECT_COLOR_BIT},
    {"VK_IMAGE_ASPECT_DEPTH_BIT", VK_IMAGE_ASPECT_DEPTH_BIT},
    {"VK_IMAGE_ASPECT_STENCIL_BIT", VK_IMAGE_ASPECT_STENCIL_BIT},
    {"VK_IMAGE_ASPECT_METADATA_BIT", VK_IMAGE_ASPECT_METADATA_BIT},
    {"VK_IMAGE_ASPECT_PLANE_0_BIT", VK_IMAGE_ASPECT_PLANE_0_BIT},
    {"VK_IMAGE_ASPECT_PLANE_1_BIT", VK_IMAGE_ASPECT_PLANE_1_BIT},
    {"VK_IMAGE_ASPECT_PLANE_2_BIT", VK_IMAGE_ASPECT_PLANE_2_BIT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT},
    {"VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT", VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT},
    {"VK_IMAGE_ASPECT_PLANE_0_BIT_KHR", VK_IMAGE_ASPECT_PLANE_0_BIT_KHR},
    {"VK_IMAGE_ASPECT_PLANE_1_BIT_KHR", VK_IMAGE_ASPECT_PLANE_1_BIT_KHR},
    {"VK_IMAGE_ASPECT_PLANE_2_BIT_KHR", VK_IMAGE_ASPECT_PLANE_2_BIT_KHR},
};

template <> inline VkImageAspectFlagBits get_enum_val<VkImageAspectFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_image_aspect_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkCommandBufferUsageFlagBits>
        vk_command_buffer_usage_flag_bits_from_str =
{
    {"VK_COMMAND_BUFFER_USAGE_NONE", (VkCommandBufferUsageFlagBits)0},
    {"VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT",
            VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT},
    {"VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT",
            VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT},
    {"VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT",
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT},
};

template <> inline VkCommandBufferUsageFlagBits get_enum_val<VkCommandBufferUsageFlagBits>(
        fkyaml::node &n)
{
    return get_enum_val(n, vk_command_buffer_usage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkPipelineBindPoint> vk_pipeline_bind_point_from_str = {
    {"VK_PIPELINE_BIND_POINT_GRAPHICS",
            VK_PIPELINE_BIND_POINT_GRAPHICS},
    {"VK_PIPELINE_BIND_POINT_COMPUTE",
            VK_PIPELINE_BIND_POINT_COMPUTE},
    {"VK_PIPELINE_BIND_POINT_RAY_TRACING_NV",
            VK_PIPELINE_BIND_POINT_RAY_TRACING_NV},
};

template <> inline VkPipelineBindPoint get_enum_val<VkPipelineBindPoint>(fkyaml::node &n) {
    return get_enum_val(n, vk_pipeline_bind_point_from_str);
}

inline std::unordered_map<std::string, VkIndexType> vk_index_type_from_str = {
    {"VK_INDEX_TYPE_UINT16", VK_INDEX_TYPE_UINT16},
    {"VK_INDEX_TYPE_UINT32", VK_INDEX_TYPE_UINT32},
    {"VK_INDEX_TYPE_NONE_NV", VK_INDEX_TYPE_NONE_NV},
    {"VK_INDEX_TYPE_UINT8_EXT", VK_INDEX_TYPE_UINT8_EXT},
};

template <> inline VkIndexType get_enum_val<VkIndexType>(fkyaml::node &n) {
    return get_enum_val(n, vk_index_type_from_str);
}

inline std::unordered_map<std::string, VkPipelineStageFlagBits>
        vk_pipeline_stage_flag_bits_from_str =
{
    {"VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT",
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
    {"VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT",
            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
    {"VK_PIPELINE_STAGE_VERTEX_INPUT_BIT",
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {"VK_PIPELINE_STAGE_VERTEX_SHADER_BIT",
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT",
            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT",
            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT},
    {"VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT",
            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
    {"VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT",
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
    {"VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT",
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
    {"VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT",
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
    {"VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT",
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    {"VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT",
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    {"VK_PIPELINE_STAGE_TRANSFER_BIT",
            VK_PIPELINE_STAGE_TRANSFER_BIT},
    {"VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT",
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
    {"VK_PIPELINE_STAGE_HOST_BIT",
            VK_PIPELINE_STAGE_HOST_BIT},
    {"VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT",
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT},
    {"VK_PIPELINE_STAGE_ALL_COMMANDS_BIT",
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
    {"VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT",
            VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT},
    {"VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT",
            VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT},
    {"VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT",
            VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT},
    {"VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV",
            VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV},
    {"VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV},
    {"VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV",
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV},
    {"VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV},
    {"VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV",
            VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV},
};

template <> inline VkPipelineStageFlagBits get_enum_val<VkPipelineStageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_pipeline_stage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkFormat> vk_format_from_str = {
    {"VK_FORMAT_UNDEFINED", VK_FORMAT_UNDEFINED},
    {"VK_FORMAT_R4G4_UNORM_PACK8", VK_FORMAT_R4G4_UNORM_PACK8},
    {"VK_FORMAT_R4G4B4A4_UNORM_PACK16", VK_FORMAT_R4G4B4A4_UNORM_PACK16},
    {"VK_FORMAT_B4G4R4A4_UNORM_PACK16", VK_FORMAT_B4G4R4A4_UNORM_PACK16},
    {"VK_FORMAT_R5G6B5_UNORM_PACK16", VK_FORMAT_R5G6B5_UNORM_PACK16},
    {"VK_FORMAT_B5G6R5_UNORM_PACK16", VK_FORMAT_B5G6R5_UNORM_PACK16},
    {"VK_FORMAT_R5G5B5A1_UNORM_PACK16", VK_FORMAT_R5G5B5A1_UNORM_PACK16},
    {"VK_FORMAT_B5G5R5A1_UNORM_PACK16", VK_FORMAT_B5G5R5A1_UNORM_PACK16},
    {"VK_FORMAT_A1R5G5B5_UNORM_PACK16", VK_FORMAT_A1R5G5B5_UNORM_PACK16},
    {"VK_FORMAT_R8_UNORM", VK_FORMAT_R8_UNORM},
    {"VK_FORMAT_R8_SNORM", VK_FORMAT_R8_SNORM},
    {"VK_FORMAT_R8_USCALED", VK_FORMAT_R8_USCALED},
    {"VK_FORMAT_R8_SSCALED", VK_FORMAT_R8_SSCALED},
    {"VK_FORMAT_R8_UINT", VK_FORMAT_R8_UINT},
    {"VK_FORMAT_R8_SINT", VK_FORMAT_R8_SINT},
    {"VK_FORMAT_R8_SRGB", VK_FORMAT_R8_SRGB},
    {"VK_FORMAT_R8G8_UNORM", VK_FORMAT_R8G8_UNORM},
    {"VK_FORMAT_R8G8_SNORM", VK_FORMAT_R8G8_SNORM},
    {"VK_FORMAT_R8G8_USCALED", VK_FORMAT_R8G8_USCALED},
    {"VK_FORMAT_R8G8_SSCALED", VK_FORMAT_R8G8_SSCALED},
    {"VK_FORMAT_R8G8_UINT", VK_FORMAT_R8G8_UINT},
    {"VK_FORMAT_R8G8_SINT", VK_FORMAT_R8G8_SINT},
    {"VK_FORMAT_R8G8_SRGB", VK_FORMAT_R8G8_SRGB},
    {"VK_FORMAT_R8G8B8_UNORM", VK_FORMAT_R8G8B8_UNORM},
    {"VK_FORMAT_R8G8B8_SNORM", VK_FORMAT_R8G8B8_SNORM},
    {"VK_FORMAT_R8G8B8_USCALED", VK_FORMAT_R8G8B8_USCALED},
    {"VK_FORMAT_R8G8B8_SSCALED", VK_FORMAT_R8G8B8_SSCALED},
    {"VK_FORMAT_R8G8B8_UINT", VK_FORMAT_R8G8B8_UINT},
    {"VK_FORMAT_R8G8B8_SINT", VK_FORMAT_R8G8B8_SINT},
    {"VK_FORMAT_R8G8B8_SRGB", VK_FORMAT_R8G8B8_SRGB},
    {"VK_FORMAT_B8G8R8_UNORM", VK_FORMAT_B8G8R8_UNORM},
    {"VK_FORMAT_B8G8R8_SNORM", VK_FORMAT_B8G8R8_SNORM},
    {"VK_FORMAT_B8G8R8_USCALED", VK_FORMAT_B8G8R8_USCALED},
    {"VK_FORMAT_B8G8R8_SSCALED", VK_FORMAT_B8G8R8_SSCALED},
    {"VK_FORMAT_B8G8R8_UINT", VK_FORMAT_B8G8R8_UINT},
    {"VK_FORMAT_B8G8R8_SINT", VK_FORMAT_B8G8R8_SINT},
    {"VK_FORMAT_B8G8R8_SRGB", VK_FORMAT_B8G8R8_SRGB},
    {"VK_FORMAT_R8G8B8A8_UNORM", VK_FORMAT_R8G8B8A8_UNORM},
    {"VK_FORMAT_R8G8B8A8_SNORM", VK_FORMAT_R8G8B8A8_SNORM},
    {"VK_FORMAT_R8G8B8A8_USCALED", VK_FORMAT_R8G8B8A8_USCALED},
    {"VK_FORMAT_R8G8B8A8_SSCALED", VK_FORMAT_R8G8B8A8_SSCALED},
    {"VK_FORMAT_R8G8B8A8_UINT", VK_FORMAT_R8G8B8A8_UINT},
    {"VK_FORMAT_R8G8B8A8_SINT", VK_FORMAT_R8G8B8A8_SINT},
    {"VK_FORMAT_R8G8B8A8_SRGB", VK_FORMAT_R8G8B8A8_SRGB},
    {"VK_FORMAT_B8G8R8A8_UNORM", VK_FORMAT_B8G8R8A8_UNORM},
    {"VK_FORMAT_B8G8R8A8_SNORM", VK_FORMAT_B8G8R8A8_SNORM},
    {"VK_FORMAT_B8G8R8A8_USCALED", VK_FORMAT_B8G8R8A8_USCALED},
    {"VK_FORMAT_B8G8R8A8_SSCALED", VK_FORMAT_B8G8R8A8_SSCALED},
    {"VK_FORMAT_B8G8R8A8_UINT", VK_FORMAT_B8G8R8A8_UINT},
    {"VK_FORMAT_B8G8R8A8_SINT", VK_FORMAT_B8G8R8A8_SINT},
    {"VK_FORMAT_B8G8R8A8_SRGB", VK_FORMAT_B8G8R8A8_SRGB},
    {"VK_FORMAT_A8B8G8R8_UNORM_PACK32", VK_FORMAT_A8B8G8R8_UNORM_PACK32},
    {"VK_FORMAT_A8B8G8R8_SNORM_PACK32", VK_FORMAT_A8B8G8R8_SNORM_PACK32},
    {"VK_FORMAT_A8B8G8R8_USCALED_PACK32", VK_FORMAT_A8B8G8R8_USCALED_PACK32},
    {"VK_FORMAT_A8B8G8R8_SSCALED_PACK32", VK_FORMAT_A8B8G8R8_SSCALED_PACK32},
    {"VK_FORMAT_A8B8G8R8_UINT_PACK32", VK_FORMAT_A8B8G8R8_UINT_PACK32},
    {"VK_FORMAT_A8B8G8R8_SINT_PACK32", VK_FORMAT_A8B8G8R8_SINT_PACK32},
    {"VK_FORMAT_A8B8G8R8_SRGB_PACK32", VK_FORMAT_A8B8G8R8_SRGB_PACK32},
    {"VK_FORMAT_A2R10G10B10_UNORM_PACK32", VK_FORMAT_A2R10G10B10_UNORM_PACK32},
    {"VK_FORMAT_A2R10G10B10_SNORM_PACK32", VK_FORMAT_A2R10G10B10_SNORM_PACK32},
    {"VK_FORMAT_A2R10G10B10_USCALED_PACK32", VK_FORMAT_A2R10G10B10_USCALED_PACK32},
    {"VK_FORMAT_A2R10G10B10_SSCALED_PACK32", VK_FORMAT_A2R10G10B10_SSCALED_PACK32},
    {"VK_FORMAT_A2R10G10B10_UINT_PACK32", VK_FORMAT_A2R10G10B10_UINT_PACK32},
    {"VK_FORMAT_A2R10G10B10_SINT_PACK32", VK_FORMAT_A2R10G10B10_SINT_PACK32},
    {"VK_FORMAT_A2B10G10R10_UNORM_PACK32", VK_FORMAT_A2B10G10R10_UNORM_PACK32},
    {"VK_FORMAT_A2B10G10R10_SNORM_PACK32", VK_FORMAT_A2B10G10R10_SNORM_PACK32},
    {"VK_FORMAT_A2B10G10R10_USCALED_PACK32", VK_FORMAT_A2B10G10R10_USCALED_PACK32},
    {"VK_FORMAT_A2B10G10R10_SSCALED_PACK32", VK_FORMAT_A2B10G10R10_SSCALED_PACK32},
    {"VK_FORMAT_A2B10G10R10_UINT_PACK32", VK_FORMAT_A2B10G10R10_UINT_PACK32},
    {"VK_FORMAT_A2B10G10R10_SINT_PACK32", VK_FORMAT_A2B10G10R10_SINT_PACK32},
    {"VK_FORMAT_R16_UNORM", VK_FORMAT_R16_UNORM},
    {"VK_FORMAT_R16_SNORM", VK_FORMAT_R16_SNORM},
    {"VK_FORMAT_R16_USCALED", VK_FORMAT_R16_USCALED},
    {"VK_FORMAT_R16_SSCALED", VK_FORMAT_R16_SSCALED},
    {"VK_FORMAT_R16_UINT", VK_FORMAT_R16_UINT},
    {"VK_FORMAT_R16_SINT", VK_FORMAT_R16_SINT},
    {"VK_FORMAT_R16_SFLOAT", VK_FORMAT_R16_SFLOAT},
    {"VK_FORMAT_R16G16_UNORM", VK_FORMAT_R16G16_UNORM},
    {"VK_FORMAT_R16G16_SNORM", VK_FORMAT_R16G16_SNORM},
    {"VK_FORMAT_R16G16_USCALED", VK_FORMAT_R16G16_USCALED},
    {"VK_FORMAT_R16G16_SSCALED", VK_FORMAT_R16G16_SSCALED},
    {"VK_FORMAT_R16G16_UINT", VK_FORMAT_R16G16_UINT},
    {"VK_FORMAT_R16G16_SINT", VK_FORMAT_R16G16_SINT},
    {"VK_FORMAT_R16G16_SFLOAT", VK_FORMAT_R16G16_SFLOAT},
    {"VK_FORMAT_R16G16B16_UNORM", VK_FORMAT_R16G16B16_UNORM},
    {"VK_FORMAT_R16G16B16_SNORM", VK_FORMAT_R16G16B16_SNORM},
    {"VK_FORMAT_R16G16B16_USCALED", VK_FORMAT_R16G16B16_USCALED},
    {"VK_FORMAT_R16G16B16_SSCALED", VK_FORMAT_R16G16B16_SSCALED},
    {"VK_FORMAT_R16G16B16_UINT", VK_FORMAT_R16G16B16_UINT},
    {"VK_FORMAT_R16G16B16_SINT", VK_FORMAT_R16G16B16_SINT},
    {"VK_FORMAT_R16G16B16_SFLOAT", VK_FORMAT_R16G16B16_SFLOAT},
    {"VK_FORMAT_R16G16B16A16_UNORM", VK_FORMAT_R16G16B16A16_UNORM},
    {"VK_FORMAT_R16G16B16A16_SNORM", VK_FORMAT_R16G16B16A16_SNORM},
    {"VK_FORMAT_R16G16B16A16_USCALED", VK_FORMAT_R16G16B16A16_USCALED},
    {"VK_FORMAT_R16G16B16A16_SSCALED", VK_FORMAT_R16G16B16A16_SSCALED},
    {"VK_FORMAT_R16G16B16A16_UINT", VK_FORMAT_R16G16B16A16_UINT},
    {"VK_FORMAT_R16G16B16A16_SINT", VK_FORMAT_R16G16B16A16_SINT},
    {"VK_FORMAT_R16G16B16A16_SFLOAT", VK_FORMAT_R16G16B16A16_SFLOAT},
    {"VK_FORMAT_R32_UINT", VK_FORMAT_R32_UINT},
    {"VK_FORMAT_R32_SINT", VK_FORMAT_R32_SINT},
    {"VK_FORMAT_R32_SFLOAT", VK_FORMAT_R32_SFLOAT},
    {"VK_FORMAT_R32G32_UINT", VK_FORMAT_R32G32_UINT},
    {"VK_FORMAT_R32G32_SINT", VK_FORMAT_R32G32_SINT},
    {"VK_FORMAT_R32G32_SFLOAT", VK_FORMAT_R32G32_SFLOAT},
    {"VK_FORMAT_R32G32B32_UINT", VK_FORMAT_R32G32B32_UINT},
    {"VK_FORMAT_R32G32B32_SINT", VK_FORMAT_R32G32B32_SINT},
    {"VK_FORMAT_R32G32B32_SFLOAT", VK_FORMAT_R32G32B32_SFLOAT},
    {"VK_FORMAT_R32G32B32A32_UINT", VK_FORMAT_R32G32B32A32_UINT},
    {"VK_FORMAT_R32G32B32A32_SINT", VK_FORMAT_R32G32B32A32_SINT},
    {"VK_FORMAT_R32G32B32A32_SFLOAT", VK_FORMAT_R32G32B32A32_SFLOAT},
    {"VK_FORMAT_R64_UINT", VK_FORMAT_R64_UINT},
    {"VK_FORMAT_R64_SINT", VK_FORMAT_R64_SINT},
    {"VK_FORMAT_R64_SFLOAT", VK_FORMAT_R64_SFLOAT},
    {"VK_FORMAT_R64G64_UINT", VK_FORMAT_R64G64_UINT},
    {"VK_FORMAT_R64G64_SINT", VK_FORMAT_R64G64_SINT},
    {"VK_FORMAT_R64G64_SFLOAT", VK_FORMAT_R64G64_SFLOAT},
    {"VK_FORMAT_R64G64B64_UINT", VK_FORMAT_R64G64B64_UINT},
    {"VK_FORMAT_R64G64B64_SINT", VK_FORMAT_R64G64B64_SINT},
    {"VK_FORMAT_R64G64B64_SFLOAT", VK_FORMAT_R64G64B64_SFLOAT},
    {"VK_FORMAT_R64G64B64A64_UINT", VK_FORMAT_R64G64B64A64_UINT},
    {"VK_FORMAT_R64G64B64A64_SINT", VK_FORMAT_R64G64B64A64_SINT},
    {"VK_FORMAT_R64G64B64A64_SFLOAT", VK_FORMAT_R64G64B64A64_SFLOAT},
    {"VK_FORMAT_B10G11R11_UFLOAT_PACK32", VK_FORMAT_B10G11R11_UFLOAT_PACK32},
    {"VK_FORMAT_E5B9G9R9_UFLOAT_PACK32", VK_FORMAT_E5B9G9R9_UFLOAT_PACK32},
    {"VK_FORMAT_D16_UNORM", VK_FORMAT_D16_UNORM},
    {"VK_FORMAT_X8_D24_UNORM_PACK32", VK_FORMAT_X8_D24_UNORM_PACK32},
    {"VK_FORMAT_D32_SFLOAT", VK_FORMAT_D32_SFLOAT},
    {"VK_FORMAT_S8_UINT", VK_FORMAT_S8_UINT},
    {"VK_FORMAT_D16_UNORM_S8_UINT", VK_FORMAT_D16_UNORM_S8_UINT},
    {"VK_FORMAT_D24_UNORM_S8_UINT", VK_FORMAT_D24_UNORM_S8_UINT},
    {"VK_FORMAT_D32_SFLOAT_S8_UINT", VK_FORMAT_D32_SFLOAT_S8_UINT},
    {"VK_FORMAT_BC1_RGB_UNORM_BLOCK", VK_FORMAT_BC1_RGB_UNORM_BLOCK},
    {"VK_FORMAT_BC1_RGB_SRGB_BLOCK", VK_FORMAT_BC1_RGB_SRGB_BLOCK},
    {"VK_FORMAT_BC1_RGBA_UNORM_BLOCK", VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {"VK_FORMAT_BC1_RGBA_SRGB_BLOCK", VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
    {"VK_FORMAT_BC2_UNORM_BLOCK", VK_FORMAT_BC2_UNORM_BLOCK},
    {"VK_FORMAT_BC2_SRGB_BLOCK", VK_FORMAT_BC2_SRGB_BLOCK},
    {"VK_FORMAT_BC3_UNORM_BLOCK", VK_FORMAT_BC3_UNORM_BLOCK},
    {"VK_FORMAT_BC3_SRGB_BLOCK", VK_FORMAT_BC3_SRGB_BLOCK},
    {"VK_FORMAT_BC4_UNORM_BLOCK", VK_FORMAT_BC4_UNORM_BLOCK},
    {"VK_FORMAT_BC4_SNORM_BLOCK", VK_FORMAT_BC4_SNORM_BLOCK},
    {"VK_FORMAT_BC5_UNORM_BLOCK", VK_FORMAT_BC5_UNORM_BLOCK},
    {"VK_FORMAT_BC5_SNORM_BLOCK", VK_FORMAT_BC5_SNORM_BLOCK},
    {"VK_FORMAT_BC6H_UFLOAT_BLOCK", VK_FORMAT_BC6H_UFLOAT_BLOCK},
    {"VK_FORMAT_BC6H_SFLOAT_BLOCK", VK_FORMAT_BC6H_SFLOAT_BLOCK},
    {"VK_FORMAT_BC7_UNORM_BLOCK", VK_FORMAT_BC7_UNORM_BLOCK},
    {"VK_FORMAT_BC7_SRGB_BLOCK", VK_FORMAT_BC7_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK},
    {"VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK", VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK},
    {"VK_FORMAT_EAC_R11_UNORM_BLOCK", VK_FORMAT_EAC_R11_UNORM_BLOCK},
    {"VK_FORMAT_EAC_R11_SNORM_BLOCK", VK_FORMAT_EAC_R11_SNORM_BLOCK},
    {"VK_FORMAT_EAC_R11G11_UNORM_BLOCK", VK_FORMAT_EAC_R11G11_UNORM_BLOCK},
    {"VK_FORMAT_EAC_R11G11_SNORM_BLOCK", VK_FORMAT_EAC_R11G11_SNORM_BLOCK},
    {"VK_FORMAT_ASTC_4x4_UNORM_BLOCK", VK_FORMAT_ASTC_4x4_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_4x4_SRGB_BLOCK", VK_FORMAT_ASTC_4x4_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_5x4_UNORM_BLOCK", VK_FORMAT_ASTC_5x4_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_5x4_SRGB_BLOCK", VK_FORMAT_ASTC_5x4_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_5x5_UNORM_BLOCK", VK_FORMAT_ASTC_5x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_5x5_SRGB_BLOCK", VK_FORMAT_ASTC_5x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_6x5_UNORM_BLOCK", VK_FORMAT_ASTC_6x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_6x5_SRGB_BLOCK", VK_FORMAT_ASTC_6x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_6x6_UNORM_BLOCK", VK_FORMAT_ASTC_6x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_6x6_SRGB_BLOCK", VK_FORMAT_ASTC_6x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x5_UNORM_BLOCK", VK_FORMAT_ASTC_8x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x5_SRGB_BLOCK", VK_FORMAT_ASTC_8x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x6_UNORM_BLOCK", VK_FORMAT_ASTC_8x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x6_SRGB_BLOCK", VK_FORMAT_ASTC_8x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_8x8_UNORM_BLOCK", VK_FORMAT_ASTC_8x8_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_8x8_SRGB_BLOCK", VK_FORMAT_ASTC_8x8_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x5_UNORM_BLOCK", VK_FORMAT_ASTC_10x5_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x5_SRGB_BLOCK", VK_FORMAT_ASTC_10x5_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x6_UNORM_BLOCK", VK_FORMAT_ASTC_10x6_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x6_SRGB_BLOCK", VK_FORMAT_ASTC_10x6_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x8_UNORM_BLOCK", VK_FORMAT_ASTC_10x8_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x8_SRGB_BLOCK", VK_FORMAT_ASTC_10x8_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_10x10_UNORM_BLOCK", VK_FORMAT_ASTC_10x10_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_10x10_SRGB_BLOCK", VK_FORMAT_ASTC_10x10_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_12x10_UNORM_BLOCK", VK_FORMAT_ASTC_12x10_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_12x10_SRGB_BLOCK", VK_FORMAT_ASTC_12x10_SRGB_BLOCK},
    {"VK_FORMAT_ASTC_12x12_UNORM_BLOCK", VK_FORMAT_ASTC_12x12_UNORM_BLOCK},
    {"VK_FORMAT_ASTC_12x12_SRGB_BLOCK", VK_FORMAT_ASTC_12x12_SRGB_BLOCK},
};

template <> inline VkFormat get_enum_val<VkFormat>(fkyaml::node &n) {
    return get_enum_val(n, vk_format_from_str);
}

inline std::unordered_map<std::string, VkVertexInputRate> vk_vertex_input_rate_from_str = {
    {"VK_VERTEX_INPUT_RATE_VERTEX", VK_VERTEX_INPUT_RATE_VERTEX},
    {"VK_VERTEX_INPUT_RATE_INSTANCE", VK_VERTEX_INPUT_RATE_INSTANCE},
};

template <> inline VkVertexInputRate get_enum_val<VkVertexInputRate>(fkyaml::node &n) {
    return get_enum_val(n, vk_vertex_input_rate_from_str);
}

inline std::unordered_map<std::string, VkShaderStageFlagBits> vk_shader_stage_flag_bits_from_str = {
    {"VK_SHADER_STAGE_VERTEX_BIT", VK_SHADER_STAGE_VERTEX_BIT},
    {"VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT", VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
    {"VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT", VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
    {"VK_SHADER_STAGE_GEOMETRY_BIT", VK_SHADER_STAGE_GEOMETRY_BIT},
    {"VK_SHADER_STAGE_FRAGMENT_BIT", VK_SHADER_STAGE_FRAGMENT_BIT},
    {"VK_SHADER_STAGE_COMPUTE_BIT", VK_SHADER_STAGE_COMPUTE_BIT},
    {"VK_SHADER_STAGE_ALL_GRAPHICS", VK_SHADER_STAGE_ALL_GRAPHICS},
    {"VK_SHADER_STAGE_ALL", VK_SHADER_STAGE_ALL},
    {"VK_SHADER_STAGE_RAYGEN_BIT_NV", VK_SHADER_STAGE_RAYGEN_BIT_NV},
    {"VK_SHADER_STAGE_ANY_HIT_BIT_NV", VK_SHADER_STAGE_ANY_HIT_BIT_NV},
    {"VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV", VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV},
    {"VK_SHADER_STAGE_MISS_BIT_NV", VK_SHADER_STAGE_MISS_BIT_NV},
    {"VK_SHADER_STAGE_INTERSECTION_BIT_NV", VK_SHADER_STAGE_INTERSECTION_BIT_NV},
    {"VK_SHADER_STAGE_CALLABLE_BIT_NV", VK_SHADER_STAGE_CALLABLE_BIT_NV},
    {"VK_SHADER_STAGE_TASK_BIT_NV", VK_SHADER_STAGE_TASK_BIT_NV},
    {"VK_SHADER_STAGE_MESH_BIT_NV", VK_SHADER_STAGE_MESH_BIT_NV},
};

template <> inline VkShaderStageFlagBits get_enum_val<VkShaderStageFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_shader_stage_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkDescriptorType> vk_descriptor_type_from_str = {
    {"VK_DESCRIPTOR_TYPE_SAMPLER", VK_DESCRIPTOR_TYPE_SAMPLER}, 
    {"VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER}, 
    {"VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE", VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_IMAGE", VK_DESCRIPTOR_TYPE_STORAGE_IMAGE}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_BUFFER", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}, 
    {"VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC}, 
    {"VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC", VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC}, 
    {"VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT", VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT}, 
    {"VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV", VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV},
    {"VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT", VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT},
};

template <> inline VkDescriptorType get_enum_val<VkDescriptorType>(fkyaml::node &n) {
    return get_enum_val(n, vk_descriptor_type_from_str);
}

inline std::unordered_map<std::string, vku_shader_stage_e> shader_stage_from_string = {
    {"VKU_SPIRV_VERTEX",    VKU_SPIRV_VERTEX},
    {"VKU_SPIRV_FRAGMENT",  VKU_SPIRV_FRAGMENT},
    {"VKU_SPIRV_COMPUTE",   VKU_SPIRV_COMPUTE},
    {"VKU_SPIRV_GEOMETRY",  VKU_SPIRV_GEOMETRY},
    {"VKU_SPIRV_TESS_CTRL", VKU_SPIRV_TESS_CTRL},
    {"VKU_SPIRV_TESS_EVAL", VKU_SPIRV_TESS_EVAL},
};

template <> inline vku_shader_stage_e get_enum_val<vku_shader_stage_e>(fkyaml::node &n) {
    return get_enum_val(n, shader_stage_from_string);
}

inline std::unordered_map<std::string, VkSemaphoreType> vk_semaphore_type_from_str = {
    {"VK_SEMAPHORE_TYPE_BINARY",   VK_SEMAPHORE_TYPE_BINARY},
    {"VK_SEMAPHORE_TYPE_TIMELINE", VK_SEMAPHORE_TYPE_TIMELINE},
};

template <> inline VkSemaphoreType get_enum_val<VkSemaphoreType>(fkyaml::node &n) {
    return get_enum_val(n, vk_semaphore_type_from_str);
}

inline std::unordered_map<std::string, VkAccessFlagBits> vk_access_flag_bits_from_str = {
    {"VK_ACCESS_INDIRECT_COMMAND_READ_BIT",
            VK_ACCESS_INDIRECT_COMMAND_READ_BIT},
    {"VK_ACCESS_INDEX_READ_BIT",
            VK_ACCESS_INDEX_READ_BIT},
    {"VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT",
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT},
    {"VK_ACCESS_UNIFORM_READ_BIT",
            VK_ACCESS_UNIFORM_READ_BIT},
    {"VK_ACCESS_INPUT_ATTACHMENT_READ_BIT",
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT},
    {"VK_ACCESS_SHADER_READ_BIT",
            VK_ACCESS_SHADER_READ_BIT},
    {"VK_ACCESS_SHADER_WRITE_BIT",
            VK_ACCESS_SHADER_WRITE_BIT},
    {"VK_ACCESS_COLOR_ATTACHMENT_READ_BIT",
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT},
    {"VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT",
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT},
    {"VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT",
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
    {"VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT",
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
    {"VK_ACCESS_TRANSFER_READ_BIT",
            VK_ACCESS_TRANSFER_READ_BIT},
    {"VK_ACCESS_TRANSFER_WRITE_BIT",
            VK_ACCESS_TRANSFER_WRITE_BIT},
    {"VK_ACCESS_HOST_READ_BIT",
            VK_ACCESS_HOST_READ_BIT},
    {"VK_ACCESS_HOST_WRITE_BIT",
            VK_ACCESS_HOST_WRITE_BIT},
    {"VK_ACCESS_MEMORY_READ_BIT",
            VK_ACCESS_MEMORY_READ_BIT},
    {"VK_ACCESS_MEMORY_WRITE_BIT",
            VK_ACCESS_MEMORY_WRITE_BIT},
    // {"VK_ACCESS_NONE",
    //         VK_ACCESS_NONE},
    {"VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT",
            VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT},
    {"VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT",
            VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT},
    {"VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT",
            VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT},
    {"VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT",
            VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT},
    {"VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT",
            VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT},
    // {"VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR",
    //         VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR},
    // {"VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR",
    //         VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
    {"VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT",
            VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT},
    // {"VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR",
    //         VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR},
    // {"VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_EXT",
    //         VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_EXT},
    // {"VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_EXT",
    //         VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_EXT},
    {"VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV",
            VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV},
    {"VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV",
            VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV},
    {"VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV",
            VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV},
    // {"VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV",
    //         VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV},
    // {"VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV",
    //         VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV},
};

template <> inline VkAccessFlagBits get_enum_val<VkAccessFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_access_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkDependencyFlagBits> vk_dependency_flag_bits_from_str = {
    {"VK_DEPENDENCY_BY_REGION_BIT",
            VK_DEPENDENCY_BY_REGION_BIT},
    {"VK_DEPENDENCY_DEVICE_GROUP_BIT",
            VK_DEPENDENCY_DEVICE_GROUP_BIT},
    {"VK_DEPENDENCY_VIEW_LOCAL_BIT",
            VK_DEPENDENCY_VIEW_LOCAL_BIT},
    // {"VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT",
    //         VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT},
    // {"VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR",
    //         VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR},
    // {"VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR",
    //         VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR},
    {"VK_DEPENDENCY_VIEW_LOCAL_BIT_KHR",
            VK_DEPENDENCY_VIEW_LOCAL_BIT_KHR},
    // {"VK_DEPENDENCY_DEVICE_GROUP_BIT_KHR",
    //         VK_DEPENDENCY_DEVICE_GROUP_BIT_KHR},
};

template <> inline VkDependencyFlagBits get_enum_val<VkDependencyFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_dependency_flag_bits_from_str);
}

inline std::unordered_map<std::string, VkFenceCreateFlagBits> vk_fence_create_flag_bits_from_str = {
    {"VK_FENCE_CREATE_SIGNALED_BIT", VK_FENCE_CREATE_SIGNALED_BIT}
};

template <> inline VkFenceCreateFlagBits get_enum_val<VkFenceCreateFlagBits>(fkyaml::node &n) {
    return get_enum_val(n, vk_fence_create_flag_bits_from_str);
}

/* TODO: this needs to be implemented in a newer version of vulkan, tested and as such */
// inline std::unordered_map<std::string, VkPipelineStageFlagBits2>
//         vk_pipeline_stage_flag_bits2_from_str =
// {
//     {"VK_PIPELINE_STAGE_2_NONE",
//             VK_PIPELINE_STAGE_2_NONE},
//     {"VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT",
//             VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT},
//     {"VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT",
//             VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT},
//     {"VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT",
//             VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT},
//     {"VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT",
//             VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT},
//     {"VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT",
//             VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT},
//     {"VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT",
//             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT},
//     {"VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT",
//             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT},
//     {"VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT",
//             VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT},
//     {"VK_PIPELINE_STAGE_2_TRANSFER_BIT",
//             VK_PIPELINE_STAGE_2_TRANSFER_BIT},
//     {"VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT",
//             VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT},
//     {"VK_PIPELINE_STAGE_2_HOST_BIT",
//             VK_PIPELINE_STAGE_2_HOST_BIT},
//     {"VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT",
//             VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT},
//     {"VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT",
//             VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT},
//     {"VK_PIPELINE_STAGE_2_COPY_BIT",
//             VK_PIPELINE_STAGE_2_COPY_BIT},
//     {"VK_PIPELINE_STAGE_2_RESOLVE_BIT",
//             VK_PIPELINE_STAGE_2_RESOLVE_BIT},
//     {"VK_PIPELINE_STAGE_2_BLIT_BIT",
//             VK_PIPELINE_STAGE_2_BLIT_BIT},
//     {"VK_PIPELINE_STAGE_2_CLEAR_BIT",
//             VK_PIPELINE_STAGE_2_CLEAR_BIT},
//     {"VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT",
//             VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT},
//     {"VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT",
//             VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT},
//     {"VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT",
//             VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT},
//     {"VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR",
//             VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR",
//             VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_NONE_KHR",
//             VK_PIPELINE_STAGE_2_NONE_KHR},
//     {"VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR",
//             VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR",
//             VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR",
//             VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR",
//             VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_HOST_BIT_KHR",
//             VK_PIPELINE_STAGE_2_HOST_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR",
//             VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR",
//             VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_COPY_BIT_KHR",
//             VK_PIPELINE_STAGE_2_COPY_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_RESOLVE_BIT_KHR",
//             VK_PIPELINE_STAGE_2_RESOLVE_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_BLIT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_BLIT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_CLEAR_BIT_KHR",
//             VK_PIPELINE_STAGE_2_CLEAR_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT_KHR",
//             VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT",
//             VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT",
//             VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV",
//             VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_EXT",
//             VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_SHADING_RATE_IMAGE_BIT_NV",
//             VK_PIPELINE_STAGE_2_SHADING_RATE_IMAGE_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR",
//             VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR",
//             VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_NV",
//             VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_NV",
//             VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT",
//             VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV",
//             VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV",
//             VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT",
//             VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT",
//             VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI",
//             VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI},
//     {"VK_PIPELINE_STAGE_2_SUBPASS_SHADING_BIT_HUAWEI",
//             VK_PIPELINE_STAGE_2_SUBPASS_SHADING_BIT_HUAWEI},
//     {"VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI",
//             VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI},
//     {"VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR",
//             VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT",
//             VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT},
//     {"VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI",
//             VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI},
//     {"VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV",
//             VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_CONVERT_COOPERATIVE_VECTOR_MATRIX_BIT_NV",
//             VK_PIPELINE_STAGE_2_CONVERT_COOPERATIVE_VECTOR_MATRIX_BIT_NV},
//     {"VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM",
//             VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM},
//     {"VK_PIPELINE_STAGE_2_COPY_INDIRECT_BIT_KHR",
//             VK_PIPELINE_STAGE_2_COPY_INDIRECT_BIT_KHR},
//     {"VK_PIPELINE_STAGE_2_MEMORY_DECOMPRESSION_BIT_EXT",
//             VK_PIPELINE_STAGE_2_MEMORY_DECOMPRESSION_BIT_EXT},
// };
// template <> inline VkPipelineStageFlagBits2 get_enum_val<VkPipelineStageFlagBits2>(fkyaml::node &n) {
//     return get_enum_val(n, vk_pipeline_stage_flag_bits2_from_str);
// }
// inline std::unordered_map<std::string, VkAccessFlagBits2> vk_access_flag_bits2_from_str = {
//     {"VK_ACCESS_2_NONE",
//             VK_ACCESS_2_NONE},
//     {"VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT",
//             VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT},
//     {"VK_ACCESS_2_INDEX_READ_BIT",
//             VK_ACCESS_2_INDEX_READ_BIT},
//     {"VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT",
//             VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT},
//     {"VK_ACCESS_2_UNIFORM_READ_BIT",
//             VK_ACCESS_2_UNIFORM_READ_BIT},
//     {"VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT",
//             VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT},
//     {"VK_ACCESS_2_SHADER_READ_BIT",
//             VK_ACCESS_2_SHADER_READ_BIT},
//     {"VK_ACCESS_2_SHADER_WRITE_BIT",
//             VK_ACCESS_2_SHADER_WRITE_BIT},
//     {"VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT",
//             VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT},
//     {"VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT",
//             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT},
//     {"VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT",
//             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT},
//     {"VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT",
//             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
//     {"VK_ACCESS_2_TRANSFER_READ_BIT",
//             VK_ACCESS_2_TRANSFER_READ_BIT},
//     {"VK_ACCESS_2_TRANSFER_WRITE_BIT",
//             VK_ACCESS_2_TRANSFER_WRITE_BIT},
//     {"VK_ACCESS_2_HOST_READ_BIT",
//             VK_ACCESS_2_HOST_READ_BIT},
//     {"VK_ACCESS_2_HOST_WRITE_BIT",
//             VK_ACCESS_2_HOST_WRITE_BIT},
//     {"VK_ACCESS_2_MEMORY_READ_BIT",
//             VK_ACCESS_2_MEMORY_READ_BIT},
//     {"VK_ACCESS_2_MEMORY_WRITE_BIT",
//             VK_ACCESS_2_MEMORY_WRITE_BIT},
//     {"VK_ACCESS_2_SHADER_SAMPLED_READ_BIT",
//             VK_ACCESS_2_SHADER_SAMPLED_READ_BIT},
//     {"VK_ACCESS_2_SHADER_STORAGE_READ_BIT",
//             VK_ACCESS_2_SHADER_STORAGE_READ_BIT},
//     {"VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT",
//             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT},
//     {"VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR",
//             VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR},
//     {"VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR",
//             VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_SAMPLER_HEAP_READ_BIT_EXT",
//             VK_ACCESS_2_SAMPLER_HEAP_READ_BIT_EXT},
//     {"VK_ACCESS_2_RESOURCE_HEAP_READ_BIT_EXT",
//             VK_ACCESS_2_RESOURCE_HEAP_READ_BIT_EXT},
//     {"VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR",
//             VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR},
//     {"VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR",
//             VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_TILE_ATTACHMENT_READ_BIT_QCOM",
//             VK_ACCESS_2_SHADER_TILE_ATTACHMENT_READ_BIT_QCOM},
//     {"VK_ACCESS_2_SHADER_TILE_ATTACHMENT_WRITE_BIT_QCOM",
//             VK_ACCESS_2_SHADER_TILE_ATTACHMENT_WRITE_BIT_QCOM},
//     {"VK_ACCESS_2_NONE_KHR",
//             VK_ACCESS_2_NONE_KHR},
//     {"VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR",
//             VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR},
//     {"VK_ACCESS_2_INDEX_READ_BIT_KHR",
//             VK_ACCESS_2_INDEX_READ_BIT_KHR},
//     {"VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR",
//             VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR},
//     {"VK_ACCESS_2_UNIFORM_READ_BIT_KHR",
//             VK_ACCESS_2_UNIFORM_READ_BIT_KHR},
//     {"VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT_KHR",
//             VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_READ_BIT_KHR",
//             VK_ACCESS_2_SHADER_READ_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_WRITE_BIT_KHR",
//             VK_ACCESS_2_SHADER_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR",
//             VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR},
//     {"VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR",
//             VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR",
//             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR},
//     {"VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR",
//             VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_TRANSFER_READ_BIT_KHR",
//             VK_ACCESS_2_TRANSFER_READ_BIT_KHR},
//     {"VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR",
//             VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_HOST_READ_BIT_KHR",
//             VK_ACCESS_2_HOST_READ_BIT_KHR},
//     {"VK_ACCESS_2_HOST_WRITE_BIT_KHR",
//             VK_ACCESS_2_HOST_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_MEMORY_READ_BIT_KHR",
//             VK_ACCESS_2_MEMORY_READ_BIT_KHR},
//     {"VK_ACCESS_2_MEMORY_WRITE_BIT_KHR",
//             VK_ACCESS_2_MEMORY_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR",
//             VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR",
//             VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR},
//     {"VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR",
//             VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT",
//             VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT},
//     {"VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT",
//             VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT},
//     {"VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT",
//             VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT},
//     {"VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT",
//             VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT},
//     {"VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV",
//             VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV},
//     {"VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV",
//             VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV},
//     {"VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_EXT",
//             VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_EXT},
//     {"VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_EXT",
//             VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_EXT},
//     {"VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR",
//             VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR},
//     {"VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV",
//             VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV},
//     {"VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR",
//             VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR},
//     {"VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR",
//             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR},
//     {"VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_NV",
//             VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_NV},
//     {"VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_NV",
//             VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_NV},
//     {"VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT",
//             VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT},
//     {"VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT",
//             VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT},
//     {"VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT",
//             VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT},
//     {"VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI",
//             VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI},
//     {"VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR",
//             VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR},
//     {"VK_ACCESS_2_MICROMAP_READ_BIT_EXT",
//             VK_ACCESS_2_MICROMAP_READ_BIT_EXT},
//     {"VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT",
//             VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT},
//     {"VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV",
//             VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV},
//     {"VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV",
//             VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV},
//     {"VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM",
//             VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM},
//     {"VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM",
//             VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM},
//     {"VK_ACCESS_2_MEMORY_DECOMPRESSION_READ_BIT_EXT",
//             VK_ACCESS_2_MEMORY_DECOMPRESSION_READ_BIT_EXT},
//     {"VK_ACCESS_2_MEMORY_DECOMPRESSION_WRITE_BIT_EXT",
//             VK_ACCESS_2_MEMORY_DECOMPRESSION_WRITE_BIT_EXT},
// };
// template <> inline VkAccessFlagBits2 get_enum_val<VkAccessFlagBits2>(fkyaml::node &n) {
//     return get_enum_val(n, vk_access_flag_bits2_from_str);
// }

inline std::unordered_map<std::string, VkImageLayout> vk_image_layout_from_str = {
    {"VK_IMAGE_LAYOUT_UNDEFINED",
            VK_IMAGE_LAYOUT_UNDEFINED},
    {"VK_IMAGE_LAYOUT_GENERAL",
            VK_IMAGE_LAYOUT_GENERAL},
    {"VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL",
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    {"VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
    {"VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL},
    {"VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    {"VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL",
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL},
    {"VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL",
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL},
    {"VK_IMAGE_LAYOUT_PREINITIALIZED",
            VK_IMAGE_LAYOUT_PREINITIALIZED},
    {"VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL},
    {"VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL},
    {"VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL},
    {"VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL",
            VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL},
    {"VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL",
            VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL},
    {"VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL",
            VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL},
    // {"VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL",
    //         VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL},
    // {"VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL",
    //         VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL},
    // {"VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ",
    //         VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ},
    {"VK_IMAGE_LAYOUT_PRESENT_SRC_KHR",
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR},
    {"VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR",
            VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR},
    {"VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT",
            VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT},
    // {"VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR",
    //         VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR},
    // {"VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR},
    // {"VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT",
    //         VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT},
    // {"VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM",
    //         VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM},
    // {"VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR",
    //         VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR},
    // {"VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT",
    //         VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT},
    {"VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR},
    {"VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR},
    {"VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV",
            VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV},
    // {"VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR",
    //         VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ_KHR},
    {"VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR},
    {"VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR},
    {"VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR},
    {"VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR",
            VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR},
    // {"VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR",
    //         VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR},
    // {"VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR",
    //         VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR},
};

template <> inline VkImageLayout get_enum_val<VkImageLayout>(fkyaml::node &n) {
    return get_enum_val(n, vk_image_layout_from_str);
}

} /* virt_composer */

#endif
