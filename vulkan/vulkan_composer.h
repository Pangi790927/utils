#ifndef VULKAN_COMPOSER_H
#define VULKAN_COMPOSER_H

/*! @file This file will be used to auto-initialize Vulkan. This file will describe the structure
 * of the Vulkan objects and create an vulkan_obj_t with the specific type and an associated name.
 * In this way you can configure all the semaphores and buffers from here and ask for them inside
 * the program. The objective is to have the entire Vulkan pipeline described from outside of the
 * source code. */

/* TODO: figure out what to do with this: */
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

/* This holds the obhects reference such that lua can use them */
struct object_ref_t {
    vku::ref_t<vku::object_t> obj;  /* The actual reference to the vku/vkc object */

    std::string name;               /* this is required such that when this object gets removed it
                                       also gets removed from objects_map */
};

/* Holds all the references to the objects, it is held as on object so it can be initialized by
the lua code and upon success, merged */
struct ref_state_t {
    std::vector<object_ref_t> objects;
    std::map<std::string, int> objects_map;
    std::map<std::string, std::vector<co::state_t *>> wanted_objects;
    std::deque<std::coroutine_handle<void>> work;
    std::vector<int> free_objects;

    ref_state_t() : objects(MAX_NUMBER_OF_OBJECTS) {
        for (int i = MAX_NUMBER_OF_OBJECTS-1; i >= 1; i--)
            free_objects.push_back(i);
    }

    /* TODO: this is stupid slow, must be made faster (create a clear interface of adding and
    removing objects and make get_new and append return the internal held update list) */

    std::vector<int> get_new(const ref_state_t &oth) {
        std::set<int> free_objects_this(free_objects.begin(), free_objects.end());
        std::set<int> free_objects_other(oth.free_objects.begin(), oth.free_objects.end());

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
            throw vku::err_t(
                    "How could the state change while we where creating a new object? huh?");
        return new_other;
    }

    void append(const ref_state_t &oth) {
        std::set<int> free_objects_this(free_objects.begin(), free_objects.end());
        std::set<int> free_objects_other(oth.free_objects.begin(), oth.free_objects.end());

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
            this->objects[idx] = oth.objects[idx];
            this->objects_map[oth.objects[idx].name] = idx;
        }
        this->free_objects = std::vector<int>(free_objects_other.begin(), free_objects_other.end());
    }
};

/* The state is initially created to have  */
inline ref_state_t g_rs;
inline int g_lua_table;

struct lua_var_t : public vku::object_t {
    std::string name;

    static  vku_object_type_e type_id_static() { return VKC_TYPE_LUA_VARIABLE; }
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

    static  vku_object_type_e type_id_static() { return VKC_TYPE_LUA_SCRIPT; }
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

    static  vku_object_type_e type_id_static() { return VKC_TYPE_INTEGER; }
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

    static  vku_object_type_e type_id_static() { return VKC_TYPE_FLOAT; }
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

    static  vku_object_type_e type_id_static() { return VKC_TYPE_STRING; }
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

    static  vku_object_type_e type_id_static() { return VKC_TYPE_SPIRV; }
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

struct vertex_input_desc_t : public vku::object_t {
    vku::vertex_input_desc_t vid;

    static vku_object_type_e type_id_static() { return VKC_TYPE_VERTEX_INPUT_DESC; }
    static vku::ref_t<vertex_input_desc_t> create(const vku::vertex_input_desc_t& vid) {
        auto ret = vku::ref_t<vertex_input_desc_t>::create_obj_ref(
                std::make_unique<vertex_input_desc_t>(), {});
        ret->vid = vid;
        return ret;
    }

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_VERTEX_INPUT_DESC; }

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

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

struct binding_desc_t : public vku::object_t {
    VkDescriptorSetLayoutBinding bd;

    static vku_object_type_e type_id_static() { return VKC_TYPE_BINDING_DESC; }
    static vku::ref_t<binding_desc_t> create(const VkDescriptorSetLayoutBinding& bd) {
        auto ret = vku::ref_t<binding_desc_t>::create_obj_ref(
                std::make_unique<binding_desc_t>(), {});
        ret->bd = bd;
        return ret;
    }

    virtual vku_object_type_e type_id() const override { return VKC_TYPE_BINDING_DESC; }

    inline std::string to_string() const override {
        return std::format("[binding={}, type={}, stage={}]",
                bd.binding,
                vku::to_string(bd.descriptorType),
                vku::to_string((VkShaderStageFlagBits)bd.stageFlags));
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
    depend_resolver_t(ref_state_t *rs, std::string required_depend)
    : required_depend(required_depend), rs(rs) {}

    /* If we already have the dependency we can already retur */
    bool await_ready() noexcept { return has(rs->objects_map, required_depend); }

    /* Else we place ourselves on the waiting queue */
    template <typename P>
    co::handle<void> await_suspend(co::handle<P> caller) noexcept {
        auto state = co::external_on_suspend(caller);
        /* We place ourselves on the waiting queue: */
        rs->wanted_objects[required_depend].push_back(state);

        /* Else we return the next work in line that can be done */
        return co::external_wait_next_task(state->pool);
    }

    vku::ref_t<VkuT> await_resume() {
        if (!has(rs->objects_map, required_depend)) {
            DBG("Object not found");
            throw vku::err_t(std::format("Object not found, {}", required_depend));
        }
        if (!rs->objects[rs->objects_map[required_depend]].obj) {
            DBG("For some reason this object now holds a nullptr...");
            throw vku::err_t("nullptr object");
        }
        auto ret = rs->objects[rs->objects_map[required_depend]].obj.to_related<VkuT>();
        if (!ret) {
            DBG("Invalid ref...");
            throw vku::err_t(sformat("Invalid reference, maybe cast doesn't work?: [cast: %s to: %s]",
                    demangle<4>(typeid(rs->objects[rs->objects_map[required_depend]].obj.get()).name()).c_str(),
                    demangle<VkuT, 4>().c_str()));
        }
        return ret;
    }

    std::string required_depend;
    ref_state_t *rs;
};

void mark_dependency_solved(ref_state_t *rs,
        std::string depend_name, vku::ref_t<vku::object_t> depend)
{
    /* First remember the dependency: */
    if (!depend) {
        DBG("Object into nullptr");
        throw vku::err_t{std::format("Object turned into nullptr: {}", depend_name)};
    }
    if (has(rs->objects_map, depend_name)) {
        DBG("Name taken");
        throw vku::err_t{std::format("Tag name already exists: {}", depend_name)};
    }
    int new_id = rs->free_objects.back();
    rs->free_objects.pop_back();

    DBG("Adding object: %s [%d]", depend->to_string().c_str(), new_id);
    rs->objects_map[depend_name] = new_id;
    depend->cbks = std::make_shared<vku::object_cbks_t>();
    depend->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});
    rs->objects[new_id].obj = depend;
    rs->objects[new_id].name = depend_name;

    /* Second, awake all the ones waiting for the respective dependency */
    if (vkc::has(rs->wanted_objects, depend_name)) {
        for (auto s : rs->wanted_objects[depend_name])
            co::external_sched_resume(s);
        rs->wanted_objects.erase(depend_name);
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

co::task_t build_pseudo_object(ref_state_t *rs, const std::string& name, fkyaml::node& node) {
    if (node.is_integer()) {
        auto obj = integer_t::create(node.as_int());
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return 0;
    }

    if (node.is_string()) {
        auto obj = string_t::create(node.as_str());
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
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
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());

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
            mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
            co_return 0;
        }

        if (node.contains("m_source_path")) {
            std::string source = get_file_string_content(node["m_source_path"].as_str());

            auto obj = lua_script_t::create(source);
            mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
            co_return 0;
        }
    }

    DBG("Failed to build anything from this object[%s], so the object is invalid", name.c_str());
    co_return -1;
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<int64_t> resolve_int(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<integer_t>(rs, node.as_str()))->value;
    co_return node.as_int();
}

/*! This either follows a reference to an integer or it returns the direct value if available */
co::task<double> resolve_float(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<float_t>(rs, node.as_str()))->value;
    co_return node.as_float();
}

/*! This either follows a reference to a string or it returns the direct value if available */
co::task<std::string> resolve_str(ref_state_t *rs, fkyaml::node& node) {
    if (node.has_tag_name() && node.get_tag_name() == "!ref")
        co_return (co_await depend_resolver_t<string_t>(rs, node.as_str()))->value;
    co_return node.as_str();
}

co::task<vku::ref_t<vku::object_t>> build_object(ref_state_t *rs,
        const std::string& name, fkyaml::node& node);

inline int64_t anonymous_increment = 0;
inline std::string new_anon_name() {
    return "anonymous_" + std::to_string(anonymous_increment++);
}

template <typename VkuT>
co::task<vku::ref_t<VkuT>> resolve_obj(ref_state_t *rs, fkyaml::node& node) {
    /* Check -- How objects work in the configuration file -- */

    if (node.has_tag_name() && node.get_tag_name() == "!ref") {
        /* This is simply a reference to an object m_field: !ref tag_name*/
        co_return co_await depend_resolver_t<VkuT>(rs, node.as_str());
    }
    else if (node.is_mapping() && node.as_map().size() == 1
            && node.as_map().begin()->second.contains("m_type"))
    {
        /* This is in the form m_field: tag_name: m_type: "..." */
        std::string tag = node.as_map().begin()->first.as_str();
        auto ref = co_await build_object(rs, tag, node.as_map().begin()->second);
        co_return ref.template to_related<VkuT>();
    }
    else if (node.contains("m_type")) {
        /* This is in the form m_field: m_type: "...", ie, inlined object */
        std::string tag = node.contains("m_tag") ?
                node["m_tag"].as_str() : new_anon_name();
        auto ref = co_await build_object(rs, tag, node);
        co_return ref.template to_related<VkuT>();
    }

    /* None of the above */
    co_return nullptr;
}

co::task<vku::ref_t<vku::object_t>> build_object(ref_state_t *rs,
        const std::string& name, fkyaml::node& node)
{
    if (!node.is_mapping()) {
        DBG("Error node: %s not a mapping", fkyaml::node::serialize(node).c_str());
        co_return nullptr;
    }
    if (false);
    else if (node["m_type"] == "vkc::vertex_input_desc_t") {
        std::vector<VkVertexInputAttributeDescription> attrs;
        for (auto attr : node["m_attrs"].as_seq()) {
            auto m_location = co_await resolve_int(rs, attr["m_location"]);
            auto m_binding = co_await resolve_int(rs, attr["m_binding"]);
            auto m_format = get_enum_val<VkFormat>(node["m_format"]);
            auto m_offset = co_await resolve_int(rs, attr["m_offset"]);
            attrs.push_back(VkVertexInputAttributeDescription{
                .location = (uint32_t)m_location,
                .binding = (uint32_t)m_binding,
                .format = m_format,
                .offset = (uint32_t)m_offset
            });
        }
        auto m_binding = co_await resolve_int(rs, node["m_binding"]);
        auto m_stride = co_await resolve_int(rs, node["m_stride"]);
        auto m_in_rate = get_enum_val<VkVertexInputRate>(node["m_in_rate"]);
        auto obj = vertex_input_desc_t::create(vku::vertex_input_desc_t{
            .bind_desc = {
                .binding = (uint32_t)m_binding,
                .stride = (uint32_t)m_stride,
                .inputRate = m_in_rate,
            },
            .attr_desc = attrs,
        });
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::binding_desc_t") {
        auto m_binding = co_await resolve_int(rs, node["m_binding"]);;
        auto m_stage = get_enum_val<VkShaderStageFlagBits>(node["m_stage"]);
        auto m_desc_type = get_enum_val<VkDescriptorType>(node["m_desc_type"]);
        auto obj = binding_desc_t::create(VkDescriptorSetLayoutBinding{
            .binding = (uint32_t)m_binding,
            .descriptorType = m_desc_type,
            .descriptorCount = 1,
            .stageFlags = m_stage,
            .pImmutableSamplers = nullptr
        });
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vkc::lua_var_t") { /* not sure how is this usefull */
        /* lua_var has the same tag_name as the var name */
        auto obj = lua_var_t::create(name);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::instance_t") {
        auto obj = vku::instance_t::create();
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::window_t") {
        auto w = co_await resolve_int(rs, node["m_width"]);
        auto h = co_await resolve_int(rs, node["m_height"]);
        auto window_name = co_await resolve_str(rs, node["m_name"]);
        auto obj = vku::window_t::create(w, h, window_name);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::surface_t") {
        auto window = co_await resolve_obj<vku::window_t>(rs, node["m_window"]);
        auto instance = co_await resolve_obj<vku::instance_t>(rs, node["m_instance"]);
        auto obj = vku::surface_t::create(window, instance);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::device_t") {
        auto surf = co_await resolve_obj<vku::surface_t>(rs, node["m_surface"]);
        auto obj = vku::device_t::create(surf);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdpool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::cmdpool_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::image_t") {
        auto cp = co_await resolve_obj<vku::cmdpool_t>(rs, node["m_cmdpool"]);
        auto path = co_await resolve_str(rs, node["m_path"]);
        auto obj = load_image(cp, path);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_view_t") {
        auto img = co_await resolve_obj<vku::image_t>(rs, node["m_image"]);
        auto aspect_mask = get_enum_val<VkImageAspectFlagBits>(node["m_aspect_mask"]);
        auto obj = vku::img_view_t::create(img, aspect_mask);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::img_sampl_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::img_sampl_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::buffer_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        size_t sz = co_await resolve_int(rs, node["m_size"]);
        auto usage_flags = get_enum_val<VkBufferUsageFlagBits>(node["m_usage_flags"]);
        auto share_mode = get_enum_val<VkSharingMode>(node["m_sharing_mode"]);
        auto memory_flags = get_enum_val<VkMemoryPropertyFlagBits>(node["m_memory_flags"]);
        auto obj = vku::buffer_t::create(dev, sz, usage_flags, share_mode, memory_flags);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t") {
        std::vector<vku::ref_t<vku::binding_desc_set_t::binding_desc_t>> bindings;
        for (auto& subnode : node["m_descriptors"])
            bindings.push_back(
                    co_await resolve_obj<vku::binding_desc_set_t::binding_desc_t>(rs, subnode));
        auto obj = vku::binding_desc_set_t::create(bindings);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::buff_binding_t") {
        auto buff = co_await resolve_obj<vku::buffer_t>(rs, node["m_buff"]);
        auto desc = co_await resolve_obj<vkc::binding_desc_t>(rs, node["m_desc"]);
        auto obj = vku::binding_desc_set_t::buff_binding_t::create(desc->bd, buff);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::binding_desc_set_t::sampl_binding_t") {
        auto view = co_await resolve_obj<vku::img_view_t>(rs, node["m_view"]);
        auto sampler = co_await resolve_obj<vku::img_sampl_t>(rs, node["m_sampler"]);
        auto desc = co_await resolve_obj<vkc::binding_desc_t>(rs, node["m_desc"]);
        auto obj = vku::binding_desc_set_t::sampl_binding_t::create(desc->bd, view, sampler);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::shader_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto spirv = co_await resolve_obj<spirv_t>(rs, node["m_spirv"]);
        auto obj = vku::shader_t::create(dev, spirv->spirv);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::swapchain_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::swapchain_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::renderpass_t") {
        auto swc = co_await resolve_obj<vku::swapchain_t>(rs, node["m_swapchain"]);
        auto obj = vku::renderpass_t::create(swc);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::pipeline_t") {
        auto w = co_await resolve_int(rs, node["m_width"]);
        auto h = co_await resolve_int(rs, node["m_height"]);
        auto rp = co_await resolve_obj<vku::renderpass_t>(rs, node["m_renderpass"]);
        std::vector<vku::ref_t<vku::shader_t>> shaders;
        for (auto& sh : node["m_shaders"])
            shaders.push_back(co_await resolve_obj<vku::shader_t>(rs, sh));
        auto topol = get_enum_val<VkPrimitiveTopology>(node["m_topology"]);
        auto indesc = vku::vertex3d_t::get_input_desc(); /* TODO: resolve this */
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        auto obj = vku::pipeline_t::create(w, h, rp, shaders, topol, indesc, binds);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::framebuffs_t") {
        auto rp = co_await resolve_obj<vku::renderpass_t>(rs, node["m_renderpass"]);
        auto obj = vku::framebuffs_t::create(rp);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::sem_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::sem_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::fence_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto obj = vku::fence_t::create(dev);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::cmdbuff_t") {
        auto cp = co_await resolve_obj<vku::cmdpool_t>(rs, node["m_cmdpool"]);
        auto obj = vku::cmdbuff_t::create(cp);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_pool_t") {
        auto dev = co_await resolve_obj<vku::device_t>(rs, node["m_device"]);
        auto binds = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        int cnt = co_await resolve_int(rs, node["m_cnt"]);
        auto obj = vku::desc_pool_t::create(dev, binds, cnt);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }
    else if (node["m_type"] == "vku::desc_set_t") {
        auto descritpor_pool = co_await resolve_obj<vku::desc_pool_t>(rs, node["m_descritpor_pool"]);
        auto pipeline = co_await resolve_obj<vku::pipeline_t>(rs, node["m_pipeline"]);
        auto bindings = co_await resolve_obj<vku::binding_desc_set_t>(rs, node["m_bindings"]);
        auto obj = vku::desc_set_t::create(descritpor_pool, pipeline, bindings);
        mark_dependency_solved(rs, name, obj.to_base<vku::object_t>());
        co_return obj.to_base<vku::object_t>();
    }

    DBG("Object m_type is not known: %s", node["m_type"].as_str().c_str());
    throw vku::err_t{std::format("Invalid object type: {}", node["m_type"].as_str())};
}


co::task_t build_schema(ref_state_t *rs, fkyaml::node& root) {
    ASSERT_BOOL_CO(root.is_mapping());

    for (auto &[name, node] : root.as_map()) {
        if (!node.contains("m_type")) {
            co_await co::sched(build_pseudo_object(rs, name.as_str(), node));
        }
        else {
            co_await co::sched(build_object(rs, name.as_str(), node));
        }
    }

    co_return 0;
}

inline vkc_error_e parse_config(const char *path) {
    std::ifstream file(path);

    try {
        auto config = fkyaml::node::deserialize(file);

        auto pool = co::create_pool();
        pool->sched(build_schema(&g_rs, config));

        if (pool->run() != co::RUN_OK) {
            DBG("Failed to create the schema");
            return VKC_ERROR_GENERIC;
        }

        if (g_rs.wanted_objects.size()) {
            for (auto &[k, v]: g_rs.wanted_objects) {
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

/* TODO: fix layout of this code... it is bad */
/* TODO:
    + We need to be able to parse dicts to yaml and build objects from it (same as initial parse)
    - We still need to fix some functions
    - We need a generic way to store some special types (mvp for example)
    - We need to fix the stupidity that is descriptors
    - We need a way to set buffers (based on descripors maybe?)
    - We need some new types (matrices, vectors)
    ~ We need to make std::vector and std::map an acceptable parameter/return type
    + We need to make std::tuple an acceptable parameter
    - We need to expose the way that we pack functions and members to the outside
            (so that an user can use them to add his own functions (that user is me))

    Once those are done, I think this is done */

inline std::string lua_example_str = R"___(
vku = require("vulkan_utils")

print(debug.getregistry())


t = {
    m_type = "vkc::lua_var_t",
    var1 = 1,
    var2 = 2,
    var3 = {
        var4 = "str"
    }
}
to = vku.create_object("tag_name", t)
print(to)

function on_loop_run()
    vku.glfw_pool_events() -- needed for keys to be available
    if vku.get_key(vku.window, vku.GLFW_KEY_ESCAPE) == vku.GLFW_PRESS then
        vku.signal_close() -- this will close the loop
    end

    img_idx = vku.aquire_next_img(vku.swc, vku.img_sem)

    -- do mvp update somehow?

    vku.cbuff:begin(vku.VK_COMMAND_BUFFER_USAGE_NONE)
    vku.cbuff:begin_rpass(vku.fbs, img_idx)
    vku.cbuff:bind_vert_buffs(0, {{vku.vbuff, 0}})
    vku.cbuff:bind_idx_buff(vku.ibuff, 0, vku.VK_INDEX_TYPE_UINT16)
    vku.cbuff:bind_desc_set(vku.VK_PIPELINE_BIND_POINT_GRAPHICS, vku.pl, vku.desc_set)
    vku.cbuff:draw_idx(vku.pl, 3)
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


inline void* luaw_to_user_data(int index) { return (void*)(intptr_t)(index); }
inline int luaw_from_user_data(void *val) { return (int)(intptr_t)(val); }

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

/* This is here just to hold the diverse bitmaps */
template <typename T>
struct bm_t {
    using type = T;
};

template <typename Param, ssize_t index>
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

/* This resolves userdata(vku::ref) received from lua to an vku parameter */
template <typename T, ssize_t index>
struct luaw_param_t<vku::ref_t<T>, index> {
    vku::ref_t<T> luaw_single_param(lua_State *L) {
        if (lua_isnil(L, index))
            return vku::ref_t<T>{}; /* if the user intended to pass a nill, we give it as a nullptr */
        int obj_index = luaw_from_user_data(lua_touserdata(L, index));
        if (obj_index == 0) {
            luaw_push_error(L, std::format("Invalid parameter at index {} of expected type {} but "
                    "got [{}] instead",
                    index, demangle<T>(), lua_typename(L, lua_type(L, index))));
            lua_error(L);
        }
        return g_rs.objects[obj_index].obj.to_related<T>();
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
                lua_error(L);
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
                lua_error(L);
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
                    lua_error(L);
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
            lua_error(L);
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
            lua_error(L);
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
            lua_error(L);
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
            lua_error(L);
        }
        return ret;
    }
};

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
            lua_error(L);
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
            lua_error(L);
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

template <typename T>
struct luaw_returner_t {
    void luaw_ret_push(lua_State *L, T&& t) {
        (void)t;
        demangle_static_assert<false, T>(" - Is not a valid return type");
    }
};

template <std::integral Integer>
struct luaw_returner_t<Integer> {
    void luaw_ret_push(lua_State *L, Integer&& x) {
        lua_pushinteger(L, x);
    }
};

template <auto function, typename ...Params, size_t ...I>
int luaw_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
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

template <typename VkuT, auto member_ptr, typename ...Params, size_t ...I>
int luaw_member_function_wrapper_impl(lua_State *L, std::index_sequence<I...>) {
    int index = luaw_from_user_data(lua_touserdata(L, 1));
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't call member function!");
        lua_error(L);
    }
    auto &o = g_rs.objects[index];
    if (!o.obj) {
        luaw_push_error(L, "internal_error: Nil user object can't call member function!");
        lua_error(L);
    }
    auto obj = o.obj.to_related<VkuT>();

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

template <auto function, typename ...Params>
int luaw_function_wrapper(lua_State *L) {
    try {
        return luaw_function_wrapper_impl<function, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename VkuT, auto member_ptr, typename ...Params>
int luaw_member_function_wrapper(lua_State *L) {
    try {
        return luaw_member_function_wrapper_impl<VkuT, member_ptr, Params...>(
                L, std::index_sequence_for<Params...>{});
    }
    catch (...) { return luaw_catch_exception(L); }
}


// helper to detect if a type is vku::ref_t<...>
template <typename>
struct is_vku_ref_t : std::false_type {};

template <typename T>
struct is_vku_ref_t<vku::ref_t<T>> : std::true_type {};

template <typename VkuT, auto member_ptr>
int luaw_member_object_wrapper(lua_State *L) {
    try {
        int index = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
        if (index == 0) {
            luaw_push_error(L, "Nil user object can't get member!");
            lua_error(L);
        }
        auto &o = g_rs.objects[index];
        if (!o.obj) {
            luaw_push_error(L, "internal_error: Nil user object can't get member!");
            lua_error(L);
        }
        auto obj = o.obj.to_related<VkuT>();
        auto &member = obj.get()->*member_ptr;

        using member_type = std::decay_t<decltype(member)>;

        if constexpr (std::is_same_v<member_type, std::string>) {
            lua_pushstring(L, member.c_str());
            return 1;
        }
        else if constexpr (std::is_integral_v<member_type>) {
            lua_pushinteger(L, member);
            return 1;
        }
        else if constexpr (std::is_floating_point_v<member_type>) {
            lua_pushnumber(L, member);
            return 1;
        }
        else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
            lua_createtable(L, member.size(), 0);
            for (size_t i = 1; auto &str : member) {
                lua_pushstring(L, str.c_str());
                lua_rawseti(L, -2, i++);
            }
            return 1;
        }
        else if constexpr (is_vku_ref_t<member_type>::value) {
            if (!member) {
                lua_pushnil(L);
                return 1;
            }
            if (!member->cbks) {
                luaw_push_error(L, "internal_error: How did this object get known to lua ?!");
                lua_error(L);
            }
            if (!member->cbks->usr_ptr) {
                /* So this object was no longer known by the lua side, we must resurect it */

                /* We first get it a new id */
                int new_id = g_rs.free_objects.back();
                g_rs.free_objects.pop_back();

                /* make it reference it's own id */
                member->cbks->usr_ptr = std::shared_ptr<void>((void *)(intptr_t)new_id, [](void *){});

                /* add it's lua-name-mapping and it's lua-id-mapping */
                std::string name = new_anon_name();
                g_rs.objects_map[name] = new_id;
                g_rs.objects[new_id].obj = member;
                g_rs.objects[new_id].name = name;
            }
            int member_id = (intptr_t)member->cbks->usr_ptr.get();
            if (member_id >= g_rs.objects.size() || member_id < 0) {
                luaw_push_error(L, "internal_error: Integrity check failed");
                lua_error(L);
            }
            lua_pushlightuserdata(L, luaw_to_user_data(member_id));
            luaL_setmetatable(L, "__vku_metatable");
            return 1;
        }
        else {
            demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
            return 0;
        }
    }
    catch (...) { return luaw_catch_exception(L); }
}

template <typename VkuT, auto member_ptr>
int luaw_member_setter_object_wrapper(lua_State *L) {
    int index = luaw_from_user_data(lua_touserdata(L, -3)); /* an int, ok on unwind */
    if (index == 0) {
        luaw_push_error(L, "Nil user object can't set member!");
        lua_error(L);
    }
    auto &o = g_rs.objects[index];
    if (!o.obj) {
        luaw_push_error(L, "internal_error: Nil user object can't set member!");
        lua_error(L);
    }
    auto obj = o.obj.to_related<VkuT>();
    auto &member = obj.get()->*member_ptr;

    using member_type = std::decay_t<decltype(member)>;

    if constexpr (std::is_same_v<member_type, std::string>) {
        const char *str = lua_tostring(L, -1);
        member = str ? str : "";
        return 0;
    }
    else if constexpr (std::is_integral_v<member_type>) {
        uint64_t val = lua_tointeger(L, -1);
        member = (member_type)val;
        return 0;
    }
    else if constexpr (std::is_floating_point_v<member_type>) {
        double val = lua_tonumber(L, -1);
        member = (member_type)val;
        return 0;
    }
    else if constexpr (std::is_same_v<member_type, std::vector<std::string>>) {
        if (!lua_istable(L, -1)) {
            luaw_push_error(L, "You need a table for this assignment!");
            lua_error(L);
        }
        int len = lua_rawlen(L, -1);
        std::vector<std::string> to_asign;
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, -1, i);
            const char *str = lua_tostring(L, -1);
            to_asign.push_back(str ? str : "");
            lua_pop(L, 1);
        }
        member = to_asign;
        return 0;
    }
    else if constexpr (is_vku_ref_t<member_type>::value) {
        int index = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
        if (index == 0) {
            member = nullptr;
            return 0;
        }
        member = g_rs.objects[index].obj;
        return 0;
    }
    else {
        demangle_static_assert<false, decltype(member)>(" - Is not a valid member type");
        return 0;
    }
}

enum luaw_member_e {
    LUAW_MEMBER_FUNCTION,
    LUAW_MEMBER_OBJECT,
};
struct luaw_member_t {
    lua_CFunction fn;
    luaw_member_e member_type;
};
inline std::unordered_map<std::string, luaw_member_t> lua_class_members[VKU_TYPE_CNT];
inline std::unordered_map<std::string, lua_CFunction> lua_class_member_setters[VKU_TYPE_CNT];

template <typename VkuT, auto member_ptr, typename ...Params>
void luaw_register_member_function(const char *function_name) {
    lua_class_members[VkuT::type_id_static()][function_name] = luaw_member_t{
        .fn = &luaw_member_function_wrapper<VkuT, member_ptr, Params...>,
        .member_type = LUAW_MEMBER_FUNCTION
    };
}

template <typename VkuT, auto member_ptr>
void luaw_register_member_object(const char *member_name) {
    lua_class_members[VkuT::type_id_static()][member_name] = luaw_member_t{
        .fn = &luaw_member_object_wrapper<VkuT, member_ptr>,
        .member_type = LUAW_MEMBER_OBJECT
    };

    lua_class_member_setters[VkuT::type_id_static()][member_name] =
            &luaw_member_setter_object_wrapper<VkuT, member_ptr>;
}

#define VKC_REG_MEMB(obj_type, memb)    \
luaw_register_member_object<            \
/* self        */   obj_type,           \
/* member      */   &obj_type::memb>    \
/* member name */   (#memb)

#define VKC_REG_FN(obj_type, fn, ...)   \
luaw_register_member_function<          \
/* self     */  obj_type,               \
/* function */  &obj_type::fn,          \
/* params   */  ##__VA_ARGS__>          \
/* fn name  */  (#fn)

inline void glfw_pool_events() {
    glfwPollEvents();
}

inline uint32_t glfw_get_key(vku::ref_t<vku::window_t> window, uint32_t key) {
    return glfwGetKey(window->get_window(), key);
}

inline void internal_signal_close() {
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
        lua_error(L);
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
        lua_error(L);
    }

    if (array_detected) {
        int len = lua_rawlen(L, index);
        fkyaml::node to_ret(fkyaml::node_type::SEQUENCE);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, index, i);
            auto to_add = create_yaml_from_lua_object(L, -1);
            DBG("ADDING TYPE: %d", (int)to_add.get_type());
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
                DBG("ADDING VAL TYPE: %d", (int)to_add.get_type());
                to_ret[key] = to_add;
            }
            lua_pop(L, 1);
        }
        return to_ret;
    }

    luaw_push_error(L, "internal_error: shouldn't reach here");
    lua_error(L);
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
    ref_state_t ref_state = g_rs;

    DBG("create_object: %s", fkyaml::node::serialize(object_description).c_str());
    auto pool = co::create_pool();

    if (!object_description.contains("m_type")) {
        pool->sched(build_pseudo_object(&ref_state, name, object_description));
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

    auto new_idx = g_rs.get_new(ref_state);

    DBG("Getting lua table...");

    /* Get back the vulkan_utils table */
    lua_rawgeti(L, LUA_REGISTRYINDEX, g_lua_table);

    for (int id : new_idx) {
        if (!ref_state.objects[id].obj) {
            DBG("Null user object?");
        }
        DBG("Registering object: %s", ref_state.objects[id].name.c_str());
        /* this makes vulkan_utils.key = object_id and sets it's metadata */
        lua_pushlightuserdata(L, luaw_to_user_data(id));
        luaL_setmetatable(L, "__vku_metatable");
        lua_setfield(L, -2, ref_state.objects[id].name.c_str());
    }

    lua_getfield(L, -1, name);
    lua_remove(L, -2); /* pops vulkan_utils table */

    /* actualize the global state */
    g_rs.append(ref_state);

    /* Eventual errors are catched outside of this function */
    return 1;
}

inline const luaL_Reg vku_tab_funcs[] = {
    {"glfw_pool_events",luaw_function_wrapper<glfw_pool_events>},
    {"get_key",         luaw_function_wrapper<glfw_get_key, vku::ref_t<vku::window_t>, uint32_t>},
    {"signal_close",    luaw_function_wrapper<internal_signal_close>},
    {"aquire_next_img", luaw_function_wrapper<internal_aquire_next_img,
            vku::ref_t<vku::swapchain_t>, vku::ref_t<vku::sem_t>>},
    {"submit_cmdbuff",  luaw_function_wrapper<vku::submit_cmdbuff,
            std::vector<std::pair<vku::ref_t<vku::sem_t>, bm_t<VkPipelineStageFlagBits>>>,
            vku::ref_t<vku::cmdbuff_t>, vku::ref_t<vku::fence_t>,
            std::vector<vku::ref_t<vku::sem_t>>>},
    {"create_object", internal_create_object},
    {NULL, NULL}
};

inline void luaw_set_glfw_fields(lua_State *L);

inline int luaopen_vku(lua_State *L) {
    int top = lua_gettop(L);

    {
        /* This metatable describes a generic vkc/vku object inside lua. Practically, it expososes
        member objects and functions to lua. */
        luaL_newmetatable(L, "__vku_metatable");

        /* params: 1.usrptr, 2.key -> returns: 1.value */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__index: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -1); /* an const char *, ok on unwind */

            DBG("usr_id: %d", id);
            DBG("member_name: %s", member_name);

            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            vku_object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VKU_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
                lua_error(L);
            }
            if (!has(lua_class_members[class_id], member_name)) {
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
                lua_error(L);
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
                lua_error(L);
            }
            luaw_push_error(L, std::format("INTERNAL ERROR: shouldn't reach here"));
            lua_error(L);
            return 0;
        });
        lua_setfield(L, -2, "__index");

        /* params: 1.usrptr, 2.key, 3.value  */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__newindex: %d", lua_gettop(L));
            int id = luaw_from_user_data(lua_touserdata(L, -2)); /* an int, ok on unwind */
            const char *member_name = lua_tostring(L, -1); /* an const char *, ok on unwind */

            DBG("usr_id: %d", id);
            DBG("member_name: %s", member_name);

            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                luaw_push_error(L, std::format("invalid object id: {}", id));
                lua_error(L);
            }
            vku_object_type_e class_id = o.obj->type_id(); /* an int, still ok on unwind */
            if (class_id < 0 || class_id >= VKU_TYPE_CNT) {
                luaw_push_error(L, std::format("invalid class id: {}", vku::to_string(class_id)));
                lua_error(L);
            }
            if (!has(lua_class_member_setters[class_id], member_name)) {
                luaw_push_error(L, std::format("class id {} doesn't have member: {}",
                        vku::to_string(class_id), member_name));
                lua_error(L);
            }
            auto &member = lua_class_member_setters[class_id][member_name];
            return member(L);
        });
        lua_setfield(L, -2, "__newindex");        

        /* params: 1.usrptr */
        lua_pushcfunction(L, [](lua_State *L) {
            DBG("__gc");

            int id = luaw_from_user_data(lua_touserdata(L, -1)); /* an int, ok on unwind */
            auto &o = g_rs.objects[id]; /* a reference, ok on unwind? (if err) */
            if (!o.obj) {
                return 0;
            }

            /* The object is no longer known to lua, as such we also delete it's slot. Obs: It may
            still be alive, meaning, it is known by the c++ side, just not by the lua side.
            !!! It will also loose it's name with this operation (Is that really ok?) */
            o.obj->cbks->usr_ptr = nullptr;

            /* we clean it's name mapping, it's reference and free it's id */
            g_rs.objects_map.erase(o.name);
            o = object_ref_t{};
            g_rs.free_objects.push_back(id);
            return 0;
        });
        lua_setfield(L, -2, "__gc");

        /* vku::window_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::window_t, window_name);
        VKC_REG_MEMB(vku::window_t, width);
        VKC_REG_MEMB(vku::window_t, height);

        /* vku::instance_t
        ----------------------------------------------------------------------------------------- */

        VKC_REG_MEMB(vku::instance_t, app_name);
        VKC_REG_MEMB(vku::instance_t, engine_name);
        VKC_REG_MEMB(vku::instance_t, extensions);
        VKC_REG_MEMB(vku::instance_t, layers);

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
        luaL_newlib(L, vku_tab_funcs);

        /* Registers this lua table for later use */
        g_lua_table = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, g_lua_table);

        /* Registers glfw enum in this library */
        luaw_set_glfw_fields(L); /* This adds all glfw enums tokens */

        /* Registers objects loaded from the yaml confing as objects in the library */
        for (auto &[k, id] : g_rs.objects_map) {
            if (!g_rs.objects[id].obj) {
                DBG("Null user object?");
            }
            DBG("Registering object: %s", k.c_str());
            /* this makes vulkan_utils.key = object_id and sets it's metadata */
            lua_pushlightuserdata(L, luaw_to_user_data(id));
            luaL_setmetatable(L, "__vku_metatable");
            lua_setfield(L, -2, k.c_str());
        }

        auto register_flag_mapping = [](lua_State *L, auto &mapping) {
            for (auto& [k, v] : mapping) {
                lua_pushinteger(L, (uint32_t)v);
                lua_setfield(L, -2, k.c_str());
            }
        };

        /* Registers vulkan enums in the lua library */
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
    }

    DBG("top: %d gettop: %d", top, lua_gettop(L));
    ASSERT_FN(CHK_BOOL(top + 1 == lua_gettop(L))); /* sanity check */

    return 1;
}

#undef VKC_REG_MEMB
#undef VKC_REG_FN

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

inline void luaw_set_glfw_fields(lua_State *L) {
    lua_pushinteger(L, GLFW_VERSION_MAJOR);
    lua_setfield(L, -2, "GLFW_VERSION_MAJOR"); 
    lua_pushinteger(L, GLFW_VERSION_MINOR);
    lua_setfield(L, -2, "GLFW_VERSION_MINOR"); 
    lua_pushinteger(L, GLFW_VERSION_REVISION);
    lua_setfield(L, -2, "GLFW_VERSION_REVISION"); 
    lua_pushinteger(L, GLFW_TRUE);
    lua_setfield(L, -2, "GLFW_TRUE"); 
    lua_pushinteger(L, GLFW_FALSE);
    lua_setfield(L, -2, "GLFW_FALSE"); 
    lua_pushinteger(L, GLFW_RELEASE);
    lua_setfield(L, -2, "GLFW_RELEASE"); 
    lua_pushinteger(L, GLFW_PRESS);
    lua_setfield(L, -2, "GLFW_PRESS"); 
    lua_pushinteger(L, GLFW_REPEAT);
    lua_setfield(L, -2, "GLFW_REPEAT"); 
    lua_pushinteger(L, GLFW_HAT_CENTERED);
    lua_setfield(L, -2, "GLFW_HAT_CENTERED"); 
    lua_pushinteger(L, GLFW_HAT_UP);
    lua_setfield(L, -2, "GLFW_HAT_UP"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT"); 
    lua_pushinteger(L, GLFW_HAT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_DOWN"); 
    lua_pushinteger(L, GLFW_HAT_LEFT);
    lua_setfield(L, -2, "GLFW_HAT_LEFT"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT_UP);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT_UP"); 
    lua_pushinteger(L, GLFW_HAT_RIGHT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_RIGHT_DOWN"); 
    lua_pushinteger(L, GLFW_HAT_LEFT_UP);
    lua_setfield(L, -2, "GLFW_HAT_LEFT_UP"); 
    lua_pushinteger(L, GLFW_HAT_LEFT_DOWN);
    lua_setfield(L, -2, "GLFW_HAT_LEFT_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_UNKNOWN);
    lua_setfield(L, -2, "GLFW_KEY_UNKNOWN"); 
    lua_pushinteger(L, GLFW_KEY_SPACE);
    lua_setfield(L, -2, "GLFW_KEY_SPACE"); 
    lua_pushinteger(L, GLFW_KEY_APOSTROPHE);
    lua_setfield(L, -2, "GLFW_KEY_APOSTROPHE"); 
    lua_pushinteger(L, GLFW_KEY_COMMA);
    lua_setfield(L, -2, "GLFW_KEY_COMMA"); 
    lua_pushinteger(L, GLFW_KEY_MINUS);
    lua_setfield(L, -2, "GLFW_KEY_MINUS"); 
    lua_pushinteger(L, GLFW_KEY_PERIOD);
    lua_setfield(L, -2, "GLFW_KEY_PERIOD"); 
    lua_pushinteger(L, GLFW_KEY_SLASH);
    lua_setfield(L, -2, "GLFW_KEY_SLASH"); 
    lua_pushinteger(L, GLFW_KEY_0);
    lua_setfield(L, -2, "GLFW_KEY_0"); 
    lua_pushinteger(L, GLFW_KEY_1);
    lua_setfield(L, -2, "GLFW_KEY_1"); 
    lua_pushinteger(L, GLFW_KEY_2);
    lua_setfield(L, -2, "GLFW_KEY_2"); 
    lua_pushinteger(L, GLFW_KEY_3);
    lua_setfield(L, -2, "GLFW_KEY_3"); 
    lua_pushinteger(L, GLFW_KEY_4);
    lua_setfield(L, -2, "GLFW_KEY_4"); 
    lua_pushinteger(L, GLFW_KEY_5);
    lua_setfield(L, -2, "GLFW_KEY_5"); 
    lua_pushinteger(L, GLFW_KEY_6);
    lua_setfield(L, -2, "GLFW_KEY_6"); 
    lua_pushinteger(L, GLFW_KEY_7);
    lua_setfield(L, -2, "GLFW_KEY_7"); 
    lua_pushinteger(L, GLFW_KEY_8);
    lua_setfield(L, -2, "GLFW_KEY_8"); 
    lua_pushinteger(L, GLFW_KEY_9);
    lua_setfield(L, -2, "GLFW_KEY_9"); 
    lua_pushinteger(L, GLFW_KEY_SEMICOLON);
    lua_setfield(L, -2, "GLFW_KEY_SEMICOLON"); 
    lua_pushinteger(L, GLFW_KEY_EQUAL);
    lua_setfield(L, -2, "GLFW_KEY_EQUAL"); 
    lua_pushinteger(L, GLFW_KEY_A);
    lua_setfield(L, -2, "GLFW_KEY_A"); 
    lua_pushinteger(L, GLFW_KEY_B);
    lua_setfield(L, -2, "GLFW_KEY_B"); 
    lua_pushinteger(L, GLFW_KEY_C);
    lua_setfield(L, -2, "GLFW_KEY_C"); 
    lua_pushinteger(L, GLFW_KEY_D);
    lua_setfield(L, -2, "GLFW_KEY_D"); 
    lua_pushinteger(L, GLFW_KEY_E);
    lua_setfield(L, -2, "GLFW_KEY_E"); 
    lua_pushinteger(L, GLFW_KEY_F);
    lua_setfield(L, -2, "GLFW_KEY_F"); 
    lua_pushinteger(L, GLFW_KEY_G);
    lua_setfield(L, -2, "GLFW_KEY_G"); 
    lua_pushinteger(L, GLFW_KEY_H);
    lua_setfield(L, -2, "GLFW_KEY_H"); 
    lua_pushinteger(L, GLFW_KEY_I);
    lua_setfield(L, -2, "GLFW_KEY_I"); 
    lua_pushinteger(L, GLFW_KEY_J);
    lua_setfield(L, -2, "GLFW_KEY_J"); 
    lua_pushinteger(L, GLFW_KEY_K);
    lua_setfield(L, -2, "GLFW_KEY_K"); 
    lua_pushinteger(L, GLFW_KEY_L);
    lua_setfield(L, -2, "GLFW_KEY_L"); 
    lua_pushinteger(L, GLFW_KEY_M);
    lua_setfield(L, -2, "GLFW_KEY_M"); 
    lua_pushinteger(L, GLFW_KEY_N);
    lua_setfield(L, -2, "GLFW_KEY_N"); 
    lua_pushinteger(L, GLFW_KEY_O);
    lua_setfield(L, -2, "GLFW_KEY_O"); 
    lua_pushinteger(L, GLFW_KEY_P);
    lua_setfield(L, -2, "GLFW_KEY_P"); 
    lua_pushinteger(L, GLFW_KEY_Q);
    lua_setfield(L, -2, "GLFW_KEY_Q"); 
    lua_pushinteger(L, GLFW_KEY_R);
    lua_setfield(L, -2, "GLFW_KEY_R"); 
    lua_pushinteger(L, GLFW_KEY_S);
    lua_setfield(L, -2, "GLFW_KEY_S"); 
    lua_pushinteger(L, GLFW_KEY_T);
    lua_setfield(L, -2, "GLFW_KEY_T"); 
    lua_pushinteger(L, GLFW_KEY_U);
    lua_setfield(L, -2, "GLFW_KEY_U"); 
    lua_pushinteger(L, GLFW_KEY_V);
    lua_setfield(L, -2, "GLFW_KEY_V"); 
    lua_pushinteger(L, GLFW_KEY_W);
    lua_setfield(L, -2, "GLFW_KEY_W"); 
    lua_pushinteger(L, GLFW_KEY_X);
    lua_setfield(L, -2, "GLFW_KEY_X"); 
    lua_pushinteger(L, GLFW_KEY_Y);
    lua_setfield(L, -2, "GLFW_KEY_Y"); 
    lua_pushinteger(L, GLFW_KEY_Z);
    lua_setfield(L, -2, "GLFW_KEY_Z"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_BRACKET);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_BRACKET"); 
    lua_pushinteger(L, GLFW_KEY_BACKSLASH);
    lua_setfield(L, -2, "GLFW_KEY_BACKSLASH"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_BRACKET);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_BRACKET"); 
    lua_pushinteger(L, GLFW_KEY_GRAVE_ACCENT);
    lua_setfield(L, -2, "GLFW_KEY_GRAVE_ACCENT"); 
    lua_pushinteger(L, GLFW_KEY_WORLD_1);
    lua_setfield(L, -2, "GLFW_KEY_WORLD_1"); 
    lua_pushinteger(L, GLFW_KEY_WORLD_2);
    lua_setfield(L, -2, "GLFW_KEY_WORLD_2"); 
    lua_pushinteger(L, GLFW_KEY_ESCAPE);
    lua_setfield(L, -2, "GLFW_KEY_ESCAPE"); 
    lua_pushinteger(L, GLFW_KEY_ENTER);
    lua_setfield(L, -2, "GLFW_KEY_ENTER"); 
    lua_pushinteger(L, GLFW_KEY_TAB);
    lua_setfield(L, -2, "GLFW_KEY_TAB"); 
    lua_pushinteger(L, GLFW_KEY_BACKSPACE);
    lua_setfield(L, -2, "GLFW_KEY_BACKSPACE"); 
    lua_pushinteger(L, GLFW_KEY_INSERT);
    lua_setfield(L, -2, "GLFW_KEY_INSERT"); 
    lua_pushinteger(L, GLFW_KEY_DELETE);
    lua_setfield(L, -2, "GLFW_KEY_DELETE"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT"); 
    lua_pushinteger(L, GLFW_KEY_DOWN);
    lua_setfield(L, -2, "GLFW_KEY_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_UP);
    lua_setfield(L, -2, "GLFW_KEY_UP"); 
    lua_pushinteger(L, GLFW_KEY_PAGE_UP);
    lua_setfield(L, -2, "GLFW_KEY_PAGE_UP"); 
    lua_pushinteger(L, GLFW_KEY_PAGE_DOWN);
    lua_setfield(L, -2, "GLFW_KEY_PAGE_DOWN"); 
    lua_pushinteger(L, GLFW_KEY_HOME);
    lua_setfield(L, -2, "GLFW_KEY_HOME"); 
    lua_pushinteger(L, GLFW_KEY_END);
    lua_setfield(L, -2, "GLFW_KEY_END"); 
    lua_pushinteger(L, GLFW_KEY_CAPS_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_CAPS_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_SCROLL_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_SCROLL_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_NUM_LOCK);
    lua_setfield(L, -2, "GLFW_KEY_NUM_LOCK"); 
    lua_pushinteger(L, GLFW_KEY_PRINT_SCREEN);
    lua_setfield(L, -2, "GLFW_KEY_PRINT_SCREEN"); 
    lua_pushinteger(L, GLFW_KEY_PAUSE);
    lua_setfield(L, -2, "GLFW_KEY_PAUSE"); 
    lua_pushinteger(L, GLFW_KEY_F1);
    lua_setfield(L, -2, "GLFW_KEY_F1"); 
    lua_pushinteger(L, GLFW_KEY_F2);
    lua_setfield(L, -2, "GLFW_KEY_F2"); 
    lua_pushinteger(L, GLFW_KEY_F3);
    lua_setfield(L, -2, "GLFW_KEY_F3"); 
    lua_pushinteger(L, GLFW_KEY_F4);
    lua_setfield(L, -2, "GLFW_KEY_F4"); 
    lua_pushinteger(L, GLFW_KEY_F5);
    lua_setfield(L, -2, "GLFW_KEY_F5"); 
    lua_pushinteger(L, GLFW_KEY_F6);
    lua_setfield(L, -2, "GLFW_KEY_F6"); 
    lua_pushinteger(L, GLFW_KEY_F7);
    lua_setfield(L, -2, "GLFW_KEY_F7"); 
    lua_pushinteger(L, GLFW_KEY_F8);
    lua_setfield(L, -2, "GLFW_KEY_F8"); 
    lua_pushinteger(L, GLFW_KEY_F9);
    lua_setfield(L, -2, "GLFW_KEY_F9"); 
    lua_pushinteger(L, GLFW_KEY_F10);
    lua_setfield(L, -2, "GLFW_KEY_F10"); 
    lua_pushinteger(L, GLFW_KEY_F11);
    lua_setfield(L, -2, "GLFW_KEY_F11"); 
    lua_pushinteger(L, GLFW_KEY_F12);
    lua_setfield(L, -2, "GLFW_KEY_F12"); 
    lua_pushinteger(L, GLFW_KEY_F13);
    lua_setfield(L, -2, "GLFW_KEY_F13"); 
    lua_pushinteger(L, GLFW_KEY_F14);
    lua_setfield(L, -2, "GLFW_KEY_F14"); 
    lua_pushinteger(L, GLFW_KEY_F15);
    lua_setfield(L, -2, "GLFW_KEY_F15"); 
    lua_pushinteger(L, GLFW_KEY_F16);
    lua_setfield(L, -2, "GLFW_KEY_F16"); 
    lua_pushinteger(L, GLFW_KEY_F17);
    lua_setfield(L, -2, "GLFW_KEY_F17"); 
    lua_pushinteger(L, GLFW_KEY_F18);
    lua_setfield(L, -2, "GLFW_KEY_F18"); 
    lua_pushinteger(L, GLFW_KEY_F19);
    lua_setfield(L, -2, "GLFW_KEY_F19"); 
    lua_pushinteger(L, GLFW_KEY_F20);
    lua_setfield(L, -2, "GLFW_KEY_F20"); 
    lua_pushinteger(L, GLFW_KEY_F21);
    lua_setfield(L, -2, "GLFW_KEY_F21"); 
    lua_pushinteger(L, GLFW_KEY_F22);
    lua_setfield(L, -2, "GLFW_KEY_F22"); 
    lua_pushinteger(L, GLFW_KEY_F23);
    lua_setfield(L, -2, "GLFW_KEY_F23"); 
    lua_pushinteger(L, GLFW_KEY_F24);
    lua_setfield(L, -2, "GLFW_KEY_F24"); 
    lua_pushinteger(L, GLFW_KEY_F25);
    lua_setfield(L, -2, "GLFW_KEY_F25"); 
    lua_pushinteger(L, GLFW_KEY_KP_0);
    lua_setfield(L, -2, "GLFW_KEY_KP_0"); 
    lua_pushinteger(L, GLFW_KEY_KP_1);
    lua_setfield(L, -2, "GLFW_KEY_KP_1"); 
    lua_pushinteger(L, GLFW_KEY_KP_2);
    lua_setfield(L, -2, "GLFW_KEY_KP_2"); 
    lua_pushinteger(L, GLFW_KEY_KP_3);
    lua_setfield(L, -2, "GLFW_KEY_KP_3"); 
    lua_pushinteger(L, GLFW_KEY_KP_4);
    lua_setfield(L, -2, "GLFW_KEY_KP_4"); 
    lua_pushinteger(L, GLFW_KEY_KP_5);
    lua_setfield(L, -2, "GLFW_KEY_KP_5"); 
    lua_pushinteger(L, GLFW_KEY_KP_6);
    lua_setfield(L, -2, "GLFW_KEY_KP_6"); 
    lua_pushinteger(L, GLFW_KEY_KP_7);
    lua_setfield(L, -2, "GLFW_KEY_KP_7"); 
    lua_pushinteger(L, GLFW_KEY_KP_8);
    lua_setfield(L, -2, "GLFW_KEY_KP_8"); 
    lua_pushinteger(L, GLFW_KEY_KP_9);
    lua_setfield(L, -2, "GLFW_KEY_KP_9"); 
    lua_pushinteger(L, GLFW_KEY_KP_DECIMAL);
    lua_setfield(L, -2, "GLFW_KEY_KP_DECIMAL"); 
    lua_pushinteger(L, GLFW_KEY_KP_DIVIDE);
    lua_setfield(L, -2, "GLFW_KEY_KP_DIVIDE"); 
    lua_pushinteger(L, GLFW_KEY_KP_MULTIPLY);
    lua_setfield(L, -2, "GLFW_KEY_KP_MULTIPLY"); 
    lua_pushinteger(L, GLFW_KEY_KP_SUBTRACT);
    lua_setfield(L, -2, "GLFW_KEY_KP_SUBTRACT"); 
    lua_pushinteger(L, GLFW_KEY_KP_ADD);
    lua_setfield(L, -2, "GLFW_KEY_KP_ADD"); 
    lua_pushinteger(L, GLFW_KEY_KP_ENTER);
    lua_setfield(L, -2, "GLFW_KEY_KP_ENTER"); 
    lua_pushinteger(L, GLFW_KEY_KP_EQUAL);
    lua_setfield(L, -2, "GLFW_KEY_KP_EQUAL"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_SHIFT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_SHIFT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_CONTROL);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_CONTROL"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_ALT);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_ALT"); 
    lua_pushinteger(L, GLFW_KEY_LEFT_SUPER);
    lua_setfield(L, -2, "GLFW_KEY_LEFT_SUPER"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_SHIFT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_SHIFT"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_CONTROL);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_CONTROL"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_ALT);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_ALT"); 
    lua_pushinteger(L, GLFW_KEY_RIGHT_SUPER);
    lua_setfield(L, -2, "GLFW_KEY_RIGHT_SUPER"); 
    lua_pushinteger(L, GLFW_KEY_MENU);
    lua_setfield(L, -2, "GLFW_KEY_MENU"); 
    lua_pushinteger(L, GLFW_KEY_LAST);
    lua_setfield(L, -2, "GLFW_KEY_LAST"); 
    lua_pushinteger(L, GLFW_MOD_SHIFT);
    lua_setfield(L, -2, "GLFW_MOD_SHIFT"); 
    lua_pushinteger(L, GLFW_MOD_CONTROL);
    lua_setfield(L, -2, "GLFW_MOD_CONTROL"); 
    lua_pushinteger(L, GLFW_MOD_ALT);
    lua_setfield(L, -2, "GLFW_MOD_ALT"); 
    lua_pushinteger(L, GLFW_MOD_SUPER);
    lua_setfield(L, -2, "GLFW_MOD_SUPER"); 
    lua_pushinteger(L, GLFW_MOD_CAPS_LOCK);
    lua_setfield(L, -2, "GLFW_MOD_CAPS_LOCK"); 
    lua_pushinteger(L, GLFW_MOD_NUM_LOCK);
    lua_setfield(L, -2, "GLFW_MOD_NUM_LOCK"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_1);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_1"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_2);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_2"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_3);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_3"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_4);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_4"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_5);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_5"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_6);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_6"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_7);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_7"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_8);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_8"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_LAST);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_LAST"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_LEFT);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_LEFT"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_RIGHT);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_RIGHT"); 
    lua_pushinteger(L, GLFW_MOUSE_BUTTON_MIDDLE);
    lua_setfield(L, -2, "GLFW_MOUSE_BUTTON_MIDDLE"); 
    lua_pushinteger(L, GLFW_JOYSTICK_1);
    lua_setfield(L, -2, "GLFW_JOYSTICK_1"); 
    lua_pushinteger(L, GLFW_JOYSTICK_2);
    lua_setfield(L, -2, "GLFW_JOYSTICK_2"); 
    lua_pushinteger(L, GLFW_JOYSTICK_3);
    lua_setfield(L, -2, "GLFW_JOYSTICK_3"); 
    lua_pushinteger(L, GLFW_JOYSTICK_4);
    lua_setfield(L, -2, "GLFW_JOYSTICK_4"); 
    lua_pushinteger(L, GLFW_JOYSTICK_5);
    lua_setfield(L, -2, "GLFW_JOYSTICK_5"); 
    lua_pushinteger(L, GLFW_JOYSTICK_6);
    lua_setfield(L, -2, "GLFW_JOYSTICK_6"); 
    lua_pushinteger(L, GLFW_JOYSTICK_7);
    lua_setfield(L, -2, "GLFW_JOYSTICK_7"); 
    lua_pushinteger(L, GLFW_JOYSTICK_8);
    lua_setfield(L, -2, "GLFW_JOYSTICK_8"); 
    lua_pushinteger(L, GLFW_JOYSTICK_9);
    lua_setfield(L, -2, "GLFW_JOYSTICK_9"); 
    lua_pushinteger(L, GLFW_JOYSTICK_10);
    lua_setfield(L, -2, "GLFW_JOYSTICK_10"); 
    lua_pushinteger(L, GLFW_JOYSTICK_11);
    lua_setfield(L, -2, "GLFW_JOYSTICK_11"); 
    lua_pushinteger(L, GLFW_JOYSTICK_12);
    lua_setfield(L, -2, "GLFW_JOYSTICK_12"); 
    lua_pushinteger(L, GLFW_JOYSTICK_13);
    lua_setfield(L, -2, "GLFW_JOYSTICK_13"); 
    lua_pushinteger(L, GLFW_JOYSTICK_14);
    lua_setfield(L, -2, "GLFW_JOYSTICK_14"); 
    lua_pushinteger(L, GLFW_JOYSTICK_15);
    lua_setfield(L, -2, "GLFW_JOYSTICK_15"); 
    lua_pushinteger(L, GLFW_JOYSTICK_16);
    lua_setfield(L, -2, "GLFW_JOYSTICK_16"); 
    lua_pushinteger(L, GLFW_JOYSTICK_LAST);
    lua_setfield(L, -2, "GLFW_JOYSTICK_LAST"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_A);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_A"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_B);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_B"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LEFT_BUMPER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_BACK);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_BACK"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_START);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_START"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_GUIDE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_GUIDE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LEFT_THUMB);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LEFT_THUMB"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_RIGHT_THUMB"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_UP);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_UP"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_RIGHT"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_DOWN);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_DOWN"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_DPAD_LEFT);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_DPAD_LEFT"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_LAST);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_LAST"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_CROSS);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_CROSS"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_CIRCLE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_CIRCLE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_SQUARE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_SQUARE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_BUTTON_TRIANGLE);
    lua_setfield(L, -2, "GLFW_GAMEPAD_BUTTON_TRIANGLE"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_X);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_X"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_Y);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_Y"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LEFT_TRIGGER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER"); 
    lua_pushinteger(L, GLFW_GAMEPAD_AXIS_LAST);
    lua_setfield(L, -2, "GLFW_GAMEPAD_AXIS_LAST"); 
    lua_pushinteger(L, GLFW_NO_ERROR);
    lua_setfield(L, -2, "GLFW_NO_ERROR"); 
    lua_pushinteger(L, GLFW_NOT_INITIALIZED);
    lua_setfield(L, -2, "GLFW_NOT_INITIALIZED"); 
    lua_pushinteger(L, GLFW_NO_CURRENT_CONTEXT);
    lua_setfield(L, -2, "GLFW_NO_CURRENT_CONTEXT"); 
    lua_pushinteger(L, GLFW_INVALID_ENUM);
    lua_setfield(L, -2, "GLFW_INVALID_ENUM"); 
    lua_pushinteger(L, GLFW_INVALID_VALUE);
    lua_setfield(L, -2, "GLFW_INVALID_VALUE"); 
    lua_pushinteger(L, GLFW_OUT_OF_MEMORY);
    lua_setfield(L, -2, "GLFW_OUT_OF_MEMORY"); 
    lua_pushinteger(L, GLFW_API_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_API_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_VERSION_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_VERSION_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_PLATFORM_ERROR);
    lua_setfield(L, -2, "GLFW_PLATFORM_ERROR"); 
    lua_pushinteger(L, GLFW_FORMAT_UNAVAILABLE);
    lua_setfield(L, -2, "GLFW_FORMAT_UNAVAILABLE"); 
    lua_pushinteger(L, GLFW_NO_WINDOW_CONTEXT);
    lua_setfield(L, -2, "GLFW_NO_WINDOW_CONTEXT"); 
    lua_pushinteger(L, GLFW_FOCUSED);
    lua_setfield(L, -2, "GLFW_FOCUSED"); 
    lua_pushinteger(L, GLFW_ICONIFIED);
    lua_setfield(L, -2, "GLFW_ICONIFIED"); 
    lua_pushinteger(L, GLFW_RESIZABLE);
    lua_setfield(L, -2, "GLFW_RESIZABLE"); 
    lua_pushinteger(L, GLFW_VISIBLE);
    lua_setfield(L, -2, "GLFW_VISIBLE"); 
    lua_pushinteger(L, GLFW_DECORATED);
    lua_setfield(L, -2, "GLFW_DECORATED"); 
    lua_pushinteger(L, GLFW_AUTO_ICONIFY);
    lua_setfield(L, -2, "GLFW_AUTO_ICONIFY"); 
    lua_pushinteger(L, GLFW_FLOATING);
    lua_setfield(L, -2, "GLFW_FLOATING"); 
    lua_pushinteger(L, GLFW_MAXIMIZED);
    lua_setfield(L, -2, "GLFW_MAXIMIZED"); 
    lua_pushinteger(L, GLFW_CENTER_CURSOR);
    lua_setfield(L, -2, "GLFW_CENTER_CURSOR"); 
    lua_pushinteger(L, GLFW_TRANSPARENT_FRAMEBUFFER);
    lua_setfield(L, -2, "GLFW_TRANSPARENT_FRAMEBUFFER"); 
    lua_pushinteger(L, GLFW_HOVERED);
    lua_setfield(L, -2, "GLFW_HOVERED"); 
    lua_pushinteger(L, GLFW_FOCUS_ON_SHOW);
    lua_setfield(L, -2, "GLFW_FOCUS_ON_SHOW"); 
    lua_pushinteger(L, GLFW_RED_BITS);
    lua_setfield(L, -2, "GLFW_RED_BITS"); 
    lua_pushinteger(L, GLFW_GREEN_BITS);
    lua_setfield(L, -2, "GLFW_GREEN_BITS"); 
    lua_pushinteger(L, GLFW_BLUE_BITS);
    lua_setfield(L, -2, "GLFW_BLUE_BITS"); 
    lua_pushinteger(L, GLFW_ALPHA_BITS);
    lua_setfield(L, -2, "GLFW_ALPHA_BITS"); 
    lua_pushinteger(L, GLFW_DEPTH_BITS);
    lua_setfield(L, -2, "GLFW_DEPTH_BITS"); 
    lua_pushinteger(L, GLFW_STENCIL_BITS);
    lua_setfield(L, -2, "GLFW_STENCIL_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_RED_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_RED_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_GREEN_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_GREEN_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_BLUE_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_BLUE_BITS"); 
    lua_pushinteger(L, GLFW_ACCUM_ALPHA_BITS);
    lua_setfield(L, -2, "GLFW_ACCUM_ALPHA_BITS"); 
    lua_pushinteger(L, GLFW_AUX_BUFFERS);
    lua_setfield(L, -2, "GLFW_AUX_BUFFERS"); 
    lua_pushinteger(L, GLFW_STEREO);
    lua_setfield(L, -2, "GLFW_STEREO"); 
    lua_pushinteger(L, GLFW_SAMPLES);
    lua_setfield(L, -2, "GLFW_SAMPLES"); 
    lua_pushinteger(L, GLFW_SRGB_CAPABLE);
    lua_setfield(L, -2, "GLFW_SRGB_CAPABLE"); 
    lua_pushinteger(L, GLFW_REFRESH_RATE);
    lua_setfield(L, -2, "GLFW_REFRESH_RATE"); 
    lua_pushinteger(L, GLFW_DOUBLEBUFFER);
    lua_setfield(L, -2, "GLFW_DOUBLEBUFFER"); 
    lua_pushinteger(L, GLFW_CLIENT_API);
    lua_setfield(L, -2, "GLFW_CLIENT_API"); 
    lua_pushinteger(L, GLFW_CONTEXT_VERSION_MAJOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_VERSION_MAJOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_VERSION_MINOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_VERSION_MINOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_REVISION);
    lua_setfield(L, -2, "GLFW_CONTEXT_REVISION"); 
    lua_pushinteger(L, GLFW_CONTEXT_ROBUSTNESS);
    lua_setfield(L, -2, "GLFW_CONTEXT_ROBUSTNESS"); 
    lua_pushinteger(L, GLFW_OPENGL_FORWARD_COMPAT);
    lua_setfield(L, -2, "GLFW_OPENGL_FORWARD_COMPAT"); 
    lua_pushinteger(L, GLFW_OPENGL_DEBUG_CONTEXT);
    lua_setfield(L, -2, "GLFW_OPENGL_DEBUG_CONTEXT"); 
    lua_pushinteger(L, GLFW_OPENGL_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_PROFILE"); 
    lua_pushinteger(L, GLFW_CONTEXT_RELEASE_BEHAVIOR);
    lua_setfield(L, -2, "GLFW_CONTEXT_RELEASE_BEHAVIOR"); 
    lua_pushinteger(L, GLFW_CONTEXT_NO_ERROR);
    lua_setfield(L, -2, "GLFW_CONTEXT_NO_ERROR"); 
    lua_pushinteger(L, GLFW_CONTEXT_CREATION_API);
    lua_setfield(L, -2, "GLFW_CONTEXT_CREATION_API"); 
    lua_pushinteger(L, GLFW_SCALE_TO_MONITOR);
    lua_setfield(L, -2, "GLFW_SCALE_TO_MONITOR"); 
    lua_pushinteger(L, GLFW_COCOA_RETINA_FRAMEBUFFER);
    lua_setfield(L, -2, "GLFW_COCOA_RETINA_FRAMEBUFFER"); 
    lua_pushinteger(L, GLFW_COCOA_FRAME_NAME);
    lua_setfield(L, -2, "GLFW_COCOA_FRAME_NAME"); 
    lua_pushinteger(L, GLFW_COCOA_GRAPHICS_SWITCHING);
    lua_setfield(L, -2, "GLFW_COCOA_GRAPHICS_SWITCHING"); 
    lua_pushinteger(L, GLFW_X11_CLASS_NAME);
    lua_setfield(L, -2, "GLFW_X11_CLASS_NAME"); 
    lua_pushinteger(L, GLFW_X11_INSTANCE_NAME);
    lua_setfield(L, -2, "GLFW_X11_INSTANCE_NAME"); 
    lua_pushinteger(L, GLFW_NO_API);
    lua_setfield(L, -2, "GLFW_NO_API"); 
    lua_pushinteger(L, GLFW_OPENGL_API);
    lua_setfield(L, -2, "GLFW_OPENGL_API"); 
    lua_pushinteger(L, GLFW_OPENGL_ES_API);
    lua_setfield(L, -2, "GLFW_OPENGL_ES_API"); 
    lua_pushinteger(L, GLFW_NO_ROBUSTNESS);
    lua_setfield(L, -2, "GLFW_NO_ROBUSTNESS"); 
    lua_pushinteger(L, GLFW_NO_RESET_NOTIFICATION);
    lua_setfield(L, -2, "GLFW_NO_RESET_NOTIFICATION"); 
    lua_pushinteger(L, GLFW_LOSE_CONTEXT_ON_RESET);
    lua_setfield(L, -2, "GLFW_LOSE_CONTEXT_ON_RESET"); 
    lua_pushinteger(L, GLFW_OPENGL_ANY_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_ANY_PROFILE"); 
    lua_pushinteger(L, GLFW_OPENGL_CORE_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_CORE_PROFILE"); 
    lua_pushinteger(L, GLFW_OPENGL_COMPAT_PROFILE);
    lua_setfield(L, -2, "GLFW_OPENGL_COMPAT_PROFILE"); 
    lua_pushinteger(L, GLFW_CURSOR);
    lua_setfield(L, -2, "GLFW_CURSOR"); 
    lua_pushinteger(L, GLFW_STICKY_KEYS);
    lua_setfield(L, -2, "GLFW_STICKY_KEYS"); 
    lua_pushinteger(L, GLFW_STICKY_MOUSE_BUTTONS);
    lua_setfield(L, -2, "GLFW_STICKY_MOUSE_BUTTONS"); 
    lua_pushinteger(L, GLFW_LOCK_KEY_MODS);
    lua_setfield(L, -2, "GLFW_LOCK_KEY_MODS"); 
    lua_pushinteger(L, GLFW_RAW_MOUSE_MOTION);
    lua_setfield(L, -2, "GLFW_RAW_MOUSE_MOTION"); 
    lua_pushinteger(L, GLFW_CURSOR_NORMAL);
    lua_setfield(L, -2, "GLFW_CURSOR_NORMAL"); 
    lua_pushinteger(L, GLFW_CURSOR_HIDDEN);
    lua_setfield(L, -2, "GLFW_CURSOR_HIDDEN"); 
    lua_pushinteger(L, GLFW_CURSOR_DISABLED);
    lua_setfield(L, -2, "GLFW_CURSOR_DISABLED"); 
    lua_pushinteger(L, GLFW_ANY_RELEASE_BEHAVIOR);
    lua_setfield(L, -2, "GLFW_ANY_RELEASE_BEHAVIOR"); 
    lua_pushinteger(L, GLFW_RELEASE_BEHAVIOR_FLUSH);
    lua_setfield(L, -2, "GLFW_RELEASE_BEHAVIOR_FLUSH"); 
    lua_pushinteger(L, GLFW_RELEASE_BEHAVIOR_NONE);
    lua_setfield(L, -2, "GLFW_RELEASE_BEHAVIOR_NONE"); 
    lua_pushinteger(L, GLFW_NATIVE_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_NATIVE_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_EGL_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_EGL_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_OSMESA_CONTEXT_API);
    lua_setfield(L, -2, "GLFW_OSMESA_CONTEXT_API"); 
    lua_pushinteger(L, GLFW_ARROW_CURSOR);
    lua_setfield(L, -2, "GLFW_ARROW_CURSOR"); 
    lua_pushinteger(L, GLFW_IBEAM_CURSOR);
    lua_setfield(L, -2, "GLFW_IBEAM_CURSOR"); 
    lua_pushinteger(L, GLFW_CROSSHAIR_CURSOR);
    lua_setfield(L, -2, "GLFW_CROSSHAIR_CURSOR"); 
    lua_pushinteger(L, GLFW_HRESIZE_CURSOR);
    lua_setfield(L, -2, "GLFW_HRESIZE_CURSOR"); 
    lua_pushinteger(L, GLFW_VRESIZE_CURSOR);
    lua_setfield(L, -2, "GLFW_VRESIZE_CURSOR"); 
    lua_pushinteger(L, GLFW_HAND_CURSOR);
    lua_setfield(L, -2, "GLFW_HAND_CURSOR"); 
    lua_pushinteger(L, GLFW_CONNECTED);
    lua_setfield(L, -2, "GLFW_CONNECTED"); 
    lua_pushinteger(L, GLFW_DISCONNECTED);
    lua_setfield(L, -2, "GLFW_DISCONNECTED"); 
    lua_pushinteger(L, GLFW_JOYSTICK_HAT_BUTTONS);
    lua_setfield(L, -2, "GLFW_JOYSTICK_HAT_BUTTONS"); 
    lua_pushinteger(L, GLFW_COCOA_CHDIR_RESOURCES);
    lua_setfield(L, -2, "GLFW_COCOA_CHDIR_RESOURCES"); 
    lua_pushinteger(L, GLFW_COCOA_MENUBAR);
    lua_setfield(L, -2, "GLFW_COCOA_MENUBAR"); 
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
