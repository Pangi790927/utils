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

/* TODO: figure out what to do with this: */
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

inline constexpr const int MAX_NUMBER_OF_OBJECTS = 16384;

inline std::string app_path = std::filesystem::canonical("./");

inline std::stack<int> free_objects = [](){
    std::stack<int> ret;
    for (int i = MAX_NUMBER_OF_OBJECTS-1; i >= 1; i--)
        ret.push(i);
    return ret;
}();

/* This holds the obhects reference such that lua can use them */
struct object_ref_t {
    vku::ref_t<vku::object_t> obj;  /* The actual reference to the vku/vkc object */

    std::string name;               /* this is required such that when this object gets removed it
                                       also gets removed from objects_map */
};

inline std::vector<object_ref_t> objects(MAX_NUMBER_OF_OBJECTS);
inline std::map<std::string, int> objects_map;
inline std::map<std::string, std::vector<co::state_t *>> wanted_objects;
inline std::deque<std::coroutine_handle<void>> work;

struct lua_var_t : public vku::object_t {
    std::string name;
    static vku::ref_t<lua_var_t> create(std::string name) {
        auto ret = vku::ref_t<lua_var_t>::create_obj_ref(std::make_unique<lua_var_t>(), {});
        ret->name = name;
        return ret;
    }

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_LUA_VARIABLE; }

    inline std::string to_string() const override {
        return std::format("vkc::lua_var[{}]: m_name={} ", (void*)this, name);
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

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_LUA_SCRIPT; }

    inline std::string to_string() const override {
        return std::format("vkc::lua_script[{}]: m_content=\n{}", (void*)this, content);
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

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_INTEGER; }

    inline std::string to_string() const override {
        return std::format("vkc::integer[{}]: value={} ", (void*)this, value);
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

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_FLOAT; }

    inline std::string to_string() const override {
        return std::format("vkc::float[{}]: value={} ", (void*)this, value);
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

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_STRING; }

    inline std::string to_string() const override {
        return std::format("vkc::string[{}]: value={} ", (void*)this, value);
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

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_SPIRV; }

    inline std::string to_string() const override {
        return std::format("vkc::spirv[{}]: spirv-type={} spirv-content=\n{}", (void*)this,
                vku_utils::to_string(spirv.type),
                hexdump_str((void *)spirv.content.data(), spirv.content.size() * sizeof(uint32_t)));
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
    bool await_ready() noexcept { return has(objects_map, required_depend); }

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
        if (!has(objects_map, required_depend)) {
            DBG("Object not found");
            throw vku::err_t(std::format("Object not found, {}", required_depend));
        }
        if (!objects[objects_map[required_depend]].obj) {
            DBG("For some reason this object now holds a nullptr...");
            throw vku::err_t("nullptr object");
        }
        auto ret = objects[objects_map[required_depend]].obj.to_derived<VkuT>();
        if (!ret) {
            DBG("Invalid ref...");
            throw vku::err_t(sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    demangle<4>(typeid(objects[objects_map[required_depend]].obj.get()).name()).c_str(),
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
    if (has(objects_map, depend_name)) {
        DBG("Name taken");
        throw vku::err_t{std::format("Tag name already exists: {}", depend_name)};
    }
    int new_id = free_objects.top();
    free_objects.pop();

    DBG("Adding object: %s [%d]", depend->to_string().c_str(), new_id);
    objects_map[depend_name] = new_id;
    objects[new_id].obj = depend;
    objects[new_id].name = depend_name;

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

template <typename T>
inline T get_enum_val(fkyaml::node &node, const std::unordered_map<std::string, T>& enum_vals) {
    if (node.is_string()) {
        if (!has(enum_vals, node.as_str()))
            throw vku::err_t(std::format("Unknown enum({}) value: {}", demangle<T>(), node.as_str()));
        return enum_vals.find(node.as_str())->second;
    }
    if (node.is_sequence()) {
        uint32_t ret = 0;
        for (auto &val : node.as_seq())
            ret |= (uint32_t)get_enum_val(val, enum_vals);
        return (T)ret;
    }
    throw vku::err_t{std::format("Node({}), can't be converted to an enum of type ({})",
            fkyaml::node::serialize(node), demangle<T>())};
}

template <typename T>
inline T get_enum_val(fkyaml::node &n) {
    throw vku::err_t{std::format("Type {} not implemented", demangle<T>())};
}

inline std::unordered_map<std::string, VkBufferUsageFlagBits> vk_buffer_usage_flag_bits_from_str = {
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

inline auto get_from_map(auto &m, const std::string& str) {
    if (!has(m, str))
        throw vku::err_t(std::format("Failed to get object: {} from: {}",
                str, demangle<decltype(m), 2>()));
    return m[str];
}

inline std::string get_file_string_content(const std::string& file_path_relative) {
    std::string file_path = std::filesystem::canonical(file_path_relative);

    if (!starts_with(file_path, app_path)) {
        DBG("The path is restricted to the application main directory");
        throw vku::err_t(std::format("File_error [{} vs {}]", file_path, app_path));
    }

    std::ifstream ifs(file_path.c_str());

    if (!ifs.good()) {
        DBG("Failed to open path: %s", file_path.c_str());
        throw std::runtime_error("File_error");
    }

    return std::string((std::istreambuf_iterator<char>(ifs)),
                       (std::istreambuf_iterator<char>()));
}

inline auto load_image(auto cp, std::string path) {
    int w, h, chans;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &chans, STBI_rgb_alpha);

    /* TODO: some more logs around here */
    VkDeviceSize imag_sz = w*h*4;
    if (!pixels) {
        throw vku::err_t("Failed to load image");
    }

    auto img = vku::image_t::create(cp->dev, w, h, VK_FORMAT_R8G8B8A8_SRGB);
    img->set_data(cp, pixels, imag_sz);

    stbi_image_free(pixels);

    return img;
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
        auto instance = co_await resolve_obj<vku::instance_t>(node["m_instance"]);
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
        auto cp = co_await resolve_obj<vku::cmdpool_t>(node["m_cmdpool"]);
        auto path = co_await resolve_str(node["m_path"]);
        auto obj = load_image(cp, path);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_view_t") {
        auto img = co_await resolve_obj<vku::image_t>(node["m_image"]);
        auto aspect_mask = get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]);
        auto obj = vku::img_view_t::create(img, aspect_mask);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_sampl_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto obj = vku::img_sampl_t::create(dev);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::buffer_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        size_t sz = co_await resolve_int(node["m_size"]);
        auto usage_flags = get_enum_val<VkBufferUsageFlagBits>(node["m_usage_flags"]);
        auto share_mode = get_enum_val<VkSharingMode>(node["m_sharing_mode"]);
        auto memory_flags = get_enum_val<VkMemoryPropertyFlagBits>(node["m_memory_flags"]);
        auto obj = vku::buffer_t::create(dev, sz, usage_flags, share_mode, memory_flags);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
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
        /* TODO: add layout (see get_desc_set) */
        auto buff = co_await resolve_obj<vku::buffer_t>(node["m_buff"]);
        auto obj = vku::binding_desc_set_t::buff_binding_t::create(
                vku::ubo_t::get_desc_set(0, VK_SHADER_STAGE_VERTEX_BIT), /* TODO: resolve this */
                buff);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::sampl_binding_t") {
        /* TODO: add layout (see get_desc_set) */
        auto view = co_await resolve_obj<vku::img_view_t>(node["m_view"]);
        auto sampler = co_await resolve_obj<vku::img_sampl_t>(node["m_sampler"]);
        auto obj = vku::binding_desc_set_t::sampl_binding_t::create(
                vku::img_sampl_t::get_desc_set(1, VK_SHADER_STAGE_FRAGMENT_BIT), /* TODO: resolve this */
                view,
                sampler);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::shader_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto spirv = co_await resolve_obj<spirv_t>(node["m_spirv"]);
        auto obj = vku::shader_t::create(dev, spirv->spirv);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::swapchain_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto obj = vku::swapchain_t::create(dev);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::renderpass_t") {
        auto swc = co_await resolve_obj<vku::swapchain_t>(node["m_swapchain"]);
        auto obj = vku::renderpass_t::create(swc);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::pipeline_t") {
        auto w = co_await resolve_int(node["m_width"]);
        auto h = co_await resolve_int(node["m_height"]);
        auto rp = co_await resolve_obj<vku::renderpass_t>(node["m_renderpass"]);
        std::vector<vku::ref_t<vku::shader_t>> shaders;
        for (auto& sh : node["m_shaders"])
            shaders.push_back(co_await resolve_obj<vku::shader_t>(sh));
        auto topol = get_enum_val<VkPrimitiveTopology>(node["m_topology"]);
        auto indesc = vku::vertex3d_t::get_input_desc(); /* TODO: resolve this */
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(node["m_bindings"]);
        auto obj = vku::pipeline_t::create(w, h, rp, shaders, topol, indesc, binds);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::framebuffs_t") {
        auto rp = co_await resolve_obj<vku::renderpass_t>(node["m_renderpass"]);
        auto obj = vku::framebuffs_t::create(rp);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::sem_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto obj = vku::sem_t::create(dev);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::fence_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto obj = vku::fence_t::create(dev);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdbuff_t") {
        auto cp = co_await resolve_obj<vku::cmdpool_t>(node["m_cmdpool"]);
        auto obj = vku::cmdbuff_t::create(cp);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_pool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(node["m_device"]);
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(node["m_bindings"]);
        int cnt = co_await resolve_int(node["m_cnt"]);
        auto obj = vku::desc_pool_t::create(dev, binds, cnt);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_set_t") {
        auto descritpor_pool = co_await resolve_obj<vku::desc_pool_t>(node["m_descritpor_pool"]);
        auto pipeline = co_await resolve_obj<vku::pipeline_t>(node["m_pipeline"]);
        auto bindings = co_await resolve_obj<vku::binding_desc_set_t>(node["m_bindings"]);
        auto obj = vku::desc_set_t::create(descritpor_pool, pipeline, bindings);
        mark_dependency_solved(name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vku::err_t{std::format("Invalid object type: {}", node["m_type"].as_str())};
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

print(debug.getregistry())

function f1()
    vku.cbuff:begin()
end

function f2()
    f1()
end

function f3()
    f2()
end

function on_loop_run()
    vku.glfw_pool_events() -- needed for keys to be available
    if vku.get_key(vku.window, vku.GLFW_KEY_ESCAPE) == vku.GLFW_PRESS then
        vku.signal_close() -- this will close the loop
    end

    img_idx = vku.aquire_next_img(vku.swc, vku.img_sem)

    -- do mvp update somehow?

    f3()
    vku.cbuff:begin()
    vku.cbuff:begin_rpass(vku.fbs, img_idx)
    vku.cbuff:bind_vert_buffs(0, {{vku.vbuff, 0}})
    vku.cbuff:bind_idx_buff(vku.ibuff, 0, vku.VK_INDEX_TYPE_UINT16)
    vku.cbuff:bind_desc_set(vku.VK_PIPELINE_BIND_POINT_GRAPHICS, vku.pl, vku.desc_set)
    vku.cbuff:draw_idx(vku.pl, indices.size())
    vku.cbuff:end_rpass()
    vku.cbuff:end_begin()

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

// inline lua_metatable_t lua_metatable[VKU_TYPE_CNT];
// inline int init_lua_meta() {
//     lua_metatable[VKU_TYPE_WINDOW].cbk =                 [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_INSTANCE].cbk =               [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_SURFACE].cbk =                [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_DEVICE].cbk =                 [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_SWAPCHAIN].cbk =              [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_SHADER].cbk =                 [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_RENDERPASS].cbk =             [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_PIPELINE].cbk =               [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_COMPUTE_PIPELINE].cbk =       [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_FRAMEBUFFERS].cbk =           [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_COMMAND_POOL].cbk =           [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_COMMAND_BUFFER].cbk =         [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_SEMAPHORE].cbk =              [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_FENCE].cbk =                  [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_BUFFER].cbk =                 [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_IMAGE].cbk =                  [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_IMAGE_VIEW].cbk =             [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_IMAGE_SAMPLER].cbk =          [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_DESCRIPTOR_SET].cbk =         [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_DESCRIPTOR_POOL].cbk =        [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_SAMPLER_BINDING].cbk =        [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_BUFFER_BINDING].cbk =         [](lua_State *L){ return 0; };
//     lua_metatable[VKU_TYPE_BINDING_DESCRIPTOR_SET].cbk = [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_SPIRV].cbk =                  [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_STRING].cbk =                 [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_FLOAT].cbk =                  [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_INTEGER].cbk =                [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_LUA_SCRIPT].cbk =             [](lua_State *L){ return 0; };
//     lua_metatable[VKC_TYPE_LUA_VARIABLE].cbk =           [](lua_State *L){ return 0; };
// }

inline const luaL_Reg vku_tab_funcs[] = {
    {"glfw_pool_events", impl_TODO},
    {"get_key", impl_TODO},
    {"signal_close", impl_TODO},
    {"aquire_next_img", impl_TODO},
    {NULL, NULL}
};

inline void* luaw_to_user_data(int index) { return (void*)(intptr_t)(index); }
inline int luaw_from_user_data(void *val) { return (int)(intptr_t)(val); }

/* This is here just to hold the diverse bitmaps */
template <typename T>
struct bm_t { using type = T; };

template <typename Param, size_t index>
struct luaw_param_t{
    void luaw_single_param(lua_State *L) {
        /* What a parameter can be:
        1. vku::ref_t of some object
        2. a std::string
        3. an integer bitmap
        4. an integer */

        /* If this is resolved to a void it will error out, which is ok, because this case is either
        way an error */
        demangle_static_assert<false, Param>(" - Is not a valid parameter type");
    }
};

template <typename T, size_t index>
struct luaw_param_t<vku::ref_t<T>, index> {
    vku::ref_t<T> luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return vku::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
        int obj_index = luaw_from_user_data(lua_touserdata(L, index));
        if (obj_index == 0) {
            luaw_push_error(L, std::format("Invalid parameter at index {} of expected type {}",
                    index, demangle<T>));
            lua_error(L);
        }
        return objects[obj_index].obj.to_derived<T>();
    }
};


template <typename T, size_t index>
struct luaw_param_t<bm_t<T>, index> {
    T luaw_single_param(lua_State *L) {
        /* TODO checks */
        int64_t intval = lua_tointeger(L, index);
        return (T)intval;
    }
};

template <size_t index>
struct luaw_param_t<char *, index> {
    char *luaw_single_param(lua_State *L) {
        /* TODO checks */
        return lua_tostring(L, index);
    }
};

template <typename T>
void luaw_ret_push(lua_State *L, T&& t) {
    (void)t;
    demangle_static_assert<false, T>(" - Is not a valid return type");
}

template <>
void luaw_ret_push<int>(lua_State *L, int&& x) {
    lua_pushinteger(L, x);
}

void luaw_push_error(lua_State *L, std::string err_str) {
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
}

template <typename VkuT, auto member_ptr, typename ...Params, size_t ...I>
int luaw_member_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    int index = luaw_from_user_data(lua_touserdata(L, 1));
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't call member function!");
        lua_error(L);
    }
    auto &o = objects[index];
    if (!o.obj) {
        luaw_push_error(L, "internal_error: Nil user object can't call member function!");
        lua_error(L);
    }
    auto obj = o.obj.to_derived<VkuT>();

    using RetType = decltype((obj.get()->*member_ptr)(
            luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));

    if constexpr (std::is_void_v<RetType>) {
        (obj.get()->*member_ptr)(luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...);
        return 0;
    }
    else {
        luaw_ret_push(L, (obj.get()->*member_ptr)(
                luaw_param_t<Params, I + 2>{}.luaw_single_param(L)...));
        return 1;
    }    
}

/* TODO: all function(LUA ONES) calls should redirect exceptions through this */
int luaw_catch_exception(lua_State *L) {
    /* We don't let errors get out of the call because we don't want to break lua. As such, we catch
    any error and propagate it as a lua error. */
    try {
        throw ; // re-throw the current exception
    }
    catch (vku::err_t &vkerr) {
        luaw_push_error(L, std::format("Invalid call: {}", vkerr.what()));
        lua_error(L);
    }
    catch (fkyaml::exception &e) {
        luaw_push_error(L, std::format("fkyaml::exception: {}", e.what()));
        lua_error(L);
    }
    catch (std::exception &e) {
        luaw_push_error(L, std::format("std::exception: {}", e.what()));
        lua_error(L);
    }
    catch (...) {
        throw ; /* most probably the lua string */
    }

    return 0;
}

template <typename VkuT, auto member_ptr, typename ...Params>
int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<VkuT, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename VkuT, auto member_ptr, typename ...Params>
void luaw_register_function(lua_State *L, const char *function_name) {
    lua_CFunction f = &luaw_member_function_wrapper<VkuT, member_ptr, Params...>;
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, function_name);
}

inline int luaopen_vku (lua_State *L) {

    /* This is also a great example of how to register all the types:
        - all the nmes will be registered and depending of their types
        - all their members and functions will also be registered
        - all the numbers will be also registered as integers
        - each type that is not a constant will have a _t that holds the user ptr
        */

    int top = lua_gettop(L);

    {
        luaL_newmetatable(L, "__vku_metatable");

        /* params: 1.usrptr, 2.key -> returns: 1.value */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__index: %d", lua_gettop(L));

            lua_getmetatable(L, 1);             /* Get current object metatable */
            lua_getfield(L, -1, "cbuff:begin"); /* Get the function from inside the metatable */
            lua_remove(L, -2);                  /* pop lua_getmetatable */

            return 1;   /* we return the function */
        });
        lua_setfield(L, -2, "__index");

        /* params: 1.usrptr */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__gc");

            /* TODO: garbage collect the obhect */
            (void)L;
            return 0;
        });
        lua_setfield(L, -2, "__gc");

        /* params: 1.usrptr [2.error] */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__close");
            (void)L;
            return 0;
        });
        lua_setfield(L, -2, "__close");

        luaw_register_function<
        /* self, fn */  vku::cmdbuff_t, &vku::cmdbuff_t::begin,
        /* params   */  bm_t<VkCommandBufferUsageFlagBits>>(L, "cbuff:begin");

        lua_pushstring(L, "locked");
        lua_setfield(L, -2, "__metatable");

        lua_pop(L, 1); /* pop luaL_newmetatable */
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top == lua_gettop(L))); /* sanity check */

    {
        /* TODO: all loaded, named types must also be registered here, such that the lua object
        holds it's only reference, and upon __gc, it frees it (that is creates a reference, deletes
        it from the mapping and gives lua the pointer to that reference) */
        /* TODO: all enum types must be here registered as integers inside this library */
        /* TODO: create some sor of creator function, that can be given a description as the ones
        above, transforms it into yaml and finally uses the functions above to generate the
        object */

        luaL_newlib(L, vku_tab_funcs);

        /* this makes vulkan_utils.cbuff = &false_user_data and sets it's metadata */
        int false_user_data = 22;
        lua_pushlightuserdata(L, luaw_to_user_data(false_user_data));
        luaL_setmetatable(L, "__vku_metatable");
        lua_setfield(L, -2, "cbuff");
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top + 1 == lua_gettop(L))); /* sanity check */

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


/*!
 * WIP TUTORIAL
 * 
 * OBS: Only vku objects ierarhize from one-another, vkc objects do not do that.
 * TODO: spirv should be part of hierarchy, hence part of vku, to enable it to be re-created?
 * 
 * How objects work in the configuration file:
 * ===========================================
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