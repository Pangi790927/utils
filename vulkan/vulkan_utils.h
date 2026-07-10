#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

/*! TODO:
 * 
 * + the descriptor mess must be handled somehow
 * + composer must get the changes
 * + composer must be able to pass around vk_layout as parameter in lua, somehow
 * + some structs must be added to composer, as such it will make things easyer (all those things
 * that have more than one handle are suspicious, in particular pipeline_t)
 * + some structs must be added to yaml parser of composer (not sure how or if I really should)
 * + sync primitives must be added to vku and vkc
 * + some way to rebuild the resize window would be nice
 * - search for all "TODO:" in virt_composer.h/cpp vulkan_composer.h vulkan_utils.h
 *   - Create an ImGui backend using this helper
 *   - Add the "#include ..." macro for shaders and test if the rest work as expected
 *   - Add the option to use multiple include dirs for shader compilation
 *   - this needs to be implemented in a newer version of vulkan, tested and as such
 *      inline std::string to_string(VkPipelineStageFlagBits2 flags);
 *      inline std::string to_string(VkAccessFlagBits2 flags);
 *   - pipeline Layouts should have multiple sets that can be bound
 */

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_NONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "debug.h"
#include "demangle.h"
#include "cpp_backtrace.h"
#include "virt_composer.h"

#if __has_include(<glslang/Include/glslang_c_interface.h>)
# define VKU_HAS_NEW_GLSLANG
# include <glslang/Include/glslang_c_interface.h>
// # include <glslang/Public/resource_limits_c.h>
#else
# include <glslang/SPIRV/GlslangToSpv.h>
#endif 

#include <vulkan/vulkan.h>
#include <exception>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#ifdef VULKAN_UTILS_ADD_TYPE
# error "VULKAN_UTILS_ADD_TYPE already defined"
#endif

#define VK_ASSERT(fn_call)                                                                         \
do {                                                                                               \
    VkResult vk_err = VkResult(fn_call);                                                           \
    if (vk_err != VK_SUCCESS) {                                                                    \
        DBG("Failed vk assert: [%s: %d] %s", vk_err_cstr(vk_err), vk_err, #fn_call);               \
        throw vulkan_utils::except_t(vk_err);                                                      \
    }                                                                                              \
} while (false);

enum vku_shader_stage_e {
    VKU_SPIRV_VERTEX,
    VKU_SPIRV_FRAGMENT,
    VKU_SPIRV_COMPUTE,
    VKU_SPIRV_GEOMETRY,
    VKU_SPIRV_TESS_CTRL,
    VKU_SPIRV_TESS_EVAL,
};

namespace vulkan_utils {

namespace vo = virt_object;
namespace vc = virt_composer;
namespace vku = vulkan_utils;

/* This is a common type enumeration for all the types that can be derived from vku_object_t */
using object_type_e = vc::object_type_e;


/* Those are the types from this file, this file promises not to invalidate the counter, you can
use it later on. */
/* object_t is pure virtual, so no object should have this type */

VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_BINDING_DESC);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DEPENDENCY_INFO);
// VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DEPENDENCY_INFO2);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_IMAGE_SUBRESOURCE_RANGE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_OBJECT);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_WINDOW);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_INSTANCE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_SURFACE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DEVICE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_SWAPCHAIN);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_SHADER);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_RENDERPASS);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_PIPELINE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_COMPUTE_PIPELINE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_FRAMEBUFFERS);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_COMMAND_POOL);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_COMMAND_BUFFER);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_SEMAPHORE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_EVENT);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_FENCE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_BUFFER);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_IMAGE);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_IMAGE_VIEW);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_IMAGE_SAMPLER);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_PIPELINE_LAYOUT);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DESCRIPTOR_SET);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DESCRIPTOR_SET_LAYOUT);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DESCRIPTOR_POOL);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_SAMPLER_BINDING);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_BUFFER_BINDING);
VIRT_COMPOSER_REGISTER_TYPE(VKU_TYPE_DESCRIPTOR_SET_INITIALIZER);

struct gpu_family_ids_t;
struct spirv_t;

struct vertex_input_desc_t;
struct vertex_p2n0c3t2_t;
struct vertex_p3n3c3t2_t;

struct desc_set_initializer_t;
struct mvp_t;
struct ubo_t;
struct ssbo_t;

using object_t = vc::object_t;

template <typename VkuT>
using ref_t = vc::ref_t<VkuT>;

struct instance_t;          /* uses (opts) */
struct surface_t;           /* uses (instance) */
struct device_t;            /* uses (surface) */
struct swapchain_t;         /* uses (device) */
struct shader_t;            /* uses (device, shader_data) */
struct renderpass_t;        /* uses (swapchain) */
struct desc_set_layout_t;   /* uses (desc_set_initializer, device) */
struct pipeline_layout_t;   /* uses (desc_set_layout) */
struct pipeline_t;          /* uses (opts, renderpass, shaders, vertex_input_desc, pipeline_layout)*/
struct framebuffs_t;        /* uses (renderpass) */
struct cmdpool_t;           /* uses (device) */
struct cmdbuff_t;           /* uses (cmdpool) */
struct sem_t;               /* uses (device) */
struct fence_t;             /* uses (device) */
struct event_t;             /* uses (device) */
struct buffer_t;            /* uses (device) */
struct image_t;             /* uses (device) */
struct img_view_t;          /* uses (imag) */
struct img_sampl_t;         /* uses (device) */
struct desc_pool_t;         /* uses (device, ?buff?, ?pipeline?) */
struct desc_set_t;          /* uses (desc_pool) */

inline vc::ret_t init();
inline vc::ret_t uninit();

inline void wait_fences(std::vector<ref_t<fence_t>> fences, bool wait_all, uint64_t timeout);
inline void reset_fences(std::vector<ref_t<fence_t>> fences);

void wait_semaphores(const std::vector<ref_t<sem_t>> &sems, const std::vector<uint64_t> vals,
            bool wait_any = false, uint64_t timeo_ns = -1);

inline void aquire_next_img(
        ref_t<swapchain_t> swc,
        ref_t<sem_t> sem,
        uint32_t *img_idx);

inline void submit_cmdbuff(
        std::vector<std::pair<ref_t<sem_t>, VkPipelineStageFlagBits>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<ref_t<sem_t>> sig_sems,
        uint32_t queue_id = 0);

/* specialization for timeline semaphore extension */
inline void submit_cmdbuff_tl(
        std::vector<std::tuple<ref_t<sem_t>, VkPipelineStageFlagBits, uint64_t>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<std::tuple<ref_t<sem_t>, uint64_t>> sig_sems,
        uint32_t queue_id = 0);

inline void present(
        ref_t<swapchain_t> swc,
        std::vector<ref_t<sem_t>> wait_sems,
        uint32_t img_idx);

/* if no command buffer is provided, one will be allocated from the command pool */
inline void copy_buff(
        ref_t<cmdpool_t> cp,
        ref_t<buffer_t> dst,
        ref_t<buffer_t> src,
        VkDeviceSize sz,
        ref_t<cmdbuff_t> /* optional */ cbuff);

vc::ref_t<vku::image_t> load_image(vc::ref_t<vku::cmdpool_t> cp, std::string path);

/* To string for own enums: */
inline std::string to_string(vku_shader_stage_e stage);
inline std::string to_string(const vertex_input_desc_t& input_desc);

/* To string for external types */
inline std::string to_string(VkVertexInputRate rate);
inline std::string to_string(VkFormat format);
inline std::string to_string(VkPrimitiveTopology topol);
inline std::string to_string(VkSharingMode shmod);
inline std::string to_string(VkFilter shmod);
inline std::string to_string(VkDescriptorType dtype);
inline std::string to_string(const VkDescriptorSetLayoutBinding& bind);
inline std::string to_string(VkImageLayout lay);

/* To string for external flags */
/* OBS: You can't use those directly because the actual values are in the form VkFlags not
VkFlagBits */
inline std::string to_string(VkFenceCreateFlagBits flags);
inline std::string to_string(VkBufferUsageFlagBits flags);
inline std::string to_string(VkMemoryPropertyFlagBits flags);
inline std::string to_string(VkImageUsageFlagBits flags);
inline std::string to_string(VkImageAspectFlagBits flags);
inline std::string to_string(VkShaderStageFlagBits flags);
inline std::string to_string(VkPipelineStageFlagBits flags);
inline std::string to_string(VkAccessFlagBits flags);
inline std::string to_string(VkDependencyFlagBits flags);
inline std::string to_string(VkQueueFlagBits flags);

/* VKU Objects:
================================================================================================= */

/* Those are needed here just for bellow objects */
inline const char *vk_err_cstr(vc::ret_t res);
inline std::string vk_err_str(vc::ret_t res);
inline std::string glfw_err();


struct gpu_family_ids_t {
    uint32_t max_graphics_queue_cnt = 0;
    union {
        int graphics_id = -1;   /* same as compute id */
        int compute_id;
    };
    int present_id = -1;
};

struct spirv_t {
    std::vector<uint32_t> content;
    vku_shader_stage_e type;
};

struct vertex_input_desc_t {
    VkVertexInputBindingDescription                 bind_desc;
    std::vector<VkVertexInputAttributeDescription>  attr_desc;
};

struct vertex_p2n0c3t2_t {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 tex;

    static vertex_input_desc_t get_input_desc();
};

struct vertex_p3n3c3t2_t {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 tex;

    static vertex_input_desc_t get_input_desc();
};

struct mvp_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/* Uniform Buffer Object */
struct ubo_t {
    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding,
            VkShaderStageFlags stage);
};

/* Shader Storage Buffer Object */
struct ssbo_t {
    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding,
            VkShaderStageFlags stage);
};

using vertex2d_t = vertex_p2n0c3t2_t;
using vertex3d_t = vertex_p3n3c3t2_t;

/*!
 * 
 * vkc::binding_t
 * --------------
 *
 * Description: Represents a generic descriptor set binding in Vulkan.
 * This object wraps a VkDescriptorSetLayoutBinding structure, which defines 
 * the binding index, descriptor type, number of descriptors, and shader stage flags.
 * It's main purpose is to keep it around in Lua and reference it in C++ programs.
 *
 * Init: create(bd)
 *   - Parameters:
 *     - bd: A VkDescriptorSetLayoutBinding structure describing the binding.
 *
 * Notes:
 * - Serves as a base or standalone representation for descriptor set bindings.
 * - Can be used to initialize more specific binding types such as buffer or 
 *   sampler bindings.
 * 
 */
struct binding_t : public object_t {
    VkDescriptorSetLayoutBinding bd;

    binding_t(object_t::Private priv) : object_t(priv) {}
    virtual ~binding_t() {}

    static vku::object_type_e type_id_static() { return VKU_TYPE_BINDING_DESC; }
    virtual vku::object_type_e type_id() const override { return VKU_TYPE_BINDING_DESC; }

    static vku::ref_t<binding_t> create(const VkDescriptorSetLayoutBinding& bd);
    inline std::string to_string() const override;
};

struct dependency_info_t : public object_t {
    VkPipelineStageFlags m_src_stage_mask;
    VkPipelineStageFlags m_dst_stage_mask;
    VkDependencyFlags m_dep_flags;

    std::vector<VkMemoryBarrier> mem_bars;
    std::vector<VkBufferMemoryBarrier> buff_mem_bars;
    std::vector<VkImageMemoryBarrier> img_mem_bars;

    dependency_info_t(object_t::Private priv) : object_t(priv) {}
    virtual ~dependency_info_t() {}

    static vku::object_type_e type_id_static() { return VKU_TYPE_DEPENDENCY_INFO; }
    virtual vku::object_type_e type_id() const override { return VKU_TYPE_DEPENDENCY_INFO; }

    static vku::ref_t<dependency_info_t> create(
            VkPipelineStageFlagBits src_stage_mask,
            VkPipelineStageFlagBits dst_stage_mask,
            const std::vector<VkMemoryBarrier> &mem_bars,
            const std::vector<VkBufferMemoryBarrier> &buff_mem_bars,
            const std::vector<VkImageMemoryBarrier> &img_mem_bars,
            VkDependencyFlags dep_flags = 0);

    inline std::string to_string() const override;
};


/* TODO: this needs to be implemented in a newer version of vulkan, tested and as such */
/* TODO: description
    OBS: there exists only VkDependencyInfo, but all the associated types have a 2, as such this
    struct will also have a 2 */
/* TODO: add it in composer */
/* TODO: add macro include guards for vk1.3 versioning */
// struct dependency_info2_t : public vku::object_t {
//     VkDependencyInfo dep_info;
//     std::vector<VkMemoryBarrier2> mem_bars;
//     std::vector<VkBufferMemoryBarrier2> buff_mem_bars;
//     std::vector<VkImageMemoryBarrier2> img_mem_bars;

//     static vku::object_type_e type_id_static() { return VKU_TYPE_DEPENDENCY_INFO2; }
//     virtual vku::object_type_e type_id() const override { return VKU_TYPE_DEPENDENCY_INFO2; }

//     static vku::ref_t<dependency_info2_t> create(
//             const VkDependencyInfo& dep_info,
//             const std::vector<VkMemoryBarrier2> &mem_bars,
//             const std::vector<VkBufferMemoryBarrier2> &buff_mem_bars,
//             const std::vector<VkImageMemoryBarrier2> &img_mem_bars);

//     virtual vc::ret_t init() override { return (update_ptrs(), VK_SUCCESS); }
//     virtual vc::ret_t uninit() override { return VK_SUCCESS; }

//     inline std::string to_string() const override;

//     VkDependencyInfo *get_dep() { return &dep_info; }
//     void update_ptrs();
// };

/* TODO: desc */
struct image_subresource_range_t : public object_t {
    VkImageSubresourceRange m_img_subrange;

    image_subresource_range_t(object_t::Private priv) : object_t(priv) {}
    virtual ~image_subresource_range_t() {}

    static vku::object_type_e type_id_static() { return VKU_TYPE_IMAGE_SUBRESOURCE_RANGE; }
    virtual vku::object_type_e type_id() const override { return VKU_TYPE_IMAGE_SUBRESOURCE_RANGE; }

    static vku::ref_t<image_subresource_range_t> create(const VkImageSubresourceRange& img_subrange);

    inline std::string to_string() const override;
};

/*!
 * vku::window_t
 * -------------
 *
 * Description: Represents a GLFW window. This object manages a platform-specific window
 * and serves as the target for Vulkan rendering. It allows resizing, title changes, and
 * provides access to the underlying GLFWwindow* for integration with other libraries.
 *
 * Members:
 * - m_name: Window title.
 * - m_width, m_height: Window dimensions.
 *
 * Init: create(width, height, name)
 *   - Parameters:
 *     - width: Initial width of the window (default: 800).
 *     - height: Initial height of the window (default: 600).
 *     - name: Title of the window (default: "vku::window_name_placeholder").
 */
struct window_t : public object_t {
    /* Those can be modified at any time, but they need a rebuild to actually take effect (see
    ref_t::rebuild()) */
    std::string m_name;
    int m_width;
    int m_height;

    window_t(object_t::Private priv) : object_t(priv) {}
    virtual ~window_t() { uninit(); }

    static ref_t<window_t> create(int width = 800, int height = 600,
            std::string name = "vku::window_name_placeholder");

    virtual object_type_e type_id() const override { return VKU_TYPE_WINDOW; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_WINDOW; }
    GLFWwindow *get_window() const { return _window; }

private:
    vc::ret_t init();
    vc::ret_t uninit();

    GLFWwindow *_window = NULL;
};

/*!
 * vku::instance_t
 * ---------------
 *
 * Description: Represents a Vulkan instance. A Vulkan instance is the foundational 
 * object that initializes the Vulkan library for a specific application. It manages 
 * the connection between the application and the Vulkan runtime, and it enables 
 * creation of devices, surfaces, and other Vulkan objects. This object also supports 
 * optional debug layers for development and validation.
 *
 * Members:
 * - m_app_name: Name of the application. Used for debugging and identification.
 * - m_engine_name: Name of the engine. Used for debugging and identification.
 * - m_extensions: List of Vulkan extensions to enable on creation.
 * - m_layers: List of Vulkan layers (such as validation layers) to enable.
 *
 * Init: create(app_name, engine_name, extensions, layers)
 *   - Parameters:
 *     - app_name: Name of the application (default: "vku::app_name_placeholder").
 *     - engine_name: Name of the engine (default: "vku::engine_name_placeholder").
 *     - extensions: Vector of extension names to enable (default: { "VK_EXT_debug_utils" }).
 *     - layers: Vector of layer names to enable (default: { "VK_LAYER_KHRONOS_validation" }).
 *
 * Notes:
 * - Debug layers can be optionally enabled to catch errors and warnings during development.
 */
struct instance_t : public object_t {
    VkInstance                  vk_instance;
    VkDebugUtilsMessengerEXT    vk_dbg_messenger;

    std::string                 m_app_name;
    std::string                 m_engine_name;
    std::vector<std::string>    m_extensions;
    std::vector<std::string>    m_layers;

    instance_t(object_t::Private priv) : object_t(priv) {}
    virtual ~instance_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_INSTANCE; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_INSTANCE; }
    static ref_t<instance_t> create(
            const std::string app_name = "vku::app_name_placeholder",
            const std::string engine_name = "vku::engine_name_placeholder",
            const std::vector<std::string>& extensions = { "VK_EXT_debug_utils" },
            const std::vector<std::string>& layers = { "VK_LAYER_KHRONOS_validation" });

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::surface_t
 * --------------
 *
 * Description: Wraps a Vulkan surface. A Vulkan surface is an abstraction that allows 
 * rendering to be presented to a window system. This object manages the relationship 
 * between a Vulkan instance and a platform-specific window (GLFW in this case). 
 * It handles creation and destruction of the VkSurfaceKHR handle and ensures that 
 * the surface is properly associated with the correct window and Vulkan instance.
 *
 * Members:
 * - m_window: Reference to a window_t object. This is the window that the surface 
 *   is associated with. The surface will present images to this window.
 * - m_instance: Reference to an instance_t object. The Vulkan instance that created 
 *   and manages this surface.
 *
 * Init: create(window, instance)
 *   - Parameters:
 *     - window: A reference to a window_t object to associate the surface with.
 *     - instance: A reference to an instance_t object used to create the surface.
 *   - Returns: A reference-counted surface_t object with a valid VkSurfaceKHR handle.
 */
struct surface_t : public object_t {
    VkSurfaceKHR        vk_surface = NULL;

    ref_t<window_t>     m_window;
    ref_t<instance_t>   m_instance;

    surface_t(object_t::Private priv) : object_t(priv) {}
    virtual ~surface_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_SURFACE; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_SURFACE; }
    static ref_t<surface_t> create(ref_t<window_t> window, ref_t<instance_t> inst);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::device_t
 * -------------
 *
 * Description: Represents a Vulkan logical device. This object abstracts a physical 
 * GPU and provides access to queues for graphics and presentation. It is used to 
 * create buffers, images, pipelines, and other Vulkan resources.
 *
 * Members:
 * - m_surface: Reference to a surface_t object. The surface used for presentation and 
 *   swapchain creation.
 *
 * Init: create(surface)
 *   - Parameters:
 *     - surface: A reference to a surface_t object that this device will render to.
 *
 * Notes:
 * - Automatically selects suitable graphics and presentation queues.
 * - Provides access to device-local and host-visible memory through buffer objects.
 * 
 * TODO:
 * - Add options for selecting the phys dev (no idea what exactly to do)
 */
struct device_t : public object_t {
    VkPhysicalDevice            vk_phy_dev;
    VkDevice                    vk_dev;
    std::vector<VkQueue>        vk_graphics_que;
    VkQueue                     vk_present_que;
    std::set<int>               m_que_ids;
    gpu_family_ids_t            m_que_fams;

    ref_t<instance_t>           m_instance;
    ref_t<surface_t>            m_surface;
    std::vector<std::string>    m_extensions;
    std::vector<std::string>    m_layers;

    device_t(object_t::Private priv) : object_t(priv) {}
    virtual ~device_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_DEVICE; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_DEVICE; }
    static ref_t<device_t> create(ref_t<instance_t> inst, ref_t<surface_t> surf = nullptr,
            std::vector<std::string> exts = {},
            std::vector<std::string> layers = {});

    uint32_t get_graphics_queue_cnt() { return vk_graphics_que.size(); }

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::swapchain_t
 * ----------------
 *
 * Description: Wraps a Vulkan swapchain. A swapchain manages a set of images that 
 * are presented to a window in a controlled manner. This object handles creation 
 * of the VkSwapchainKHR, its images, and associated image views.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device used to create and manage 
 *   the swapchain.
 * - m_depth_imag: Reference to a depth image used for depth testing (automatically created).
 * - m_depth_view: Reference to an image view for the depth image (automatically created).
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object.
 *
 * Notes:
 * - Automatically chooses the surface format, present mode, and image count.
 * - Provides access to the swapchain images and their views for rendering.
 * 
 * TODO:
 * - more init hints?
 */
struct swapchain_t : public object_t {
    VkSurfaceFormatKHR          vk_surf_fmt;
    VkPresentModeKHR            vk_present_mode;
    VkExtent2D                  vk_extent;
    VkSwapchainKHR              vk_swapchain;
    std::vector<VkImage>        vk_sc_images;
    std::vector<VkImageView>    vk_sc_image_views;
    ref_t<image_t>              m_depth_imag;
    ref_t<img_view_t>           m_depth_view;

    ref_t<device_t>             m_device;
    ref_t<surface_t>            m_surface;

    swapchain_t(object_t::Private priv) : object_t(priv) {}
    virtual ~swapchain_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_SWAPCHAIN; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_SWAPCHAIN; }
    static ref_t<swapchain_t> create(ref_t<device_t> dev, ref_t<surface_t> surf);

    vc::ret_t init();
    vc::ret_t uninit();

    uint32_t img_count() { return vk_sc_images.size(); }
};

/*!
 * vku::shader_t
 * -------------
 *
 * Description: Wraps a Vulkan shader module. This object represents a compiled 
 * shader in SPIR-V format and can be used in graphics or compute pipelines. It 
 * supports initialization from a SPIR-V object or directly from a precompiled file.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this shader.
 * - m_type: Shader stage (vertex, fragment, compute, etc.).
 * - m_spirv: Reference to a spirv_t object containing the compiled SPIR-V code.
 * - m_path: Path to the shader file (used if initialized from file).
 * - m_init_from_path: Flag indicating whether the shader was initialized from a file.
 *
 * Init:
 * - create(device, spirv): Initialize from a spirv_t object.
 * - create(device, path, type): Initialize from a compiled shader file.
 *
 * Notes:
 * - For graphics pipelines, shaders must match the pipeline’s stage requirements.
 */
struct shader_t : public object_t {
    VkShaderModule      vk_shader;

    bool                m_init_from_path; /* implicit param */
    std::string         m_path = "not-initialized-from-path";
    vku_shader_stage_e  m_type;

    ref_t<device_t>     m_device;
    spirv_t             m_spirv;

    shader_t(object_t::Private priv) : object_t(priv) {}
    virtual ~shader_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_SHADER; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_SHADER; }
    /* not init from path */
    static ref_t<shader_t> create(ref_t<device_t> dev, const spirv_t& spirv);

    /* init from path */
    /* Obs: loads shader in binary format, i.e. already compiled */
    static ref_t<shader_t> create(ref_t<device_t> dev, const char *path, vku_shader_stage_e type);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::renderpass_t
 * -----------------
 *
 * Description: Wraps a Vulkan render pass. A render pass defines how framebuffer 
 * attachments are used during rendering, including their load/store operations 
 * and the subpass dependencies. This object manages the creation of a VkRenderPass 
 * for a given swapchain.
 *
 * Members:
 * - m_swapchain: Reference to a swapchain_t object. The swapchain whose images 
 *   will be rendered into using this render pass.
 *
 * Init: create(swc)
 *   - Parameters:
 *     - swc: Reference to a swapchain_t object that will provide the framebuffer images.
 *
 * Notes:
 * - Handles attachment descriptions, subpass definitions, and dependencies automatically.
 */
struct renderpass_t : public object_t {
    VkRenderPass        vk_render_pass;

    ref_t<swapchain_t>  m_swapchain;

    renderpass_t(object_t::Private priv) : object_t(priv) {}
    virtual ~renderpass_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_RENDERPASS; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_RENDERPASS; }
    static ref_t<renderpass_t> create(ref_t<swapchain_t> swc);

    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::pipeline_t
 * ---------------
 *
 * Description: Wraps a Vulkan graphics pipeline. This object encapsulates the entire 
 * pipeline state, including shaders, vertex input, topology, viewport, rasterization, 
 * and descriptor set bindings. It is used for rendering commands submitted to a 
 * command buffer.
 *
 * Members:
 * - m_renderpass: Reference to a renderpass_t object. The render pass this pipeline 
 *   will be used with.
 * - m_shaders: Vector of references to shader_t objects. The shaders used in the 
 *   pipeline stages.
 * - m_topology: Primitive topology (triangle list, line list, etc.).
 * - m_input_desc: Vertex input description (binding, attributes, stride, input rate).
 * - m_bindings_initer: Reference to a desc_set_initializer_t object. Descriptor sets used 
 *   by the pipeline.
 * - m_width, m_height: Pipeline viewport dimensions.
 *
 * Init: create(width, height, renderpass, shaders, topology, input_desc, bindings)
 *   - Parameters:
 *     - width, height: Pipeline viewport dimensions.
 *     - renderpass: Reference to the renderpass_t object.
 *     - shaders: Vector of shader_t references for each stage.
 *     - topology: Primitive topology.
 *     - input_desc: Vertex input description.
 *     - bindings: Reference to desc_set_initializer_t describing descriptor sets.
 *
 * TODO:
 * - maybe get rid of m_width, m_height, create new objects for viewport and stuff
 */
struct pipeline_t : public object_t {
    VkPipeline                      vk_pipeline;

    int                             m_width;
    int                             m_height;
    ref_t<renderpass_t>             m_renderpass;
    std::vector<ref_t<shader_t>>    m_shaders;
    VkPrimitiveTopology             m_topology;
    vertex_input_desc_t             m_input_desc;
    ref_t<pipeline_layout_t>        m_pipeline_layout;

    pipeline_t(object_t::Private priv) : object_t(priv) {}
    virtual ~pipeline_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_PIPELINE; }
    static  object_type_e type_id_static() { return VKU_TYPE_PIPELINE; }

    static ref_t<pipeline_t> create(
            int                                 width,
            int                                 height,
            ref_t<renderpass_t>                 rp,
            const std::vector<ref_t<shader_t>>& shaders,
            VkPrimitiveTopology                 topology,
            vertex_input_desc_t                 input_desc,
            ref_t<pipeline_layout_t>            pipeline_layout);
    virtual std::string to_string() const override;

    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::compute_pipeline_t
 * -----------------------
 *
 * Description: Wraps a Vulkan compute pipeline. This object encapsulates a compute 
 * shader and the descriptor sets it uses. It is used to dispatch compute workloads 
 * on the GPU.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device that owns this compute pipeline.
 * - m_shader: Reference to a shader_t object containing the compute shader.
 * - m_bindings_initer: Reference to a desc_set_initializer_t object describing descriptor sets 
 *   used by the shader.
 *
 * Init: create(device, shader, bindings)
 *   - Parameters:
 *     - device: Reference to the device_t object.
 *     - shader: Reference to the compute shader (shader_t).
 *     - bindings: Reference to desc_set_initializer_t describing descriptor sets.
 */
struct compute_pipeline_t : public object_t {
    VkPipeline                      vk_pipeline;

    ref_t<device_t>                 m_device;
    ref_t<shader_t>                 m_shader;
    ref_t<pipeline_layout_t>        m_pipeline_layout;

    compute_pipeline_t(object_t::Private priv) : object_t(priv) {}
    virtual ~compute_pipeline_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_COMPUTE_PIPELINE; }
    static  object_type_e type_id_static() { return VKU_TYPE_COMPUTE_PIPELINE; }

    static ref_t<compute_pipeline_t> create(
            ref_t<device_t>                 dev,
            ref_t<shader_t>                 shader,
            ref_t<pipeline_layout_t>        pipeline_layout);
    virtual std::string to_string() const override;

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::framebuffs_t
 * -----------------
 *
 * Description: Wraps Vulkan framebuffers. A framebuffer represents a collection of 
 * attachments (color, depth, etc.) used by a render pass for rendering. This object 
 * manages the creation of VkFramebuffer objects corresponding to the swapchain images.
 *
 * Members:
 * - m_renderpass: Reference to a renderpass_t object. The render pass that these 
 *   framebuffers are compatible with.
 *
 * Init: create(renderpass)
 *   - Parameters:
 *     - renderpass: Reference to the renderpass_t object these framebuffers will be used with.
 */
struct framebuffs_t : public object_t {
    std::vector<VkFramebuffer>  vk_fbuffs;      /* needs separate object framebuffer_t */

    ref_t<renderpass_t>         m_renderpass;

    framebuffs_t(object_t::Private priv) : object_t(priv) {}
    virtual ~framebuffs_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_FRAMEBUFFERS; }
    static  object_type_e type_id_static() { return VKU_TYPE_FRAMEBUFFERS; }

    static ref_t<framebuffs_t> create(ref_t<renderpass_t> rp);
    virtual std::string to_string() const override;

    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::cmdpool_t
 * --------------
 *
 * Description: Wraps a Vulkan command pool. A command pool manages the memory and 
 * allocation of command buffers, which record rendering and compute commands. This 
 * object simplifies creation and management of command buffers for a device.
 *
 * Members:
 * - m_device: Reference to a device_t object. The device that owns this command pool.
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this command pool.
 *
 * Notes:
 * - All command buffers allocated from this pool are implicitly associated with
 * the device’s queues.
 */
struct cmdpool_t : public object_t {
    VkCommandPool   vk_pool;

    ref_t<device_t> m_device;

    cmdpool_t(object_t::Private priv) : object_t(priv) {}
    virtual ~cmdpool_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_COMMAND_POOL; }
    static  object_type_e type_id_static() { return VKU_TYPE_COMMAND_POOL; }

    virtual std::string to_string() const override;
    static ref_t<cmdpool_t> create(ref_t<device_t> dev);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::cmdbuff_t
 * --------------
 *
 * Description: Wraps a Vulkan command buffer. Command buffers record rendering and 
 * compute commands that are submitted to a queue for execution. This object manages 
 * allocation, recording, and submission of commands, and provides utility functions 
 * for common operations like binding vertex buffers, descriptor sets, and drawing.
 *
 * Members:
 * - m_cmdpool: Reference to a cmdpool_t object. The command pool from which this 
 *   command buffer was allocated.
 * - m_host_free: TODO explanation
 *
 * Member functions:
 * - begin(flags): Begin recording commands with specified usage flags.
 * - begin_rpass(fbs, img_idx): Begin a render pass using the specified framebuffers.
 * - bind_vert_buffs(first_bind, buffs): Bind vertex buffers.
 * - bind_idx_buff(ibuff, offset, idx_type): Bind an index buffer.
 * - bind_desc_set(bind_point, pl, desc_set): Bind a descriptor set for the pipeline.
 * - draw(pl, vert_cnt): Issue a non-indexed draw call.
 * - draw_idx(pl, vert_cnt): Issue an indexed draw call.
 * - end_rpass(): End the current render pass.
 * - end(): Finish recording commands.
 * - reset(): Reset the command buffer for reuse.
 * - bind_compute(cpl): Bind a compute pipeline.
 * - dispatch_compute(x, y, z): Dispatch compute shader workgroups.
 *
 * Init: create(cmdpool, host_free=false)
 *   - Parameters:
 *     - cmdpool: Reference to the cmdpool_t object from which this buffer will be allocated.
 *     - host_free: Optional flag indicating whether the buffer is host-allocated (default: false).
 */
struct cmdbuff_t : public object_t {
    VkCommandBuffer     vk_buff;

    ref_t<cmdpool_t>    m_cmdpool;
    bool                m_host_free;

    cmdbuff_t(object_t::Private priv) : object_t(priv) {}
    virtual ~cmdbuff_t() { uninit(); }

    static  object_type_e type_id_static() { return VKU_TYPE_COMMAND_BUFFER; }
    virtual object_type_e type_id() const override { return VKU_TYPE_COMMAND_BUFFER; }

    static ref_t<cmdbuff_t> create(ref_t<cmdpool_t> cp, bool host_free = false);
    virtual std::string to_string() const override;

    void begin(VkCommandBufferUsageFlags flags);
    void begin_rpass(ref_t<framebuffs_t> fbs, uint32_t img_idx);
    void bind_vert_buffs(uint32_t first_bind,
            std::vector<std::pair<ref_t<buffer_t>, VkDeviceSize>> buffs);
    void bind_desc_set(VkPipelineBindPoint bind_point, ref_t<pipeline_layout_t> pl,
            ref_t<desc_set_t> desc_set);
    void bind_idx_buff(ref_t<buffer_t> ibuff, uint64_t off, VkIndexType idx_type);
    void draw(ref_t<pipeline_t> pl, uint64_t vert_cnt);
    void draw_idx(ref_t<pipeline_t> pl, uint64_t vert_cnt);
    void end_rpass();
    void end();

    void reset();

    void bind_compute(ref_t<compute_pipeline_t> cpl);
    void dispatch_compute(uint32_t x, uint32_t y = 1, uint32_t z = 1);

    void set_event(ref_t<event_t> event, VkPipelineStageFlags stage);
    void reset_event(ref_t<event_t> event, VkPipelineStageFlags stage);
    void wait_events(const std::vector<ref_t<event_t>>& events, ref_t<dependency_info_t> dep_info);

    void pipeline_barrier(ref_t<dependency_info_t> dep_info);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::sem_t
 * ----------
 *
 * Description: Wraps a Vulkan semaphore. Semaphores are synchronization primitives 
 * used to coordinate execution between command buffers and queues, and to synchronize 
 * presentation with rendering.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this semaphore.
 *
 * Init: create(device)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this semaphore.
 *
 * Notes:
 * - Typically used to signal when an image is available from the swapchain or when 
 *   rendering is complete.
 */
struct sem_t : public object_t {
    VkSemaphore     vk_sem;

    uint64_t        m_initial;
    VkSemaphoreType m_sem_type;
    ref_t<device_t> m_device;

    sem_t(object_t::Private priv) : object_t(priv) {}
    virtual ~sem_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_SEMAPHORE; }
    static object_type_e type_id_static() { return VKU_TYPE_SEMAPHORE; }

    static ref_t<sem_t> create(ref_t<device_t> dev,
            VkSemaphoreType sem_type = VK_SEMAPHORE_TYPE_BINARY, uint64_t inital = 0);

    virtual std::string to_string() const override;

    uint64_t get_counter();
    void signal(uint64_t val);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

struct event_t : public object_t {
    VkEvent vk_event;

    ref_t<device_t> m_device;

    event_t(object_t::Private priv) : object_t(priv) {}
    virtual ~event_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_EVENT; }
    static object_type_e type_id_static() { return VKU_TYPE_EVENT; }

    static ref_t<event_t> create(ref_t<device_t> dev);
    virtual std::string to_string() const override;

    VkResult get_status() {  return vkGetEventStatus(m_device->vk_dev, vk_event); }
    VkResult set_event() {   return vkSetEvent(m_device->vk_dev, vk_event); }
    VkResult reset_event() { return vkResetEvent(m_device->vk_dev, vk_event); }

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::fence_t
 * ------------
 *
 * Description: Wraps a Vulkan fence. Fences are synchronization primitives used to 
 * coordinate CPU and GPU operations. They allow the host to wait for GPU execution 
 * to complete, ensuring proper synchronization between command submissions.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this fence.
 * - m_flags: Fence creation flags. These control initial fence state (e.g., signaled 
 *   or unsignaled).
 *
 * Init: create(device, flags=0)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this fence.
 *     - flags: Optional fence creation flags (default: 0).
 *
 * Notes:
 * - Fences can be waited on from the CPU to ensure GPU completion of submitted work.
 */
struct fence_t : public object_t {
    VkFence             vk_fence;

    ref_t<device_t>     m_device;
    VkFenceCreateFlags  m_flags;

    fence_t(object_t::Private priv) : object_t(priv) {}
    virtual ~fence_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_FENCE; }
    static object_type_e type_id_static() { return VKU_TYPE_FENCE; }

    static ref_t<fence_t> create(ref_t<device_t> dev, VkFenceCreateFlags flags = 0);
    virtual std::string to_string() const override;

    VkResult get_status() { return vkGetFenceStatus(m_device->vk_dev, vk_fence); }

    /* TODO: check (IF NEEDED) handle exports (fd or handle) */

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::buffer_t
 * -------------
 *
 * Description: Wraps a Vulkan buffer and its associated device memory. Buffers are 
 * linear memory resources used to store vertex data, index data, uniform data, or 
 * any other structured GPU-accessible data. This object manages both the VkBuffer 
 * and its backing VkDeviceMemory, and provides helper functions for mapping and 
 * unmapping memory for CPU access.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this buffer.
 * - m_size: The total size of the buffer in bytes.
 * - m_usage_flags: Vulkan usage flags (e.g., VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) that 
 *   determine how the buffer will be used by the GPU.
 * - m_sharing_mode: Vulkan sharing mode (exclusive or concurrent) that determines 
 *   how the buffer is accessed across multiple queue families.
 * - m_memory_flags: Vulkan memory property flags that specify the type of memory 
 *   allocation (e.g., host-visible, device-local, coherent).
 * - m_map_ptr: Pointer to mapped memory (valid only when the buffer is mapped).
 *
 * Member functions:
 * - map_data(offset, size): Maps the buffer's device memory to CPU-visible address space 
 *   so that data can be written directly from the host.
 * - unmap_data(): Unmaps the buffer's device memory after CPU writes are complete.
 *
 * Init: create(device, size, usage_flags, sharing_mode, memory_flags)
 *   - Parameters:
 *     - device: Reference to the device_t that will own this buffer.
 *     - size: The total size of the buffer in bytes.
 *     - usage_flags: Vulkan usage flags indicating how the buffer will be used.
 *     - sharing_mode: How the buffer is shared across queue families.
 *     - memory_flags: Memory properties for the buffer’s allocation.
 */
struct buffer_t : public object_t {
    VkBuffer                vk_buff;
    VkDeviceMemory          vk_mem; /* needs separate object memory_t */
    void                    *m_map_ptr = nullptr;

    ref_t<device_t>         m_device;
    size_t                  m_size;
    VkBufferUsageFlags      m_usage_flags;
    VkSharingMode           m_sharing_mode;
    VkMemoryPropertyFlags   m_memory_flags;
    void *                  m_host_ptr = nullptr;

    buffer_t(object_t::Private priv) : object_t(priv) {}
    virtual ~buffer_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_BUFFER; }
    static object_type_e type_id_static() { return VKU_TYPE_BUFFER; }

    static ref_t<buffer_t> create(
            ref_t<device_t>         dev,
            size_t                  size,
            VkBufferUsageFlags      usage,
            VkSharingMode           sh_mode,
            VkMemoryPropertyFlags   mem_flags,
            void *                  host_ptr = nullptr);
    virtual std::string to_string() const override;

    void *map_data(VkDeviceSize offset = 0, VkDeviceSize size = 0);
    void unmap_data();

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::image_t
 * ------------
 *
 * Description: Wraps a Vulkan image and its associated device memory. Images are
 * multidimensional resources used for textures, color attachments, depth buffers,
 * and other GPU-readable or -writable image data. This object manages the VkImage
 * handle, its VkDeviceMemory allocation, and supports layout transitions and view creation.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this image.
 * - m_width, m_height: Dimensions of the image in pixels.
 * - m_format: Vulkan format (e.g., VK_FORMAT_R8G8B8A8_SRGB) that defines the pixel layout.
 *   how the image will be used.
 * - m_usage: Vulkan usage flags (e.g., VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) indicating 
 *   how the image will be used.
 *
 * Member functions:
 * - transition_layout(cmd_buff, old_layout, new_layout): Records a layout transition
 *   command for this image, changing how the GPU interprets its contents.
 * - set_data(cmd_pool, ptr, size, cmd_buff=nullptr): Copies data from a buffer into the image.
 *
 * Init: create(device, width, height, format, usage)
 *   - Parameters:
 *     - device: Reference to the device_t that will own this image.
 *     - width, height: Dimensions of the image.
 *     - format: Image format.
 *     - usage: Usage flags describing intended image use.
 *
 * TODO:
 * - add m_mem_flags? 
 */
struct image_t : public object_t {
    VkImage             vk_img;
    VkDeviceMemory      vk_img_mem;

    ref_t<device_t>     m_device;
    uint32_t            m_width;
    uint32_t            m_height;
    VkFormat            m_format;
    VkImageUsageFlags   m_usage;
    VkImageTiling       m_tiling;

    image_t(object_t::Private priv) : object_t(priv) {}
    virtual ~image_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_IMAGE; }
    static object_type_e type_id_static() { return VKU_TYPE_IMAGE; }

    static ref_t<image_t> create(
            ref_t<device_t>     dev,
            uint32_t            width,
            uint32_t            height,
            VkFormat            fmt,
            VkImageUsageFlags   usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                      | VK_IMAGE_USAGE_SAMPLED_BIT,
            VkImageTiling       tiling = VK_IMAGE_TILING_OPTIMAL);
    virtual std::string to_string() const override;

    /* if no command buffer is provided, one will be allocated from the command pool */
    void transition_layout(
            ref_t<cmdpool_t>    cp,
            VkImageLayout       old_layout,
            VkImageLayout       new_layout,
            ref_t<cmdbuff_t>    cbuff = nullptr);

    /* if no command buffer is provided, one will be allocated from the command pool */
    void set_data(
            ref_t<cmdpool_t>    cp,
            void*               data,
            uint32_t            sz,
            ref_t<cmdbuff_t>    cbuff = nullptr);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::img_view_t
 * ---------------
 *
 * Description: Wraps a Vulkan image view. An image view defines how a Vulkan image’s
 * data can be accessed and interpreted within shaders or as attachments. This object
 * manages the VkImageView handle and ensures it is correctly associated with its
 * underlying VkImage.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this image view.
 * - m_image: Reference to the image_t object that this view is based on.
 * - m_aspect_flags: Aspect flags defining which parts of the image are accessible 
 *   (e.g., color, depth, or stencil).
 *
 * Init: create(device, image, aspect_flags)
 *   - Parameters:
 *     - device: Reference to the device_t object that owns this image view.
 *     - image: Reference to the image_t object to create a view for.
 *     - aspect_flags: Aspect flags specifying which parts of the image the view will access.
 *
 * Notes:
 * - Required for using images as color or depth attachments, or as sampled textures.
 * - The view type (1D, 2D, 3D, or cube) is inferred from the image and usage flags.
 * - Multiple views can be created from the same image to represent different mip levels
 *   or aspects.
 */
struct img_view_t : public object_t {
    VkImageView         vk_view;

    ref_t<image_t>      m_image;
    VkImageAspectFlags  m_aspect_mask;

    img_view_t(object_t::Private priv) : object_t(priv) {}
    virtual ~img_view_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_IMAGE_VIEW; }
    virtual std::string to_string() const override;

    static object_type_e type_id_static() { return VKU_TYPE_IMAGE_VIEW; }
    static ref_t<img_view_t> create(ref_t<image_t> img, VkImageAspectFlags aspect_mask);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vku::img_sampl_t
 * ----------------
 *
 * Description: Wraps a Vulkan sampler. A sampler defines how image data is read in shaders, 
 * including filtering, addressing modes, and mipmap behavior. This object manages the 
 * VkSampler handle and its configuration, and is used in combination with an img_view_t 
 * when binding textures to descriptor sets.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this sampler.
 * - m_filter: Filtering mode used for magnification and minification (e.g., 
 *   VK_FILTER_LINEAR, VK_FILTER_NEAREST).
 *
 * Init: create(device, filter)
 *   - Parameters:
 *     - device: Reference to the device_t object that will own this sampler.
 *     - filter: Filtering mode for magnification and minification.
 *
 * Notes:
 * - Samplers are independent of specific images and can be reused across multiple textures.
 * 
 * TODO:
 * - add m_address_mode, max_anisotropy, mipmap_mode
 */
struct img_sampl_t : public object_t {
    VkSampler       vk_sampler;

    ref_t<device_t> m_device;
    VkFilter        m_filter;

    img_sampl_t(object_t::Private priv) : object_t(priv) {}
    virtual ~img_sampl_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_IMAGE_SAMPLER; }
    static object_type_e type_id_static() { return VKU_TYPE_IMAGE_SAMPLER; }

    static ref_t<img_sampl_t> create(ref_t<device_t> dev, VkFilter filter = VK_FILTER_LINEAR);
    virtual std::string to_string() const override;

    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding, VkShaderStageFlags stage);

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vkc::desc_pool_t
 * ----------------
 *
 * Description: Represents a Vulkan descriptor pool. A descriptor pool allocates 
 * and manages descriptor sets, which are used to bind resources (buffers, 
 * images, samplers) to shaders. This object wraps the VkDescriptorPool handle 
 * and tracks associated bindings and allocation count.
 *
 * Members:
 * - m_device: Reference to the device_t object that owns this pool.
 * - m_bindings_initer: Reference to a desc_set_initializer_t object describing the types 
 *   of bindings this pool can allocate.
 * - m_cnt: Number of descriptor sets this pool can allocate.
 * - vk_descpool: Vulkan descriptor pool handle.
 *
 *
 * Init: create(dev, bindings, cnt)
 *   - Parameters:
 *     - dev: Reference to the device_t object used to create the pool.
 *     - bindings: Reference to a desc_set_initializer_t describing the binding types.
 *     - cnt: Maximum number of descriptor sets that can be allocated from this pool.
 */
struct desc_pool_t : public object_t {
    VkDescriptorPool                vk_descpool;

    ref_t<device_t>                 m_device;
    ref_t<desc_set_initializer_t>   m_bindings_initer;
    uint32_t                        m_cnt;

    desc_pool_t(object_t::Private priv) : object_t(priv) {}
    virtual ~desc_pool_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_POOL; }
    static  object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_POOL; }

    static ref_t<desc_pool_t> create(
            ref_t<device_t>                 dev,
            ref_t<desc_set_initializer_t>   bindings_initer,
            uint32_t                        cnt);
    virtual std::string to_string() const override;

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * vkc::desc_set_t
 * ---------------
 *
 * Description: Represents a Vulkan descriptor set. Descriptor sets are allocated 
 * from a descriptor pool and define how resources (buffers, images, samplers) 
 * are bound to shaders in a pipeline. This object wraps the VkDescriptorSet handle 
 * and tracks the pool, pipeline, and bindings associated with the set.
 *
 * Members:
 * - m_descriptor_pool: Reference to the desc_pool_t object that allocated this set.
 * - m_pipeline: Reference to the pipeline_t object using this descriptor set.
 * - m_bindings_initer: Reference to a desc_set_initializer_t object describing the resources 
 *   bound to this descriptor set.
 * - vk_desc_set: Vulkan descriptor set handle.
 *
 * Member functions:
 * - update(): Updates the GPU descriptor set with the current resources from the
 *   associated desc_set_initializer_t. Should be called after changing any buffers, images,
 *   or samplers in the bindings. This function must be called before binding the descriptor set in
 *   a command buffer if any resources have changed.
 *
 * Init: create(dp, pl, bindings)
 *   - Parameters:
 *     - dp: Reference to the desc_pool_t to allocate the descriptor set from.
 *     - pl: Reference to the pipeline_t that will use this descriptor set.
 *     - bindings: Reference to a desc_set_initializer_t describing the bindings for this set.
 * 
 * Notes:
 * - desc_set_t represents an actual Vulkan descriptor set allocated from a descriptor pool.
 *   It implements the resources described by a desc_set_initializer_t. While desc_set_initializer_t
 *   defines the layout and points to the specific resources (buffers, images, samplers),
 *   desc_set_t is the concrete GPU object that can be bound in a pipeline. Multiple 
 *   desc_set_t instances can share the same desc_set_initializer_t layout.
 * - Buffers and samplers are stored in the desc_set_initializer_t bindings. To change the
 *   resource used by a desc_set_t, modify the resource in the corresponding 
 *   desc_set_initializer_t::binding_desc_t and call update() on the desc_set_t. Alternatively,
 *   calling update() on the desc_set_initializer_t itself also works, as desc_set_t depends
 *   on it and will propagate the changes automatically.
 */
struct desc_set_t : public object_t {
    VkDescriptorSet                 vk_desc_set;

    ref_t<desc_pool_t>              m_descriptor_pool;
    ref_t<desc_set_layout_t>        m_desc_set_layout;
    ref_t<desc_set_initializer_t>   m_bindings_initer;

    desc_set_t(object_t::Private priv) : object_t(priv) {}
    virtual ~desc_set_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_SET; }
    static object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_SET; }

    static ref_t<desc_set_t> create(
            ref_t<desc_pool_t>              dp,
            ref_t<desc_set_layout_t>        desc_set_layout,
            ref_t<desc_set_initializer_t>   bindings_initer);
    virtual std::string to_string() const override;

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*! TODO: desc */
struct desc_set_layout_t : public object_t {
    VkDescriptorSetLayout           vk_desc_set_layout;

    ref_t<device_t>                 m_device;
    ref_t<desc_set_initializer_t>   m_bindings_initer;

    desc_set_layout_t(object_t::Private priv) : object_t(priv) {}
    virtual ~desc_set_layout_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_SET_LAYOUT; }
    static object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_SET_LAYOUT; }

    static ref_t<desc_set_layout_t> create(
            ref_t<device_t> dev, ref_t<desc_set_initializer_t> bindings_initer);
    virtual std::string to_string() const override;

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*! TODO: desc */
struct pipeline_layout_t : public object_t {
    VkPipelineLayout            vk_pipeline_layout;

    ref_t<desc_set_layout_t>    m_desc_set_layout;

    pipeline_layout_t(object_t::Private priv) : object_t(priv) {}
    virtual ~pipeline_layout_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_PIPELINE_LAYOUT; }
    static object_type_e type_id_static() { return VKU_TYPE_PIPELINE_LAYOUT; }

    static ref_t<pipeline_layout_t> create(ref_t<desc_set_layout_t> bindings_initer);
    virtual std::string to_string() const override;

private:
    vc::ret_t init();
    vc::ret_t uninit();
};

/*!
 * 
 * A layout is like the type of a descriptor set, while a descriptor set is the actual instance of
 * a layout. For myself I can imagine how having to keep both a layout around and to manualy set the
 * descriptors into the descriptor sets would be a pain, so this structure desc_set_initializer_t
 * is meant to hold both the structure and the descriptor values with the help with which both the
 * layer and the descriptors will be made. Furthermore, this structure will help to update the
 * descriptor sets when such updating will be needed. It may sacrifice some speed, but it is for now
 * a sacrifice I am willing to make.
 * 
 * To view the idea, this is how I understand vulkan views this whole thing:
 * ------------------------
 * template <Int binding, typename DescType, Int DescCount>
 * struct SetLayoutBinding {
 *     DescType descriptors[DescCount];    
 * };
 * 
 * template <Int N>
 * struct SetLayout {
 *     tuple<SetLayoutBinding1, SetLayoutBinding2, ... SetLayoutBinding_N> bindings;
 * };
 * 
 * VkDescriptorPool<SetLayout> pool;
 * SetLayout desc_set(pool);
 * ------------------------
 * 
 * In this example:
 * - the type SetLayout is held by VkDescriptorSetLayout.
 * - the pool needs to hold some information about the layout, as such it is dependent on it, you
 *   create descriptor sets from it.
 * - the SetLayout is made out of multiple SetLayoutBindings, or VkDescriptorSetLayoutBinding, those
 *   describe the binding
 * - the desc_set is the final container for the descriptors, placed in such a way that the shaders
 *   would be able to understand it's layout, VkWriteDescriptorSet hold those containers
 * 
 * desc_set_initializer_t holds both the values of an initial setup for the descriptors referenced
 * in the descriptor set and their location in the shader.
 * 
 * binding_desc_t and it's derivatives hold single binding information.
 * 
 * USAGE:
 * Create this structure, with some initial descriptors and the desired location, types, etc.
 * Create a layout from it.
 * Create sets from it.
 * Further in time, if needed, modify the descriptors inside the bindings and update the descriptor
 * sets 
 * 
 * vku::desc_set_initializer_t
 * -----------------------
 *
 * Description: Represents a collection of Vulkan descriptor bindings that define 
 * how GPU resources (buffers, images, samplers) are connected to shaders. 
 * This object holds a list of binding descriptors (either buffer or sampler bindings) 
 * and provides helper functions to produce Vulkan structures used during 
 * descriptor set and layout creation.
 *
 * Members:
 * - m_binds: Vector of binding_desc_t objects, each describing a single resource 
 *   binding (uniform buffer, storage buffer, or combined image sampler).
 *
 * Nested types:
 * - binding_desc_t: Abstract base class for a descriptor binding. 
 *   Defines a common interface for obtaining a VkWriteDescriptorSet structure.
 * - buff_binding_t: Represents a buffer descriptor binding. Holds a reference 
 *   to a buffer_t and its associated VkDescriptorBufferInfo.
 * - sampl_binding_t: Represents a combined image sampler binding. Holds references 
 *   to an img_view_t and an img_sampl_t, as well as the associated VkDescriptorImageInfo.
 *
 * Member functions:
 * - get_writes(): Returns a vector of VkWriteDescriptorSet structures for updating 
 *   descriptor sets.
 * - get_descriptors(): Returns a vector of VkDescriptorSetLayoutBinding structures 
 *   describing all bindings for layout creation.
 *
 * Init: create(binds)
 *   - Parameters:
 *     - binds: Vector of binding_desc_t references (each created using buff_binding_t::create 
 *       or sampl_binding_t::create).
 *
 * Notes:
 * - This object is used by desc_pool_t and desc_set_t to create and populate 
 *   Vulkan descriptor sets.
 * 
 * 
 * vku::desc_set_initializer_t::buff_binding_t
 * ---------------------------------------
 *
 * Description: Represents a buffer binding within a Vulkan descriptor set. 
 * This binding connects a GPU buffer (uniform or storage) to a shader stage. 
 * It wraps a buffer_t reference and the corresponding VkDescriptorBufferInfo needed 
 * for descriptor updates.
 *
 * Members:
 * - m_buffer: Reference to a buffer_t object containing the GPU buffer.
 *
 * Member functions:
 * - get_write(): Returns a VkWriteDescriptorSet structure suitable for updating 
 *   a descriptor set with this buffer binding.
 *
 * Init: create(desc, buffer)
 *   - Parameters:
 *     - desc: VkDescriptorSetLayoutBinding describing this binding (binding index, 
 *       descriptor type, stage flags, etc.).
 *     - buffer: Reference to a buffer_t object to bind.
 *
 * Notes:
 * - Typically used for uniform buffers or storage buffers in graphics or compute pipelines.
 * 
 * 
 * vku::desc_set_initializer_t::sampl_binding_t
 * ----------------------------------------
 *
 * Description: Represents a combined image sampler binding within a Vulkan descriptor set. 
 * This binding connects a GPU image (via img_view_t) and a sampler (img_sampl_t) 
 * to a shader stage, allowing shaders to sample textures.
 *
 * Members:
 * - m_view: Reference to an img_view_t object representing the image to be sampled.
 * - m_sampler: Reference to an img_sampl_t object representing the sampler used for filtering.
 *
 * Member functions:
 * - get_write(): Returns a VkWriteDescriptorSet structure suitable for updating 
 *   a descriptor set with this image-sampler binding.
 *
 * Init: create(desc, view, sampler)
 *   - Parameters:
 *     - desc: VkDescriptorSetLayoutBinding describing this binding (binding index, 
 *       descriptor type, stage flags, etc.).
 *     - view: Reference to an img_view_t object to bind.
 *     - sampler: Reference to an img_sampl_t object to bind.
 */
struct desc_set_initializer_t : public object_t {
    struct binding_desc_t : public object_t {
        VkDescriptorSetLayoutBinding m_desc;

        binding_desc_t(object_t::Private priv) : object_t(priv) {}
        virtual ~binding_desc_t() {}

        virtual VkWriteDescriptorSet get_write() const = 0;
    };

    struct buff_binding_t : public binding_desc_t {
        VkDescriptorBufferInfo  desc_buff_info;

        buff_binding_t(object_t::Private priv) : binding_desc_t(priv) {}
        virtual ~buff_binding_t() {}

        virtual VkWriteDescriptorSet get_write() const override;
        virtual object_type_e type_id() const override { return VKU_TYPE_BUFFER_BINDING; }
        virtual std::string to_string() const override;

        static  object_type_e type_id_static() { return VKU_TYPE_BUFFER_BINDING; }
        static ref_t<buff_binding_t> create(
                VkDescriptorSetLayoutBinding    desc,
                ref_t<buffer_t>                 buff = nullptr,
                size_t                          offset = 0,
                size_t                          size = 0);

        void set_buffer(ref_t<buffer_t> buff, size_t offset = 0, size_t size = 0);
    };

    struct sampl_binding_t : public binding_desc_t {
        VkDescriptorImageInfo   imag_info;

        sampl_binding_t(object_t::Private priv) : binding_desc_t(priv) {}
        virtual ~sampl_binding_t() {}

        virtual VkWriteDescriptorSet get_write() const override;
        virtual object_type_e type_id() const override { return VKU_TYPE_SAMPLER_BINDING; }
        virtual std::string to_string() const;

        static  object_type_e type_id_static() { return VKU_TYPE_SAMPLER_BINDING; }
        static ref_t<sampl_binding_t> create(
                VkDescriptorSetLayoutBinding    desc,
                ref_t<img_view_t>               view = nullptr,
                ref_t<img_sampl_t>              sampl = nullptr,
                VkImageLayout                   layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        void set_view(ref_t<img_view_t> sampl);
        void set_sampler(ref_t<img_sampl_t> view);
        void set_layout(VkImageLayout layout);
    };

    desc_set_initializer_t(object_t::Private priv) : object_t(priv) {}
    virtual ~desc_set_initializer_t() { uninit(); }

    virtual object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_SET_INITIALIZER; }
    virtual std::string to_string() const override;

    static  object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_SET_INITIALIZER; }
    static ref_t<desc_set_initializer_t> create(std::vector<ref_t<binding_desc_t>> binds);

    ref_t<binding_desc_t> get_binding(uint32_t i);
    void update_set(ref_t<desc_set_t> ds);

    std::vector<VkWriteDescriptorSet> get_writes(VkDescriptorSet dst_set) const;
    std::vector<VkDescriptorSetLayoutBinding> get_descriptors() const;

    std::vector<ref_t<binding_desc_t>> m_binds;
};

/* Internal:
=================================================================================================
=================================================================================================
================================================================================================= */

struct except_t : vc::except_t {
    VkResult vk_err = VK_ERROR_UNKNOWN;
    except_t(const std::string &str) : vc::except_t(str) {}
    except_t(VkResult vk_err) : vc::except_t(vk_err_str(vk_err)), vk_err(vk_err) {}
};

struct swapchain_details_t {
    VkSurfaceCapabilitiesKHR        capab;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
};

inline vc::ret_t create_dbg_messenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* dbg_info,
        const VkAllocationCallbacks* alloc,
        VkDebugUtilsMessengerEXT* dbg_msg);

inline void destroy_dbg_messenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT dbg_msg,
        const VkAllocationCallbacks* alloc);

inline VKAPI_ATTR VkBool32 VKAPI_CALL dbg_cbk(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* _data,
        void* ctx);

inline  swapchain_details_t get_swapchain_details(VkPhysicalDevice dev,
        VkSurfaceKHR surf);

inline gpu_family_ids_t find_queue_families(VkPhysicalDevice dev,
        VkSurfaceKHR surface);

inline VkExtent2D choose_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capab);

inline int score_phydev(VkPhysicalDevice dev, VkSurfaceKHR surf);
inline VkShaderStageFlagBits get_shader_type(vku_shader_stage_e own_type);

#ifndef VKU_HAS_NEW_GLSLANG
inline TBuiltInResource spirv_resources = {};
#else
inline glslang_resource_t spirv_resources = {};
#endif

inline void spirv_uninit();
inline void spirv_init();
inline spirv_t spirv_compile(vku_shader_stage_e vk_stage, const char *code);
inline int spirv_save(const spirv_t& code, const char *filepath);

inline uint32_t find_memory_type(ref_t<device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties, size_t sz);

/* IMPLEMENTATION:
=================================================================================================
=================================================================================================
================================================================================================= */

inline bool init_state = false;
inline vc::ret_t init() {
    if (init_state)
        return VK_SUCCESS;

    spirv_init();
    if (glfwInit() != GLFW_TRUE) {
        DBG("Failed to init glfw: %s", glfw_err().c_str());
        spirv_uninit();
        return VK_ERROR_UNKNOWN;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    init_state = true;
    return VK_SUCCESS;
}

inline vc::ret_t uninit() {
    if (!init_state)
        return VK_ERROR_UNKNOWN;
    glfwTerminate();
    spirv_uninit();
    init_state = false;
    return VK_SUCCESS;
}

inline vertex_input_desc_t vertex_p2n0c3t2_t::get_input_desc() {
    return {
        .bind_desc = {
            .binding = 0,
            .stride = sizeof(vertex_p2n0c3t2_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        .attr_desc = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vertex_p2n0c3t2_t, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_p2n0c3t2_t, color)
            },
            {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vertex_p2n0c3t2_t, tex)
            }
        }
    };
}

inline vertex_input_desc_t vertex_p3n3c3t2_t::get_input_desc() {
    return {
        .bind_desc = {
            .binding = 0,
            .stride = sizeof(vertex_p3n3c3t2_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        .attr_desc = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_p3n3c3t2_t, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_p3n3c3t2_t, normal)
            },
            {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vertex_p3n3c3t2_t, color)
            },
            {
                .location = 3,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vertex_p3n3c3t2_t, tex)
            }
        }
    };
}

inline VkDescriptorSetLayoutBinding ubo_t::get_desc_set(uint32_t binding,
        VkShaderStageFlags stage)
{
    return {
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = stage,
        .pImmutableSamplers = nullptr,
    };
}

inline VkDescriptorSetLayoutBinding ssbo_t::get_desc_set(uint32_t binding,
        VkShaderStageFlags stage)
{
    return {
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = stage,
        .pImmutableSamplers = nullptr,
    };
}

/* binding_t
================================================================================================= */

inline vku::ref_t<binding_t> binding_t::create(const VkDescriptorSetLayoutBinding& bd) {
    auto ret = std::make_shared<binding_t>(object_t::Private{type_id_static()});
    ret->bd = bd;
    return ret;
}


inline std::string binding_t::to_string() const {
    return std::format("vku::binding_t[{}]: binding={}, descriptorType={}, descriptorCount={} "
            "stageFlags={} pImmutableSamplers={}]",
            (void *)this, bd.binding, vku::to_string(bd.descriptorType), bd.descriptorCount,
            vku::to_string((VkShaderStageFlagBits)bd.stageFlags), (void *)bd.pImmutableSamplers);
}

/* dependency_info_t
================================================================================================= */

inline vku::ref_t<dependency_info_t> dependency_info_t::create(
        VkPipelineStageFlagBits src_stage_mask,
        VkPipelineStageFlagBits dst_stage_mask,
        const std::vector<VkMemoryBarrier> &mem_bars,
        const std::vector<VkBufferMemoryBarrier> &buff_mem_bars,
        const std::vector<VkImageMemoryBarrier> &img_mem_bars,
        VkDependencyFlags dep_flags)
{
    auto ret = std::make_shared<dependency_info_t>(object_t::Private{type_id_static()});
    ret->m_src_stage_mask = src_stage_mask;
    ret->m_dst_stage_mask = dst_stage_mask;
    ret->m_dep_flags = dep_flags;
    ret->mem_bars = mem_bars;
    ret->buff_mem_bars = buff_mem_bars;
    ret->img_mem_bars = img_mem_bars;
    return ret;
}

inline std::string dependency_info_t::to_string() const {
    std::string ret;
    ret += std::format("vku::dependency_info_t[{}]:\n"
            "\tsrc_stage_mask={}\n"
            "\tdst_stage_mask={}\n"
            "\tdependencyFlags={}\n"
            "\tMemoryBarriers = ",
            (void *)this,
            vku::to_string((VkPipelineStageFlagBits)m_src_stage_mask),
            vku::to_string((VkPipelineStageFlagBits)m_dst_stage_mask),
            vku::to_string((VkDependencyFlagBits)m_dep_flags));
    ret += "\t{\n";
    for (auto barrier : mem_bars) {
        ret += std::format("\t\tsrcAccessMask={},\n",
                vku::to_string((VkAccessFlagBits)barrier.srcAccessMask));
        ret += std::format("\t\tdstAccessMask={}\n",
                vku::to_string((VkAccessFlagBits)barrier.dstAccessMask));
    }
    ret += "\t},\nBufferMemoryBarriers = {\n";
    for (auto barrier : buff_mem_bars) {
        ret += std::format("\t\tsrcAccessMask={},\n",
                vku::to_string((VkAccessFlagBits)barrier.srcAccessMask));
        ret += std::format("\t\tdstAccessMask={}\n",
                vku::to_string((VkAccessFlagBits)barrier.dstAccessMask));
        ret += std::format("\t\tsrcQueueFamilyIndex={}\n", barrier.srcQueueFamilyIndex);
        ret += std::format("\t\tdstQueueFamilyIndex={}\n", barrier.dstQueueFamilyIndex);
        ret += std::format("\t\tbuffer={}\n",              (void *)barrier.buffer);
        ret += std::format("\t\toffset={}\n",              (size_t)barrier.offset);
        ret += std::format("\t\tsize={}\n",                (size_t)barrier.size);
    }
    ret += "\t},\nImageMemoryBarriers = {\n";
    for (auto barrier : img_mem_bars) {
        ret += std::format("\t\tsrcAccessMask={},\n",
                vku::to_string((VkAccessFlagBits)barrier.srcAccessMask));
        ret += std::format("\t\tdstAccessMask={}\n",
                vku::to_string((VkAccessFlagBits)barrier.dstAccessMask));
        ret += std::format("\t\toldLayout={}\n",           vku::to_string(barrier.oldLayout));
        ret += std::format("\t\tnewLayout={}\n",           vku::to_string(barrier.newLayout));
        ret += std::format("\t\tsrcQueueFamilyIndex={}\n", barrier.srcQueueFamilyIndex);
        ret += std::format("\t\tdstQueueFamilyIndex={}\n", barrier.dstQueueFamilyIndex);
        ret += std::format("\t\timage={}\n",               (void *)barrier.image);
        ret += "\tsubresourceRange = {\n";
        ret += std::format("\t\t\taspectMask={},\n",
                vku::to_string((VkImageAspectFlagBits)barrier.subresourceRange.aspectMask));
        ret += std::format("\t\t\tbaseMipLevel={},\n",     barrier.subresourceRange.baseMipLevel);
        ret += std::format("\t\t\tlevelCount={},\n",       barrier.subresourceRange.levelCount);
        ret += std::format("\t\t\tbaseArrayLayer={},\n",   barrier.subresourceRange.baseArrayLayer);
        ret += std::format("\t\t\tlayerCount={},\n",       barrier.subresourceRange.layerCount);
        ret += "\t}\n";
    }
    ret += "\t}\n";
    return ret;
}

/* dependency_info2_t
================================================================================================= */

/* TODO: this needs to be implemented in a newer version of vulkan, tested and as such */
// static vku::ref_t<dependency_info2_t> dependency_info2_t::create(
//         const VkDependencyInfo& dep_info,
//         const std::vector<VkMemoryBarrier2> &mem_bars,
//         const std::vector<VkBufferMemoryBarrier2> &buff_mem_bars,
//         const std::vector<VkImageMemoryBarrier2> &img_mem_bars)
// {
//     auto ret = std::make_shared<dependency_info2_t>(object_t::Private{type_id_static()});
//     ret->dep_info = dep_info;
//     ret->mem_bars = mem_bars;
//     ret->buff_mem_bars = buff_mem_bars;
//     ret->img_mem_bars = img_mem_bars;
//     ret->init();
//     return ret;
// }

// inline std::string dependency_info2_t::to_string() const {
//     std::string ret;
//     ret += std::format("vku::dependency_info2_t[{}]: dependencyFlags={} MemoryBarriers = ",
//             (void *)this, vku::to_string(bd.dependencyFlags));
//     ret += "{\n";
//     for (auto barrier : mem_bars) {
//         ret += std::format("\tsrcStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.srcStageMask));
//         ret += std::format("\tsrcAccessMask={},\n",
//                 vku::to_string((VkAccessFlags2)barrier.srcAccessMask));
//         ret += std::format("\tdstStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.dstStageMask));
//         ret += std::format("\tdstAccessMask={}\n",
//                 vku::to_string((VkAccessFlags2)barrier.dstAccessMask));
//     }
//     ret += "},\nBufferMemoryBarriers = {\n";
//     for (auto barrier : buff_mem_bars) {
//         ret += std::format("\tsrcStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.srcStageMask));
//         ret += std::format("\tsrcAccessMask={},\n",
//                 vku::to_string((VkAccessFlags2)barrier.srcAccessMask));
//         ret += std::format("\tdstStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.dstStageMask));
//         ret += std::format("\tdstAccessMask={}\n",
//                 vku::to_string((VkAccessFlags2)barrier.dstAccessMask));
//         ret += std::format("\tsrcQueueFamilyIndex={}\n", barrier.srcQueueFamilyIndex);
//         ret += std::format("\tdstQueueFamilyIndex={}\n", barrier.dstQueueFamilyIndex);
//         ret += std::format("\tbuffer={}\n",              (void *)barrier.buffer);
//         ret += std::format("\toffset={}\n",              (size_t)barrier.offset);
//         ret += std::format("\tsize={}\n",                (size_t)barrier.size);
//     }
//     ret += "},\nImageMemoryBarriers = {\n";
//     for (auto barrier : img_mem_bars) {
//         ret += std::format("\tsrcStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.srcStageMask));
//         ret += std::format("\tsrcAccessMask={},\n",
//                 vku::to_string((VkAccessFlags2)barrier.srcAccessMask));
//         ret += std::format("\tdstStageMask={},\n",
//                 vku::to_string((VkPipelineStageFlagBits2)barrier.dstStageMask));
//         ret += std::format("\tdstAccessMask={}\n",
//                 vku::to_string((VkAccessFlags2)barrier.dstAccessMask));
//         ret += std::format("\toldLayout={}\n",           vku::to_string(barrier.oldLayout));
//         ret += std::format("\tnewLayout={}\n",           vku::to_string(barrier.newLayout));
//         ret += std::format("\tsrcQueueFamilyIndex={}\n", barrier.srcQueueFamilyIndex);
//         ret += std::format("\tdstQueueFamilyIndex={}\n", barrier.dstQueueFamilyIndex);
//         ret += std::format("\timage={}\n",               (void *)barrier.image);
//         ret += "\tsubresourceRange = {\n";
//         ret += std::format("\t\taspectMask={},\n",
//                 vku::to_string((VkImageAspectFlagBits)barrier.subresourceRange.aspectMask));
//         ret += std::format("\t\tbaseMipLevel={},\n",     barrier.subresourceRange.baseMipLevel);
//         ret += std::format("\t\tlevelCount={},\n",       barrier.subresourceRange.levelCount);
//         ret += std::format("\t\tbaseArrayLayer={},\n",   barrier.subresourceRange.baseArrayLayer);
//         ret += std::format("\t\tlayerCount={},\n",       barrier.subresourceRange.layerCount);
//         ret += "\t}\n";
//     }
//     ret += "}\n";
//     return ret;
// }

// inline void dependency_info2_t::update_ptrs() {
//     dep_info.memoryBarrierCount = (uint32_t)mem_bars.size();
//     dep_info.pMemoryBarriers = mem_bars.data();
//     dep_info.bufferMemoryBarrierCount = (uint32_t)buff_mem_bars.size();
//     dep_info.pBufferMemoryBarriers = buff_mem_bars.data();
//     dep_info.imageMemoryBarrierCount = (uint32_t)img_mem_bars.size();
//     dep_info.pImageMemoryBarriers = img_mem_bars.data();
// }

/* image_subresource_range_t
================================================================================================= */

inline vku::ref_t<image_subresource_range_t> image_subresource_range_t::create(
        const VkImageSubresourceRange& img_subrange)
{
    auto ret = std::make_shared<image_subresource_range_t>(object_t::Private{type_id_static()});
    ret->m_img_subrange = img_subrange;
    return ret;
}

inline std::string image_subresource_range_t::to_string() const {
    return std::format("vku::image_subresource_range_t[{}]: taspectMask={}, baseMipLevel={}, "
            "levelCount={} baseArrayLayer={} layerCount={}", 
            (void *)this, vku::to_string((VkImageAspectFlagBits)m_img_subrange.aspectMask),
            m_img_subrange.baseMipLevel, m_img_subrange.levelCount, m_img_subrange.baseArrayLayer,
            m_img_subrange.layerCount);
}

/* window_t
================================================================================================= */

inline ref_t<window_t> window_t::create(int width, int height, std::string name) {
    auto ret = std::make_shared<window_t>(object_t::Private{type_id_static()});
    ret->m_name = name;
    ret->m_width = width;
    ret->m_height = height;

    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t window_t::uninit() {
    if (_window)
        glfwDestroyWindow(_window);
    return VK_SUCCESS;
}

inline vc::ret_t window_t::init() {
    VK_ASSERT(vulkan_utils::init());
    _window = glfwCreateWindow(m_width, m_height, m_name.c_str(), NULL, NULL);
    if (!_window) {
        DBG("Failed to create a glfw window: %s", glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    DBG("Created Window %p", this);
    return VK_SUCCESS;
}

inline std::string window_t::to_string() const {
    return std::format("vku::window[{}]: m_width={}, m_height={}, m_window_name={}",
            (void*)this, m_width, m_height, m_name);
}

/* instance_t
================================================================================================= */

inline ref_t<instance_t> instance_t::create(
        const std::string app_name,
        const std::string engine_name,
        const std::vector<std::string>& extensions,
        const std::vector<std::string>& layers)
{
    auto ret = std::make_shared<instance_t>(object_t::Private{type_id_static()});
    ret->m_app_name = app_name;
    ret->m_engine_name = engine_name;
    ret->m_extensions = extensions;
    ret->m_layers = layers;

    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t instance_t::init() {
    FnScope err_scope;

    /* The instance can be used without a window, so it must also init vku if it was not already
    initialized */
    VK_ASSERT(vulkan_utils::init());

    /* get version: */
    uint32_t ver;
    VK_ASSERT(vkEnumerateInstanceVersion(&ver));
    DBG("VK ver: %d.%d.%d",  VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver), VK_VERSION_PATCH(ver));
    ver &= ~(0xFFFU);
    
    /* get required extensions: */
    uint32_t glfw_ext_count = 0;
    const char** glfw_exts;
    glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

    if (!glfw_exts) {
        DBG("Failed to get required extensions for glfw: %s", glfw_err().c_str());
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    std::vector<const char*> exts_c(glfw_exts, glfw_exts + glfw_ext_count);
    for (auto &e : this->m_extensions)
        exts_c.push_back(e.c_str());
    for (auto e : exts_c)
        DBG("Required extension: %s", e);

    /* get supported extensions */
    uint32_t ext_cnt = 0;
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(NULL, &ext_cnt, NULL));
    std::vector<VkExtensionProperties> exts(ext_cnt);
    VK_ASSERT(vkEnumerateInstanceExtensionProperties(NULL, &ext_cnt, exts.data()));

    /* check for compatibility */
    for (auto req_e : exts_c) {
        bool found = false;
        for (auto sup_e : exts)
            if (!strcmp(req_e, sup_e.extensionName))
                found = true;
        if (!found) {
            DBG("Required extension %s is not supported", req_e);
            throw vku::except_t(VK_ERROR_UNKNOWN);
        }
    }

    /* set required layers */
    std::vector<const char*> layers_c;
    for (auto &l : this->m_layers)
        layers_c.push_back(l.c_str());
    for (auto l : layers_c)
        DBG("Required layer: %s", l);

    /* get supported layers */
    uint32_t layer_cnt = 0;
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layer_cnt, NULL));
    std::vector<VkLayerProperties> sup_layers(layer_cnt);
    VK_ASSERT(vkEnumerateInstanceLayerProperties(&layer_cnt, sup_layers.data()));

    /* check for compatibility */
    for (auto req_l : layers_c) {
        bool found = false;
        for (auto sup_l : sup_layers)
            if (!strcmp(req_l, sup_l.layerName))
                found = true;
        if (!found) {
            DBG("Required extension %s is not supported", req_l);
            throw vku::except_t(VK_ERROR_UNKNOWN);
        }
    }

    VkApplicationInfo vk_app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = m_app_name.c_str(),
        .applicationVersion = ver,
        .pEngineName = m_engine_name.c_str(),
        .engineVersion = ver,
        .apiVersion = ver,
    };

    VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pApplicationInfo = &vk_app_info,
        .enabledLayerCount = (uint32_t)layers_c.size(),
        .ppEnabledLayerNames = layers_c.data(),
        .enabledExtensionCount = (uint32_t)exts_c.size(),
        .ppEnabledExtensionNames = exts_c.data(),
    };

    /* create instance */
    VK_ASSERT(vkCreateInstance(&inst_info, NULL, &vk_instance));
    err_scope([&]{ vkDestroyInstance(vk_instance, NULL); });

    DBG("Created a vulkan instance! %p", this);

    /* Create Vulkan Debug Messenger
    ============================================================================================= */

    VkDebugUtilsMessengerCreateInfoEXT dbg_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = dbg_cbk,
        .pUserData = nullptr,
    };

    VK_ASSERT(create_dbg_messenger(vk_instance, &dbg_info, NULL, &vk_dbg_messenger));
    err_scope([&]{ destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL); });

    err_scope.disable();
    DBG("Created vulkan messenger for instance %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t instance_t::uninit() {
    destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL);
    vkDestroyInstance(vk_instance, NULL);
    return VK_SUCCESS;
}

inline std::string instance_t::to_string() const {
    std::string exts = "[";
    std::string lays;
    for (auto ext: m_extensions)
        exts += ext + ", ";
    for (auto lay: m_layers)
        lays += lay + ", ";
    exts += "]";
    lays += "]";
    return std::format("vku::instance[{}]: m_app_name={}, m_engine_name={}, m_extensions={} "
            "m_layers={}",
            (void*)this, m_app_name, m_engine_name, exts, lays);
}

/* surface_t
================================================================================================= */

inline ref_t<surface_t> surface_t::create(
        ref_t<window_t> window,
        ref_t<instance_t> inst)
{
    auto ret = std::make_shared<surface_t>(object_t::Private{type_id_static()});
    ret->m_window = window;
    ret->m_instance = inst;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t surface_t::init() {
    if (!m_instance || !m_window)
        throw except_t("Invalid internal pointers");
    if (glfwCreateWindowSurface(m_instance->vk_instance, m_window->get_window(),
            NULL, &vk_surface) != VK_SUCCESS)
    {
        DBG("Failed to get vk_surface: %s", glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    DBG("Created Surface %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t surface_t::uninit() {
    vkDestroySurfaceKHR(m_instance->vk_instance, vk_surface, NULL);
    return VK_SUCCESS;
}

inline std::string surface_t::to_string() const {
    return std::format("vku::surface[{}]: m_window={}, m_instance={}",
            (void*)this, (void*)m_instance.get(), (void*)m_window.get());
}

/* device_t
================================================================================================= */

/* TODO: device_t should only depend on instance, not on surface */
inline ref_t<device_t> device_t::create(ref_t<instance_t> inst, ref_t<surface_t> surf,
        std::vector<std::string> exts, std::vector<std::string> layers)
{
    auto ret = std::make_shared<device_t>(object_t::Private{type_id_static()});
    ret->m_instance = inst;
    ret->m_surface = surf;
    ret->m_extensions = exts;
    ret->m_layers = layers;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t device_t::init() {
    if (!m_instance)
        throw except_t("Invalid m_instance");
    uint32_t dev_cnt = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(m_instance->vk_instance, &dev_cnt, NULL));

    if (dev_cnt == 0) {
        DBG("Failed to find a GPU with Vulkan support!")
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }

    std::vector<VkPhysicalDevice> devices(dev_cnt);
    VK_ASSERT(vkEnumeratePhysicalDevices(m_instance->vk_instance, &dev_cnt, devices.data()));

    vk_phy_dev = VK_NULL_HANDLE;
    int max_score = -1;
    for (const auto &dev : devices) {
        int score = score_phydev(dev, m_surface ? m_surface->vk_surface : nullptr);
        if (score < 0)
            continue;
        if (score > max_score) {
            max_score = score;
            vk_phy_dev = dev;
        }
    }
    if (max_score < 0) {
        DBG("Failed to get a suitable physical device");
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    VkPhysicalDeviceProperties dev_prop;
    vkGetPhysicalDeviceProperties(vk_phy_dev, &dev_prop);
    DBG("Selected GPU Name: %s", dev_prop.deviceName);

    m_que_fams = find_queue_families(vk_phy_dev, m_surface ? m_surface->vk_surface : nullptr);
    m_que_ids = { m_que_fams.graphics_id, m_que_fams.present_id };
    std::vector<float> queue_prio(m_que_fams.max_graphics_queue_cnt, 1.0f);
    std::vector<VkDeviceQueueCreateInfo> dev_ques = {
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = (uint32_t)m_que_fams.graphics_id,
            .queueCount = m_que_fams.max_graphics_queue_cnt,
            .pQueuePriorities = queue_prio.data()
        },
        VkDeviceQueueCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = (uint32_t)m_que_fams.present_id,
            .queueCount = 1,
            .pQueuePriorities = queue_prio.data() /* only one and gpu_queue_cnt at least 1 so ok */
        }
    };

    VkPhysicalDeviceFeatures dev_feat{};
    vkGetPhysicalDeviceFeatures(vk_phy_dev, &dev_feat);

    std::vector<const char*> dev_exts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    for (auto &e : m_extensions)
        dev_exts.push_back(e.c_str());

    std::vector<const char*> dev_layers = { "VK_LAYER_KHRONOS_validation" };
    for (auto &l : m_layers)
        dev_layers.push_back(l.c_str());

    dev_feat.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vk11_features {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = nullptr,
        .samplerMirrorClampToEdge                           = true,
        .drawIndirectCount                                  = true,
        .storageBuffer8BitAccess                            = true,
        .uniformAndStorageBuffer8BitAccess                  = true,
        .storagePushConstant8                               = true,
        .shaderBufferInt64Atomics                           = true,
        .shaderSharedInt64Atomics                           = true,
        .shaderFloat16                                      = true,
        .shaderInt8                                         = true,
        .descriptorIndexing                                 = true,
        .shaderInputAttachmentArrayDynamicIndexing          = true,
        .shaderUniformTexelBufferArrayDynamicIndexing       = true,
        .shaderStorageTexelBufferArrayDynamicIndexing       = true,
        .shaderUniformBufferArrayNonUniformIndexing         = true,
        .shaderSampledImageArrayNonUniformIndexing          = true,
        .shaderStorageBufferArrayNonUniformIndexing         = true,
        .shaderStorageImageArrayNonUniformIndexing          = true,
        .shaderInputAttachmentArrayNonUniformIndexing       = true,
        .shaderUniformTexelBufferArrayNonUniformIndexing    = true,
        .shaderStorageTexelBufferArrayNonUniformIndexing    = true,
        .descriptorBindingUniformBufferUpdateAfterBind      = true,
        .descriptorBindingSampledImageUpdateAfterBind       = true,
        .descriptorBindingStorageImageUpdateAfterBind       = true,
        .descriptorBindingStorageBufferUpdateAfterBind      = true,
        .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
        .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
        .descriptorBindingUpdateUnusedWhilePending          = true,
        .descriptorBindingPartiallyBound                    = true,
        .descriptorBindingVariableDescriptorCount           = true,
        .runtimeDescriptorArray                             = true,
        .samplerFilterMinmax                                = true,
        .scalarBlockLayout                                  = true,
        .imagelessFramebuffer                               = true,
        .uniformBufferStandardLayout                        = true,
        .shaderSubgroupExtendedTypes                        = true,
        .separateDepthStencilLayouts                        = true,
        .hostQueryReset                                     = true,
        .timelineSemaphore                                  = true,
        .bufferDeviceAddress                                = true,
        .bufferDeviceAddressCaptureReplay                   = true,
        .bufferDeviceAddressMultiDevice                     = true,
        .vulkanMemoryModel                                  = true,
        .vulkanMemoryModelDeviceScope                       = true,
        .vulkanMemoryModelAvailabilityVisibilityChains      = true,
        .shaderOutputViewportIndex                          = true,
        .shaderOutputLayer                                  = true,
        .subgroupBroadcastDynamicId                         = true,
    };

    VkDeviceCreateInfo dev_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &vk11_features,
        .flags = 0,
        .queueCreateInfoCount = (uint32_t)dev_ques.size(),
        .pQueueCreateInfos = dev_ques.data(),
        .enabledLayerCount = (uint32_t)dev_layers.size(),
        .ppEnabledLayerNames = dev_layers.data(),
        .enabledExtensionCount = (uint32_t)dev_exts.size(),
        .ppEnabledExtensionNames = dev_exts.data(),
        .pEnabledFeatures = &dev_feat,
    };

    VK_ASSERT(vkCreateDevice(vk_phy_dev, &dev_info, NULL, &vk_dev));

    vk_graphics_que.resize(m_que_fams.max_graphics_queue_cnt);
    for (uint32_t i = 0; i < m_que_fams.max_graphics_queue_cnt; i++) {
        vkGetDeviceQueue(vk_dev, m_que_fams.graphics_id, i, &vk_graphics_que[i]);
    }
    vkGetDeviceQueue(vk_dev, m_que_fams.present_id, 0, &vk_present_que);

    DBG("Created Vulkan Logical Device %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t device_t::uninit() {
    vkDestroyDevice(vk_dev, NULL);
    return VK_SUCCESS;
}

inline std::string device_t::to_string() const {
    return std::format("vku::device[{}]: m_surface={}", (void*)this, (void*)m_surface.get());
}

/* swapchain_t
================================================================================================= */

inline ref_t<swapchain_t> swapchain_t::create(ref_t<device_t> dev, ref_t<surface_t> surf) {
    auto ret = std::make_shared<swapchain_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_surface = surf;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t swapchain_t::init() {
    if (!m_device || !m_surface)
        throw except_t("Invalid internal ptr");
    FnScope err_scope;
    auto sc_detail = get_swapchain_details(m_device->vk_phy_dev, m_surface->vk_surface);

    /* choose format */
    vk_surf_fmt = sc_detail.formats[0];
    for (const auto &f : sc_detail.formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace ==
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            vk_surf_fmt = f;
        }

    /* choose presentation mode */
    vk_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& pm : sc_detail.present_modes)
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
            vk_present_mode = pm;

    /* choose swap extent */
    vk_extent = choose_extent(m_surface->m_window->get_window(), sc_detail.capab);

    /* choose image count */
    uint32_t img_cnt = sc_detail.capab.minImageCount + 1;
    if (sc_detail.capab.maxImageCount > 0 && img_cnt > sc_detail.capab.maxImageCount)
        img_cnt = sc_detail.capab.maxImageCount;

    std::vector<uint32_t> qf_arr{ m_device->m_que_ids.begin(), m_device->m_que_ids.end() };

    VkSwapchainCreateInfoKHR sc_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = m_surface->vk_surface,
        .minImageCount = img_cnt,
        .imageFormat = vk_surf_fmt.format,
        .imageColorSpace = vk_surf_fmt.colorSpace,
        .imageExtent = vk_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = qf_arr.size() > 1 ? VK_SHARING_MODE_CONCURRENT
                                                   : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = qf_arr.size() > 1 ? 2u : 0u,
        .pQueueFamilyIndices   = qf_arr.size() > 1 ? qf_arr.data() : NULL,
        .preTransform = sc_detail.capab.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vk_present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    VK_ASSERT(vkCreateSwapchainKHR(m_device->vk_dev, &sc_info, NULL, &vk_swapchain));
    err_scope([&]{ vkDestroySwapchainKHR(m_device->vk_dev, vk_swapchain, NULL); });

    img_cnt = 0;
    VK_ASSERT(vkGetSwapchainImagesKHR(m_device->vk_dev, vk_swapchain, &img_cnt, NULL));
    vk_sc_images.resize(img_cnt);
    VK_ASSERT(vkGetSwapchainImagesKHR(m_device->vk_dev, vk_swapchain, &img_cnt,
            vk_sc_images.data()));

    DBG("Created Swapchain %p", this);

    /* Create Swapchain Image Views
    ============================================================================================= */

    vk_sc_image_views.resize(vk_sc_images.size());

    for (size_t i = 0; i < vk_sc_images.size(); i++) {
        VkImageViewCreateInfo iv_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = vk_sc_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_surf_fmt.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };
        VK_ASSERT(vkCreateImageView(m_device->vk_dev, &iv_info, NULL, &vk_sc_image_views[i]));
        err_scope([i, this]{ 
            vkDestroyImageView(m_device->vk_dev, vk_sc_image_views[i], NULL); });
    }

    /* TODO: is this still a problem? TODO: this is problematic, here depth_imag is dependent on
    dev and not on swapchain, so if we delete dev we have a double free */
    m_depth_imag = image_t::create(m_device, vk_extent.width, vk_extent.height,
            VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    m_depth_view = img_view_t::create(m_depth_imag, VK_IMAGE_ASPECT_DEPTH_BIT);

    DBG("Created Swapchain Images %p", this);

    err_scope.disable();
    return VK_SUCCESS;
}

inline vc::ret_t swapchain_t::uninit() {
    for (auto &iv : vk_sc_image_views)
        vkDestroyImageView(m_device->vk_dev, iv, NULL);
    vkDestroySwapchainKHR(m_device->vk_dev, vk_swapchain, NULL);
    return VK_SUCCESS;
}

inline std::string swapchain_t::to_string() const {
    return std::format("vku::swapchain[{}]: m_device={}", (void*)this, (void*)m_device.get());
}

/* shader_t
================================================================================================= */

inline ref_t<shader_t> shader_t::create(ref_t<device_t> dev, const spirv_t& spirv) {
    auto ret = std::make_shared<shader_t>(object_t::Private{type_id_static()});
    ret->m_init_from_path = false;
    ret->m_device = dev;
    ret->m_spirv = spirv;
    VK_ASSERT(ret->init());
    return ret;
}

inline ref_t<shader_t> shader_t::create(ref_t<device_t> dev, const char *path,
        vku_shader_stage_e type)
{
    auto ret = std::make_shared<shader_t>(object_t::Private{type_id_static()});
    ret->m_init_from_path = true;
    ret->m_path = path;
    ret->m_type = type;
    ret->m_device = dev;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t shader_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device null");
    if (m_init_from_path) {
        std::ifstream file(m_path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();

        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            DBG("Failed to read shader data");
            throw vku::except_t(VK_ERROR_UNKNOWN);
        }
        VkShaderModuleCreateInfo shader_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = buffer.size(),
            .pCode = (uint32_t *)buffer.data(),
        };
        VK_ASSERT(vkCreateShaderModule(m_device->vk_dev, &shader_info, NULL, &vk_shader));
        DBG("Loaded shader from path [%s] of size: %zu data: %p -> vk_%p",
                m_path.c_str(), buffer.size(), buffer.data(), vk_shader);
    }
    else {
        VkShaderModuleCreateInfo shader_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = m_spirv.content.size() * sizeof(uint32_t),
            .pCode = m_spirv.content.data(),
        };
        m_type = m_spirv.type;
        VK_ASSERT(vkCreateShaderModule(m_device->vk_dev, &shader_info, NULL, &vk_shader));
        DBG("Loaded shader from buffer of size: %zu data: %p -> vk_%p",
                m_spirv.content.size() * sizeof(uint32_t), m_spirv.content.data(), vk_shader);
    }
    DBG("Created Shader %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t shader_t::uninit() {
    vkDestroyShaderModule(m_device->vk_dev, vk_shader, NULL);
    return VK_SUCCESS;
}

inline std::string shader_t::to_string() const {
    return std::format("vku::shader[{}]: m_device={} m_type={} {}",
            (void*)this, (void*)m_device.get(), vulkan_utils::to_string(m_type),
            m_init_from_path ? m_path : std::string("[Initialized from string, holds only spirv.]"));
}

/* renderpass_t
================================================================================================= */

inline ref_t<renderpass_t> renderpass_t::create(ref_t<swapchain_t> swc) {
    auto ret = std::make_shared<renderpass_t>(object_t::Private{type_id_static()});
    ret->m_swapchain = swc;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t renderpass_t::init() {
    if (!m_swapchain)
        throw except_t("Invalid m_swapchain null");
    VkAttachmentDescription color_attach {
        .flags = 0,
        .format = m_swapchain->vk_surf_fmt.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    VkAttachmentDescription depth_attach {
        .flags = 0,
        .format = m_swapchain->m_depth_view->m_image->m_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    /* indexes in color_attach vector, also in shader: layout(location = 0) out vec4 outColor */
    VkAttachmentReference color_attach_ref {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depth_attach_ref {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    /* subpasses are postprocessing stages, we only use one pass for now */
    VkSubpassDescription subpass {
        .flags = 0,
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attach_ref,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment = &depth_attach_ref,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr,
    };

    VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |   
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0,
    };

    std::array<VkAttachmentDescription, 2> attachments = {color_attach, depth_attach};
    VkRenderPassCreateInfo render_pass_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = (uint32_t)attachments.size(),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };

    VK_ASSERT(vkCreateRenderPass(m_swapchain->m_device->vk_dev,
            &render_pass_info, NULL, &vk_render_pass));

    DBG("Created Renderpass %p", this);

    return VK_SUCCESS;
}

inline vc::ret_t renderpass_t::uninit() {
    vkDestroyRenderPass(m_swapchain->m_device->vk_dev, vk_render_pass, NULL);
    return VK_SUCCESS;
}

inline std::string renderpass_t::to_string() const {
    return std::format("vku::renderpass[{}]: m_swapchain={}", (void*)this, (void*)m_swapchain.get());
}

/* pipeline_t
================================================================================================= */

inline ref_t<pipeline_t> pipeline_t::create(
        int width,
        int height,
        ref_t<renderpass_t> rp,
        const std::vector<ref_t<shader_t>> &shaders,
        VkPrimitiveTopology topology,
        vertex_input_desc_t input_desc,
        ref_t<pipeline_layout_t> pipeline_layout)
{
    auto ret = std::make_shared<pipeline_t>(object_t::Private{type_id_static()});
    ret->m_width = width;
    ret->m_height = height;
    ret->m_renderpass = rp;
    ret->m_shaders = shaders;
    ret->m_topology = topology;
    ret->m_input_desc = input_desc;
    ret->m_pipeline_layout = pipeline_layout;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t pipeline_t::init() {
    if (!m_renderpass || !m_pipeline_layout)
        throw except_t("Invalid internal pointers");
    FnScope err_scope;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    for (auto sh : m_shaders) {
        if (!sh)
            throw except_t("Invalid shader pointer");
        shader_stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = get_shader_type(sh->m_type),
            .module              = sh->vk_shader,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        });
        DBG("Added shader: %p, type: %x ",
                sh->vk_shader, get_shader_type(sh->m_type));
    }

    /* mark prop of pipeline to be mutable */
    std::vector<VkDynamicState> dyn_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH
    };

    VkPipelineDynamicStateCreateInfo dyn_info {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .dynamicStateCount  = uint32_t(dyn_states.size()),
        .pDynamicStates     = dyn_states.data()
    };

    VkPipelineVertexInputStateCreateInfo vertex_info {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &m_input_desc.bind_desc,
        .vertexAttributeDescriptionCount = (uint32_t)m_input_desc.attr_desc.size(),
        .pVertexAttributeDescriptions    = m_input_desc.attr_desc.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = m_topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = float(m_width),
        .height     = float(m_height),
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = m_renderpass->m_swapchain->vk_extent,
    };

    VkPipelineViewportStateCreateInfo vp_info {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext          = nullptr,
        .flags          = 0,
        .viewportCount  = 1,
        .pViewports     = &viewport,
        .scissorCount   = 1,
        .pScissors      = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo raster_info {
        .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                      = nullptr,
        .flags                      = 0,
        .depthClampEnable           = VK_FALSE,
        .rasterizerDiscardEnable    = VK_FALSE,
        .polygonMode                = VK_POLYGON_MODE_FILL,
        .cullMode                   = VK_CULL_MODE_NONE, // VK_CULL_MODE_BACK_BIT,
        .frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable            = VK_FALSE,
        .depthBiasConstantFactor    = 0.0f,
        .depthBiasClamp             = 0.0f,
        .depthBiasSlopeFactor       = 0.0f,
        .lineWidth                  = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisample_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable    = VK_FALSE,
        .minSampleShading       = 1.0f,
        .pSampleMask            = NULL,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    VkPipelineColorBlendAttachmentState blend_attachment {
        .blendEnable            = VK_FALSE,
        .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp           = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp           = VK_BLEND_OP_ADD,
        .colorWriteMask         =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo blend_info {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .logicOpEnable      = VK_FALSE,
        .logicOp            = VK_LOGIC_OP_COPY,
        .attachmentCount    = 1,
        .pAttachments       = &blend_attachment,
        .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    VkPipelineDepthStencilStateCreateInfo depth_stancil{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = {},
        .back = {},
        .minDepthBounds = 0.0,
        .maxDepthBounds = 1.0,
    };

    VkGraphicsPipelineCreateInfo pipeline_info {
        .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .stageCount             = (uint32_t)shader_stages.size(),
        .pStages                = shader_stages.data(),
        .pVertexInputState      = &vertex_info,
        .pInputAssemblyState    = &input_assembly,
        .pTessellationState     = nullptr,
        .pViewportState         = &vp_info,
        .pRasterizationState    = &raster_info,
        .pMultisampleState      = &multisample_info,
        .pDepthStencilState     = &depth_stancil,
        .pColorBlendState       = &blend_info,
        .pDynamicState          = &dyn_info,
        .layout                 = m_pipeline_layout->vk_pipeline_layout,
        .renderPass             = m_renderpass->vk_render_pass,
        .subpass                = 0,
        .basePipelineHandle     = VK_NULL_HANDLE,
        .basePipelineIndex      = -1,
    };

    VK_ASSERT(vkCreateGraphicsPipelines(m_renderpass->m_swapchain->m_device->vk_dev,
            VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    err_scope.disable();
    DBG("Created Pipeline: %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t pipeline_t::uninit() {
    vkDestroyPipeline(m_renderpass->m_swapchain->m_device->vk_dev, vk_pipeline, NULL);
    return VK_SUCCESS;
}

inline std::string pipeline_t::to_string() const {
    std::string sh_str = "[";
    for (auto sh : m_shaders)
        sh_str += std::format("{}, ", (void*)sh.get());
    sh_str += "]";
    return std::format("vku::pipeline[{}]: m_width={} m_height={} m_renderpass={} m_shaders={} "
            "m_topology={} m_vertex_input_descriptor={} m_pipeline_layout={}",
            (void*)this, m_width, m_height, (void*)m_renderpass.get(), sh_str,
            vulkan_utils::to_string(m_topology), vulkan_utils::to_string(m_input_desc),
            (void*)m_pipeline_layout.get());
}

/* compute_pipeline_t
================================================================================================= */

inline ref_t<compute_pipeline_t> compute_pipeline_t::create(
        ref_t<device_t> dev,
        ref_t<shader_t> shader,
        ref_t<pipeline_layout_t> pipeline_layout)
{
    auto ret = std::make_shared<compute_pipeline_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_shader = shader;
    ret->m_pipeline_layout = pipeline_layout;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t compute_pipeline_t::init() {
    if (!m_device || !m_shader || !m_pipeline_layout)
        throw except_t("Invalid internal references");
    FnScope err_scope;

    if (get_shader_type(m_shader->m_type) != VK_SHADER_STAGE_COMPUTE_BIT) {
        throw vku::except_t("compute_pipeline needs a compute shader");
    }

    VkPipelineShaderStageCreateInfo shader_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .stage                  = get_shader_type(m_shader->m_type),
        .module                 = m_shader->vk_shader,
        .pName                  = "main",
        .pSpecializationInfo    = nullptr,
    };

    VkComputePipelineCreateInfo pipeline_info {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = shader_info,
        .layout             = m_pipeline_layout->vk_pipeline_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex  = 0,
    };

    VK_ASSERT(vkCreateComputePipelines(
            m_device->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    DBG("Created Compute Pipeline: %p", this);

    err_scope.disable();
    return VK_SUCCESS;
}

inline vc::ret_t compute_pipeline_t::uninit() {
    vkDestroyPipeline(m_device->vk_dev, vk_pipeline, NULL);
    return VK_SUCCESS;
}

inline std::string compute_pipeline_t::to_string() const {
    return std::format("vku::compute_pipeline[{}]: m_device={} m_shader={} m_pipeline_layout={}",
            (void*)this, (void*)m_device.get(), (void*)m_shader.get(), (void*)m_pipeline_layout.get());
}

/* framebuffs_t
================================================================================================= */

inline ref_t<framebuffs_t> framebuffs_t::create(ref_t<renderpass_t> rp){
    auto ret = std::make_shared<framebuffs_t>(object_t::Private{type_id_static()});
    ret->m_renderpass = rp;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t framebuffs_t::init() {
    if (!m_renderpass)
        throw except_t("Invalid m_renderpass");
    vk_fbuffs.resize(m_renderpass->m_swapchain->vk_sc_image_views.size());

    FnScope err_scope;
    for (size_t i = 0; i < vk_fbuffs.size(); i++) {
        VkImageView attachs[] = {
            m_renderpass->m_swapchain->vk_sc_image_views[i],
            m_renderpass->m_swapchain->m_depth_view->vk_view
        };
        
        VkFramebufferCreateInfo fbuff_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = m_renderpass->vk_render_pass,
            .attachmentCount = 2,
            .pAttachments = attachs,
            .width = m_renderpass->m_swapchain->vk_extent.width,
            .height = m_renderpass->m_swapchain->vk_extent.height,
            .layers = 1,
        };

        VK_ASSERT(vkCreateFramebuffer(m_renderpass->m_swapchain->m_device->vk_dev,
                &fbuff_info, NULL, &vk_fbuffs[i]));
        err_scope([this, i] { 
            vkDestroyFramebuffer(m_renderpass->m_swapchain->m_device->vk_dev, vk_fbuffs[i], NULL);
        });
    }

    DBG("Created Framebuffs %p", this);

    err_scope.disable();
    return VK_SUCCESS;
}

inline vc::ret_t framebuffs_t::uninit() {
    for (auto fbuff : vk_fbuffs)
        vkDestroyFramebuffer(m_renderpass->m_swapchain->m_device->vk_dev, fbuff, NULL);
    return VK_SUCCESS;
}

inline std::string framebuffs_t::to_string() const {
    return std::format("vku::framebuffs[{}]: m_renderpass={}", (void*)this,
            (void*)m_renderpass.get());
}

/* cmdpool_t
================================================================================================= */

inline ref_t<cmdpool_t> cmdpool_t::create(ref_t<device_t> dev) {
    auto ret = std::make_shared<cmdpool_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t cmdpool_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device");
    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (uint32_t)m_device->m_que_fams.graphics_id,
    };

    VK_ASSERT(vkCreateCommandPool(m_device->vk_dev, &pool_info, NULL, &vk_pool));
    DBG("Created Command Pool %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t cmdpool_t::uninit() {
    vkDestroyCommandPool(m_device->vk_dev, vk_pool, NULL);
    return VK_SUCCESS;
}

inline std::string cmdpool_t::to_string() const {
    return std::format("vku::cmdpool[{}]: m_device={}", (void*)this, (void*)m_device.get());
}

/* cmdbuff_t
================================================================================================= */

inline ref_t<cmdbuff_t> cmdbuff_t::create(ref_t<cmdpool_t> cp, bool host_free) {
    auto ret = std::make_shared<cmdbuff_t>(object_t::Private{type_id_static()});
    ret->m_cmdpool = cp;
    ret->m_host_free = host_free;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t cmdbuff_t::init() {
    if (!m_cmdpool)
        throw except_t("Invalid m_cmdpool");
    VkCommandBufferAllocateInfo buff_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = m_cmdpool->vk_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_ASSERT(vkAllocateCommandBuffers(m_cmdpool->m_device->vk_dev, &buff_info, &vk_buff));
    DBG("Created Command Buffer %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t cmdbuff_t::uninit() {
    if (m_host_free) {
        vkFreeCommandBuffers(m_cmdpool->m_device->vk_dev, m_cmdpool->vk_pool, 1, &vk_buff);
    }
    return VK_SUCCESS;
}

inline std::string cmdbuff_t::to_string() const {
    return std::format("vku::cmdbuff[{}]: m_cmdpool={} host_free={}",
            (void*)this, (void*)m_cmdpool.get(), m_host_free);
}

inline void cmdbuff_t::begin(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = NULL,
    };
    VK_ASSERT(vkBeginCommandBuffer(vk_buff, &begin_info));
}

inline void cmdbuff_t::begin_rpass(ref_t<framebuffs_t> fbs, uint32_t img_idx) {
    if (!fbs)
        throw except_t("Invalid fbs param");
    VkClearValue clear_color[] = {
        {
            .color = {{ 0.0f, 0.0f, 0.0f, 1.0f }},
        },
        {
            .depthStencil = { 1.0f, 0 },
        }
    };

    VkRenderPassBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = fbs->m_renderpass->vk_render_pass,
        .framebuffer = fbs->vk_fbuffs[img_idx],
        .renderArea = {
            .offset = {0, 0},
            .extent = fbs->m_renderpass->m_swapchain->vk_extent,
        },
        .clearValueCount = 2,
        .pClearValues = clear_color,
    };

    vkCmdBeginRenderPass(vk_buff, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

inline void cmdbuff_t::bind_vert_buffs(uint32_t first_bind,
        std::vector<std::pair<ref_t<buffer_t>, VkDeviceSize>> buffs)
{
    std::vector<VkBuffer> vk_buffs;
    std::vector<VkDeviceSize> vk_offsets;
    for (auto [b, off] : buffs) {
        if (!b)
            throw except_t("Invalid buffer in param");
        vk_buffs.push_back(b->vk_buff);
        vk_offsets.push_back(off);
    }

    vkCmdBindVertexBuffers(vk_buff, first_bind, vk_buffs.size(), vk_buffs.data(),
            vk_offsets.data());
}

inline void cmdbuff_t::bind_desc_set(VkPipelineBindPoint bind_point,
        ref_t<pipeline_layout_t> pl, ref_t<desc_set_t> desc_set)
{
    if (!pl || !desc_set)
        throw except_t("Invalid null param");
    DBGVVV("bind desc_set: %p with layout: %p bind_point: %d",
            desc_set->vk_desc_set, pl->vk_pipeline_layout, bind_point);
    vkCmdBindDescriptorSets(vk_buff, bind_point, pl->vk_pipeline_layout, 0, 1,
            &desc_set->vk_desc_set, 0, nullptr);
}

inline void cmdbuff_t::bind_idx_buff(ref_t<buffer_t> ibuff, uint64_t off,
        VkIndexType idx_type)
{
    vkCmdBindIndexBuffer(vk_buff, ibuff->vk_buff, off, idx_type);
}

inline void cmdbuff_draw_helper(VkCommandBuffer vk_buff, ref_t<pipeline_t> pl) {
    if (!pl)
        throw except_t("Invalid pipeline param");
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->vk_pipeline);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(pl->m_renderpass->m_swapchain->vk_extent.width),
        .height = float(pl->m_renderpass->m_swapchain->vk_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(vk_buff, 0, 1, &viewport);

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = pl->m_renderpass->m_swapchain->vk_extent
    };
    vkCmdSetScissor(vk_buff, 0, 1, &scissor);
}

inline void cmdbuff_t::draw(ref_t<pipeline_t> pl, uint64_t vert_cnt) {
    if (!pl)
        throw except_t("Invalid pipeline param");
    cmdbuff_draw_helper(vk_buff, pl);
    vkCmdDraw(vk_buff, vert_cnt, 1, 0, 0);
}

inline void cmdbuff_t::draw_idx(ref_t<pipeline_t> pl, uint64_t vert_cnt) {
    if (!pl)
        throw except_t("Invalid pipeline param");
    cmdbuff_draw_helper(vk_buff, pl);
    vkCmdDrawIndexed(vk_buff, vert_cnt, 1, 0, 0, 0);
}

inline void cmdbuff_t::end_rpass() {
    vkCmdEndRenderPass(vk_buff);
}

inline void cmdbuff_t::end() {
    VK_ASSERT(vkEndCommandBuffer(vk_buff));
}

inline void cmdbuff_t::reset() {
    VK_ASSERT(vkResetCommandBuffer(vk_buff, 0));
}

inline void cmdbuff_t::bind_compute(ref_t<compute_pipeline_t> cpl) {
    if (!cpl)
        throw except_t("Invalid pipeline param");
    DBGVVV("bind compute pipeline: %p", cpl->vk_pipeline);
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_COMPUTE, cpl->vk_pipeline);
}

inline void cmdbuff_t::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(vk_buff, x, y, z);
}

inline void cmdbuff_t::set_event(ref_t<event_t> event, VkPipelineStageFlags stage) {
    if (!event)
        throw except_t("Invalid event param");
    vkCmdSetEvent(vk_buff, event->vk_event, stage);
}

inline void cmdbuff_t::reset_event(ref_t<event_t> event, VkPipelineStageFlags stage) {
    if (!event)
        throw except_t("Invalid event param");
    vkCmdResetEvent(vk_buff, event->vk_event, stage);
}

inline void cmdbuff_t::wait_events(const std::vector<ref_t<event_t>>& events,
        ref_t<dependency_info_t> dep_info)
{
    if (!dep_info)
        throw except_t("Invalid dep_info param");
    std::vector<VkEvent> vk_events(events.size());
    for (size_t i = 0; i < events.size(); i++) {
        if (!events[i])
            throw except_t("Invalid events[i] param");
        vk_events[i] = events[i]->vk_event;
    }

    vkCmdWaitEvents(vk_buff, vk_events.size(), vk_events.data(),
            dep_info->m_src_stage_mask, dep_info->m_dst_stage_mask,
            dep_info->mem_bars.size(), dep_info->mem_bars.data(),
            dep_info->buff_mem_bars.size(), dep_info->buff_mem_bars.data(),
            dep_info->img_mem_bars.size(), dep_info->img_mem_bars.data());
}

inline void cmdbuff_t::pipeline_barrier(ref_t<dependency_info_t> dep_info) {
    if (!dep_info)
        throw except_t("Invalid dep_info param");
    vkCmdPipelineBarrier(vk_buff,
            dep_info->m_src_stage_mask, dep_info->m_dst_stage_mask, dep_info->m_dep_flags,
            dep_info->mem_bars.size(), dep_info->mem_bars.data(),
            dep_info->buff_mem_bars.size(), dep_info->buff_mem_bars.data(),
            dep_info->img_mem_bars.size(), dep_info->img_mem_bars.data());
}

/* sem_t
================================================================================================= */

inline ref_t<sem_t> sem_t::create(ref_t<device_t> dev, VkSemaphoreType sem_type, uint64_t initial) {
    auto ret = std::make_shared<sem_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_sem_type = sem_type;
    ret->m_initial = initial;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t sem_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkSemaphoreTypeCreateInfo sem_type_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext = nullptr,
        .semaphoreType = m_sem_type,
        .initialValue = m_initial,
    };
    VkSemaphoreCreateInfo sem_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &sem_type_info,
        .flags = 0,
    };

    VK_ASSERT(vkCreateSemaphore(m_device->vk_dev, &sem_info, NULL, &vk_sem));
    DBG("Created Semaphore %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t sem_t::uninit() {
    vkDestroySemaphore(m_device->vk_dev, vk_sem, NULL);
    return VK_SUCCESS;
}

inline std::string sem_t::to_string() const {
    return std::format("vku::sem_t[{}]: m_device={}", (void*)this, (void*)m_device.get());
}

inline uint64_t sem_t::get_counter() {
    uint64_t ret;
    VK_ASSERT(vkGetSemaphoreCounterValue(m_device->vk_dev, vk_sem, &ret));
    return ret;
}


inline void sem_t::signal(uint64_t val) {
    VkSemaphoreSignalInfo sig_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
        .pNext = nullptr,
        .semaphore = vk_sem,
        .value = val,
    };
    VK_ASSERT(vkSignalSemaphore(m_device->vk_dev, &sig_info));
}


/* event_t
================================================================================================= */

inline ref_t<event_t> event_t::create(ref_t<device_t> dev) {
    auto ret = std::make_shared<event_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t event_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkEventCreateInfo evt_info {
        .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };
    VK_ASSERT(vkCreateEvent(m_device->vk_dev, &evt_info, nullptr, &vk_event));
    return VK_SUCCESS;
}

inline vc::ret_t event_t::uninit() {
    vkDestroyEvent(m_device->vk_dev, vk_event, nullptr);
    return VK_SUCCESS;
}

inline std::string event_t::to_string() const {
    return std::format("vku::event_t[{}]: m_device={}", (void*)this, (void*)m_device.get());
}

/* fence_t
================================================================================================= */

inline ref_t<fence_t> fence_t::create(
        ref_t<device_t> dev,
        VkFenceCreateFlags flags)
{
    auto ret = std::make_shared<fence_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_flags = flags;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t fence_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkFenceCreateInfo fence_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = m_flags
    };

    VK_ASSERT(vkCreateFence(m_device->vk_dev, &fence_info, NULL, &vk_fence));
    DBG("Created Fence %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t fence_t::uninit() {
    vkDestroyFence(m_device->vk_dev, vk_fence, NULL);
    return VK_SUCCESS;
}

inline std::string fence_t::to_string() const {
    return std::format("vku::fence_t[{}]: m_device={} m_flags={}",
            (void*)this, (void*)m_device.get(),
            vulkan_utils::to_string((VkFenceCreateFlagBits)m_flags));
}

/* buffer_t
================================================================================================= */

inline ref_t<buffer_t> buffer_t::create(
        ref_t<device_t> dev,
        size_t size,
        VkBufferUsageFlags usage,
        VkSharingMode sh_mode,
        VkMemoryPropertyFlags mem_flags,
        void *host_ptr)
{
    auto ret = std::make_shared<buffer_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_size = size;
    ret->m_usage_flags = usage;
    ret->m_sharing_mode = sh_mode;
    ret->m_memory_flags = mem_flags;
    ret->m_host_ptr = host_ptr;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t buffer_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkBufferCreateInfo buff_info {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = m_size,
        .usage = m_usage_flags,
        .sharingMode = m_sharing_mode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VK_ASSERT(vkCreateBuffer(m_device->vk_dev, &buff_info, nullptr, &vk_buff));

    if (m_host_ptr) {
        /* TODO: figure out if this stuff can actually work */
        DBG("m_host_ptr: %p size %ld", m_host_ptr, m_size);

        auto vkGetMemoryHostPointerPropertiesEXT =
                (PFN_vkGetMemoryHostPointerPropertiesEXT)vkGetDeviceProcAddr(
                        m_device->vk_dev, "vkGetMemoryHostPointerPropertiesEXT");

        VkMemoryHostPointerPropertiesEXT ptr_props {
            .sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT,
            .pNext = nullptr,
            .memoryTypeBits = 1,
        };
        VK_ASSERT(vkGetMemoryHostPointerPropertiesEXT(m_device->vk_dev,
                VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
                m_host_ptr,
                &ptr_props))

        // VkMemoryRequirements mem_req;
        // vkGetBufferMemoryRequirements(m_device->vk_dev, vk_buff, &mem_req);

        VkImportMemoryHostPointerInfoEXT host_ext {
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
            .pNext = nullptr,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT,
            .pHostPointer = m_host_ptr,
        };
        VkMemoryAllocateInfo alloc_info {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &host_ext,
            .allocationSize = m_size,
            .memoryTypeIndex = find_memory_type(m_device, ptr_props.memoryTypeBits, m_memory_flags, m_size)
        };

        DBG("Will 'alloc' memory for: %p", m_host_ptr);
        VK_ASSERT(vkAllocateMemory(m_device->vk_dev, &alloc_info, nullptr, &vk_mem));
    }
    else {
        VkMemoryRequirements mem_req;
        vkGetBufferMemoryRequirements(m_device->vk_dev, vk_buff, &mem_req);

        VkMemoryAllocateInfo alloc_info {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = nullptr,
            .allocationSize = mem_req.size,
            .memoryTypeIndex = find_memory_type(m_device, mem_req.memoryTypeBits, m_memory_flags, mem_req.size)
        };        

        VK_ASSERT(vkAllocateMemory(m_device->vk_dev, &alloc_info, nullptr, &vk_mem));
    }
    
    VK_ASSERT(vkBindBufferMemory(m_device->vk_dev, vk_buff, vk_mem, 0));
    DBG("Created Buffer %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t buffer_t::uninit() {
    if (m_map_ptr)
        unmap_data();
    vkDestroyBuffer(m_device->vk_dev, vk_buff, nullptr);
    vkFreeMemory(m_device->vk_dev, vk_mem, nullptr);
    return VK_SUCCESS;
}

inline std::string buffer_t::to_string() const {
    return std::format("vku::buffer_t[{}]: m_device={} m_size={} m_usage={} m_share_mode={} "
            "m_mem_flags={}",
            (void*)this, (void*)m_device.get(), m_size,
            vulkan_utils::to_string((VkBufferUsageFlagBits)m_usage_flags),
            vulkan_utils::to_string(m_sharing_mode),
            vulkan_utils::to_string((VkMemoryPropertyFlagBits)m_memory_flags));
}

inline void *buffer_t::map_data(VkDeviceSize offset, VkDeviceSize size) {
    if (m_map_ptr) {
        DBG("Memory is already mapped!");
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    VK_ASSERT(vkMapMemory(m_device->vk_dev, vk_mem, offset, size ? size : m_size, 0, &m_map_ptr));
    return m_map_ptr;
}

inline void buffer_t::unmap_data() {
    if (!m_map_ptr) {
        DBG("Memory is not mapped, can't unmap");
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    vkUnmapMemory(m_device->vk_dev, vk_mem);
    m_map_ptr = nullptr;
}

/* image_t
================================================================================================= */

inline ref_t<image_t> image_t::create(
        ref_t<device_t> dev,
        uint32_t width,
        uint32_t height,
        VkFormat fmt,
        VkImageUsageFlags usage,
        VkImageTiling tiling)
{
    auto ret = std::make_shared<image_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_width = width;
    ret->m_height = height;
    ret->m_format = fmt;
    ret->m_usage = usage;
    ret->m_tiling = tiling;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t image_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = m_format,
        .extent = {
            .width = m_width,
            .height = m_height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = m_tiling,
        .usage = m_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_ASSERT(vkCreateImage(m_device->vk_dev, &image_info, nullptr, &vk_img));
    FnScope err_scope([&]{ vkDestroyImage(m_device->vk_dev, vk_img, nullptr); });

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(m_device->vk_dev, vk_img, &mem_req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(m_device, mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem_req.size),
    };

    VK_ASSERT(vkAllocateMemory(m_device->vk_dev, &alloc_info, nullptr, &vk_img_mem));
    VK_ASSERT(vkBindImageMemory(m_device->vk_dev, vk_img, vk_img_mem, 0));

    DBG("Created Image %p", this);
    err_scope.disable();
    return VK_SUCCESS;
}

inline vc::ret_t image_t::uninit() {
    vkDestroyImage(m_device->vk_dev, vk_img, nullptr);
    vkFreeMemory(m_device->vk_dev, vk_img_mem, nullptr);
    return VK_SUCCESS;
}

inline std::string image_t::to_string() const {
    return std::format("vku::sem_t[{}]: m_device={} m_width={} m_height={} m_format={} m_usage={}",
            (void*)this, (void*)m_device.get(), m_width, m_height, vulkan_utils::to_string(m_format),
            vulkan_utils::to_string((VkImageUsageFlagBits)m_usage));
}

inline void image_t::transition_layout(ref_t<cmdpool_t> cp,
        VkImageLayout old_layout, VkImageLayout new_layout, ref_t<cmdbuff_t> cbuff)
{
    if (!cp)
        throw except_t("Invalid cp param");
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = fence_t::create(cp->m_device);

    if (!existing_cbuff) {
        cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    }
    
    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = 0,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = vk_img,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    VkPipelineStageFlags src_stage;
    VkPipelineStageFlags dst_stage;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
             new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else {
        throw vku::except_t(sformat("unsupported layout transition for image! %d -> %d",
                old_layout, new_layout));
    }

    /* TODO: actually get rid of this transition_layout altogheter and move it as an outside fn */
    /* TODO: don't we need a transition from shader_read to transfer_dst? That for transfering
    inside the image later on? */

    vkCmdPipelineBarrier(cbuff->vk_buff, src_stage, dst_stage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

    if (!existing_cbuff) {
        cbuff->end();

        submit_cmdbuff({}, cbuff, fence, {});
        wait_fences({fence}, true, UINT64_MAX);
    }
}

inline void image_t::set_data(ref_t<cmdpool_t> cp, void *data, uint32_t sz,
        ref_t<cmdbuff_t> cbuff)
{
    if (!cp)
        throw except_t("Invalid cp ref");
    uint32_t img_sz = m_width * m_height * 4;

    if (img_sz != sz)
        throw vku::except_t(sformat("data size(%d) does not match with image size(%d)", sz, img_sz));

    auto buff = buffer_t::create(
        cp->m_device,
        img_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    memcpy(buff->map_data(0, img_sz), data, img_sz);
    buff->unmap_data();

    transition_layout(cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cbuff);

    auto fence = fence_t::create(cp->m_device);

    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }

    if (!existing_cbuff)
        cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VkBufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = { .x = 0, .y = 0, .z = 0 },
        .imageExtent = {
            .width = m_width,
            .height = m_height,
            .depth = 1,
        } 
    };
    vkCmdCopyBufferToImage(
            cbuff->vk_buff,
            buff->vk_buff,
            vk_img,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

    transition_layout(cp, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cbuff);

    if (!existing_cbuff) {
        cbuff->end();

        submit_cmdbuff({}, cbuff, fence, {});
        wait_fences({fence}, true, UINT64_MAX);
    }
}


/* img_view_t
================================================================================================= */

inline ref_t<img_view_t> img_view_t::create(ref_t<image_t> img, VkImageAspectFlags aspect_mask) {
    if (!img)
        throw except_t("Invalid img ref");
    auto ret = std::make_shared<img_view_t>(object_t::Private{type_id_static()});
    ret->m_image = img;
    ret->m_aspect_mask = aspect_mask;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t img_view_t::init() {
    VkImageViewCreateInfo view_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = m_image->vk_img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = m_image->m_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = m_aspect_mask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    VK_ASSERT(vkCreateImageView(m_image->m_device->vk_dev, &view_info, nullptr, &vk_view));
    DBG("Created Image View %p", this);
    return VK_SUCCESS;
}
inline vc::ret_t img_view_t::uninit() {
    vkDestroyImageView(m_image->m_device->vk_dev, vk_view, nullptr);
    return VK_SUCCESS;
}

inline std::string img_view_t::to_string() const {
    return std::format("vku::img_view_t[{}]: m_image={}", (void*)this, (void*)m_image.get(),
            vulkan_utils::to_string((VkImageAspectFlagBits)m_aspect_mask));
}

/* img_sampl_t
================================================================================================= */

inline ref_t<img_sampl_t> img_sampl_t::create(ref_t<device_t> dev, VkFilter filter) {
    auto ret = std::make_shared<img_sampl_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_filter = filter;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t img_sampl_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    VkPhysicalDeviceProperties dev_props;
    vkGetPhysicalDeviceProperties(m_device->vk_phy_dev, &dev_props);

    VkSamplerCreateInfo sampler_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = m_filter,
        .minFilter = m_filter,
        .mipmapMode = m_filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                                                   : VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VkBool32(m_filter == VK_FILTER_NEAREST ? VK_FALSE : VK_TRUE),
        .maxAnisotropy = dev_props.limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
    // OBS:
    // samplerInfo.anisotropyEnable = VK_FALSE;
    // samplerInfo.maxAnisotropy = 1.0f;

    VK_ASSERT(vkCreateSampler(m_device->vk_dev, &sampler_info, nullptr, &vk_sampler));
    DBG("Created Image Sampler %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t img_sampl_t::uninit() {
    vkDestroySampler(m_device->vk_dev, vk_sampler, nullptr);
    return VK_SUCCESS;
}

inline std::string img_sampl_t::to_string() const {
    return std::format("vku::img_sampl[{}]: m_device={} m_filter={}", (void*)this,
            (void*)m_device.get(), vulkan_utils::to_string(m_filter));
}

inline VkDescriptorSetLayoutBinding img_sampl_t::get_desc_set(uint32_t binding,
        VkShaderStageFlags stage)
{
    return {
        .binding = binding,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = stage,
        .pImmutableSamplers = nullptr,
    };
}

/* desc_pool_t
================================================================================================= */

inline ref_t<desc_pool_t> desc_pool_t::create(
        ref_t<device_t> dev,
        ref_t<desc_set_initializer_t> bindings_initer,
        uint32_t cnt)
{
    auto ret = std::make_shared<desc_pool_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_bindings_initer = bindings_initer;
    ret->m_cnt = cnt;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t desc_pool_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    if (!m_bindings_initer)
        throw except_t("Invalid m_bindings_initer ref");
    std::vector<VkDescriptorPoolSize> pool_sizes;
    std::map<decltype(m_bindings_initer->m_binds[0]->m_desc.descriptorType), uint32_t> type_cnt;
    for (auto &b : m_bindings_initer->m_binds)
        type_cnt[b->m_desc.descriptorType] += m_cnt;

    for (auto &[type, cnt] : type_cnt) {
        pool_sizes.push_back(VkDescriptorPoolSize{
            .type = type,
            .descriptorCount = cnt,
        });
        DBGVV("pool_size: type: %x sz: %d", type, cnt);
    }

    VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = m_cnt,
        .poolSizeCount = (uint32_t)pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_ASSERT(vkCreateDescriptorPool(m_device->vk_dev, &pool_info, nullptr, &vk_descpool));
    DBGVV("Allocated pool: %p", vk_descpool);
    DBG("Created Descriptor Pool %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t desc_pool_t::uninit() {
    vkDestroyDescriptorPool(m_device->vk_dev, vk_descpool, nullptr);
    return VK_SUCCESS;
}

inline std::string desc_pool_t::to_string() const {
    return std::format("vku::desc_pool[{}]: m_device={} m_binding_desc_set={} m_cnt={}",
            (void*)this, (void*)m_device.get(), (void*)m_bindings_initer.get(), m_cnt);
}

/* desc_set_t
================================================================================================= */

/*
Follow this logical steps (check if they are correct):
    - The layout describes the structure,
    - Then you allocate a set,
    - Then you write into it,
    - Then you bind it.

(This is nice to finally understand)
So multiple sets allow you to separate resources logically, e.g.:
    - Set 0 → global data (camera, lighting)
    - Set 1 → per-object data (model matrices, material)
    - Set 2 → textures, etc.

Barriers:
    A pipeline stage is a big block in the GPU
        (e.g., transfer engine, vertex shader, fragment shader).
    An access mask is a specific type of memory operation inside that stage (
        e.g., writing a buffer, reading a texture, writing a color attachment).
 */

inline ref_t<desc_set_t> desc_set_t::create(
        ref_t<desc_pool_t> desc_pool,
        ref_t<desc_set_layout_t> desc_set_layout,
        ref_t<desc_set_initializer_t> bindings_initer)
{
    auto ret = std::make_shared<desc_set_t>(object_t::Private{type_id_static()});
    ret->m_descriptor_pool = desc_pool;
    ret->m_desc_set_layout = desc_set_layout;
    ret->m_bindings_initer = bindings_initer;
    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t desc_set_t::init() {
    if (!m_descriptor_pool)
        throw except_t("Invalid m_descriptor_pool ref");
    if (!m_desc_set_layout)
        throw except_t("Invalid m_desc_set_layout ref");
    VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptor_pool->vk_descpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_desc_set_layout->vk_desc_set_layout,
    };

    VK_ASSERT(vkAllocateDescriptorSets(m_descriptor_pool->m_device->vk_dev, &alloc_info,
            &vk_desc_set));
    DBGVV("Allocated descriptor set: %p from pool: %p with layout: %p",
            vk_desc_set, m_descriptor_pool->vk_descpool, m_desc_set_layout->vk_desc_set_layout);

    if (m_bindings_initer)
        m_bindings_initer->update_set(this->to_related<desc_set_t>());
    DBG("Created Descriptor Set %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t desc_set_t::uninit() {
    return VK_SUCCESS;
}

inline std::string desc_set_t::to_string() const {
    return std::format("vku::desc_set[{}]: m_desc_pool={} m_desc_set_layout={} m_binding_desc_set={}",
            (void*)this, (void*)m_descriptor_pool.get(), (void*)m_desc_set_layout.get(),
            (void*)m_bindings_initer.get());
}

/* desc_set_layout_t:
================================================================================================= */

inline ref_t<desc_set_layout_t> desc_set_layout_t::create(ref_t<device_t> dev,
        ref_t<desc_set_initializer_t> bindings_initer)
{
    auto ret = std::make_shared<desc_set_layout_t>(object_t::Private{type_id_static()});
    ret->m_device = dev;
    ret->m_bindings_initer = bindings_initer;

    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t desc_set_layout_t::init() {
    if (!m_device)
        throw except_t("Invalid m_device ref");
    if (!m_bindings_initer)
        throw except_t("Invalid m_bindings_initer ref");
    auto bind_descriptors = m_bindings_initer->get_descriptors();
    DBGVV("cnt bind_descriptors: %zu", bind_descriptors.size());
    for (auto &b : bind_descriptors) {
        DBGVV("Descriptor: type: %x, bind: %d, stage: %x ",
                b.descriptorType, b.binding, b.stageFlags);
    }

    VkDescriptorSetLayoutCreateInfo desc_set_layout_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = (uint32_t)bind_descriptors.size(),
        .pBindings = bind_descriptors.data(),
    };

    VK_ASSERT(vkCreateDescriptorSetLayout(m_device->vk_dev, &desc_set_layout_info, nullptr,
            &vk_desc_set_layout));
    DBGVV("Allocated descriptor set layout: %p", vk_desc_set_layout);
    DBG("Created Descriptor Set Layout %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t desc_set_layout_t::uninit() {
    vkDestroyDescriptorSetLayout(m_device->vk_dev, vk_desc_set_layout, nullptr);
    return VK_SUCCESS;
}

inline std::string desc_set_layout_t::to_string() const {
    return std::format("vku::desc_set_layout_t[{}]: m_device={} m_bindings_initer={}",
            (void*)this, (void*)m_device.get(), (void*)m_bindings_initer.get());
}

/* pipeline_layout_t:
================================================================================================= */

inline ref_t<pipeline_layout_t> pipeline_layout_t::create(ref_t<desc_set_layout_t> desc_set_layout) {
    auto ret = std::make_shared<pipeline_layout_t>(object_t::Private{type_id_static()});
    ret->m_desc_set_layout = desc_set_layout;

    VK_ASSERT(ret->init());
    return ret;
}

inline vc::ret_t pipeline_layout_t::init() {
    if (!m_desc_set_layout)
        throw except_t("Invalid m_desc_set_layout ref");
    VkPipelineLayoutCreateInfo pipeline_layout_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &m_desc_set_layout->vk_desc_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };

    VK_ASSERT(vkCreatePipelineLayout(m_desc_set_layout->m_device->vk_dev, &pipeline_layout_info,
            NULL, &vk_pipeline_layout));
    DBGVV("Allocated pipeline layout: %p", vk_pipeline_layout);
    DBG("Created Pipeline Layout %p", this);
    return VK_SUCCESS;
}

inline vc::ret_t pipeline_layout_t::uninit() {
    vkDestroyPipelineLayout(m_desc_set_layout->m_device->vk_dev, vk_pipeline_layout, NULL);
    return VK_SUCCESS;
}

inline std::string pipeline_layout_t::to_string() const {
    return std::format("vku::desc_set_layout_t[{}]: m_desc_set_layout={}",
            (void*)this, (void*)m_desc_set_layout.get());
}

/* desc_set_initializer_t:
================================================================================================= */

inline ref_t<desc_set_initializer_t::buff_binding_t> desc_set_initializer_t::buff_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        ref_t<buffer_t> buff,
        size_t off,
        size_t sz)
{
    using bd_t = desc_set_initializer_t::buff_binding_t;
    ref_t<bd_t> ret = std::make_shared<bd_t>(object_t::Private{type_id_static()});
    ret->m_desc = desc;
    ret->set_buffer(buff, off, sz);
    return ret;
}

inline void desc_set_initializer_t::buff_binding_t::set_buffer(ref_t<buffer_t> buff,
        uint64_t offset, uint64_t size)
{
    desc_buff_info.offset = offset;
    if (buff) {
        desc_buff_info.buffer = buff->vk_buff;
        desc_buff_info.range = size ? size : buff->m_size;
    }
    else if (size) {
        desc_buff_info.range = size;
    }
}


inline std::string desc_set_initializer_t::buff_binding_t::to_string() const {
    return std::format("vku::desc_set_initializer_t::buff_binding_t[{}]: m_desc={} "
            "buff = {} range = {} offset = {}",
            (void*)this, vulkan_utils::to_string(m_desc),
            (void*)desc_buff_info.buffer, desc_buff_info.range, desc_buff_info.offset);
}

inline VkWriteDescriptorSet desc_set_initializer_t::buff_binding_t::get_write() const {
    VkWriteDescriptorSet desc_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = 0, /* will be filled later */
        .dstBinding = m_desc.binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = m_desc.descriptorType,
        .pImageInfo = nullptr,
        .pBufferInfo = &desc_buff_info,
        .pTexelBufferView = nullptr,
    };

    return desc_write;
}

inline ref_t<desc_set_initializer_t::sampl_binding_t> desc_set_initializer_t::sampl_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        ref_t<img_view_t> view,
        ref_t<img_sampl_t> sampl,
        VkImageLayout img_layout)
{
    using sb_t = desc_set_initializer_t::sampl_binding_t;
    ref_t<sb_t> ret = std::make_shared<sb_t>(object_t::Private{type_id_static()});
    ret->m_desc = desc;
    ret->set_view(view);
    ret->set_sampler(sampl);
    ret->set_layout(img_layout);
    return ret;
}

inline void desc_set_initializer_t::sampl_binding_t::set_view(ref_t<img_view_t> view) {
    imag_info.imageView = view ? view->vk_view : nullptr;
}

inline void desc_set_initializer_t::sampl_binding_t::set_sampler(ref_t<img_sampl_t> sampl) {
    imag_info.sampler = sampl ? sampl->vk_sampler : nullptr;
}

inline void desc_set_initializer_t::sampl_binding_t::set_layout(VkImageLayout img_layout) {
    imag_info.imageLayout = img_layout;
}

inline std::string desc_set_initializer_t::sampl_binding_t::to_string() const {
    return std::format("vku::desc_set_initializer_t::sampl_binding_t[{}]: m_desc={} "
            "view={} sampler={} layout={}",
            (void*)this, vulkan_utils::to_string(m_desc),
            (void*)imag_info.imageView, (void*)imag_info.sampler, (uint32_t)imag_info.imageLayout);
}

inline VkWriteDescriptorSet desc_set_initializer_t::sampl_binding_t::get_write() const {
    VkWriteDescriptorSet desc_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = 0, /* will be filled later */
        .dstBinding = m_desc.binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = m_desc.descriptorType,
        .pImageInfo = &imag_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    return desc_write;
}

inline ref_t<desc_set_initializer_t> desc_set_initializer_t::create(
        std::vector<ref_t<binding_desc_t>> binds)
{
    auto ret = std::make_shared<desc_set_initializer_t>(object_t::Private{type_id_static()});
    for (auto &b : binds)
        if (!b)
            throw vku::except_t("invalid binding ref in binds");
    ret->m_binds = binds;
    return ret;
}

inline std::string desc_set_initializer_t::to_string() const {
    std::string binds_str = "[";
    for (auto b : m_binds)
        binds_str += std::format("{}, ", (void*)b.get());
    binds_str += "]";
    return std::format("vku::desc_set_initializer_t[{}]: m_bindings={} ",
            (void*)this, binds_str);
}

inline ref_t<desc_set_initializer_t::binding_desc_t> desc_set_initializer_t::get_binding(uint32_t i) {
    if (i >= m_binds.size())
        throw vku::except_t("outside of bounds!");
    return m_binds[i];
}

inline void desc_set_initializer_t::update_set(ref_t<desc_set_t> ds) {
    if (!ds)
        throw vku::except_t("invalid ds ref");
    /* TODO: maybe make a per-thread arrat of desc writes because this allocates memory for no
    reason */
    auto desc_writes = get_writes(ds->vk_desc_set);

    DBG("writes: %zu", desc_writes.size());
    for (auto &w : desc_writes) {
        DBG("write: type: %s, bind: %d, dst_set: %p .pBufferInfo: %p .pImageInfo: %p",
                vulkan_utils::to_string(w.descriptorType).c_str(), w.dstBinding, w.dstSet,
                w.pBufferInfo, w.pImageInfo);
    }

    vkUpdateDescriptorSets(ds->m_descriptor_pool->m_device->vk_dev,
            (uint32_t)desc_writes.size(), desc_writes.data(), 0, nullptr);
}

inline std::vector<VkWriteDescriptorSet> desc_set_initializer_t::get_writes(
        VkDescriptorSet dst_set) const
{
    std::vector<VkWriteDescriptorSet> ret;

    for (auto &b : m_binds) {
        ret.push_back(b->get_write());
        ret.back().dstSet = dst_set;
    }

    return ret;
}

inline std::vector<VkDescriptorSetLayoutBinding> desc_set_initializer_t::get_descriptors() const {
    std::vector<VkDescriptorSetLayoutBinding> ret;
    for (auto &b : m_binds)
        ret.push_back(b->m_desc);
    return ret;
}


/* Functions:
================================================================================================= */

inline void wait_fences(std::vector<ref_t<fence_t>> fences, bool wait_all, uint64_t timeout) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences) {
        if (!f)
            throw except_t("Invalid fence in fences");
        vk_fences.push_back(f->vk_fence);
    }

    VK_ASSERT(vkWaitForFences(fences[0]->m_device->vk_dev,
            vk_fences.size(), vk_fences.data(), wait_all, timeout));
}

inline void reset_fences(std::vector<ref_t<fence_t>> fences) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->vk_fence);
    VK_ASSERT(vkResetFences(fences[0]->m_device->vk_dev,
            vk_fences.size(), vk_fences.data()));
}

inline void wait_semaphores(const std::vector<ref_t<sem_t>> &sems, const std::vector<uint64_t> vals,
        bool wait_any, uint64_t timeo_ns)
{
    if (sems.empty())
        return ;
    if (sems.size() != vals.size())
        throw vku::except_t("sems.size() must be equal to vals.size()");
    std::vector<VkSemaphore> vk_sems(sems.size());
    for (size_t i = 0; i < sems.size(); i++) {
        if (!sems[i])
            throw except_t("Invalid semaphore in sems");
        vk_sems[i] = sems[i]->vk_sem;
    }
    VkSemaphoreWaitInfo wait_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = (uint32_t)(wait_any ? VK_SEMAPHORE_WAIT_ANY_BIT : 0),
        .semaphoreCount = (uint32_t)sems.size(),
        .pSemaphores = &vk_sems[0],
        .pValues = &vals[0],
    };
    VK_ASSERT(vkWaitSemaphores(sems[0]->m_device->vk_dev, &wait_info, timeo_ns));
}


inline void aquire_next_img(ref_t<swapchain_t> swc, ref_t<sem_t> sem,
        uint32_t *img_idx)
{
    if (!swc)
        throw except_t("Invalid swc ref");
    if (!sem)
        throw except_t("Invalid sem ref");
    VK_ASSERT(vkAcquireNextImageKHR(swc->m_device->vk_dev, swc->vk_swapchain,
            UINT64_MAX, sem->vk_sem, VK_NULL_HANDLE, img_idx));
}

inline void submit_cmdbuff(
        std::vector<std::pair<ref_t<sem_t>, VkPipelineStageFlagBits>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<ref_t<sem_t>> sig_sems,
        uint32_t queue_id)
{
    if (!cbuff)
        throw except_t("Invalid cbuff ref");
    std::vector<VkPipelineStageFlags> vk_wait_stages;
    std::vector<VkSemaphore> vk_wait_sems;
    std::vector<VkSemaphore> vk_sig_sems;

    for (auto [s, wait_stage] : wait_sems) {
        if (!s)
            throw except_t("Invalid sem in wait_sems");
        vk_wait_stages.push_back((VkPipelineStageFlags)wait_stage);
        vk_wait_sems.push_back(s->vk_sem);
    }
    for (auto s : sig_sems) {
        if (!s)
            throw except_t("Invalid sem in sig_sems");
        vk_sig_sems.push_back(s->vk_sem);
    }
    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)vk_wait_sems.size(),
        .pWaitSemaphores = vk_wait_sems.size() == 0 ? nullptr : vk_wait_sems.data(),
        .pWaitDstStageMask = vk_wait_sems.size() == 0 ? nullptr : vk_wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cbuff->vk_buff,
        .signalSemaphoreCount = (uint32_t)vk_sig_sems.size(),
        .pSignalSemaphores = vk_sig_sems.size() == 0 ? nullptr : vk_sig_sems.data(),
    };

    if (queue_id > cbuff->m_cmdpool->m_device->vk_graphics_que.size())
        throw vku::except_t("Invalid queue index");
    VK_ASSERT(vkQueueSubmit(cbuff->m_cmdpool->m_device->vk_graphics_que[queue_id], 1, &submit_info,
            fence == nullptr ? nullptr : fence->vk_fence));
}

inline void submit_cmdbuff_tl(
        std::vector<std::tuple<ref_t<sem_t>, VkPipelineStageFlagBits, uint64_t>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<std::tuple<ref_t<sem_t>, uint64_t>> sig_sems,
        uint32_t queue_id)
{
    if (!cbuff)
        throw except_t("Invalid cbuff ref");
    std::vector<VkPipelineStageFlags> vk_wait_stages;
    std::vector<VkSemaphore> vk_wait_sems;
    std::vector<VkSemaphore> vk_sig_sems;
    std::vector<uint64_t> wait_vals;
    std::vector<uint64_t> sig_vals;

    for (auto [s, wait_stage, val] : wait_sems) {
        if (!s)
            throw except_t("Invalid sem in wait_sems");
        vk_wait_stages.push_back((VkPipelineStageFlags)wait_stage);
        vk_wait_sems.push_back(s->vk_sem);
        wait_vals.push_back(val);
    }
    for (auto [s, val] : sig_sems) {
        if (!s)
            throw except_t("Invalid sem in sig_sems");
        vk_sig_sems.push_back(s->vk_sem);
        sig_vals.push_back(val);
    }
    VkTimelineSemaphoreSubmitInfo tl_info {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreValueCount = (uint32_t)wait_vals.size(),
        .pWaitSemaphoreValues = wait_vals.data(),
        .signalSemaphoreValueCount = (uint32_t)sig_vals.size(),
        .pSignalSemaphoreValues = sig_vals.data(),
    };
    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &tl_info,
        .waitSemaphoreCount = (uint32_t)vk_wait_sems.size(),
        .pWaitSemaphores = vk_wait_sems.size() == 0 ? nullptr : vk_wait_sems.data(),
        .pWaitDstStageMask = vk_wait_sems.size() == 0 ? nullptr : vk_wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cbuff->vk_buff,
        .signalSemaphoreCount = (uint32_t)vk_sig_sems.size(),
        .pSignalSemaphores = vk_sig_sems.size() == 0 ? nullptr : vk_sig_sems.data(),
    };

    if (queue_id > cbuff->m_cmdpool->m_device->vk_graphics_que.size())
        throw vku::except_t("Invalid queue index");
    VK_ASSERT(vkQueueSubmit(cbuff->m_cmdpool->m_device->vk_graphics_que[queue_id], 1, &submit_info,
            fence == nullptr ? nullptr : fence->vk_fence));
}

inline void present(
        ref_t<swapchain_t> swc,
        std::vector<ref_t<sem_t>> wait_sems,
        uint32_t img_idx)
{
    if (!swc)
        throw except_t("Invalid swc ref");
    std::vector<VkSemaphore> vk_wait_sems;

    for (auto s : wait_sems) {
        if (!s)
            throw except_t("Invalid semaphore in wait_sems");
        vk_wait_sems.push_back(s->vk_sem);
    }

    VkPresentInfoKHR pres_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)vk_wait_sems.size(),
        .pWaitSemaphores = vk_wait_sems.data(),
        .swapchainCount = 1,
        .pSwapchains = &swc->vk_swapchain,
        .pImageIndices = &img_idx,
        .pResults = NULL,
    };

    VK_ASSERT(vkQueuePresentKHR(swc->m_device->vk_present_que, &pres_info));
}

inline void copy_buff(ref_t<cmdpool_t> cp, ref_t<buffer_t> dst,
        ref_t<buffer_t> src, VkDeviceSize sz, ref_t<cmdbuff_t> cbuff)
{
    if (!dst)
        throw except_t("Invalid dst buff");
    if (!src)
        throw except_t("Invalid src buff");
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = fence_t::create(cp->m_device);

    if (!existing_cbuff)
        cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sz
    };
    vkCmdCopyBuffer(cbuff->vk_buff, src->vk_buff, dst->vk_buff,
            1, &copy_region);

    if (!existing_cbuff) {
        cbuff->end();

        submit_cmdbuff({}, cbuff, fence, {});
        wait_fences({fence}, true, UINT64_MAX);
    }
}

inline std::string to_string(vku_shader_stage_e stage) {
    switch (stage) {
        case VKU_SPIRV_VERTEX: return "VKU_SPIRV_VERTEX";
        case VKU_SPIRV_FRAGMENT: return "VKU_SPIRV_FRAGMENT";
        case VKU_SPIRV_COMPUTE: return "VKU_SPIRV_COMPUTE";
        case VKU_SPIRV_GEOMETRY: return "VKU_SPIRV_GEOMETRY";
        case VKU_SPIRV_TESS_CTRL: return "VKU_SPIRV_TESS_CTRL";
        case VKU_SPIRV_TESS_EVAL: return "VKU_SPIRV_TESS_EVAL";
    }
    return "VKU_UNKNOWN_SHADER_STAGE";
}

inline std::string to_string(const vertex_input_desc_t& m_input_desc) {
    std::string ret = "vertex_input_desc_t{";
    ret += std::format(" .binding={}, .stride={}, input_rate={}, .attrs=[",
            m_input_desc.bind_desc.binding, m_input_desc.bind_desc.stride, to_string(
            m_input_desc.bind_desc.inputRate));
    for (auto &adesc : m_input_desc.attr_desc)
        ret += std::format("{{.location={}, .binding={}, format={}, .offset={}}},",
                adesc.location, adesc.binding, to_string(adesc.format), adesc.offset);
    return ret + "}";
}

inline std::string to_string(VkVertexInputRate rate) {
    switch (rate) {
        case VK_VERTEX_INPUT_RATE_VERTEX: return "VK_VERTEX_INPUT_RATE_VERTEX";
        case VK_VERTEX_INPUT_RATE_INSTANCE: return "VK_VERTEX_INPUT_RATE_INSTANCE";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkFormat format) {
    switch (format) {
        case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
        case VK_FORMAT_R4G4_UNORM_PACK8: return "VK_FORMAT_R4G4_UNORM_PACK8";
        case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
        case VK_FORMAT_R5G6B5_UNORM_PACK16: return "VK_FORMAT_R5G6B5_UNORM_PACK16";
        case VK_FORMAT_B5G6R5_UNORM_PACK16: return "VK_FORMAT_B5G6R5_UNORM_PACK16";
        case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
        case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
        case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
        case VK_FORMAT_R8_UNORM: return "VK_FORMAT_R8_UNORM";
        case VK_FORMAT_R8_SNORM: return "VK_FORMAT_R8_SNORM";
        case VK_FORMAT_R8_USCALED: return "VK_FORMAT_R8_USCALED";
        case VK_FORMAT_R8_SSCALED: return "VK_FORMAT_R8_SSCALED";
        case VK_FORMAT_R8_UINT: return "VK_FORMAT_R8_UINT";
        case VK_FORMAT_R8_SINT: return "VK_FORMAT_R8_SINT";
        case VK_FORMAT_R8_SRGB: return "VK_FORMAT_R8_SRGB";
        case VK_FORMAT_R8G8_UNORM: return "VK_FORMAT_R8G8_UNORM";
        case VK_FORMAT_R8G8_SNORM: return "VK_FORMAT_R8G8_SNORM";
        case VK_FORMAT_R8G8_USCALED: return "VK_FORMAT_R8G8_USCALED";
        case VK_FORMAT_R8G8_SSCALED: return "VK_FORMAT_R8G8_SSCALED";
        case VK_FORMAT_R8G8_UINT: return "VK_FORMAT_R8G8_UINT";
        case VK_FORMAT_R8G8_SINT: return "VK_FORMAT_R8G8_SINT";
        case VK_FORMAT_R8G8_SRGB: return "VK_FORMAT_R8G8_SRGB";
        case VK_FORMAT_R8G8B8_UNORM: return "VK_FORMAT_R8G8B8_UNORM";
        case VK_FORMAT_R8G8B8_SNORM: return "VK_FORMAT_R8G8B8_SNORM";
        case VK_FORMAT_R8G8B8_USCALED: return "VK_FORMAT_R8G8B8_USCALED";
        case VK_FORMAT_R8G8B8_SSCALED: return "VK_FORMAT_R8G8B8_SSCALED";
        case VK_FORMAT_R8G8B8_UINT: return "VK_FORMAT_R8G8B8_UINT";
        case VK_FORMAT_R8G8B8_SINT: return "VK_FORMAT_R8G8B8_SINT";
        case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
        case VK_FORMAT_B8G8R8_UNORM: return "VK_FORMAT_B8G8R8_UNORM";
        case VK_FORMAT_B8G8R8_SNORM: return "VK_FORMAT_B8G8R8_SNORM";
        case VK_FORMAT_B8G8R8_USCALED: return "VK_FORMAT_B8G8R8_USCALED";
        case VK_FORMAT_B8G8R8_SSCALED: return "VK_FORMAT_B8G8R8_SSCALED";
        case VK_FORMAT_B8G8R8_UINT: return "VK_FORMAT_B8G8R8_UINT";
        case VK_FORMAT_B8G8R8_SINT: return "VK_FORMAT_B8G8R8_SINT";
        case VK_FORMAT_B8G8R8_SRGB: return "VK_FORMAT_B8G8R8_SRGB";
        case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_USCALED: return "VK_FORMAT_R8G8B8A8_USCALED";
        case VK_FORMAT_R8G8B8A8_SSCALED: return "VK_FORMAT_R8G8B8A8_SSCALED";
        case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_USCALED: return "VK_FORMAT_B8G8R8A8_USCALED";
        case VK_FORMAT_B8G8R8A8_SSCALED: return "VK_FORMAT_B8G8R8A8_SSCALED";
        case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
        case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
        case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
        case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
        case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
        case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
        case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
        case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
        case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
        case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
        case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
        case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
        case VK_FORMAT_R16_UNORM: return "VK_FORMAT_R16_UNORM";
        case VK_FORMAT_R16_SNORM: return "VK_FORMAT_R16_SNORM";
        case VK_FORMAT_R16_USCALED: return "VK_FORMAT_R16_USCALED";
        case VK_FORMAT_R16_SSCALED: return "VK_FORMAT_R16_SSCALED";
        case VK_FORMAT_R16_UINT: return "VK_FORMAT_R16_UINT";
        case VK_FORMAT_R16_SINT: return "VK_FORMAT_R16_SINT";
        case VK_FORMAT_R16_SFLOAT: return "VK_FORMAT_R16_SFLOAT";
        case VK_FORMAT_R16G16_UNORM: return "VK_FORMAT_R16G16_UNORM";
        case VK_FORMAT_R16G16_SNORM: return "VK_FORMAT_R16G16_SNORM";
        case VK_FORMAT_R16G16_USCALED: return "VK_FORMAT_R16G16_USCALED";
        case VK_FORMAT_R16G16_SSCALED: return "VK_FORMAT_R16G16_SSCALED";
        case VK_FORMAT_R16G16_UINT: return "VK_FORMAT_R16G16_UINT";
        case VK_FORMAT_R16G16_SINT: return "VK_FORMAT_R16G16_SINT";
        case VK_FORMAT_R16G16_SFLOAT: return "VK_FORMAT_R16G16_SFLOAT";
        case VK_FORMAT_R16G16B16_UNORM: return "VK_FORMAT_R16G16B16_UNORM";
        case VK_FORMAT_R16G16B16_SNORM: return "VK_FORMAT_R16G16B16_SNORM";
        case VK_FORMAT_R16G16B16_USCALED: return "VK_FORMAT_R16G16B16_USCALED";
        case VK_FORMAT_R16G16B16_SSCALED: return "VK_FORMAT_R16G16B16_SSCALED";
        case VK_FORMAT_R16G16B16_UINT: return "VK_FORMAT_R16G16B16_UINT";
        case VK_FORMAT_R16G16B16_SINT: return "VK_FORMAT_R16G16B16_SINT";
        case VK_FORMAT_R16G16B16_SFLOAT: return "VK_FORMAT_R16G16B16_SFLOAT";
        case VK_FORMAT_R16G16B16A16_UNORM: return "VK_FORMAT_R16G16B16A16_UNORM";
        case VK_FORMAT_R16G16B16A16_SNORM: return "VK_FORMAT_R16G16B16A16_SNORM";
        case VK_FORMAT_R16G16B16A16_USCALED: return "VK_FORMAT_R16G16B16A16_USCALED";
        case VK_FORMAT_R16G16B16A16_SSCALED: return "VK_FORMAT_R16G16B16A16_SSCALED";
        case VK_FORMAT_R16G16B16A16_UINT: return "VK_FORMAT_R16G16B16A16_UINT";
        case VK_FORMAT_R16G16B16A16_SINT: return "VK_FORMAT_R16G16B16A16_SINT";
        case VK_FORMAT_R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case VK_FORMAT_R32_UINT: return "VK_FORMAT_R32_UINT";
        case VK_FORMAT_R32_SINT: return "VK_FORMAT_R32_SINT";
        case VK_FORMAT_R32_SFLOAT: return "VK_FORMAT_R32_SFLOAT";
        case VK_FORMAT_R32G32_UINT: return "VK_FORMAT_R32G32_UINT";
        case VK_FORMAT_R32G32_SINT: return "VK_FORMAT_R32G32_SINT";
        case VK_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
        case VK_FORMAT_R32G32B32_UINT: return "VK_FORMAT_R32G32B32_UINT";
        case VK_FORMAT_R32G32B32_SINT: return "VK_FORMAT_R32G32B32_SINT";
        case VK_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
        case VK_FORMAT_R32G32B32A32_UINT: return "VK_FORMAT_R32G32B32A32_UINT";
        case VK_FORMAT_R32G32B32A32_SINT: return "VK_FORMAT_R32G32B32A32_SINT";
        case VK_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case VK_FORMAT_R64_UINT: return "VK_FORMAT_R64_UINT";
        case VK_FORMAT_R64_SINT: return "VK_FORMAT_R64_SINT";
        case VK_FORMAT_R64_SFLOAT: return "VK_FORMAT_R64_SFLOAT";
        case VK_FORMAT_R64G64_UINT: return "VK_FORMAT_R64G64_UINT";
        case VK_FORMAT_R64G64_SINT: return "VK_FORMAT_R64G64_SINT";
        case VK_FORMAT_R64G64_SFLOAT: return "VK_FORMAT_R64G64_SFLOAT";
        case VK_FORMAT_R64G64B64_UINT: return "VK_FORMAT_R64G64B64_UINT";
        case VK_FORMAT_R64G64B64_SINT: return "VK_FORMAT_R64G64B64_SINT";
        case VK_FORMAT_R64G64B64_SFLOAT: return "VK_FORMAT_R64G64B64_SFLOAT";
        case VK_FORMAT_R64G64B64A64_UINT: return "VK_FORMAT_R64G64B64A64_UINT";
        case VK_FORMAT_R64G64B64A64_SINT: return "VK_FORMAT_R64G64B64A64_SINT";
        case VK_FORMAT_R64G64B64A64_SFLOAT: return "VK_FORMAT_R64G64B64A64_SFLOAT";
        case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
        case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
        case VK_FORMAT_D16_UNORM: return "VK_FORMAT_D16_UNORM";
        case VK_FORMAT_X8_D24_UNORM_PACK32: return "VK_FORMAT_X8_D24_UNORM_PACK32";
        case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
        case VK_FORMAT_S8_UINT: return "VK_FORMAT_S8_UINT";
        case VK_FORMAT_D16_UNORM_S8_UINT: return "VK_FORMAT_D16_UNORM_S8_UINT";
        case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
        case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
        case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
        case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
        case VK_FORMAT_BC2_UNORM_BLOCK: return "VK_FORMAT_BC2_UNORM_BLOCK";
        case VK_FORMAT_BC2_SRGB_BLOCK: return "VK_FORMAT_BC2_SRGB_BLOCK";
        case VK_FORMAT_BC3_UNORM_BLOCK: return "VK_FORMAT_BC3_UNORM_BLOCK";
        case VK_FORMAT_BC3_SRGB_BLOCK: return "VK_FORMAT_BC3_SRGB_BLOCK";
        case VK_FORMAT_BC4_UNORM_BLOCK: return "VK_FORMAT_BC4_UNORM_BLOCK";
        case VK_FORMAT_BC4_SNORM_BLOCK: return "VK_FORMAT_BC4_SNORM_BLOCK";
        case VK_FORMAT_BC5_UNORM_BLOCK: return "VK_FORMAT_BC5_UNORM_BLOCK";
        case VK_FORMAT_BC5_SNORM_BLOCK: return "VK_FORMAT_BC5_SNORM_BLOCK";
        case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
        case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
        case VK_FORMAT_BC7_UNORM_BLOCK: return "VK_FORMAT_BC7_UNORM_BLOCK";
        case VK_FORMAT_BC7_SRGB_BLOCK: return "VK_FORMAT_BC7_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
        case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
        case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
        case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
        case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
        case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
        case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkPrimitiveTopology topol) {
    switch (topol) {
        case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
                return "VK_PRIMITIVE_TOPOLOGY_POINT_LIST";
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
                return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
                return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
                return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
                return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
                return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN";
        case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
                return "VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
                return "VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
                return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
                return "VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY";
        case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
                return "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkSharingMode shmod) {
    switch (shmod) {
        case VK_SHARING_MODE_EXCLUSIVE: return "VK_SHARING_MODE_EXCLUSIVE";
        case VK_SHARING_MODE_CONCURRENT: return "VK_SHARING_MODE_CONCURRENT";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkFilter shmod) {
    switch (shmod) {
        case VK_FILTER_NEAREST: return "VK_FILTER_NEAREST";
        case VK_FILTER_LINEAR: return "VK_FILTER_LINEAR";
        case VK_FILTER_CUBIC_EXT: return "VK_FILTER_CUBIC_EXT";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkDescriptorType dtype) {
    switch (dtype) {
        case VK_DESCRIPTOR_TYPE_SAMPLER: 
            return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: 
            return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: 
            return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: 
            return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: 
            return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: 
            return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: 
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: 
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: 
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: 
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: 
            return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:
            return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
        case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:
            return "VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT";
        default: return "VK_UNKNOWN_TYPE";
    }
}

inline std::string to_string(VkImageLayout lay) {
    switch (lay) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
                return "VK_IMAGE_LAYOUT_UNDEFINED";
        case VK_IMAGE_LAYOUT_GENERAL:
                return "VK_IMAGE_LAYOUT_GENERAL";
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                return "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return "VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL";
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL";
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
                return "VK_IMAGE_LAYOUT_PREINITIALIZED";
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
                return "VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL";
        case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
                return "VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL";
        case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
                return "VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL";
        // case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
        //         return "VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL";
        // case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
        //         return "VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL";
        // case VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ:
        //         return "VK_IMAGE_LAYOUT_RENDERING_LOCAL_READ";
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR";
        case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
                return "VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR";
        case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
                return "VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT";
        // case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
        //         return "VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR";
        // case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR";
        // case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
        //         return "VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT";
        // case VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM:
        //         return "VK_IMAGE_LAYOUT_TENSOR_ALIASING_ARM";
        // case VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR:
        //         return "VK_IMAGE_LAYOUT_VIDEO_ENCODE_QUANTIZATION_MAP_KHR";
        // case VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT:
        //         return "VK_IMAGE_LAYOUT_ZERO_INITIALIZED_EXT";
        default: return "VK_UNKNOWN_LAYOUT_TYPE";
    }
}


inline std::string to_string(const VkDescriptorSetLayoutBinding& bind) {
    return std::format("VkDescriptorSetLayoutBinding{{ .binding={} .type={} .count={} "
            ".stage_flags={} .immutable_samplers={} }}",
            bind.binding,
            vulkan_utils::to_string(bind.descriptorType),
            bind.descriptorCount,
            vulkan_utils::to_string(VkShaderStageFlagBits(bind.stageFlags)),
            (void*)bind.pImmutableSamplers);
}


inline std::string to_string(VkFenceCreateFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_FENCE_CREATE_SIGNALED_BIT) ret += "VK_FENCE_CREATE_SIGNALED_BIT, ";
    return ret + "]";
}

inline std::string to_string(VkBufferUsageFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
        ret += "VK_BUFFER_USAGE_TRANSFER_SRC_BIT, ";
    if (flags & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
        ret += "VK_BUFFER_USAGE_TRANSFER_DST_BIT, ";
    if (flags & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
        ret += "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, ";
    if (flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
        ret += "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, ";
    if (flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT)
        ret += "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT, ";
    if (flags & VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT)
        ret += "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT, ";
    if (flags & VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT)
        ret += "VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT, ";
    if (flags & VK_BUFFER_USAGE_RAY_TRACING_BIT_NV)
        ret += "VK_BUFFER_USAGE_RAY_TRACING_BIT_NV, ";
    if (flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT)
        ret += "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT, ";
    if (flags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR)
        ret += "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR, ";
    return ret + "]";
}

inline std::string to_string(VkMemoryPropertyFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        ret += "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        ret += "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT";
    if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        ret += "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT";
    if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
        ret += "VK_MEMORY_PROPERTY_HOST_CACHED_BIT";
    if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
        ret += "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT";
    if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
        ret += "VK_MEMORY_PROPERTY_PROTECTED_BIT";
    if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
        ret += "VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD";
    if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
        ret += "VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD";
    return ret + "]";
}

inline std::string to_string(VkImageUsageFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        ret += "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
    if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        ret += "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
    if (flags & VK_IMAGE_USAGE_SAMPLED_BIT)
        ret += "VK_IMAGE_USAGE_SAMPLED_BIT";
    if (flags & VK_IMAGE_USAGE_STORAGE_BIT)
        ret += "VK_IMAGE_USAGE_STORAGE_BIT";
    if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        ret += "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
    if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        ret += "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
    if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        ret += "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
    if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
        ret += "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
    if (flags & VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT)
        ret += "VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT";
    if (flags & VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV)
        ret += "VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV";
    return ret + "]";
}

inline std::string to_string(VkImageAspectFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_IMAGE_ASPECT_COLOR_BIT)
        ret += "VK_IMAGE_ASPECT_COLOR_BIT, ";
    if (flags & VK_IMAGE_ASPECT_DEPTH_BIT)
        ret += "VK_IMAGE_ASPECT_DEPTH_BIT, ";
    if (flags & VK_IMAGE_ASPECT_STENCIL_BIT)
        ret += "VK_IMAGE_ASPECT_STENCIL_BIT, ";
    if (flags & VK_IMAGE_ASPECT_METADATA_BIT)
        ret += "VK_IMAGE_ASPECT_METADATA_BIT, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_0_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_0_BIT, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_1_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_1_BIT, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_2_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_2_BIT, ";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT, ";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT, ";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT, ";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_0_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_0_BIT_KHR, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_1_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_1_BIT_KHR, ";
    if (flags & VK_IMAGE_ASPECT_PLANE_2_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_2_BIT_KHR, ";
    return ret + "]";
}

inline std::string to_string(VkShaderStageFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_SHADER_STAGE_VERTEX_BIT)
        ret += "VK_SHADER_STAGE_VERTEX_BIT, ";
    if (flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
        ret += "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, ";
    if (flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
        ret += "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, ";
    if (flags & VK_SHADER_STAGE_GEOMETRY_BIT)
        ret += "VK_SHADER_STAGE_GEOMETRY_BIT, ";
    if (flags & VK_SHADER_STAGE_FRAGMENT_BIT)
        ret += "VK_SHADER_STAGE_FRAGMENT_BIT, ";
    if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
        ret += "VK_SHADER_STAGE_COMPUTE_BIT, ";
    if (flags & VK_SHADER_STAGE_ALL_GRAPHICS)
        ret += "VK_SHADER_STAGE_ALL_GRAPHICS, ";
    if (flags & VK_SHADER_STAGE_ALL)
        ret += "VK_SHADER_STAGE_ALL, ";
    if (flags & VK_SHADER_STAGE_RAYGEN_BIT_NV)
        ret += "VK_SHADER_STAGE_RAYGEN_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_ANY_HIT_BIT_NV)
        ret += "VK_SHADER_STAGE_ANY_HIT_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV)
        ret += "VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_MISS_BIT_NV)
        ret += "VK_SHADER_STAGE_MISS_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_INTERSECTION_BIT_NV)
        ret += "VK_SHADER_STAGE_INTERSECTION_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_CALLABLE_BIT_NV)
        ret += "VK_SHADER_STAGE_CALLABLE_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_TASK_BIT_NV)
        ret += "VK_SHADER_STAGE_TASK_BIT_NV, ";
    if (flags & VK_SHADER_STAGE_MESH_BIT_NV)
        ret += "VK_SHADER_STAGE_MESH_BIT_NV, ";
    return ret + "]";
}

inline std::string to_string(VkPipelineStageFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)
        ret += "VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, ";
    if (flags & VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)
        ret += "VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, ";
    if (flags & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)
        ret += "VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, ";
    if (flags & VK_PIPELINE_STAGE_VERTEX_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT)
        ret += "VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, ";
    if (flags & VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)
        ret += "VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, ";
    if (flags & VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)
        ret += "VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, ";
    if (flags & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
        ret += "VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_TRANSFER_BIT)
        ret += "VK_PIPELINE_STAGE_TRANSFER_BIT, ";
    if (flags & VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)
        ret += "VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, ";
    if (flags & VK_PIPELINE_STAGE_HOST_BIT)
        ret += "VK_PIPELINE_STAGE_HOST_BIT, ";
    if (flags & VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT)
        ret += "VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, ";
    if (flags & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
        ret += "VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, ";
    if (flags & VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT)
        ret += "VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT, ";
    if (flags & VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT)
        ret += "VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT, ";
    // if (flags & VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR)
    //     ret += "VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, ";
    // if (flags & VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR)
    //     ret += "VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, ";
    if (flags & VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT)
        ret += "VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT, ";
    // if (flags & VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
    //     ret += "VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, ";
    // if (flags & VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT)
    //     ret += "VK_PIPELINE_STAGE_TASK_SHADER_BIT_EXT, ";
    // if (flags & VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT)
    //     ret += "VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT, ";
    // if (flags & VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_EXT)
    //     ret += "VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_EXT, ";
    return ret + "]";
}

inline std::string to_string(VkAccessFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
        ret += "VK_ACCESS_INDIRECT_COMMAND_READ_BIT, ";
    if (flags & VK_ACCESS_INDEX_READ_BIT)
        ret += "VK_ACCESS_INDEX_READ_BIT, ";
    if (flags & VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)
        ret += "VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, ";
    if (flags & VK_ACCESS_UNIFORM_READ_BIT)
        ret += "VK_ACCESS_UNIFORM_READ_BIT, ";
    if (flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
        ret += "VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, ";
    if (flags & VK_ACCESS_SHADER_READ_BIT)
        ret += "VK_ACCESS_SHADER_READ_BIT, ";
    if (flags & VK_ACCESS_SHADER_WRITE_BIT)
        ret += "VK_ACCESS_SHADER_WRITE_BIT, ";
    if (flags & VK_ACCESS_COLOR_ATTACHMENT_READ_BIT)
        ret += "VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, ";
    if (flags & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        ret += "VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, ";
    if (flags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
        ret += "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, ";
    if (flags & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
        ret += "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, ";
    if (flags & VK_ACCESS_TRANSFER_READ_BIT)
        ret += "VK_ACCESS_TRANSFER_READ_BIT, ";
    if (flags & VK_ACCESS_TRANSFER_WRITE_BIT)
        ret += "VK_ACCESS_TRANSFER_WRITE_BIT, ";
    if (flags & VK_ACCESS_HOST_READ_BIT)
        ret += "VK_ACCESS_HOST_READ_BIT, ";
    if (flags & VK_ACCESS_HOST_WRITE_BIT)
        ret += "VK_ACCESS_HOST_WRITE_BIT, ";
    if (flags & VK_ACCESS_MEMORY_READ_BIT)
        ret += "VK_ACCESS_MEMORY_READ_BIT, ";
    if (flags & VK_ACCESS_MEMORY_WRITE_BIT)
        ret += "VK_ACCESS_MEMORY_WRITE_BIT, ";
    if (flags & VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT)
        ret += "VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT, ";
    if (flags & VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT)
        ret += "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT, ";
    if (flags & VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT)
        ret += "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT, ";
    if (flags & VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT)
        ret += "VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT, ";
    if (flags & VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT)
        ret += "VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT, ";
    // if (flags & VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR)
    //     ret += "VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR, ";
    // if (flags & VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR)
    //     ret += "VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, ";
    if (flags & VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT)
        ret += "VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT, ";
    // if (flags & VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)
    //     ret += "VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR, ";
    // if (flags & VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_EXT)
    //     ret += "VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_EXT, ";
    // if (flags & VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_EXT)
    //     ret += "VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_EXT, ";
    if (flags & VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV)
        ret += "VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV, ";
    if (flags & VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV)
        ret += "VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV, ";
    if (flags & VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV)
        ret += "VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV, ";
    // if (flags & VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV)
    //     ret += "VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV, ";
    // if (flags & VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV)
    //     ret += "VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV, ";
    return ret + "]";
}

inline std::string to_string(VkDependencyFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_DEPENDENCY_BY_REGION_BIT)
        ret += "VK_DEPENDENCY_BY_REGION_BIT, ";
    if (flags & VK_DEPENDENCY_DEVICE_GROUP_BIT)
        ret += "VK_DEPENDENCY_DEVICE_GROUP_BIT, ";
    if (flags & VK_DEPENDENCY_VIEW_LOCAL_BIT)
        ret += "VK_DEPENDENCY_VIEW_LOCAL_BIT, ";
    // if (flags & VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT)
    //     ret += "VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT, ";
    // if (flags & VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR)
    //     ret += "VK_DEPENDENCY_QUEUE_FAMILY_OWNERSHIP_TRANSFER_USE_ALL_STAGES_BIT_KHR, ";
    // if (flags & VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR)
    //     ret += "VK_DEPENDENCY_ASYMMETRIC_EVENT_BIT_KHR, ";
    return ret + "]";
}

inline std::string to_string(VkQueueFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_QUEUE_GRAPHICS_BIT)
        ret += "VK_QUEUE_GRAPHICS_BIT, ";
    if (flags & VK_QUEUE_COMPUTE_BIT)
        ret += "VK_QUEUE_COMPUTE_BIT, ";
    if (flags & VK_QUEUE_TRANSFER_BIT)
        ret += "VK_QUEUE_TRANSFER_BIT, ";
    if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
        ret += "VK_QUEUE_SPARSE_BINDING_BIT, ";
    if (flags & VK_QUEUE_PROTECTED_BIT)
        ret += "VK_QUEUE_PROTECTED_BIT, ";
    if (flags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
        ret += "VK_QUEUE_VIDEO_DECODE_BIT_KHR, ";
    if (flags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
        ret += "VK_QUEUE_VIDEO_ENCODE_BIT_KHR, ";
    if (flags & VK_QUEUE_OPTICAL_FLOW_BIT_NV)
        ret += "VK_QUEUE_OPTICAL_FLOW_BIT_NV, ";
    if (flags & VK_QUEUE_DATA_GRAPH_BIT_ARM)
        ret += "VK_QUEUE_DATA_GRAPH_BIT_ARM, ";
    return ret + "]";
}



/* TODO: this needs to be implemented in a newer version of vulkan, tested and as such */
// inline std::string to_string(VkPipelineStageFlagBits2 flags) {
//     std::string ret = "[";
//     if (flags & VK_PIPELINE_STAGE_2_NONE)
//         ret += "VK_PIPELINE_STAGE_2_NONE, ";
//     if (flags & VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT)
//         ret += "VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT)
//         ret += "VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT)
//         ret += "VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_TRANSFER_BIT)
//         ret += "VK_PIPELINE_STAGE_2_TRANSFER_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT)
//         ret += "VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_HOST_BIT)
//         ret += "VK_PIPELINE_STAGE_2_HOST_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)
//         ret += "VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)
//         ret += "VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_COPY_BIT)
//         ret += "VK_PIPELINE_STAGE_2_COPY_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_RESOLVE_BIT)
//         ret += "VK_PIPELINE_STAGE_2_RESOLVE_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_BLIT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_BLIT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_CLEAR_BIT)
//         ret += "VK_PIPELINE_STAGE_2_CLEAR_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT)
//         ret += "VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT, ";
//     if (flags & VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_VIDEO_ENCODE_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_NONE_KHR)
//         ret += "VK_PIPELINE_STAGE_2_NONE_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_TRANSFER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_HOST_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_HOST_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_COPY_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_COPY_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_RESOLVE_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_RESOLVE_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_BLIT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_BLIT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_CLEAR_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_CLEAR_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_SHADING_RATE_IMAGE_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_SHADING_RATE_IMAGE_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_FRAGMENT_DENSITY_PROCESS_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI)
//         ret += "VK_PIPELINE_STAGE_2_SUBPASS_SHADER_BIT_HUAWEI, ";
//     if (flags & VK_PIPELINE_STAGE_2_SUBPASS_SHADING_BIT_HUAWEI)
//         ret += "VK_PIPELINE_STAGE_2_SUBPASS_SHADING_BIT_HUAWEI, ";
//     if (flags & VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI)
//         ret += "VK_PIPELINE_STAGE_2_INVOCATION_MASK_BIT_HUAWEI, ";
//     if (flags & VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_MICROMAP_BUILD_BIT_EXT, ";
//     if (flags & VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI)
//         ret += "VK_PIPELINE_STAGE_2_CLUSTER_CULLING_SHADER_BIT_HUAWEI, ";
//     if (flags & VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_OPTICAL_FLOW_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_CONVERT_COOPERATIVE_VECTOR_MATRIX_BIT_NV)
//         ret += "VK_PIPELINE_STAGE_2_CONVERT_COOPERATIVE_VECTOR_MATRIX_BIT_NV, ";
//     if (flags & VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM)
//         ret += "VK_PIPELINE_STAGE_2_DATA_GRAPH_BIT_ARM, ";
//     if (flags & VK_PIPELINE_STAGE_2_COPY_INDIRECT_BIT_KHR)
//         ret += "VK_PIPELINE_STAGE_2_COPY_INDIRECT_BIT_KHR, ";
//     if (flags & VK_PIPELINE_STAGE_2_MEMORY_DECOMPRESSION_BIT_EXT)
//         ret += "VK_PIPELINE_STAGE_2_MEMORY_DECOMPRESSION_BIT_EXT, ";
//     return ret + "]";
// }
// inline std::string to_string(VkAccessFlagBits2 flags) {
//     std::string ret = "[";
//     if (flags & VK_ACCESS_2_NONE)
//         ret += "VK_ACCESS_2_NONE, ";
//     if (flags & VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT)
//         ret += "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT, ";
//     if (flags & VK_ACCESS_2_INDEX_READ_BIT)
//         ret += "VK_ACCESS_2_INDEX_READ_BIT, ";
//     if (flags & VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)
//         ret += "VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT, ";
//     if (flags & VK_ACCESS_2_UNIFORM_READ_BIT)
//         ret += "VK_ACCESS_2_UNIFORM_READ_BIT, ";
//     if (flags & VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT)
//         ret += "VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT, ";
//     if (flags & VK_ACCESS_2_SHADER_READ_BIT)
//         ret += "VK_ACCESS_2_SHADER_READ_BIT, ";
//     if (flags & VK_ACCESS_2_SHADER_WRITE_BIT)
//         ret += "VK_ACCESS_2_SHADER_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT)
//         ret += "VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, ";
//     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
//         ret += "VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
//         ret += "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT, ";
//     if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
//         ret += "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_TRANSFER_READ_BIT)
//         ret += "VK_ACCESS_2_TRANSFER_READ_BIT, ";
//     if (flags & VK_ACCESS_2_TRANSFER_WRITE_BIT)
//         ret += "VK_ACCESS_2_TRANSFER_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_HOST_READ_BIT)
//         ret += "VK_ACCESS_2_HOST_READ_BIT, ";
//     if (flags & VK_ACCESS_2_HOST_WRITE_BIT)
//         ret += "VK_ACCESS_2_HOST_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_MEMORY_READ_BIT)
//         ret += "VK_ACCESS_2_MEMORY_READ_BIT, ";
//     if (flags & VK_ACCESS_2_MEMORY_WRITE_BIT)
//         ret += "VK_ACCESS_2_MEMORY_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
//         ret += "VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, ";
//     if (flags & VK_ACCESS_2_SHADER_STORAGE_READ_BIT)
//         ret += "VK_ACCESS_2_SHADER_STORAGE_READ_BIT, ";
//     if (flags & VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)
//         ret += "VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, ";
//     if (flags & VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SAMPLER_HEAP_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_SAMPLER_HEAP_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_RESOURCE_HEAP_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_RESOURCE_HEAP_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_VIDEO_ENCODE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_VIDEO_ENCODE_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_TILE_ATTACHMENT_READ_BIT_QCOM)
//         ret += "VK_ACCESS_2_SHADER_TILE_ATTACHMENT_READ_BIT_QCOM, ";
//     if (flags & VK_ACCESS_2_SHADER_TILE_ATTACHMENT_WRITE_BIT_QCOM)
//         ret += "VK_ACCESS_2_SHADER_TILE_ATTACHMENT_WRITE_BIT_QCOM, ";
//     if (flags & VK_ACCESS_2_NONE_KHR)
//         ret += "VK_ACCESS_2_NONE_KHR, ";
//     if (flags & VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_INDEX_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_INDEX_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_UNIFORM_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_UNIFORM_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_TRANSFER_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_TRANSFER_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_HOST_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_HOST_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_HOST_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_HOST_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_MEMORY_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_MEMORY_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_MEMORY_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_MEMORY_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_SAMPLED_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT)
//         ret += "VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT)
//         ret += "VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV)
//         ret += "VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_NV, ";
//     if (flags & VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV)
//         ret += "VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV, ";
//     if (flags & VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_COMMAND_PREPROCESS_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_EXT)
//         ret += "VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV)
//         ret += "VK_ACCESS_2_SHADING_RATE_IMAGE_READ_BIT_NV, ";
//     if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR)
//         ret += "VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_NV)
//         ret += "VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_NV, ";
//     if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_NV)
//         ret += "VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_NV, ";
//     if (flags & VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_FRAGMENT_DENSITY_MAP_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT)
//         ret += "VK_ACCESS_2_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_DESCRIPTOR_BUFFER_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI)
//         ret += "VK_ACCESS_2_INVOCATION_MASK_READ_BIT_HUAWEI, ";
//     if (flags & VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR)
//         ret += "VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR, ";
//     if (flags & VK_ACCESS_2_MICROMAP_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_MICROMAP_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT)
//         ret += "VK_ACCESS_2_MICROMAP_WRITE_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV)
//         ret += "VK_ACCESS_2_OPTICAL_FLOW_READ_BIT_NV, ";
//     if (flags & VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV)
//         ret += "VK_ACCESS_2_OPTICAL_FLOW_WRITE_BIT_NV, ";
//     if (flags & VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM)
//         ret += "VK_ACCESS_2_DATA_GRAPH_READ_BIT_ARM, ";
//     if (flags & VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM)
//         ret += "VK_ACCESS_2_DATA_GRAPH_WRITE_BIT_ARM, ";
//     if (flags & VK_ACCESS_2_MEMORY_DECOMPRESSION_READ_BIT_EXT)
//         ret += "VK_ACCESS_2_MEMORY_DECOMPRESSION_READ_BIT_EXT, ";
//     if (flags & VK_ACCESS_2_MEMORY_DECOMPRESSION_WRITE_BIT_EXT)
//         ret += "VK_ACCESS_2_MEMORY_DECOMPRESSION_WRITE_BIT_EXT, ";
//     return ret + "]";
// }


/* Internal Functions:
================================================================================================= */

inline std::string glfw_err() {
    const char *errstr = NULL;
    int err = glfwGetError(&errstr);
    return sformat("[%s:%d]", errstr, err);
}

inline vc::ret_t create_dbg_messenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* dbg_info,
        const VkAllocationCallbacks* alloc,
        VkDebugUtilsMessengerEXT* dbg_msg)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance,
                (const VkDebugUtilsMessengerCreateInfoEXT *)dbg_info,
                (const VkAllocationCallbacks *)alloc, dbg_msg);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void destroy_dbg_messenger(
        VkInstance                      instance,
        VkDebugUtilsMessengerEXT        dbg_msg,
        const VkAllocationCallbacks*    alloc)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, dbg_msg, (const VkAllocationCallbacks *)alloc);
    }
}

inline VKAPI_ATTR VkBool32 VKAPI_CALL dbg_cbk(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* ctx)
{
    (void)msg_type;
    (void)ctx;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        DBG("[VK_DBG]: <---------------->\n%s", data->pMessage);
        DBG("[VK_DBG]: >----------------<");
    }
    else {
        DBG("%s", data->pMessage);
    }
    return VK_FALSE;
}

inline swapchain_details_t get_swapchain_details(VkPhysicalDevice dev,
        VkSurfaceKHR surf)
{
    swapchain_details_t ret = {};
    uint32_t format_cnt = 0;
    uint32_t present_modes_cnt = 0;

    VK_ASSERT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surf, &ret.capab));
    VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surf, &format_cnt, NULL));

    if (format_cnt) {
        ret.formats.resize(format_cnt);
        VK_ASSERT(vkGetPhysicalDeviceSurfaceFormatsKHR(
                dev, surf, &format_cnt, ret.formats.data()));
    }

    VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &present_modes_cnt, NULL));
    if (present_modes_cnt) {
        ret.present_modes.resize(present_modes_cnt);
        VK_ASSERT(vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surf, &present_modes_cnt,
                ret.present_modes.data()));
    }

    return ret;
}

inline gpu_family_ids_t find_queue_families(VkPhysicalDevice dev,
        VkSurfaceKHR surface)
{
    gpu_family_ids_t ret;

    uint32_t cnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &cnt, NULL);
    std::vector<VkQueueFamilyProperties> queue_families(cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &cnt, queue_families.data());

    ret.max_graphics_queue_cnt = 0;
    for (int i = 0; auto qf : queue_families) {
        DBG("Family[%d]: queueCount: %d flags: %s",
                i, qf.queueCount, vku::to_string((VkQueueFlagBits)qf.queueFlags).c_str());
        if ((qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qf.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            if (qf.queueCount > ret.max_graphics_queue_cnt) {
                ret.graphics_id = i;
                ret.max_graphics_queue_cnt = qf.queueCount;
            }
        }
        VkBool32 res = 0;
        if (surface)
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &res);
        if (res)
            ret.present_id = i;
        i++;
    }
    return ret;
}

inline VkExtent2D choose_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capab) {
    if (capab.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capab.currentExtent;

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D ret = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    ret.width  = std::clamp(ret.width,  capab.minImageExtent.width,  capab.maxImageExtent.width);
    ret.height = std::clamp(ret.height, capab.minImageExtent.height, capab.minImageExtent.height);

    return ret;
}

inline int score_phydev(VkPhysicalDevice dev, VkSurfaceKHR surf) {
    int score = 0;

    VkPhysicalDeviceProperties dev_prop;
    VkPhysicalDeviceFeatures dev_feat;
    vkGetPhysicalDeviceProperties(dev, &dev_prop);
    vkGetPhysicalDeviceFeatures(dev, &dev_feat);

    DBG("GPU Candidate Name: %s", dev_prop.deviceName);

    if (dev_prop.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 10;
    if (dev_feat.geometryShader)
        score += 10;

    if (!dev_feat.samplerAnisotropy) {
        DBG("sampler_anisotropy must be supported, but it is not supported by this GPU");
        return -1;
    }

    std::set<std::string> required_ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t ext_cnt;
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(dev, NULL, &ext_cnt, NULL));

    std::vector<VkExtensionProperties> avail_ext(ext_cnt);
    VK_ASSERT(vkEnumerateDeviceExtensionProperties(dev, NULL, &ext_cnt, avail_ext.data()));

    for (auto ext : avail_ext)
        required_ext.erase(ext.extensionName);

    if (!required_ext.empty()) {
        DBG("Required extensions not supported");
        return -1;
    }

    auto fams = find_queue_families(dev, surf);
    if (fams.graphics_id < 0 || fams.present_id < 0) {
        DBG("No queue family support");
        return -1;
    }

    swapchain_details_t details = get_swapchain_details(dev, surf);
    if (details.formats.empty() || details.present_modes.empty()) {
        DBG("Swapchain support is not adequate");
        return -1;
    }

    return score;
}

inline VkShaderStageFlagBits get_shader_type(vku_shader_stage_e own_type) {
    switch (own_type) {
        case VKU_SPIRV_VERTEX:    return VK_SHADER_STAGE_VERTEX_BIT;
        case VKU_SPIRV_FRAGMENT:  return VK_SHADER_STAGE_FRAGMENT_BIT;
        case VKU_SPIRV_COMPUTE:   return VK_SHADER_STAGE_COMPUTE_BIT;
        case VKU_SPIRV_GEOMETRY:  return VK_SHADER_STAGE_GEOMETRY_BIT;
        case VKU_SPIRV_TESS_CTRL: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case VKU_SPIRV_TESS_EVAL: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }
    return VK_SHADER_STAGE_ALL;
}

inline int spirv_save(const spirv_t& code, const char *filepath) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    file.write((const char *)code.content.data(), code.content.size() * sizeof(uint32_t));
    return file.good() ? 0 : -1;
}

#ifdef VKU_HAS_NEW_GLSLANG

inline spirv_t spirv_compile(vku_shader_stage_e vku_stage, const char *code) {
    VK_ASSERT(init());
    DBG("NEW GLSLANG COMPILE");

    glslang_stage_t gs_stage;
    switch (vku_stage) {
        case VKU_SPIRV_VERTEX:    gs_stage = GLSLANG_STAGE_VERTEX;         break;
        case VKU_SPIRV_TESS_CTRL: gs_stage = GLSLANG_STAGE_TESSCONTROL;    break;
        case VKU_SPIRV_TESS_EVAL: gs_stage = GLSLANG_STAGE_TESSEVALUATION; break;
        case VKU_SPIRV_GEOMETRY:  gs_stage = GLSLANG_STAGE_GEOMETRY;       break;
        case VKU_SPIRV_FRAGMENT:  gs_stage = GLSLANG_STAGE_FRAGMENT;       break;
        case VKU_SPIRV_COMPUTE:   gs_stage = GLSLANG_STAGE_COMPUTE;        break;
        default:
            DBG("Unknown shader stage type: %d", (uint32_t)vku_stage);
            throw vku::except_t(VK_ERROR_UNKNOWN);
    }

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = gs_stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = code,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &spirv_resources,
        .callbacks = {
            .include_system = nullptr,
            .include_local = nullptr,
            .free_include_result = nullptr,
        },
        .callbacks_ctx = nullptr,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        DBG("GLSL preprocessing failed \n");
        DBG("info_log: %s", glslang_shader_get_info_log(shader));
        DBG("debug_log: %s", glslang_shader_get_info_debug_log(shader));
        DBG("source_code: %s", input.code);
        glslang_shader_delete(shader);
        throw vku::except_t("GLSL preprocessing failed");
    }

    auto line_cnt_append = [](const std::string& to_change) {
        std::string ret;
        auto sv = ssplit_empty(to_change, "\n");
        int line = 1;
        for (auto &s : sv)
            ret += sformat("%3d: %s\n", line++, s.c_str());
        return ret;
    };

    if (!glslang_shader_parse(shader, &input)) {
        DBG("GLSL parsing failed");
        DBG("%s", glslang_shader_get_info_log(shader));
        DBG("%s", glslang_shader_get_info_debug_log(shader));
        DBG("%s", line_cnt_append(glslang_shader_get_preprocessed_code(shader)).c_str());
        glslang_shader_delete(shader);
        throw vku::except_t("GLSL parsing failed");
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        DBG("%s", glslang_program_get_info_log(program));
        DBG("%s", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        throw vku::except_t("GLSL linking failed");
    }

    glslang_program_SPIRV_generate(program, gs_stage);

    int size = glslang_program_SPIRV_get_size(program);
    spirv_t ret;
    ret.type = vku_stage;
    ret.content.resize(size);
    glslang_program_SPIRV_get(program, ret.content.data());

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages) {
        DBG("(SHADER) %s\b", spirv_messages);
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return ret;
}

inline void spirv_init() {
    if (!glslang_initialize_process()) {
        throw vku::except_t("Failed glslang_initialize_process");
    }

    spirv_resources.max_lights = 32;
    spirv_resources.max_clip_planes = 6;
    spirv_resources.max_texture_units = 32;
    spirv_resources.max_texture_coords = 32;
    spirv_resources.max_vertex_attribs = 64;
    spirv_resources.max_vertex_uniform_components = 4096;
    spirv_resources.max_varying_floats = 64;
    spirv_resources.max_vertex_texture_image_units = 32;
    spirv_resources.max_combined_texture_image_units = 80;
    spirv_resources.max_texture_image_units = 32;
    spirv_resources.max_fragment_uniform_components = 4096;
    spirv_resources.max_draw_buffers = 32;
    spirv_resources.max_vertex_uniform_vectors = 128;
    spirv_resources.max_varying_vectors = 8;
    spirv_resources.max_fragment_uniform_vectors = 16;
    spirv_resources.max_vertex_output_vectors = 16;
    spirv_resources.max_fragment_input_vectors = 15;
    spirv_resources.min_program_texel_offset = -8;
    spirv_resources.max_program_texel_offset = 7;
    spirv_resources.max_clip_distances = 8;
    spirv_resources.max_compute_work_group_count_x = 65535;
    spirv_resources.max_compute_work_group_count_y = 65535;
    spirv_resources.max_compute_work_group_count_z = 65535;
    spirv_resources.max_compute_work_group_size_x = 1024;
    spirv_resources.max_compute_work_group_size_y = 1024;
    spirv_resources.max_compute_work_group_size_z = 64;
    spirv_resources.max_compute_uniform_components = 1024;
    spirv_resources.max_compute_texture_image_units = 16;
    spirv_resources.max_compute_image_uniforms = 8;
    spirv_resources.max_compute_atomic_counters = 8;
    spirv_resources.max_compute_atomic_counter_buffers = 1;
    spirv_resources.max_varying_components = 60;
    spirv_resources.max_vertex_output_components = 64;
    spirv_resources.max_geometry_input_components = 64;
    spirv_resources.max_geometry_output_components = 128;
    spirv_resources.max_fragment_input_components = 128;
    spirv_resources.max_image_units = 8;
    spirv_resources.max_combined_image_units_and_fragment_outputs = 8;
    spirv_resources.max_combined_shader_output_resources = 8;
    spirv_resources.max_image_samples = 0;
    spirv_resources.max_vertex_image_uniforms = 0;
    spirv_resources.max_tess_control_image_uniforms = 0;
    spirv_resources.max_tess_evaluation_image_uniforms = 0;
    spirv_resources.max_geometry_image_uniforms = 0;
    spirv_resources.max_fragment_image_uniforms = 8;
    spirv_resources.max_combined_image_uniforms = 8;
    spirv_resources.max_geometry_texture_image_units = 16;
    spirv_resources.max_geometry_output_vertices = 256;
    spirv_resources.max_geometry_total_output_components = 1024;
    spirv_resources.max_geometry_uniform_components = 1024;
    spirv_resources.max_geometry_varying_components = 64;
    spirv_resources.max_tess_control_input_components = 128;
    spirv_resources.max_tess_control_output_components = 128;
    spirv_resources.max_tess_control_texture_image_units = 16;
    spirv_resources.max_tess_control_uniform_components = 1024;
    spirv_resources.max_tess_control_total_output_components = 4096;
    spirv_resources.max_tess_evaluation_input_components = 128;
    spirv_resources.max_tess_evaluation_output_components = 128;
    spirv_resources.max_tess_evaluation_texture_image_units = 16;
    spirv_resources.max_tess_evaluation_uniform_components = 1024;
    spirv_resources.max_tess_patch_components = 120;
    spirv_resources.max_patch_vertices = 32;
    spirv_resources.max_tess_gen_level = 64;
    spirv_resources.max_viewports = 16;
    spirv_resources.max_vertex_atomic_counters = 0;
    spirv_resources.max_tess_control_atomic_counters = 0;
    spirv_resources.max_tess_evaluation_atomic_counters = 0;
    spirv_resources.max_geometry_atomic_counters = 0;
    spirv_resources.max_fragment_atomic_counters = 8;
    spirv_resources.max_combined_atomic_counters = 8;
    spirv_resources.max_atomic_counter_bindings = 1;
    spirv_resources.max_vertex_atomic_counter_buffers = 0;
    spirv_resources.max_tess_control_atomic_counter_buffers = 0;
    spirv_resources.max_tess_evaluation_atomic_counter_buffers = 0;
    spirv_resources.max_geometry_atomic_counter_buffers = 0;
    spirv_resources.max_fragment_atomic_counter_buffers = 1;
    spirv_resources.max_combined_atomic_counter_buffers = 1;
    spirv_resources.max_atomic_counter_buffer_size = 16384;
    spirv_resources.max_transform_feedback_buffers = 4;
    spirv_resources.max_transform_feedback_interleaved_components = 64;
    spirv_resources.max_cull_distances = 8;
    spirv_resources.max_combined_clip_and_cull_distances = 8;
    spirv_resources.max_samples = 4;
    spirv_resources.max_mesh_output_vertices_nv = 256;
    spirv_resources.max_mesh_output_primitives_nv = 512;
    spirv_resources.max_mesh_work_group_size_x_nv = 32;
    spirv_resources.max_mesh_work_group_size_y_nv = 1;
    spirv_resources.max_mesh_work_group_size_z_nv = 1;
    spirv_resources.max_task_work_group_size_x_nv = 32;
    spirv_resources.max_task_work_group_size_y_nv = 1;
    spirv_resources.max_task_work_group_size_z_nv = 1;
    spirv_resources.max_mesh_view_count_nv = 4;
    spirv_resources.limits.non_inductive_for_loops = 1;
    spirv_resources.limits.while_loops = 1;
    spirv_resources.limits.do_while_loops = 1;
    spirv_resources.limits.general_uniform_indexing = 1;
    spirv_resources.limits.general_attribute_matrix_vector_indexing = 1;
    spirv_resources.limits.general_varying_indexing = 1;
    spirv_resources.limits.general_sampler_indexing = 1;
    spirv_resources.limits.general_variable_indexing = 1;
    spirv_resources.limits.general_constant_matrix_vector_indexing = 1;
    spirv_resources.maxDualSourceDrawBuffersEXT = 1;
}

inline void spirv_uninit() {
    glslang_finalize_process();
}

#else /* VKU_HAS_NEW_GLSLANG */

inline spirv_t spirv_compile(vku_shader_stage_e stage, const char *code) {
    VK_ASSERT(init());

    EShLanguage esh_stage;
    switch (stage) {
        case VKU_SPIRV_VERTEX:    esh_stage = EShLangVertex;         break;
        case VKU_SPIRV_TESS_CTRL: esh_stage = EShLangTessControl;    break;
        case VKU_SPIRV_TESS_EVAL: esh_stage = EShLangTessEvaluation; break;
        case VKU_SPIRV_GEOMETRY:  esh_stage = EShLangGeometry;       break;
        case VKU_SPIRV_FRAGMENT:  esh_stage = EShLangFragment;       break;
        case VKU_SPIRV_COMPUTE:   esh_stage = EShLangCompute;        break;
        default:
            DBG("Unknown shader stage type: %d", (uint32_t)stage);
            throw vku::except_t(VK_ERROR_UNKNOWN);
    }
    glslang::TShader shader(esh_stage);
    glslang::TProgram program;
    const char *shader_strings[] = { code };

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    shader.setStrings(shader_strings, 1);
    if (!shader.parse(&spirv_resources, 100, false, messages)) {
        DBG("Parse Failed(Log): [%s]", shader.getInfoLog());
        DBG("Parse Failed(Dbg): [%s]", shader.getInfoDebugLog());
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }

    program.addShader(&shader);
    if (!program.link(messages)) {
        DBG("Link Failed(Log): [%s]", shader.getInfoLog());
        DBG("Link Failed(Dbg): [%s]", shader.getInfoDebugLog());
        throw vku::except_t(VK_ERROR_UNKNOWN);
    }

    spirv_t ret;
    glslang::GlslangToSpv(*program.getIntermediate(esh_stage), ret.content);
    ret.type = stage;
    return ret;
}

inline void spirv_init() {
    glslang::InitializeProcess();
    spirv_resources.maxLights = 32;
    spirv_resources.maxClipPlanes = 6;
    spirv_resources.maxTextureUnits = 32;
    spirv_resources.maxTextureCoords = 32;
    spirv_resources.maxVertexAttribs = 64;
    spirv_resources.maxVertexUniformComponents = 4096;
    spirv_resources.maxVaryingFloats = 64;
    spirv_resources.maxVertexTextureImageUnits = 32;
    spirv_resources.maxCombinedTextureImageUnits = 80;
    spirv_resources.maxTextureImageUnits = 32;
    spirv_resources.maxFragmentUniformComponents = 4096;
    spirv_resources.maxDrawBuffers = 32;
    spirv_resources.maxVertexUniformVectors = 128;
    spirv_resources.maxVaryingVectors = 8;
    spirv_resources.maxFragmentUniformVectors = 16;
    spirv_resources.maxVertexOutputVectors = 16;
    spirv_resources.maxFragmentInputVectors = 15;
    spirv_resources.minProgramTexelOffset = -8;
    spirv_resources.maxProgramTexelOffset = 7;
    spirv_resources.maxClipDistances = 8;
    spirv_resources.maxComputeWorkGroupCountX = 65535;
    spirv_resources.maxComputeWorkGroupCountY = 65535;
    spirv_resources.maxComputeWorkGroupCountZ = 65535;
    spirv_resources.maxComputeWorkGroupSizeX = 1024;
    spirv_resources.maxComputeWorkGroupSizeY = 1024;
    spirv_resources.maxComputeWorkGroupSizeZ = 64;
    spirv_resources.maxComputeUniformComponents = 1024;
    spirv_resources.maxComputeTextureImageUnits = 16;
    spirv_resources.maxComputeImageUniforms = 8;
    spirv_resources.maxComputeAtomicCounters = 8;
    spirv_resources.maxComputeAtomicCounterBuffers = 1;
    spirv_resources.maxVaryingComponents = 60;
    spirv_resources.maxVertexOutputComponents = 64;
    spirv_resources.maxGeometryInputComponents = 64;
    spirv_resources.maxGeometryOutputComponents = 128;
    spirv_resources.maxFragmentInputComponents = 128;
    spirv_resources.maxImageUnits = 8;
    spirv_resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    spirv_resources.maxCombinedShaderOutputResources = 8;
    spirv_resources.maxImageSamples = 0;
    spirv_resources.maxVertexImageUniforms = 0;
    spirv_resources.maxTessControlImageUniforms = 0;
    spirv_resources.maxTessEvaluationImageUniforms = 0;
    spirv_resources.maxGeometryImageUniforms = 0;
    spirv_resources.maxFragmentImageUniforms = 8;
    spirv_resources.maxCombinedImageUniforms = 8;
    spirv_resources.maxGeometryTextureImageUnits = 16;
    spirv_resources.maxGeometryOutputVertices = 256;
    spirv_resources.maxGeometryTotalOutputComponents = 1024;
    spirv_resources.maxGeometryUniformComponents = 1024;
    spirv_resources.maxGeometryVaryingComponents = 64;
    spirv_resources.maxTessControlInputComponents = 128;
    spirv_resources.maxTessControlOutputComponents = 128;
    spirv_resources.maxTessControlTextureImageUnits = 16;
    spirv_resources.maxTessControlUniformComponents = 1024;
    spirv_resources.maxTessControlTotalOutputComponents = 4096;
    spirv_resources.maxTessEvaluationInputComponents = 128;
    spirv_resources.maxTessEvaluationOutputComponents = 128;
    spirv_resources.maxTessEvaluationTextureImageUnits = 16;
    spirv_resources.maxTessEvaluationUniformComponents = 1024;
    spirv_resources.maxTessPatchComponents = 120;
    spirv_resources.maxPatchVertices = 32;
    spirv_resources.maxTessGenLevel = 64;
    spirv_resources.maxViewports = 16;
    spirv_resources.maxVertexAtomicCounters = 0;
    spirv_resources.maxTessControlAtomicCounters = 0;
    spirv_resources.maxTessEvaluationAtomicCounters = 0;
    spirv_resources.maxGeometryAtomicCounters = 0;
    spirv_resources.maxFragmentAtomicCounters = 8;
    spirv_resources.maxCombinedAtomicCounters = 8;
    spirv_resources.maxAtomicCounterBindings = 1;
    spirv_resources.maxVertexAtomicCounterBuffers = 0;
    spirv_resources.maxTessControlAtomicCounterBuffers = 0;
    spirv_resources.maxTessEvaluationAtomicCounterBuffers = 0;
    spirv_resources.maxGeometryAtomicCounterBuffers = 0;
    spirv_resources.maxFragmentAtomicCounterBuffers = 1;
    spirv_resources.maxCombinedAtomicCounterBuffers = 1;
    spirv_resources.maxAtomicCounterBufferSize = 16384;
    spirv_resources.maxTransformFeedbackBuffers = 4;
    spirv_resources.maxTransformFeedbackInterleavedComponents = 64;
    spirv_resources.maxCullDistances = 8;
    spirv_resources.maxCombinedClipAndCullDistances = 8;
    spirv_resources.maxSamples = 4;
    spirv_resources.maxMeshOutputVerticesNV = 256;
    spirv_resources.maxMeshOutputPrimitivesNV = 512;
    spirv_resources.maxMeshWorkGroupSizeX_NV = 32;
    spirv_resources.maxMeshWorkGroupSizeY_NV = 1;
    spirv_resources.maxMeshWorkGroupSizeZ_NV = 1;
    spirv_resources.maxTaskWorkGroupSizeX_NV = 32;
    spirv_resources.maxTaskWorkGroupSizeY_NV = 1;
    spirv_resources.maxTaskWorkGroupSizeZ_NV = 1;
    spirv_resources.maxMeshViewCountNV = 4;
    spirv_resources.limits.nonInductiveForLoops = 1;
    spirv_resources.limits.whileLoops = 1;
    spirv_resources.limits.doWhileLoops = 1;
    spirv_resources.limits.generalUniformIndexing = 1;
    spirv_resources.limits.generalAttributeMatrixVectorIndexing = 1;
    spirv_resources.limits.generalVaryingIndexing = 1;
    spirv_resources.limits.generalSamplerIndexing = 1;
    spirv_resources.limits.generalVariableIndexing = 1;
    spirv_resources.limits.generalConstantMatrixVectorIndexing = 1;
}

inline void spirv_uninit() {
    glslang::FinalizeProcess();
}

#endif /* VKU_HAS_NEW_GLSLANG */

inline uint32_t find_memory_type(ref_t<device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties, size_t sz)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(dev->vk_phy_dev, &mem_props);

    int to_ret = -1;
    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if (type_filter & (1 << i)
                && (mem_props.memoryTypes[i].propertyFlags & properties) == properties
                && mem_props.memoryHeaps[mem_props.memoryTypes[i].heapIndex].size >= sz)
        {
            to_ret = i;
        }
    }
    if (to_ret > 0)
        return to_ret;

    DBG("Couldn't find suitable memory type");
    throw vku::except_t(VK_ERROR_UNKNOWN);
}

inline std::string vk_err_str(vc::ret_t vk_err) {
    return std::format("{}[{}]", vk_err_cstr(vk_err), (size_t)vk_err);
}

inline const char *vk_err_cstr(vc::ret_t res) {
    switch(res) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
#ifdef VK_PIPELINE_COMPILE_REQUIRED
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
#endif
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
#endif
#ifdef VK_ENABLE_BETA_EXTENSIONS
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
#endif
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
#ifdef VK_ERROR_NOT_PERMITTED_KHR
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
#endif
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
#ifdef VK_THREAD_IDLE_KHR
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
#endif
#ifdef VK_THREAD_DONE_KHR
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
#endif
#ifdef VK_OPERATION_DEFERRED_KHR
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
#endif
#ifdef VK_OPERATION_NOT_DEFERRED_KHR
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
#endif
#ifdef VK_ERROR_COMPRESSION_EXHAUSTED_EXT
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
#endif
#ifdef VK_PIPELINE_COMPILE_REQUIRED
        case VK_ERROR_NOT_PERMITTED_EXT:
            return "VK_ERROR_NOT_PERMITTED_EXT";
#endif
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
            return "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT | VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
#ifdef VK_PIPELINE_COMPILE_REQUIRED_EXT
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
#endif
#ifdef VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT
        case VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT:
            return "VK_ERROR_PIPELINE_COMPILE_REQUIRED_EXT";
#endif
        default:
            return "VK_UNKNOWN_ERR";
    }
}

} /* namespace utils */

#endif

