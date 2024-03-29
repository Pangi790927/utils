#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#include "gpu_defines.h"
#include "debug.h"
#include "misc_utils.h"

#include <glslang/SPIRV/GlslangToSpv.h>
#include <vulkan/vulkan.h>
#include <exception>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <vector>

#define VK_ASSERT(fn_call)                                                                         \
do {                                                                                               \
    vk_result_t vk_err;                                                                            \
    if ((vk_err = (fn_call)) != VK_SUCCESS) {                                                      \
        DBG("Failed vk assert: [%s: %d]", vk_err_str(vk_err), vk_err);                             \
        throw vku_err_t(vk_err);                                                                   \
    }                                                                                              \
} while (false);

/* TODO:
    - Continue the tutorial: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
    - Add compute shaders and compute things
    - Create an ImGui backend using this helper
    - Add the "#include ..." macro for shaders and test if the rest work as expected
    - Add the option to use multiple include dirs for shader compilation
 */

enum vku_shader_stage_t {
    VKU_SPIRV_VERTEX,
    VKU_SPIRV_FRAGMENT,
    VKU_SPIRV_COMPUTE,
    VKU_SPIRV_GEOMETRY,
    VKU_SPIRV_TESS_CTRL,
    VKU_SPIRV_TESS_EVAL,
};

struct vku_opts_t {
    std::vector<std::string> exts = { "VK_EXT_debug_utils" };
    std::vector<std::string> layers = { "VK_LAYER_KHRONOS_validation" };
    std::string window_name = "vk_window_name_placeholder";
    int window_width = 800;
    int window_heigth = 600;
};

struct vku_err_t;
struct vku_gpu_family_ids_t;
struct vku_spirv_t;

struct vku_vertex_input_desc_t;
struct vku_vertex_p2n0c3t2_t;

struct vku_binding_desc_t;
struct vku_mvp_t;

struct vku_object_t;
struct vku_instance_t;      /* uses (opts) */
struct vku_surface_t;       /* uses (instance) */
struct vku_device_t;        /* uses (surface) */
struct vku_swapchain_t;     /* uses (device) */
struct vku_shader_t;        /* uses (device, shader_data) */
struct vku_renderpass_t;    /* uses (swapchain) */
struct vku_pipeline_t;      /* uses (opts, renderpass, shaders, vertex_input_desc)*/
struct vku_framebuffs_t;    /* uses (renderpass) */
struct vku_cmdpool_t;       /* uses (device) */
struct vku_cmdbuff_t;       /* uses (cmdpool) */
struct vku_sem_t;           /* uses (device) */
struct vku_fence_t;         /* uses (device) */
struct vku_buffer_t;        /* uses (device) */
struct vku_desc_pool_t;     /* uses (device, ?buff?, ?pipeline?) */
struct vku_desc_set_t;      /* uses (desc_pool) */

inline void vku_wait_fences(std::vector<vku_fence_t *> fences);
inline void vku_reset_fences(std::vector<vku_fence_t *> fences);
inline void vku_aquire_next_img(vku_swapchain_t *swc, vku_sem_t *sem, uint32_t *img_idx);

inline void vku_submit_cmdbuff(
        std::vector<std::pair<vku_sem_t *, VkPipelineStageFlags>> wait_sems,
        vku_cmdbuff_t *cbuff,
        vku_fence_t *fence,
        std::vector<vku_sem_t *> sig_sems);

inline void vku_present(
        vku_swapchain_t *swc,
        std::vector<vku_sem_t *> wait_sems,
        uint32_t img_idx);

inline void vku_copy_buff(vku_cmdpool_t *cp, vku_buffer_t *dst, vku_buffer_t *src,
        vk_device_size_t sz);

/* VKU Objects: 
================================================================================================= */

struct vku_err_t : public std::exception {
    vk_result_t vk_err;
    std::string err_str;

    vku_err_t(vk_result_t vk_err);
    vku_err_t(const std::string& str);
    const char *what() const noexcept override;
};

struct vku_gpu_family_ids_t {
    union {
        int graphics_id = -1;   /* same as compute id */
        int compute_id;
    };
    int present_id = -1;
};

struct vku_spirv_t {
    std::vector<uint32_t> content;
    vku_shader_stage_t type;
};

struct vku_vertex_input_desc_t {
    vk_vertex_input_binding_description_t                bind_desc;
    std::vector<vk_vertex_input_attribute_description_t> attr_desc;
};

struct vku_binding_desc_t {
    std::vector<vk_descriptor_set_layout_binding_t> layout_bindings;
};

struct vku_vertex_p2n0c3t2_t {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 tex;

    static vku_vertex_input_desc_t get_input_desc();
};

struct vku_mvp_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;

    static vku_binding_desc_t get_desc_set();
};

using vku_vertex2d_t = vku_vertex_p2n0c3t2_t;

struct vku_object_t {
    static GLFWwindow *_window;
    static int g_ref;

    GLFWwindow *window = NULL;
    std::set<vku_object_t *> childs;

    vku_object_t();
    vku_object_t(const vku_opts_t &opts);

    virtual void add_child(vku_object_t *child);
    virtual void rm_child(vku_object_t *child);
    virtual void cleanup();
    virtual ~vku_object_t();
};

struct vku_instance_t : public vku_object_t {
    vk_instance_t                   vk_instance;
    vk_debug_utils_messenger_ext_t  vk_dbg_messenger;
    
    vku_instance_t(const vku_opts_t &opts);
    ~vku_instance_t();
};

/* VkSurfaceKHR */
struct vku_surface_t : public vku_object_t {
    vku_instance_t *inst;
    vk_surface_khr_t vk_surface = NULL;

    vku_surface_t(vku_instance_t *inst);
    ~vku_surface_t();
};

struct vku_device_t : public vku_object_t {
    vku_surface_t           *surf;
    vk_physical_device_t    vk_phy_dev;
    vk_device_t             vk_dev;
    vk_queue_t              vk_graphics_que;
    vk_queue_t              vk_present_que;
    std::set<int>           que_ids;
    vku_gpu_family_ids_t    que_fams;

    vku_device_t(vku_surface_t *surf);
    ~vku_device_t();
};

/* VkSwapchainKHR */
struct vku_swapchain_t : public vku_object_t {
    vku_device_t                    *dev;
    vk_surface_format_khr_t         vk_surf_fmt;
    vk_present_mode_khr_t           vk_present_mode;
    vk_extent2d_t                   vk_extent;
    vk_swapchain_khr_t              vk_swapchain;
    std::vector<vk_image_t>         vk_sc_images;
    std::vector<vk_image_view_t>    vk_sc_image_views;

    vku_swapchain_t(vku_device_t *dev);
    ~vku_swapchain_t();
};

struct vku_shader_t : public vku_object_t {
    vku_device_t *dev;
    vk_shader_module_t vk_shader;
    vku_shader_stage_t type;

    vku_shader_t(vku_device_t *dev, const char *path, vku_shader_stage_t type);
    vku_shader_t(vku_device_t *dev, const vku_spirv_t& spirv);
    ~vku_shader_t();
};

struct vku_renderpass_t : public vku_object_t {
    vku_swapchain_t *swc;
    vk_render_pass_t vk_render_pass;

    vku_renderpass_t(vku_swapchain_t *swc);
    ~vku_renderpass_t();
};

struct vku_pipeline_t : public vku_object_t {
    vku_renderpass_t            *rp;
    vk_pipeline_t               vk_pipeline;
    vk_pipeline_layout_t        vk_layout;
    vk_descriptor_set_layout_t  vk_desc_set_layout;

    vku_pipeline_t(const vku_opts_t &opts, vku_renderpass_t *rp,
            const std::vector<vku_shader_t *> &shaders,
            vku_vertex_input_desc_t vid, vku_binding_desc_t bd);
    ~vku_pipeline_t();
};

/*engine_create_framebuffs*/
struct vku_framebuffs_t : public vku_object_t {
    vku_renderpass_t *rp;
    std::vector<vk_framebuffer_t> vk_fbuffs;

    vku_framebuffs_t(vku_renderpass_t *rp);
    ~vku_framebuffs_t();
};

/*engine_create_cmdpool*/
struct vku_cmdpool_t : public vku_object_t {
    vku_device_t *dev;
    vk_command_pool_t vk_pool;
    
    vku_cmdpool_t(vku_device_t *dev);
    ~vku_cmdpool_t();
};

struct vku_cmdbuff_t : public vku_object_t {
    vku_cmdpool_t *cp;
    vk_command_buffer_t vk_buff;

    vku_cmdbuff_t(vku_cmdpool_t *cp);
    ~vku_cmdbuff_t();

    void begin(vk_command_buffer_usage_flags_t flags);
    void begin_rpass(vku_framebuffs_t *fbs, uint32_t img_idx);
    void bind_vert_buffs(uint32_t first_bind,
            std::vector<std::pair<vku_buffer_t *, vk_device_size_t>> buffs);
    void bind_desc_set(vk_pipeline_bind_point_t bind_point, vk_pipeline_layout_t pipeline_alyout,
            vku_desc_set_t *desc_set);
    void bind_idx_buff(vku_buffer_t *ibuff, uint64_t off, vk_index_type_t idx_type);
    void draw(vku_pipeline_t *pl, uint64_t vert_cnt);
    void draw_idx(vku_pipeline_t *pl, uint64_t vert_cnt);
    void end_rpass();
    void end();
    void reset();
};

struct vku_sem_t : public vku_object_t {
    vku_device_t *dev;
    vk_semaphore_t vk_sem;
    
    vku_sem_t(vku_device_t *dev);
    ~vku_sem_t();
};

struct vku_fence_t : public vku_object_t {
    vku_device_t *dev;
    vk_fence_t vk_fence;

    vku_fence_t(vku_device_t *dev, VkFenceCreateFlags flags = 0);
    ~vku_fence_t();
};

struct vku_buffer_t : public vku_object_t {
    vku_device_t *dev;
    vk_buffer_t vk_buff;
    vk_device_memory_t vk_mem;
    void *map_ptr = nullptr;
    size_t size;

    vku_buffer_t(vku_device_t *dev,
            size_t size,
            vk_buffer_usage_flags_t usage,
            vk_sharing_mode_t sh_mode,
            vk_memory_property_flags_t mem_flags);

    ~vku_buffer_t();

    void *map_data(vk_device_size_t offset, vk_device_size_t size);
    void unmap_data();
};

struct vku_image_t : public vku_object_t {
    vku_device_t        *dev;
    vk_image_t          vk_img;
    vk_device_memory_t  vk_img_mem;

    uint32_t width;
    uint32_t height;
    vk_format_t fmt;

    vku_image_t(vku_device_t *dev, uint32_t width, uint32_t height, vk_format_t fmt)
    : dev(dev), width(width), height(height), fmt(fmt)
    {
        vk_image_create_info_t image_info{
            .image_type = VK_IMAGE_TYPE_2D,
            .extent = {
                .width = width,
                .height = height,
                .depth = 1,
            },
            .mip_levels = 1,
            .array_layers = 1,
            .format = fmt,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
            .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .flags = 0,
        };

        VK_ASSERT(vk_create_image(dev->vk_dev, &image_info, nullptr, &vk_img));
        FnScope err_scope([&]{ vk_destroy_image(dev->vk_dev, vk_img, nullptr); });

        vk_memory_requirements_t mem_req;
        vk_get_image_memory_requirements(dev->vk_dev, vk_img, &mem_req);

        vk_memory_allocate_info_t alloc_info{
            .allocation_size = mem_req.size,
            .memory_type_index = vku_find_memory_type(dev->vk_dev, mem_req.memory_type_bits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
        };

        VK_ASSERT(vk_allocate_memory(vk->vk_dev, &alloc_info, nullptr, &vk_img_mem));
        VK_ASSERT(vk_bind_image_memory(vk->vk_dev, vk_img, vk_img_mem, 0));

        dev->add_child(this);
        err_scope.disable();
    }

    void transition_layout(vku_cmdpool_t *cp,
            vk_image_layout_t old_layout, vk_image_layout_t new_layout)
    {
        auto cbuff = std::make_unique<vku_cmdbuff_t>(cp);
        auto fence = std::make_unique<vku_fence_t>(cp->dev);

        cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        
        vk_image_memory_barrier_t barrier {
            .old_layout = old_layout,
            .new_layout = new_layout,
            .src_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
            .dst_queue_family_index = VK_QUEUE_FAMILY_IGNORED,
            .image = vk_img,
            .subresource_range = {
                .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 0,
            },
        };

        vk_pipeline_stage_flags_t src_stage;
        vk_pipeline_stage_flags_t dst_stage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.src_access_mask = 0;
            barrier.dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;

            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                 new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dst_access_mask = VK_ACCESS_SHADER_READ_BIT;

            src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        /* TODO: don't we need a transition from shader_read to transfer_dst? That for transfering
        inside the image later on? */
        else {
            throw vku_err_t(sformat("unsupported layout transition for image! %d -> %d",
                    old_layout, new_layout));
        }

        vk_cmd_pipeline_barrier(cbuff->vk_buff, src_stage, dst_stage,
                0, 0, nullptr, 0, nullptr, 1, &barrier);

        cbuff->end();

        vku_submit_cmdbuff({}, cbuff.get(), fence.get(), {});
        vku_wait_fences({fence.get()});
    }

    void set_data(vku_cmdpool_t *cp, void *data, uint32_t sz) {
        uint32_t img_sz = width * height * 4;

        if (img_sz != sz)
            throw vku_err_t(sformat("data size(%d) does not match with image size(%d)", sz, img_sz));

        auto buff = std::make_unique<vku_buffer_t>(
            cp->dev,
            img_sz,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_SHARING_MODE_EXCLUSIVE,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        memcpy(buff->map_data(0, img_sz), data, img_sz);
        buff->unmap_data();

        transition_layout(cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        auto cbuff = std::make_unique<vku_cmdbuff_t>(cp);
        auto fence = std::make_unique<vku_fence_t>(cp->dev);

        cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        vk_buffer_image_copy region{
            .buffer_offset = 0,
            .buffer_row_length = 0,
            .buffer_image_height = 0,
            .image_subresource = {
                .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mip_level = 0,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_offset = { .x = 0, .y = 0, .z = 0 },
            .image_extent = {
                .width = width,
                .height = height,
                .depth = 1,
            } 
        };
        vk_cmd_copy_buffer_to_image(
            cbuff->vk_buff,
            buff->vk_buff,
            vk_img,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region
        );

        cbuff->end();

        vku_submit_cmdbuff({}, cbuff.get(), fence.get(), {});
        vku_wait_fences({fence.get()});

        transition_layout(cp, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    ~vku_image_t() {
        cleanup();
        dev->rm_child(this);

        vk_destroy_image(dev->vk_dev, vk_img, nullptr);
        vk_free_memory(dev->vk_dev, vk_img_mem, nullptr);
    }
};

struct vku_img_view_t : public vku_object_t {
    vku_img_view_t() {
        VkImageView imageView;

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = textureImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;


        if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }
    }
    ~vku_img_view_t() {
        vkDestroyImageView(device, textureImageView, nullptr);
    }
};

struct vku_img_sampl_t : public vku_object_t {
    vku_img_sampl_t() {
        VkSampler textureSampler;

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        // samplerInfo.anisotropyEnable = VK_FALSE;
        // samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    vku_img_sampl_t() {
        vkDestroySampler(device, textureSampler, nullptr);
    }
};

struct vku_desc_pool_t : public vku_object_t {
    vku_device_t            *dev;
    vk_descriptor_pool_t    vk_descpool;

    vku_desc_pool_t(vku_device_t *dev, vk_descriptor_type_t type, uint32_t cnt);
    ~vku_desc_pool_t();
};

struct vku_desc_set_t : public vku_object_t {
    vku_desc_pool_t *dp;
    vk_descriptor_set_t vk_desc_set;

    /* TODO: should the desc_pool be based on pipeline and recreated with the pipeline? And
    on the buffer that uses it? */
    vku_desc_set_t(vku_desc_pool_t *dp, vk_descriptor_set_layout_t layout,
            vku_buffer_t *buff, uint32_t binding, vk_descriptor_type_t type);
    ~vku_desc_set_t();
};

/* Internal : 
================================================================================================= */

struct vku_swapchain_details_t {
    vk_surface_capabilities_khr_t capab;
    std::vector<vk_surface_format_khr_t> formats;
    std::vector<vk_present_mode_khr_t> present_modes;
};

inline const char *vk_err_str(vk_result_t res);
inline std::string vku_glfw_err();

inline vk_result_t vku_create_dbg_messenger(
        vk_instance_t instance,
        const vk_debug_utils_messenger_create_info_ext_t* dbg_info,
        const vk_allocation_callbacks_t* alloc,
        vk_debug_utils_messenger_ext_t* dbg_msg);

inline void vku_destroy_dbg_messenger(
        vk_instance_t instance,
        vk_debug_utils_messenger_ext_t dbg_msg,
        const vk_allocation_callbacks_t* alloc);

static VKAPI_ATTR vk_bool32_t VKAPI_CALL vku_dbg_cbk(
        vk_debug_utils_message_severity_flag_bits_ext_t     severity,
        vk_debug_utils_message_type_flags_ext_t             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* _data,
        void* ctx);

inline  vku_swapchain_details_t vku_get_swapchain_details(vk_physical_device_t dev,
        vk_surface_khr_t surf);

inline vku_gpu_family_ids_t vku_find_queue_families(vk_physical_device_t dev,
        vk_surface_khr_t surface);

inline vk_extent2d_t vku_choose_extent(GLFWwindow *window, vk_surface_capabilities_khr_t capab);

inline int vku_score_phydev(vk_physical_device_t dev, vk_surface_khr_t surf);
inline vk_shader_stage_flag_bits_t vku_get_shader_type(vku_shader_stage_t own_type);

inline TBuiltInResource vku_spirv_resources = {};

inline void vku_spirv_uninit();
inline void vku_spirv_init();
inline vku_spirv_t vku_spirv_compile(vku_instance_t *inst, vku_shader_stage_t vk_stage,
        const char *code);

inline uint32_t vku_find_memory_type(vku_device_t *dev,
        uint32_t type_filter, vk_memory_property_flags_t properties);

/* IMPLEMENTATION:
================================================================================================= */

inline vku_err_t::vku_err_t(vk_result_t vk_err) : vk_err(vk_err) {
    err_str = sformat("VULKAN_UTILS_ERR: %s[%d]", vk_err_str(vk_err), vk_err);
}

inline vku_err_t::vku_err_t(const std::string &str) : err_str(str), vk_err(-1) {}

inline const char *vku_err_t::what() const noexcept {
    return err_str.c_str();
}

inline vku_vertex_input_desc_t vku_vertex_p2n0c3t2_t::get_input_desc() {
    return {
        .bind_desc = {
            .binding = 0,
            .stride = sizeof(vku_vertex_p2n0c3t2_t),
            .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        .attr_desc = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vku_vertex_p2n0c3t2_t, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vku_vertex_p2n0c3t2_t, color)
            },
            {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vku_vertex_p2n0c3t2_t, tex)
            }
        }
    };
}

inline vku_binding_desc_t vku_mvp_t::get_desc_set() {
    return {
        .layout_bindings = {
            {
                .binding = 0,
                .descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptor_count = 1,
                .stage_flags = VK_SHADER_STAGE_VERTEX_BIT,
                .p_immutable_samplers = nullptr,
            }
        }
    };
}

inline vku_object_t::vku_object_t() {
    if (_window) {
        g_ref++;
        window = _window;
    }
    else
        throw vku_err_t(VK_ERROR_UNKNOWN); 
}

inline vku_object_t::vku_object_t(const vku_opts_t &opts) {
    FnScope err_scope;
    if (_window) {
        g_ref++;
        window = _window;
    }
    vku_spirv_init();
    if (glfw_init() != GLFW_TRUE) {
        DBG("Failed to init glfw: %s", vku_glfw_err().c_str());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    glfw_window_hint(GLFW_CLIENT_API, GLFW_NO_API);
    err_scope(glfw_terminate);

    /* obs: resizeing breaks swapchain: ?? */
    _window = glfw_create_window(opts.window_width, opts.window_heigth, opts.window_name.c_str(),
            NULL, NULL);
    if (!_window) {
        DBG("Failed to create a glfw window: %s", vku_glfw_err().c_str());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    g_ref++;
    window = _window;
    err_scope.disable();
}

inline vku_object_t::~vku_object_t() {
    g_ref--;
    if (!g_ref && window) {
        glfw_destroy_window(window);
        glfw_terminate(); /* what this? */
        vku_spirv_uninit();
    }
}

inline void vku_object_t::add_child(vku_object_t *child) {
    childs.insert(child);
}

inline void vku_object_t::rm_child(vku_object_t *child) {
    childs.erase(child);
}

inline void vku_object_t::cleanup() {
    auto childs_tmp = childs;
    for (auto c : childs_tmp)
        delete c;
}

inline GLFWwindow *vku_object_t::_window = NULL;
inline int vku_object_t::g_ref = 0;

/* VkInstance */
inline vku_instance_t::vku_instance_t(const vku_opts_t &opts) : vku_object_t(opts) {
    FnScope err_scope;

    /* get version: */
    uint32_t ver;
    VK_ASSERT(vk_enumerate_instance_version(&ver));
    DBG("VK ver: %d.%d.%d",  VK_VERSION_MAJOR(ver), VK_VERSION_MINOR(ver), VK_VERSION_PATCH(ver));
    ver &= ~(0xFFFU);
    
    /* get required extensions: */
    uint32_t glfw_ext_count = 0;
    const char** glfw_exts;
    glfw_exts = glfw_get_required_instance_extensions(&glfw_ext_count);

    if (!glfw_exts) {
        DBG("Failed to get required extensions for glfw: %s", vku_glfw_err().c_str());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    std::vector<const char*> exts(glfw_exts, glfw_exts + glfw_ext_count);
    for (auto &e : opts.exts)
        exts.push_back(e.c_str());
    for (auto e : exts)
        DBG("Required extension: %s", e);

    /* get supported extensions */
    uint32_t ext_cnt = 0;
    VK_ASSERT(vk_enumerate_instance_extension_properties(NULL, &ext_cnt, NULL));
    std::vector<vk_extension_properties_t> extensions(ext_cnt);
    VK_ASSERT(vk_enumerate_instance_extension_properties(NULL, &ext_cnt, extensions.data()));

    /* check for compatibility */
    for (auto req_e : exts) {
        bool found = false;
        for (auto sup_e : extensions)
            if (!strcmp(req_e, sup_e.extension_name))
                found = true;
        if (!found) {
            DBG("Required extension %s is not supported", req_e);
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
    }

    /* set required layers */
    std::vector<const char*> layers;
    for (auto &l : opts.layers)
        layers.push_back(l.c_str());
    for (auto l : layers)
        DBG("Required layer: %s", l);

    /* get supported layers */
    uint32_t layer_cnt = 0;
    VK_ASSERT(vk_enumerate_instance_layer_properties(&layer_cnt, NULL));
    std::vector<vk_layer_properties_t> sup_layers(layer_cnt);
    VK_ASSERT(vk_enumerate_instance_layer_properties(&layer_cnt, sup_layers.data()));

    /* check for compatibility */
    for (auto req_l : layers) {
        bool found = false;
        for (auto sup_l : sup_layers)
            if (!strcmp(req_l, sup_l.layer_name))
                found = true;
        if (!found) {
            DBG("Required extension %s is not supported", req_l);
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
    }

    vk_application_info_t vk_app_info = {
        .p_application_name = opts.window_name.c_str(),
        .application_version = ver,
        .p_engine_name = opts.window_name.c_str(),
        .engine_version = ver,
        .api_version = ver,
    };

    vk_instance_create_info_t inst_info = {
        .p_application_info = &vk_app_info,
        .enabled_layer_count = (uint32_t)layers.size(),
        .pp_enabled_layer_names = layers.data(),
        .enabled_extension_count = (uint32_t)exts.size(),
        .pp_enabled_extension_names = exts.data(),
    };

    /* create instance */
    VK_ASSERT(vk_create_instance(&inst_info, NULL, &vk_instance));
    err_scope([&]{ vk_destroy_instance(vk_instance, NULL); });

    DBG("Created a vulkan instance!");

    /* Create Vulkan Debug Messenger
    ============================================================================================= */

    vk_debug_utils_messenger_create_info_ext_t dbg_info = {
        .message_severity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .message_type =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfn_user_callback = vku_dbg_cbk,
        .p_user_data = NULL
    };

    VK_ASSERT(vku_create_dbg_messenger(vk_instance, &dbg_info, NULL, &vk_dbg_messenger));
    err_scope([&]{ vku_destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL); });

    err_scope.disable();
    DBG("Created vulkan messenger!");
}

inline vku_instance_t::~vku_instance_t() {
    cleanup();
    vku_destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL);
    vk_destroy_instance(vk_instance, NULL);
}

inline vku_surface_t::vku_surface_t(vku_instance_t *inst) : inst(inst) {
    if (glfwCreateWindowSurface(inst->vk_instance, window, NULL, &vk_surface) != VK_SUCCESS) {
        DBG("Failed to get vk_surface: %s", vku_glfw_err().c_str());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    inst->add_child(this);
}
inline vku_surface_t::~vku_surface_t() {
    cleanup();
    inst->rm_child(this);
    vk_destroy_surface_khr(inst->vk_instance, vk_surface, NULL);
}

inline vku_device_t::vku_device_t(vku_surface_t *surf) : surf(surf) {
    uint32_t dev_cnt = 0;
    VK_ASSERT(vk_enumerate_physical_devices(surf->inst->vk_instance, &dev_cnt, NULL));

    if (dev_cnt == 0) {
        DBG("Failed to find a GPU with Vulkan support!")
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    std::vector<vk_physical_device_t> devices(dev_cnt);
    VK_ASSERT(vk_enumerate_physical_devices(
            surf->inst->vk_instance, &dev_cnt, devices.data()));

    vk_phy_dev = VK_NULL_HANDLE;
    int max_score = -1;
    for (const auto &dev : devices) {
        int score = vku_score_phydev(dev, surf->vk_surface);
        if (score < 0)
            continue;
        if (score > max_score) {
            max_score = score;
            vk_phy_dev = dev;
        }
    }
    if (max_score < 0) {
        DBG("Failed to get a suitable physical device");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    que_fams = vku_find_queue_families(vk_phy_dev, surf->vk_surface);
    que_ids = { que_fams.graphics_id, que_fams.present_id };
    std::vector<vk_device_queue_create_info_t> dev_ques;

    float queue_prio = 1.0f;
    for (auto id : que_ids)
        dev_ques.push_back({
            .flags = 0,
            .queue_family_index = (uint32_t)id,
            .queue_count = 1,
            .p_queue_priorities = &queue_prio
        });

    vk_physical_device_features_t dev_feat{};
    vk_get_physical_device_features(vk_phy_dev, &dev_feat);

    std::vector<const char*> dev_exts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    std::vector<const char*> dev_layers = { "VK_LAYER_KHRONOS_validation" };
    dev_feat.sampler_anisotropy = VK_TRUE;

    vk_device_create_info_t dev_info {
        .queue_create_info_count = (uint32_t)dev_ques.size(),
        .p_queue_create_infos = dev_ques.data(),
        .enabled_layer_count = (uint32_t)dev_layers.size(),
        .pp_enabled_layer_names = dev_layers.data(),
        .enabled_extension_count = (uint32_t)dev_exts.size(),
        .pp_enabled_extension_names = dev_exts.data(),
        .p_enabled_features = &dev_feat,
    };

    VK_ASSERT(vk_create_device(vk_phy_dev, &dev_info, NULL, &vk_dev));

    vk_get_device_queue(vk_dev, que_fams.graphics_id, 0, &vk_graphics_que);
    vk_get_device_queue(vk_dev, que_fams.present_id, 0, &vk_present_que);

    DBG("Created Vulkan Logical Device");
    surf->add_child(this);
}

inline vku_device_t::~vku_device_t() {
    cleanup();
    surf->rm_child(this);
    vk_destroy_device(vk_dev, NULL);
}

inline vku_swapchain_t::vku_swapchain_t(vku_device_t *dev) : dev(dev) {
    FnScope err_scope;
    auto sc_detail = vku_get_swapchain_details(dev->vk_phy_dev, dev->surf->vk_surface);

    /* choose format */
    vk_surf_fmt = sc_detail.formats[0];
    for (const auto &f : sc_detail.formats)
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.color_space ==
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
    vk_extent = vku_choose_extent(window, sc_detail.capab);

    /* choose image count */
    uint32_t img_cnt = sc_detail.capab.min_image_count + 1;
    if (sc_detail.capab.max_image_count > 0 && img_cnt > sc_detail.capab.max_image_count)
        img_cnt = sc_detail.capab.max_image_count;

    std::vector<uint32_t> qf_arr = { dev->que_ids.begin(), dev->que_ids.end() };

    vk_swapchain_create_info_khr_t sc_info = {
        .surface = dev->surf->vk_surface,
        .min_image_count = img_cnt,
        .image_format = vk_surf_fmt.format,
        .image_color_space = vk_surf_fmt.color_space,
        .image_extent = vk_extent,
        .image_array_layers = 1,
        .image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .image_sharing_mode       = qf_arr.size() > 1 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queue_family_index_count = qf_arr.size() > 1 ? 2u : 0u,
        .p_queue_family_indices   = qf_arr.size() > 1 ? qf_arr.data() : NULL,
        .pre_transform = sc_detail.capab.current_transform,
        .composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .present_mode = vk_present_mode,
        .clipped = VK_TRUE,
        .old_swapchain = VK_NULL_HANDLE,
    };

    VK_ASSERT(vk_create_swapchain_khr(dev->vk_dev, &sc_info, NULL, &vk_swapchain));
    err_scope([&]{ vk_destroy_swapchain_khr(dev->vk_dev, vk_swapchain, NULL); });

    img_cnt = 0;
    VK_ASSERT(vk_get_swapchain_images_khr(dev->vk_dev, vk_swapchain, &img_cnt, NULL));
    vk_sc_images.resize(img_cnt);
    VK_ASSERT(vk_get_swapchain_images_khr(dev->vk_dev, vk_swapchain, &img_cnt,
            vk_sc_images.data()));

    DBG("Created Swapchain!");

    /* Create Swapchain Image Views
    ============================================================================================= */

    vk_sc_image_views.resize(vk_sc_images.size());

    for (int i = 0; i < vk_sc_images.size(); i++) {
        vk_image_view_create_info_t iv_info = {
            .image = vk_sc_images[i],
            .view_type = VK_IMAGE_VIEW_TYPE_2D,
            .format = vk_surf_fmt.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresource_range = {
                .aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT,
                .base_mip_level = 0,
                .level_count = 1,
                .base_array_layer = 0,
                .layer_count = 1,
            },
        };
        VK_ASSERT(vk_create_image_view(dev->vk_dev, &iv_info, NULL, &vk_sc_image_views[i]));
        err_scope([i, &dev, this]{ 
            vk_destroy_image_view(dev->vk_dev, vk_sc_image_views[i], NULL); });
    }

    err_scope.disable();
    dev->add_child(this);
}

inline vku_swapchain_t::~vku_swapchain_t() {
    cleanup();
    dev->rm_child(this);
    for (auto &iv : vk_sc_image_views)
        vk_destroy_image_view(dev->vk_dev, iv, NULL);
    vk_destroy_swapchain_khr(dev->vk_dev, vk_swapchain, NULL);
}

inline vku_shader_t::vku_shader_t(vku_device_t *dev, const char *path, vku_shader_stage_t type)
: dev(dev), type(type)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();

    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        DBG("Failed to read shader data");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vk_shader_module_create_info_t shader_info {
        .code_size = buffer.size(),
        .p_code = (uint32_t *)buffer.data(),
    };
    VK_ASSERT(vk_create_shader_module(dev->vk_dev, &shader_info, NULL, &vk_shader));
    dev->add_child(this);
}

inline vku_shader_t::vku_shader_t(vku_device_t *dev, const vku_spirv_t& spirv)
: dev(dev), type(spirv.type)
{
    vk_shader_module_create_info_t shader_info {
        .code_size = spirv.content.size() * sizeof(uint32_t),
        .p_code = spirv.content.data(),
    };
    VK_ASSERT(vk_create_shader_module(dev->vk_dev, &shader_info, NULL, &vk_shader));
    dev->add_child(this);
}

inline vku_shader_t::~vku_shader_t() {
    cleanup();
    dev->rm_child(this);
    vk_destroy_shader_module(dev->vk_dev, vk_shader, NULL);
}

inline vku_renderpass_t::vku_renderpass_t(vku_swapchain_t *swc) : swc(swc) {
    vk_attachment_description_t color_attach {
        .format = swc->vk_surf_fmt.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .load_op = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .store_op = VK_ATTACHMENT_STORE_OP_STORE,
        .stencil_load_op = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencil_store_op = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initial_layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .final_layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };

    /* indexes in color_attach vector, also in shader: layout(location = 0) out vec4 outColor */
    vk_attachment_reference_t color_attach_ref {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    /* subpasses are postprocessing stages, we only use one pass for now */
    vk_subpass_description_t subpass {
        .pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .color_attachment_count = 1,
        .p_color_attachments = &color_attach_ref,
    };

    /* TODO: expose dependency to exterior because else it doesn't make sense to have sems waiting
    on different stages */
    vk_subpass_dependency_t dependency {
        .src_subpass = VK_SUBPASS_EXTERNAL,
        .dst_subpass = 0,
        .src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .src_access_mask = 0,
        .dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    vk_render_pass_create_info_t render_pass_info {
        .attachment_count = 1,
        .p_attachments = &color_attach,
        .subpass_count = 1,
        .p_subpasses = &subpass,
        .dependency_count = 1,
        .p_dependencies = &dependency,
    };

    VK_ASSERT(vk_create_render_pass(swc->dev->vk_dev, &render_pass_info, NULL, &vk_render_pass));

    swc->add_child(this);
}

inline vku_renderpass_t::~vku_renderpass_t() {
    cleanup();
    swc->rm_child(this);
    vk_destroy_render_pass(swc->dev->vk_dev, vk_render_pass, NULL);
}

inline vku_pipeline_t::vku_pipeline_t(const vku_opts_t &opts, vku_renderpass_t *rp,
        const std::vector<vku_shader_t *> &shaders,
        vku_vertex_input_desc_t vid, vku_binding_desc_t bd) : rp(rp)
{
    FnScope err_scope;

    std::vector<vk_pipeline_shader_stage_create_info_t> shader_stages;
    for (auto sh : shaders) {
        shader_stages.push_back(vk_pipeline_shader_stage_create_info_t {
            .stage  = vku_get_shader_type(sh->type),
            .module = sh->vk_shader,
            .p_name = "main",
        });
    }

    /* mark prop of pipeline to be mutable */
    std::vector<vk_dynamic_state_t> dyn_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    vk_pipeline_dynamic_state_create_info_t dyn_info {
        .dynamic_state_count    = uint32_t(dyn_states.size()),
        .p_dynamic_states       = dyn_states.data()
    };

    vk_pipeline_vertex_input_state_create_info_t vertex_info {
        .vertex_binding_description_count   = 1,
        .p_vertex_binding_descriptions      = &vid.bind_desc,
        .vertex_attribute_description_count = (uint32_t)vid.attr_desc.size(),
        .p_vertex_attribute_descriptions    = vid.attr_desc.data(),
    };

    vk_pipeline_input_assembly_state_create_info_t input_assembly {
        .topology                   = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitive_restart_enable   = VK_FALSE,
    };

    vk_viewport_t viewport {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = float(opts.window_width),
        .height     = float(opts.window_heigth),
        .min_depth  = 0.0f,
        .max_depth  = 1.0f,
    };

    vk_rect2d_t scissor {
        .offset = {0, 0},
        .extent = rp->swc->vk_extent,
    };

    vk_pipeline_viewport_state_create_info_t vp_info {
        .viewport_count = 1,
        .p_viewports    = &viewport,
        .scissor_count  = 1,
        .p_scissors     = &scissor,
    };

    vk_pipeline_rasterization_state_create_info_t raster_info {
        .depth_clamp_enable         = VK_FALSE,
        .rasterizer_discard_enable  = VK_FALSE,
        .polygon_mode               = VK_POLYGON_MODE_FILL,
        .cull_mode                  = VK_CULL_MODE_BACK_BIT,
        .front_face                 = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depth_bias_enable          = VK_FALSE,
        .depth_bias_constant_factor = 0.0f,
        .depth_bias_clamp           = 0.0f,
        .depth_bias_slope_factor    = 0.0f,
        .line_width                 = 1.0f,
    };

    vk_pipeline_multisample_state_create_info_t multisample_info {
        .rasterization_samples      = VK_SAMPLE_COUNT_1_BIT,
        .sample_shading_enable      = VK_FALSE,
        .min_sample_shading         = 1.0f,
        .p_sample_mask              = NULL,
        .alpha_to_coverage_enable   = VK_FALSE,
        .alpha_to_one_enable        = VK_FALSE,
    };

    vk_pipeline_color_blend_attachment_state_t blend_attachment {
        .blend_enable           = VK_FALSE,
        .src_color_blend_factor = VK_BLEND_FACTOR_ONE,
        .dst_color_blend_factor = VK_BLEND_FACTOR_ZERO,
        .color_blend_op         = VK_BLEND_OP_ADD,
        .src_alpha_blend_factor = VK_BLEND_FACTOR_ONE,
        .dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO,
        .alpha_blend_op         = VK_BLEND_OP_ADD,
        .color_write_mask       =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT,
    };

    vk_pipeline_color_blend_state_create_info_t blend_info {
        .logic_op_enable    = VK_FALSE,
        .logic_op           = VK_LOGIC_OP_COPY,
        .attachment_count   = 1,
        .p_attachments      = &blend_attachment,
        .blend_constants    = { 0.0f, 0.0f, 0.0f, 0.0f },
    };

    vk_descriptor_set_layout_create_info_t desc_set_layout_info {
        .binding_count = (uint32_t)bd.layout_bindings.size(),
        .p_bindings = bd.layout_bindings.data(),
    };

    VK_ASSERT(vk_create_descriptor_set_layout(rp->swc->dev->vk_dev, &desc_set_layout_info, nullptr,
            &vk_desc_set_layout));
    err_scope([&]{ vk_destroy_descriptor_set_layout(
            rp->swc->dev->vk_dev, vk_desc_set_layout, nullptr); });

    vk_pipeline_layout_create_info_t pipeline_layout_info {
        .set_layout_count           = 1,
        .p_set_layouts              = &vk_desc_set_layout,
        .push_constant_range_count  = 0,
        .p_push_constant_ranges     = NULL,
    };

    VK_ASSERT(vk_create_pipeline_layout(
            rp->swc->dev->vk_dev, &pipeline_layout_info, NULL, &vk_layout));
    err_scope([&]{ vk_destroy_pipeline_layout(rp->swc->dev->vk_dev, vk_layout, NULL); });

    vk_graphics_pipeline_create_info_t pipeline_info {
        .stage_count = (uint32_t)shader_stages.size(),
        .p_stages               = shader_stages.data(),
        .p_vertex_input_state   = &vertex_info,
        .p_input_assembly_state = &input_assembly,
        .p_viewport_state       = &vp_info,
        .p_rasterization_state  = &raster_info,
        .p_multisample_state    = &multisample_info,
        .p_depth_stencil_state  = NULL,
        .p_color_blend_state    = &blend_info,
        .p_dynamic_state        = &dyn_info,
        .layout                 = vk_layout,
        .render_pass            = rp->vk_render_pass,
        .subpass                = 0,
        .base_pipeline_handle   = VK_NULL_HANDLE,
        .base_pipeline_index    = -1,
    };

    VK_ASSERT(vk_create_graphics_pipelines(
            rp->swc->dev->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));

    err_scope.disable();
    rp->add_child(this);
}

inline vku_pipeline_t::~vku_pipeline_t() {
    cleanup();
    rp->rm_child(this);
    vk_destroy_pipeline(rp->swc->dev->vk_dev, vk_pipeline, NULL);
    vk_destroy_pipeline_layout(rp->swc->dev->vk_dev, vk_layout, NULL);
    vk_destroy_descriptor_set_layout(rp->swc->dev->vk_dev, vk_desc_set_layout, nullptr);
}

inline vku_framebuffs_t::vku_framebuffs_t(vku_renderpass_t *rp) : rp(rp) {
    vk_fbuffs.resize(rp->swc->vk_sc_image_views.size());

    FnScope err_scope;
    for (int i = 0; i < vk_fbuffs.size(); i++) {
        vk_image_view_t attachs[] = { rp->swc->vk_sc_image_views[i] };
        
        vk_framebuffer_create_info_t fbuff_info {
            .render_pass = rp->vk_render_pass,
            .attachment_count = 1,
            .p_attachments = attachs,
            .width = rp->swc->vk_extent.width,
            .height = rp->swc->vk_extent.height,
            .layers = 1,
        };

        VK_ASSERT(vk_create_framebuffer(rp->swc->dev->vk_dev, &fbuff_info, NULL, &vk_fbuffs[i]));
        err_scope([this, i, rp]{ 
                vk_destroy_framebuffer(rp->swc->dev->vk_dev, vk_fbuffs[i], NULL); });
    }

    err_scope.disable();
    rp->add_child(this);
}

inline vku_framebuffs_t::~vku_framebuffs_t() {
    cleanup();
    rp->rm_child(this);
    for (auto fbuff : vk_fbuffs)
        vk_destroy_framebuffer(rp->swc->dev->vk_dev, fbuff, NULL);
}

inline vku_cmdpool_t::vku_cmdpool_t(vku_device_t *dev) : dev(dev) {
    vk_command_pool_create_info_t pool_info{
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queue_family_index = (uint32_t)dev->que_fams.graphics_id,
    };

    VK_ASSERT(vk_create_command_pool(dev->vk_dev, &pool_info, NULL, &vk_pool));
    dev->add_child(this);
}

inline vku_cmdpool_t::~vku_cmdpool_t() {
    cleanup();
    dev->rm_child(this);
    vk_destroy_command_pool(dev->vk_dev, vk_pool, NULL);
}

inline vku_cmdbuff_t::vku_cmdbuff_t(vku_cmdpool_t *cp) : cp(cp) {
    vk_command_buffer_allocate_info_t buff_info {
        .command_pool = cp->vk_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .command_buffer_count = 1,
    };

    VK_ASSERT(vk_allocate_command_buffers(cp->dev->vk_dev, &buff_info, &vk_buff));
    cp->add_child(this);
}
inline vku_cmdbuff_t::~vku_cmdbuff_t() {
    cleanup();
    cp->rm_child(this);
}

inline void vku_cmdbuff_t::begin(vk_command_buffer_usage_flags_t flags) {
    vk_command_buffer_begin_info_t begin_info {
        .flags = flags,
        .p_inheritance_info = NULL,
    };
    VK_ASSERT(vk_begin_command_buffer(vk_buff, &begin_info));
}

inline void vku_cmdbuff_t::begin_rpass(vku_framebuffs_t *fbs, uint32_t img_idx) {
    vk_clear_value_t clear_color = {{{ 0.0f, 0.0f, 0.0f, 1.0f}}};

    vk_render_pass_begin_info_t begin_info {
        .render_pass = fbs->rp->vk_render_pass,
        .framebuffer = fbs->vk_fbuffs[img_idx],
        .render_area = {
            .offset = {0, 0},
            .extent = fbs->rp->swc->vk_extent,
        },
        .clear_value_count = 1,
        .p_clear_values = &clear_color,
    };

    vk_cmd_begin_render_pass(vk_buff, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

inline void vku_cmdbuff_t::bind_vert_buffs(uint32_t first_bind,
        std::vector<std::pair<vku_buffer_t *, vk_device_size_t>> buffs)
{
    std::vector<vk_buffer_t> vk_buffs;
    std::vector<vk_device_size_t> vk_offsets;
    for (auto [b, off] : buffs) {
        vk_buffs.push_back(b->vk_buff);
        vk_offsets.push_back(off);
    }

    vk_cmd_bind_vertex_buffers(vk_buff, first_bind, vk_buffs.size(), vk_buffs.data(),
            vk_offsets.data());
}

inline void vku_cmdbuff_t::bind_desc_set(vk_pipeline_bind_point_t bind_point,
        vk_pipeline_layout_t pipeline_layout, vku_desc_set_t *desc_set)
{
    vk_cmd_bind_descriptor_sets(vk_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
            &desc_set->vk_desc_set, 0, nullptr);
}

inline void vku_cmdbuff_t::bind_idx_buff(vku_buffer_t *ibuff, uint64_t off, vk_index_type_t idx_type)
{
    vk_cmd_bind_index_buffer(vk_buff, ibuff->vk_buff, off, idx_type);
}

inline void vku_cmdbuff_draw_helper(auto vk_buff, auto pl) {
    vk_cmd_bind_pipeline(vk_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->vk_pipeline);

    vk_viewport_t viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(pl->rp->swc->vk_extent.width),
        .height = float(pl->rp->swc->vk_extent.height),
        .min_depth = 0.0f,
        .max_depth = 1.0f,
    };

    vk_cmd_set_viewport(vk_buff, 0, 1, &viewport);

    vk_rect2d_t scissor {
        .offset = {0, 0},
        .extent = pl->rp->swc->vk_extent
    };
    vk_cmd_set_scissor(vk_buff, 0, 1, &scissor);
}

inline void vku_cmdbuff_t::draw(vku_pipeline_t *pl, uint64_t vert_cnt) {
    vku_cmdbuff_draw_helper(vk_buff, pl);
    /* TODO: more than 3 vertexes... */
    vk_cmd_draw(vk_buff, vert_cnt, 1, 0, 0);
}

inline void vku_cmdbuff_t::draw_idx(vku_pipeline_t *pl, uint64_t vert_cnt) {
    vku_cmdbuff_draw_helper(vk_buff, pl);
    vk_cmd_draw_indexed(vk_buff, vert_cnt, 1, 0, 0, 0);
}

inline void vku_cmdbuff_t::end_rpass() {
    vk_cmd_end_render_pass(vk_buff);
}

inline void vku_cmdbuff_t::end() {
    VK_ASSERT(vk_end_command_buffer(vk_buff));
}

inline void vku_cmdbuff_t::reset() {
    VK_ASSERT(vk_reset_command_buffer(vk_buff, 0));
}

inline vku_sem_t::vku_sem_t(vku_device_t *dev) : dev(dev) {
    vk_semaphore_create_info_t sem_info {};

    VK_ASSERT(vk_create_semaphore(dev->vk_dev, &sem_info, NULL, &vk_sem));
    dev->add_child(this);
}

inline vku_sem_t::~vku_sem_t() {
    cleanup();
    dev->rm_child(this);
    vk_destroy_semaphore(dev->vk_dev, vk_sem, NULL);
}

inline vku_fence_t::vku_fence_t(vku_device_t *dev, VkFenceCreateFlags flags) : dev(dev) {
    vk_fence_create_info_t fence_info { .flags = flags };

    VK_ASSERT(vk_create_fence(dev->vk_dev, &fence_info, NULL, &vk_fence));
    dev->add_child(this);
}
inline vku_fence_t::~vku_fence_t() {
    cleanup();
    dev->rm_child(this);
    vk_destroy_fence(dev->vk_dev, vk_fence, NULL);
}

inline vku_buffer_t::vku_buffer_t(vku_device_t *dev,
        size_t size, vk_buffer_usage_flags_t usage, vk_sharing_mode_t sh_mode,
        vk_memory_property_flags_t mem_flags)
 : dev(dev), size(size)
 {
    vk_buffer_create_info_t buff_info{
        .size = size,
        .usage = usage,
        .sharing_mode = sh_mode
    };

    VK_ASSERT(vk_create_buffer(dev->vk_dev, &buff_info, nullptr, &vk_buff));

    vk_memory_requirements_t mem_req;
    vk_get_buffer_memory_requirements(dev->vk_dev, vk_buff, &mem_req);

    vk_memory_allocate_info_t alloc_info{
        .allocation_size = mem_req.size,
        .memory_type_index = vku_find_memory_type(dev, mem_req.memory_type_bits, mem_flags)
    };

    VK_ASSERT(vk_allocate_memory(dev->vk_dev, &alloc_info, nullptr, &vk_mem));
    VK_ASSERT(vk_bind_buffer_memory(dev->vk_dev, vk_buff, vk_mem, 0));

    dev->add_child(this);
}

inline void *vku_buffer_t::map_data(vk_device_size_t offset, vk_device_size_t size) {
    if (map_ptr) {
        DBG("Memory is already mapped!");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    VK_ASSERT(vkMapMemory(dev->vk_dev, vk_mem, offset, size, 0, &map_ptr));
    return map_ptr;
}

inline void vku_buffer_t::unmap_data() {
    if (!map_ptr) {
        DBG("Memory is not mapped, can't unmap");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vkUnmapMemory(dev->vk_dev, vk_mem);
    map_ptr = nullptr;
}

inline vku_buffer_t::~vku_buffer_t() {
    cleanup();
    dev->rm_child(this);
    if (map_ptr)
        unmap_data();
    vk_destroy_buffer(dev->vk_dev, vk_buff, nullptr);
    vk_free_memory(dev->vk_dev, vk_mem, nullptr);
}

inline vku_desc_pool_t::vku_desc_pool_t(vku_device_t *dev, vk_descriptor_type_t type, uint32_t cnt)
: dev(dev)
{
    vk_descriptor_pool_size_t pool_size{
        .type = type,
        .descriptor_count = cnt,
    };

    vk_descriptor_pool_create_info_t pool_info{
        .max_sets = cnt,
        .pool_size_count = 1,
        .p_pool_sizes = &pool_size,
    };

    VK_ASSERT(vk_create_descriptor_pool(dev->vk_dev, &pool_info, nullptr, &vk_descpool));
    dev->add_child(this);
}

inline vku_desc_pool_t::~vku_desc_pool_t() {
    cleanup();
    dev->rm_child(this);
    vk_destroy_descriptor_pool(dev->vk_dev, vk_descpool, nullptr);
}

inline vku_desc_set_t::vku_desc_set_t(vku_desc_pool_t *dp, vk_descriptor_set_layout_t layout,
            vku_buffer_t *buff, uint32_t binding, vk_descriptor_type_t type)
: dp(dp)
{
    vk_descriptor_set_allocate_info_t alloc_info {
        .descriptor_pool = dp->vk_descpool,
        .descriptor_set_count = 1,
        .p_set_layouts = &layout,
    };

    VK_ASSERT(vk_allocate_descriptor_sets(dp->dev->vk_dev, &alloc_info, &vk_desc_set));

    /* TODO: this sucks, it references the buffer, but doesn't have a mechanism to do something
    if the buffer is freed without it's knowledge. So the buffer and descriptor set must
    match in size, but the buffer doesn't know that, that's not ok. */
    vk_descriptor_buffer_info_t desc_buff_info {
        .buffer = buff->vk_buff,
        .offset = 0,
        .range = buff->size
    };

    vk_write_descriptor_set_t desc_write{
        .dst_set = vk_desc_set,
        .dst_binding = binding,
        .dst_array_element = 0,
        .descriptor_count = 1,
        .descriptor_type = type,
        .p_image_info = nullptr,
        .p_buffer_info = &desc_buff_info,
        .p_texel_buffer_view = nullptr,
    };

    vk_update_descriptor_sets(dp->dev->vk_dev, 1, &desc_write, 0, nullptr);

    dp->add_child(this);
}

inline vku_desc_set_t::~vku_desc_set_t() {
    cleanup();
    dp->rm_child(this);
}


inline void vku_wait_fences(std::vector<vku_fence_t *> fences) {
    std::vector<vk_fence_t> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->vk_fence);

    VK_ASSERT(vk_wait_for_fences(fences[0]->dev->vk_dev, vk_fences.size(), vk_fences.data(),
            VK_TRUE, UINT64_MAX));
}

inline void vku_reset_fences(std::vector<vku_fence_t *> fences) {
    std::vector<vk_fence_t> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->vk_fence);
    VK_ASSERT(vk_reset_fences(fences[0]->dev->vk_dev, vk_fences.size(), vk_fences.data()));
}

inline void vku_aquire_next_img(vku_swapchain_t *swc, vku_sem_t *sem, uint32_t *img_idx) {
    VK_ASSERT(vk_acquire_next_image_khr(swc->dev->vk_dev, swc->vk_swapchain, UINT64_MAX,
            sem->vk_sem, VK_NULL_HANDLE, img_idx));
}

inline void vku_submit_cmdbuff(
        std::vector<std::pair<vku_sem_t *, VkPipelineStageFlags>> wait_sems,
        vku_cmdbuff_t *cbuff,
        vku_fence_t *fence,
        std::vector<vku_sem_t *> sig_sems)
{
    std::vector<VkPipelineStageFlags> vk_wait_stages;
    std::vector<vk_semaphore_t> vk_wait_sems;
    std::vector<vk_semaphore_t> vk_sig_sems;

    for (auto [s, wait_stage] : wait_sems) {
        vk_wait_stages.push_back(wait_stage);
        vk_wait_sems.push_back(s->vk_sem);
    }
    for (auto s : sig_sems) {
        vk_sig_sems.push_back(s->vk_sem);
    }
    vk_submit_info_t submit_info {
        .wait_semaphore_count = (uint32_t)vk_wait_sems.size(),
        .p_wait_semaphores = vk_wait_sems.size() == 0 ? nullptr : vk_wait_sems.data(),
        .p_wait_dst_stage_mask = vk_wait_sems.size() == 0 ? nullptr : vk_wait_stages.data(),
        .command_buffer_count = 1,
        .p_command_buffers = &cbuff->vk_buff,
        .signal_semaphore_count = (uint32_t)vk_sig_sems.size(),
        .p_signal_semaphores = vk_sig_sems.size() == 0 ? nullptr : vk_sig_sems.data(),
    };

    VK_ASSERT(vk_queue_submit(cbuff->cp->dev->vk_graphics_que, 1, &submit_info,
            fence == nullptr ? nullptr : fence->vk_fence));
}

inline void vku_present(
        vku_swapchain_t *swc,
        std::vector<vku_sem_t *> wait_sems,
        uint32_t img_idx)
{
    std::vector<vk_semaphore_t> vk_wait_sems;

    for (auto s : wait_sems) {
        vk_wait_sems.push_back(s->vk_sem);
    }

    vk_present_info_khr_t pres_info {
        .wait_semaphore_count = (uint32_t)vk_wait_sems.size(),
        .p_wait_semaphores = vk_wait_sems.data(),
        .swapchain_count = 1,
        .p_swapchains = &swc->vk_swapchain,
        .p_image_indices = &img_idx,
        .p_results = NULL,
    };

    VK_ASSERT(vk_queue_present_khr(swc->dev->vk_present_que, &pres_info));
}

inline void vku_copy_buff(vku_cmdpool_t *cp, vku_buffer_t *dst, vku_buffer_t *src,
        vk_device_size_t sz)
{
    auto cbuff = std::make_unique<vku_cmdbuff_t>(cp);
    auto fence = std::make_unique<vku_fence_t>(cp->dev);

    cbuff->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vk_buffer_copy_t copy_region{
        .src_offset = 0,
        .dst_offset = 0,
        .size = sz
    };
    vk_cmd_copy_buffer(cbuff->vk_buff, src->vk_buff, dst->vk_buff, 1, &copy_region);
    cbuff->end();


    vku_submit_cmdbuff({}, cbuff.get(), fence.get(), {});
    vku_wait_fences({fence.get()});
}

inline std::string vku_glfw_err() {
    const char *errstr = NULL;
    int err = glfwGetError(&errstr);
    return sformat("[%s:%d]", errstr, err);
}

inline vk_result_t vku_create_dbg_messenger(
        vk_instance_t instance,
        const vk_debug_utils_messenger_create_info_ext_t* dbg_info,
        const vk_allocation_callbacks_t* alloc,
        vk_debug_utils_messenger_ext_t* dbg_msg)
{
    auto func = (pfn_vk_create_debug_utils_messenger_ext_fn_t) vk_get_instance_proc_addr(
            instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance,
                (const VkDebugUtilsMessengerCreateInfoEXT *)dbg_info,
                (const VkAllocationCallbacks *)alloc, dbg_msg);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void vku_destroy_dbg_messenger(
        vk_instance_t instance,
        vk_debug_utils_messenger_ext_t dbg_msg,
        const vk_allocation_callbacks_t* alloc)
{
    auto func = (pfn_vk_destroy_debug_utils_messenger_ext_fn_t) vk_get_instance_proc_addr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, dbg_msg, (const VkAllocationCallbacks *)alloc);
    }
}

static VKAPI_ATTR vk_bool32_t VKAPI_CALL vku_dbg_cbk(
        vk_debug_utils_message_severity_flag_bits_ext_t     severity,
        vk_debug_utils_message_type_flags_ext_t             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* _data,
        void* ctx)
{
    auto data = (vk_debug_utils_messenger_callback_data_ext_t *)_data;

    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        DBG("[VK_DBG]: %s", data->p_message);
    }
    return VK_FALSE;
}

inline vku_swapchain_details_t vku_get_swapchain_details(vk_physical_device_t dev,
        vk_surface_khr_t surf)
{
    vku_swapchain_details_t ret = {};
    uint32_t format_cnt = 0;
    uint32_t present_modes_cnt = 0;

    VK_ASSERT(vk_get_physical_device_surface_capabilities_khr(dev, surf, &ret.capab));
    VK_ASSERT(vk_get_physical_device_surface_formats_khr(dev, surf, &format_cnt, NULL));

    if (format_cnt) {
        ret.formats.resize(format_cnt);
        VK_ASSERT(vk_get_physical_device_surface_formats_khr(
                dev, surf, &format_cnt, ret.formats.data()));
    }

    VK_ASSERT(vk_get_physical_device_surface_present_modes_khr(dev, surf, &present_modes_cnt, NULL));
    if (present_modes_cnt) {
        ret.present_modes.resize(present_modes_cnt);
        VK_ASSERT(vk_get_physical_device_surface_present_modes_khr(dev, surf, &present_modes_cnt,
                ret.present_modes.data()));
    }

    return ret;
}

inline vku_gpu_family_ids_t vku_find_queue_families(vk_physical_device_t dev,
        vk_surface_khr_t surface)
{
    vku_gpu_family_ids_t ret;

    uint32_t cnt = 0;
    vk_get_physical_device_queue_family_properties(dev, &cnt, NULL);
    std::vector<vk_queue_family_properties_t> queue_families(cnt);
    vk_get_physical_device_queue_family_properties(dev, &cnt, queue_families.data());

    for (int i = 0; auto qf : queue_families) {
        if ((qf.queue_flags & VK_QUEUE_GRAPHICS_BIT) && (qf.queue_flags & VK_QUEUE_COMPUTE_BIT))
            ret.graphics_id = i;
        vk_bool32_t res = 0;
        vk_get_physical_device_surface_support_khr(dev, i, surface, &res);
        if (res)
            ret.present_id = i;
        i++;
    }
    return ret;
}

inline vk_extent2d_t vku_choose_extent(GLFWwindow *window, vk_surface_capabilities_khr_t capab) {
    if (capab.current_extent.width != std::numeric_limits<uint32_t>::max())
        return capab.current_extent;

    int width, height;
    glfw_get_framebuffer_size(window, &width, &height);

    vk_extent2d_t ret = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };

    ret.width  = std::clamp(ret.width,  capab.min_image_extent.width,  capab.max_image_extent.width);
    ret.height = std::clamp(ret.height, capab.min_image_extent.height, capab.min_image_extent.height);

    return ret;
}

inline int vku_score_phydev(vk_physical_device_t dev, vk_surface_khr_t surf) {
    int score = 0;

    vk_physical_device_properties_t dev_prop;
    vk_physical_device_features_t dev_feat;
    vk_get_physical_device_properties(dev, &dev_prop);
    vk_get_physical_device_features(dev, &dev_feat);

    DBG("GPU Candidate Name: %s", dev_prop.device_name);

    if (dev_prop.device_type == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 10;
    if (dev_feat.geometry_shader)
        score += 10;

    if (!def_feat.sampler_anisotropy) {
        DBG("sampler_anisotropy must be supported, but it is not supported by this GPU");
        return -1;
    }

    std::set<std::string> required_ext = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    uint32_t ext_cnt;
    VK_ASSERT(vk_enumerate_device_extension_properties(dev, NULL, &ext_cnt, NULL));

    std::vector<vk_extension_properties_t> avail_ext(ext_cnt);
    VK_ASSERT(vk_enumerate_device_extension_properties(dev, NULL, &ext_cnt, avail_ext.data()));

    for (auto ext : avail_ext)
        required_ext.erase(ext.extension_name);

    if (!required_ext.empty()) {
        DBG("Required extensions not supported");
        return -1;
    }

    auto fams = vku_find_queue_families(dev, surf);
    if (fams.graphics_id < 0 || fams.present_id < 0) {
        DBG("No queue family support");
        return -1;
    }

    vku_swapchain_details_t details = vku_get_swapchain_details(dev, surf);
    if (details.formats.empty() || details.present_modes.empty()) {
        DBG("Swapchain support is not adequate");
        return -1;
    }

    /* TODO: add logging functions for our current support */

    return score;
}

inline vk_shader_stage_flag_bits_t vku_get_shader_type(vku_shader_stage_t own_type) {
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


#include <fcntl.h>
#include <unistd.h>

inline void vku_spirv_init() {
    glslang::InitializeProcess();
    vku_spirv_resources.maxLights = 32;
    vku_spirv_resources.maxClipPlanes = 6;
    vku_spirv_resources.maxTextureUnits = 32;
    vku_spirv_resources.maxTextureCoords = 32;
    vku_spirv_resources.maxVertexAttribs = 64;
    vku_spirv_resources.maxVertexUniformComponents = 4096;
    vku_spirv_resources.maxVaryingFloats = 64;
    vku_spirv_resources.maxVertexTextureImageUnits = 32;
    vku_spirv_resources.maxCombinedTextureImageUnits = 80;
    vku_spirv_resources.maxTextureImageUnits = 32;
    vku_spirv_resources.maxFragmentUniformComponents = 4096;
    vku_spirv_resources.maxDrawBuffers = 32;
    vku_spirv_resources.maxVertexUniformVectors = 128;
    vku_spirv_resources.maxVaryingVectors = 8;
    vku_spirv_resources.maxFragmentUniformVectors = 16;
    vku_spirv_resources.maxVertexOutputVectors = 16;
    vku_spirv_resources.maxFragmentInputVectors = 15;
    vku_spirv_resources.minProgramTexelOffset = -8;
    vku_spirv_resources.maxProgramTexelOffset = 7;
    vku_spirv_resources.maxClipDistances = 8;
    vku_spirv_resources.maxComputeWorkGroupCountX = 65535;
    vku_spirv_resources.maxComputeWorkGroupCountY = 65535;
    vku_spirv_resources.maxComputeWorkGroupCountZ = 65535;
    vku_spirv_resources.maxComputeWorkGroupSizeX = 1024;
    vku_spirv_resources.maxComputeWorkGroupSizeY = 1024;
    vku_spirv_resources.maxComputeWorkGroupSizeZ = 64;
    vku_spirv_resources.maxComputeUniformComponents = 1024;
    vku_spirv_resources.maxComputeTextureImageUnits = 16;
    vku_spirv_resources.maxComputeImageUniforms = 8;
    vku_spirv_resources.maxComputeAtomicCounters = 8;
    vku_spirv_resources.maxComputeAtomicCounterBuffers = 1;
    vku_spirv_resources.maxVaryingComponents = 60;
    vku_spirv_resources.maxVertexOutputComponents = 64;
    vku_spirv_resources.maxGeometryInputComponents = 64;
    vku_spirv_resources.maxGeometryOutputComponents = 128;
    vku_spirv_resources.maxFragmentInputComponents = 128;
    vku_spirv_resources.maxImageUnits = 8;
    vku_spirv_resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
    vku_spirv_resources.maxCombinedShaderOutputResources = 8;
    vku_spirv_resources.maxImageSamples = 0;
    vku_spirv_resources.maxVertexImageUniforms = 0;
    vku_spirv_resources.maxTessControlImageUniforms = 0;
    vku_spirv_resources.maxTessEvaluationImageUniforms = 0;
    vku_spirv_resources.maxGeometryImageUniforms = 0;
    vku_spirv_resources.maxFragmentImageUniforms = 8;
    vku_spirv_resources.maxCombinedImageUniforms = 8;
    vku_spirv_resources.maxGeometryTextureImageUnits = 16;
    vku_spirv_resources.maxGeometryOutputVertices = 256;
    vku_spirv_resources.maxGeometryTotalOutputComponents = 1024;
    vku_spirv_resources.maxGeometryUniformComponents = 1024;
    vku_spirv_resources.maxGeometryVaryingComponents = 64;
    vku_spirv_resources.maxTessControlInputComponents = 128;
    vku_spirv_resources.maxTessControlOutputComponents = 128;
    vku_spirv_resources.maxTessControlTextureImageUnits = 16;
    vku_spirv_resources.maxTessControlUniformComponents = 1024;
    vku_spirv_resources.maxTessControlTotalOutputComponents = 4096;
    vku_spirv_resources.maxTessEvaluationInputComponents = 128;
    vku_spirv_resources.maxTessEvaluationOutputComponents = 128;
    vku_spirv_resources.maxTessEvaluationTextureImageUnits = 16;
    vku_spirv_resources.maxTessEvaluationUniformComponents = 1024;
    vku_spirv_resources.maxTessPatchComponents = 120;
    vku_spirv_resources.maxPatchVertices = 32;
    vku_spirv_resources.maxTessGenLevel = 64;
    vku_spirv_resources.maxViewports = 16;
    vku_spirv_resources.maxVertexAtomicCounters = 0;
    vku_spirv_resources.maxTessControlAtomicCounters = 0;
    vku_spirv_resources.maxTessEvaluationAtomicCounters = 0;
    vku_spirv_resources.maxGeometryAtomicCounters = 0;
    vku_spirv_resources.maxFragmentAtomicCounters = 8;
    vku_spirv_resources.maxCombinedAtomicCounters = 8;
    vku_spirv_resources.maxAtomicCounterBindings = 1;
    vku_spirv_resources.maxVertexAtomicCounterBuffers = 0;
    vku_spirv_resources.maxTessControlAtomicCounterBuffers = 0;
    vku_spirv_resources.maxTessEvaluationAtomicCounterBuffers = 0;
    vku_spirv_resources.maxGeometryAtomicCounterBuffers = 0;
    vku_spirv_resources.maxFragmentAtomicCounterBuffers = 1;
    vku_spirv_resources.maxCombinedAtomicCounterBuffers = 1;
    vku_spirv_resources.maxAtomicCounterBufferSize = 16384;
    vku_spirv_resources.maxTransformFeedbackBuffers = 4;
    vku_spirv_resources.maxTransformFeedbackInterleavedComponents = 64;
    vku_spirv_resources.maxCullDistances = 8;
    vku_spirv_resources.maxCombinedClipAndCullDistances = 8;
    vku_spirv_resources.maxSamples = 4;
    vku_spirv_resources.maxMeshOutputVerticesNV = 256;
    vku_spirv_resources.maxMeshOutputPrimitivesNV = 512;
    vku_spirv_resources.maxMeshWorkGroupSizeX_NV = 32;
    vku_spirv_resources.maxMeshWorkGroupSizeY_NV = 1;
    vku_spirv_resources.maxMeshWorkGroupSizeZ_NV = 1;
    vku_spirv_resources.maxTaskWorkGroupSizeX_NV = 32;
    vku_spirv_resources.maxTaskWorkGroupSizeY_NV = 1;
    vku_spirv_resources.maxTaskWorkGroupSizeZ_NV = 1;
    vku_spirv_resources.maxMeshViewCountNV = 4;
    vku_spirv_resources.limits.nonInductiveForLoops = 1;
    vku_spirv_resources.limits.whileLoops = 1;
    vku_spirv_resources.limits.doWhileLoops = 1;
    vku_spirv_resources.limits.generalUniformIndexing = 1;
    vku_spirv_resources.limits.generalAttributeMatrixVectorIndexing = 1;
    vku_spirv_resources.limits.generalVaryingIndexing = 1;
    vku_spirv_resources.limits.generalSamplerIndexing = 1;
    vku_spirv_resources.limits.generalVariableIndexing = 1;
    vku_spirv_resources.limits.generalConstantMatrixVectorIndexing = 1;
}

inline void vku_spirv_uninit() {
    glslang::FinalizeProcess();
}

inline vku_spirv_t vku_spirv_compile(vku_instance_t *inst, vku_shader_stage_t vku_stage,
        const char *code)
{
    EShLanguage stage;
    switch (vku_stage) {
        case VKU_SPIRV_VERTEX:    stage = EShLangVertex;         break;
        case VKU_SPIRV_TESS_CTRL: stage = EShLangTessControl;    break;
        case VKU_SPIRV_TESS_EVAL: stage = EShLangTessEvaluation; break;
        case VKU_SPIRV_GEOMETRY:  stage = EShLangGeometry;       break;
        case VKU_SPIRV_FRAGMENT:  stage = EShLangFragment;       break;
        case VKU_SPIRV_COMPUTE:   stage = EShLangCompute;        break;
        default:
            DBG("Unknown shader stage type: %d", (uint32_t)vku_stage);
            throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    glslang::TShader shader(stage);
    glslang::TProgram program;
    const char *shader_strings[] = { code };

    // Enable SPIR-V and Vulkan rules when parsing GLSL
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

    shader.setStrings(shader_strings, 1);
    if (!shader.parse(&vku_spirv_resources, 100, false, messages)) {
        DBG("Parse Failed(Log): [%s]", shader.getInfoLog());
        DBG("Parse Failed(Dbg): [%s]", shader.getInfoDebugLog());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    program.addShader(&shader);
    if (!program.link(messages)) {
        DBG("Link Failed(Log): [%s]", shader.getInfoLog());
        DBG("Link Failed(Dbg): [%s]", shader.getInfoDebugLog());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    vku_spirv_t ret;
    glslang::GlslangToSpv(*program.getIntermediate(stage), ret.content);
    ret.type = vku_stage;
    return ret;
}

inline uint32_t vku_find_memory_type(vku_device_t *dev,
        uint32_t type_filter, vk_memory_property_flags_t properties)
{
    vk_physical_device_memory_properties_t mem_props;
    vk_get_physical_device_memory_properties(dev->vk_phy_dev, &mem_props);

    for (uint32_t i = 0; i < mem_props.memory_type_count; i++) {
        if (type_filter & (1 << i) && 
                (mem_props.memory_types[i].property_flags & properties) == properties)
        {
            return i;
        }
    }

    DBG("Couldn't find suitable memory type");
    throw vku_err_t(VK_ERROR_UNKNOWN);
}

inline const char *vk_err_str(vk_result_t res) {
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

#endif