#ifndef VULKAN_UTILS_H
#define VULKAN_UTILS_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLFW_INCLUDE_NONE

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "debug.h"

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

/* TODO: Check if throw may be better transformed in a return. */
#define VK_ASSERT(fn_call)                                                                         \
do {                                                                                               \
    VkResult vk_err = (fn_call);                                                                \
    if (vk_err != VK_SUCCESS) {                                                                    \
        DBG("Failed vk assert: [%s: %d]", vk_err_str(vk_err), vk_err);                             \
        throw vku_err_t(vk_err);                                                                   \
    }                                                                                              \
} while (false);

namespace vku_utils {

/* TODO:
    - Add logs for all the creations/deletions of objects with type and id(ptr)
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

struct vku_err_t;
struct vku_gpu_family_ids_t;
struct vku_spirv_t;

struct vku_vertex_input_desc_t;
struct vku_vertex_p2n0c3t2_t;
struct vku_vertex_p3n3c3t2_t;

struct vku_binding_desc_t;
struct vku_mvp_t;
struct vku_ubo_t;
struct vku_ssbo_t;

template <typename VkuT>
struct vku_ref_t;

/*! The user holds those references. Not directly the objects bellow. */
template <typename VkuT>
using vku_ref_p = std::shared_ptr<vku_ref_t<VkuT>>;

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
struct vku_image_t;         /* uses (device) */
struct vku_img_view_t;      /* uses (imag) */
struct vku_img_sampl_t;     /* uses (device) */
struct vku_desc_pool_t;     /* uses (device, ?buff?, ?pipeline?) */
struct vku_desc_set_t;      /* uses (desc_pool) */

inline VkResult vku_init();
inline VkResult vku_uninit();

inline void vku_wait_fences(std::vector<vku_ref_p<vku_fence_t>> fences);
inline void vku_reset_fences(std::vector<vku_ref_p<vku_fence_t>> fences);
inline void vku_aquire_next_img(
        vku_ref_p<vku_swapchain_t> swc,
        vku_ref_p<vku_sem_t> sem,
        uint32_t *img_idx);

inline void vku_submit_cmdbuff(
        std::vector<std::pair<vku_ref_p<vku_sem_t>, VkPipelineStageFlags>> wait_sems,
        vku_ref_p<vku_cmdbuff_t> cbuff,
        vku_ref_p<vku_fence_t> fence,
        std::vector<vku_ref_p<vku_sem_t>> sig_sems);

inline void vku_present(
        vku_ref_p<vku_swapchain_t> swc,
        std::vector<vku_ref_p<vku_sem_t>> wait_sems,
        uint32_t img_idx);

/* if no command buffer is provided, one will be allocated from the command pool */
inline void vku_copy_buff(
        vku_ref_p<vku_cmdpool_t> cp,
        vku_ref_p<vku_buffer_t> dst,
        vku_ref_p<vku_buffer_t> src,
        VkDeviceSize sz,
        vku_ref_p<vku_cmdbuff_t> cbuff = nullptr);

/* VKU Objects:
================================================================================================= */

/* Those are needed here just for bellow objects */
inline const char *vk_err_str(VkResult res);
inline std::string vku_glfw_err();

/*! Callbacks used by vku_object_t for re-initializing the managed vulkan obeject */
struct vku_object_cbks_t {
    std::shared_ptr<void> usr_ptr;

    /*! This function is called when an vku_ref_p calls the vku_object_t::init function, just before
     * init is called, with the object and usr_ptr as arguments. This call is made if
     * vku_object_t::cbks is not null and if vku_object_cbks_t::pre_init is also not null. */
    std::function<void(vku_object_t *, std::shared_ptr<void> &)> pre_init;

    /*! Same as above, called after init. */
    std::function<void(vku_object_t *, std::shared_ptr<void> &)> post_init;

    /*! Same as for init, but this time for the uninit function */
    std::function<void(vku_object_t *, std::shared_ptr<void> &)> pre_uninit;

    /*! Same as above, called after uninit. */
    std::function<void(vku_object_t *, std::shared_ptr<void> &)> post_uninit;
};

/*! This is virtual only for init/uninit, which need to describe how the object should be
 * initialized once it is created and it's parameters are filled */
struct vku_object_t {
    virtual ~vku_object_t() { _call_uninit(); }

    template <typename VkuT>
    friend struct vku_ref_t;

    VkResult _call_init() {
        if (cbks && cbks->pre_init)
            cbks->pre_init(this, cbks->usr_ptr);
        auto ret = _init();
        if (cbks && cbks->post_init)
            cbks->post_init(this, cbks->usr_ptr);
        return ret;
    }

    VkResult _call_uninit() {
        if (cbks && cbks->pre_uninit)
            cbks->pre_uninit(this, cbks->usr_ptr);
        auto ret = _uninit();
        if (cbks && cbks->post_uninit)
            cbks->post_uninit(this, cbks->usr_ptr);
        return ret;
    }

private:
    virtual VkResult _init() = 0;
    virtual VkResult _uninit() { return VK_SUCCESS; };

    std::shared_ptr<vku_object_cbks_t> cbks;
};

/*!
 * The idea:
 * - No object is directly referenced, but they all are referenced by this reference. What this does
 * is it enables us to keep an internal representation of the vulkan data structures while also
 * letting us rebuild the internal object when needed. The vulkan structures will be rebuilt using
 * the last parameters that where used to build them.
 */
/* TODO: Think if it makes sense to implement a locking mechanism, especially for the rebuild stuff.

vku_ref_p practically implements a DAG of dependencies, this means that to protect a node, all it's
dependees must be also locked. An observation is that even if a dependee can pe added to a
dependency, no dependency from the target node to the leaf nodes can ever be removed before removing
the target node. As such, the ideea is to implement a spinlock for an address variable, such that
the node is protected by a spinlock and a mutex at the same time.

Let's take an example of a graph (nodes in the right, depend on those on the left, in the example,
D depends on A, B and C):

A     E              L
 \   / \            /
  \ /   \          /
B--D     G--------H---N
  / \   /        / \
 /   \ /        /   \
C     F        /     M
     / \      /
    /   \    J
   I     K    \
               \
                O

So in this example, If we want to do a modification to G, we must lock away G, H, M, N, L. If we
want to modify J, we need to lock J, H, M, N, L, O. But the only known fact is that G is a
dependency of H, We don't know that J is a lso a dependency of H, and we must stop H from...

Need to figure this out if I want to implement it, do I really need to remember depends, besides
dependees? This is kinda annoying. So I will need to also hold _depends, and figure out a way to
manage them, somehow. This also does another bad thing, stores twice as many pointers as I really
need, bacause depends are not going to go anywhere, so bad... Maybe I can keep them as raw pointers,
since I already hold them once as shared pointers?

struct node : public std::enable_shared_from_this<vku_ref_base_t> {
    std::vector<std::weak_ptr<node>>  _dependees;
    // std::vector<node *>  _depends; //  <- maybe like this? And maybe only if locking is enabled?
    Data data;
};

*/
struct vku_ref_base_t : public std::enable_shared_from_this<vku_ref_base_t> {
protected:
    /*! This is here to force the creation of references by create_obj, this makes sure */
    struct private_param_t { explicit private_param_t() = default; };

    std::unique_ptr<vku_object_t>               _obj;
    std::vector<std::weak_ptr<vku_ref_base_t>>  _dependees;

public:
    template <typename VkuT>
    vku_ref_base_t(private_param_t, std::unique_ptr<VkuT> obj) : _obj(std::move(obj)) {}

    template <typename VkuT>
    friend struct vku_ref_t;

    virtual ~vku_ref_base_t() {}
    virtual void rebuild() = 0;
    virtual void clean_deps() = 0;
    virtual void init_all() = 0;
    virtual void uninit_all() = 0;
};

/*! This holds a reference to an instance of VkuT, instance that is initiated and held by this
 * library. All objects and the user will use the instance via this reference. This is implemented
 * here because it has a small footprint and I consider making it visible would made the library
 * easier to use. */
template <typename VkuT>
class vku_ref_t : public vku_ref_base_t {
protected:
    void uninit_all() override {
        clean_deps(); /* we lazy clear the deps whenever we want to iterate over them */
        for (auto wd : _dependees)
            wd.lock()->uninit_all();
        VK_ASSERT(_obj->_call_uninit());
    }

    void init_all() override {
        VK_ASSERT(_obj->_call_init());
        for (auto wd : _dependees)
            wd.lock()->init_all();
    }


public:
    vku_ref_t(private_param_t t, std::unique_ptr<VkuT> obj)
    : vku_ref_base_t(t, std::move(obj)) {}

    VkuT *get() {
        if (!_obj) {
            DBG("Invalid held object");
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
        return static_cast<VkuT *>(_obj.get());
    }

    template <typename VkuB> requires std::derived_from<VkuT, VkuB>
    std::shared_ptr<vku_ref_t<VkuB>> to_parent() {
        return std::shared_ptr<vku_ref_t<VkuB>>(
                shared_from_this(), reinterpret_cast<vku_ref_t<VkuB> *>(this));
    }

    void clean_deps() override {
        _dependees.erase(std::remove_if(_dependees.begin(), _dependees.end(), [](auto wp){
                return wp.expired(); }), _dependees.end());
    }

    void rebuild() override {
        /* TODO: maybe thread-protect this somehow? */
        uninit_all();
        init_all();
    }

    static std::shared_ptr<vku_ref_t<VkuT>> create_obj_ref(std::unique_ptr<VkuT> obj,
            std::vector<std::shared_ptr<vku_ref_base_t>> dependencies)
    {
        auto ret = std::make_shared<vku_ref_t<VkuT>>(private_param_t{}, std::move(obj));
        for (auto &d : dependencies)
            d->_dependees.push_back(ret);
        return ret;
    }
};

struct vku_err_t : public std::exception {
    VkResult vk_err{};
    std::string err_str;

    vku_err_t(VkResult vk_err)
    : vk_err(vk_err), err_str(std::format("VKU_ERROR: {}[{}]", vk_err_str(vk_err), (size_t)vk_err)) {}

    vku_err_t(const std::string& str) : err_str(str) {}
    const char *what() const noexcept override { return err_str.c_str(); };
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
    VkVertexInputBindingDescription                 bind_desc;
    std::vector<VkVertexInputAttributeDescription>  attr_desc;
};

struct vku_vertex_p2n0c3t2_t {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 tex;

    static vku_vertex_input_desc_t get_input_desc();
};

struct vku_vertex_p3n3c3t2_t {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 tex;

    static vku_vertex_input_desc_t get_input_desc();
};

struct vku_mvp_t {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/* Uniform Buffer Object */
struct vku_ubo_t {
    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding,
            VkShaderStageFlags stage);
};

/* Shader Storage Buffer Object */
struct vku_ssbo_t {
    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding,
            VkShaderStageFlags stage);
};

using vku_vertex2d_t = vku_vertex_p2n0c3t2_t;
using vku_vertex3d_t = vku_vertex_p3n3c3t2_t;

struct vku_window_t : public vku_object_t {
    /* Those can be modified at any time, but they need a rebuild to actually take effect (see
    vku_ref_t::rebuild()) */
    std::string window_name;
    int width;
    int height;

    static vku_ref_p<vku_window_t> create(int width = 800, int height = 600,
            std::string name = "vku_window_name_placeholder");

    GLFWwindow *get_window() const { return _window; }

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;

    GLFWwindow *_window = NULL;
};

struct vku_instance_t : public vku_object_t {
    VkInstance                  vk_instance; /* TODO: figure out if those need getter */
    VkDebugUtilsMessengerEXT    vk_dbg_messenger;

    std::string app_name;
    std::string engine_name;
    std::vector<std::string> extensions;
    std::vector<std::string> layers;

    static vku_ref_p<vku_instance_t> create(
            const std::string app_name = "vku_app_name_placeholder",
            const std::string engine_name = "vku_engine_name_placeholder",
            const std::vector<std::string>& extensions = { "VK_EXT_debug_utils" },
            const std::vector<std::string>& layers = { "VK_LAYER_KHRONOS_validation" });

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* VkSurfaceKHR */
struct vku_surface_t : public vku_object_t {
    VkSurfaceKHR                vk_surface = NULL;

    vku_ref_p<vku_window_t>     window;
    vku_ref_p<vku_instance_t>   inst;

    static vku_ref_p<vku_surface_t> create(
            vku_ref_p<vku_window_t> window,
            vku_ref_p<vku_instance_t> inst);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_device_t : public vku_object_t {
    VkPhysicalDevice            vk_phy_dev;
    VkDevice                    vk_dev;
    VkQueue                     vk_graphics_que;
    VkQueue                     vk_present_que;
    std::set<int>               que_ids;
    vku_gpu_family_ids_t        que_fams;

    vku_ref_p<vku_surface_t>    surf;

    static vku_ref_p<vku_device_t> create(
            vku_ref_p<vku_surface_t> surf);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* VkSwapchainKHR */
struct vku_swapchain_t : public vku_object_t {
    VkSurfaceFormatKHR              vk_surf_fmt;
    VkPresentModeKHR                vk_present_mode;
    VkExtent2D                      vk_extent;
    VkSwapchainKHR                  vk_swapchain;
    std::vector<VkImage>            vk_sc_images;
    std::vector<VkImageView>        vk_sc_image_views;
    vku_ref_p<vku_image_t>          depth_imag;
    vku_ref_p<vku_img_view_t>       depth_view;

    vku_ref_p<vku_device_t>         dev;

    static vku_ref_p<vku_swapchain_t> create(
        vku_ref_p<vku_device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_shader_t : public vku_object_t {
    VkShaderModule          vk_shader;

    bool                    init_from_path; /* implicit param */
    vku_ref_p<vku_device_t> dev;
    std::string             path = "not-initialized-from-path";
    vku_spirv_t             spirv;
    vku_shader_stage_t      type;

    /* not init from path */
    static vku_ref_p<vku_shader_t> create(vku_ref_p<vku_device_t> dev, const vku_spirv_t& spirv);

    /* init from path */
    /* Obs: loads shader in binary format, i.e. already compiled */
    static vku_ref_p<vku_shader_t> create(vku_ref_p<vku_device_t> dev, const char *path,
            vku_shader_stage_t type);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_renderpass_t : public vku_object_t {
    VkRenderPass                vk_render_pass;

    vku_ref_p<vku_swapchain_t>  swc;

    static vku_ref_p<vku_renderpass_t> create(vku_ref_p<vku_swapchain_t> swc);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_pipeline_t : public vku_object_t {
    VkPipeline                              vk_pipeline;
    VkPipelineLayout                        vk_layout;
    VkDescriptorSetLayout                   vk_desc_set_layout;

    int                                     width;
    int                                     height;
    vku_ref_p<vku_renderpass_t>             rp;
    std::vector<vku_ref_p<vku_shader_t>>    shaders;
    VkPrimitiveTopology                     topology;
    vku_vertex_input_desc_t                 vid;
    vku_ref_p<vku_binding_desc_t>           bd;

    static vku_ref_p<vku_pipeline_t> create(
            int width,
            int height,
            vku_ref_p<vku_renderpass_t> rp,
            const std::vector<vku_ref_p<vku_shader_t>>& shaders,
            VkPrimitiveTopology topology,
            vku_vertex_input_desc_t vid,
            vku_ref_p<vku_binding_desc_t> bd);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_compute_pipeline_t : public vku_object_t {
    VkPipeline                      vk_pipeline;
    VkPipelineLayout                vk_layout;
    VkDescriptorSetLayout           vk_desc_set_layout;

    vku_ref_p<vku_device_t>         dev;
    vku_ref_p<vku_shader_t>         shader;
    vku_ref_p<vku_binding_desc_t>   bd;

    static vku_ref_p<vku_compute_pipeline_t> create(
            vku_ref_p<vku_device_t> dev,
            vku_ref_p<vku_shader_t> shader,
            vku_ref_p<vku_binding_desc_t> bd);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/*engine_create_framebuffs*/
struct vku_framebuffs_t : public vku_object_t {
    std::vector<VkFramebuffer>  vk_fbuffs;

    vku_ref_p<vku_renderpass_t> rp;

    static vku_ref_p<vku_framebuffs_t> create(vku_ref_p<vku_renderpass_t> rp);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/*engine_create_cmdpool*/
struct vku_cmdpool_t : public vku_object_t {
    VkCommandPool           vk_pool;

    vku_ref_p<vku_device_t> dev;

    static vku_ref_p<vku_cmdpool_t> create(vku_ref_p<vku_device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_cmdbuff_t : public vku_object_t {
    VkCommandBuffer             vk_buff;

    vku_ref_p<vku_cmdpool_t>    cp;
    bool                        host_free;

    static vku_ref_p<vku_cmdbuff_t> create(vku_ref_p<vku_cmdpool_t> cp, bool host_free = false);

    void begin(VkCommandBufferUsageFlags flags);
    void begin_rpass(vku_ref_p<vku_framebuffs_t> fbs, uint32_t img_idx);
    void bind_vert_buffs(uint32_t first_bind,
            std::vector<std::pair<vku_ref_p<vku_buffer_t>, VkDeviceSize>> buffs);
    void bind_desc_set(VkPipelineBindPoint bind_point, VkPipelineLayout pipeline_alyout,
            vku_ref_p<vku_desc_set_t> desc_set);
    void bind_idx_buff(vku_ref_p<vku_buffer_t> ibuff, uint64_t off, VkIndexType idx_type);
    void draw(vku_ref_p<vku_pipeline_t> pl, uint64_t vert_cnt);
    void draw_idx(vku_ref_p<vku_pipeline_t> pl, uint64_t vert_cnt);
    void end_rpass();
    void end();

    void reset();

    void bind_compute(vku_ref_p<vku_compute_pipeline_t> cpl);
    void dispatch_compute(uint32_t x, uint32_t y = 1, uint32_t z = 1);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_sem_t : public vku_object_t {
    vku_ref_p<vku_device_t> dev;
    VkSemaphore             vk_sem;

    static vku_ref_p<vku_sem_t> create(vku_ref_p<vku_device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_fence_t : public vku_object_t {
    vku_ref_p<vku_device_t> dev;
    VkFence                 vk_fence;

    VkFenceCreateFlags      flags;

    static vku_ref_p<vku_fence_t> create(
            vku_ref_p<vku_device_t> dev,
            VkFenceCreateFlags flags = 0);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_buffer_t : public vku_object_t {
    VkBuffer                    vk_buff;
    VkDeviceMemory              vk_mem;
    void                        *map_ptr = nullptr;

    vku_ref_p<vku_device_t>     dev;
    size_t                      size;
    VkBufferUsageFlags          usage;
    VkSharingMode               sh_mode;
    VkMemoryPropertyFlags       mem_flags;

    static vku_ref_p<vku_buffer_t> create(
            vku_ref_p<vku_device_t> dev,
            size_t size,
            VkBufferUsageFlags usage,
            VkSharingMode sh_mode,
            VkMemoryPropertyFlags mem_flags);

    void *map_data(VkDeviceSize offset, VkDeviceSize size);
    void unmap_data();

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_image_t : public vku_object_t {
    VkImage                 vk_img;
    VkDeviceMemory          vk_img_mem;

    vku_ref_p<vku_device_t> dev;
    uint32_t                width;
    uint32_t                height;
    VkFormat                fmt;
    VkImageUsageFlags       usage;

    static vku_ref_p<vku_image_t> create(
            vku_ref_p<vku_device_t> dev,
            uint32_t width,
            uint32_t height,
            VkFormat fmt,
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                      | VK_IMAGE_USAGE_SAMPLED_BIT);

    /* if no command buffer is provided, one will be allocated from the command pool */
    void transition_layout(
            vku_ref_p<vku_cmdpool_t> cp,
            VkImageLayout old_layout,
            VkImageLayout new_layout,
            vku_ref_p<vku_cmdbuff_t> cbuff = nullptr);

    /* if no command buffer is provided, one will be allocated from the command pool */
    void set_data(
            vku_ref_p<vku_cmdpool_t> cp,
            void *data,
            uint32_t sz,
            vku_ref_p<vku_cmdbuff_t> cbuff = nullptr);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_img_view_t : public vku_object_t {
    VkImageView             vk_view;
    vku_ref_p<vku_image_t>  img;
    VkImageAspectFlags      aspect_mask;

    static vku_ref_p<vku_img_view_t> create(
            vku_ref_p<vku_image_t> img,
            VkImageAspectFlags aspect_mask);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_img_sampl_t : public vku_object_t {
    VkSampler               vk_sampler;

    vku_ref_p<vku_device_t> dev;
    VkFilter                filter;

    static vku_ref_p<vku_img_sampl_t> create(
            vku_ref_p<vku_device_t> dev,
            VkFilter filter = VK_FILTER_LINEAR);

    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding, VkShaderStageFlags stage);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_desc_pool_t : public vku_object_t {
    VkDescriptorPool                vk_descpool;

    vku_ref_p<vku_device_t>         dev;
    vku_ref_p<vku_binding_desc_t>   binds;
    uint32_t                        cnt;

    static vku_ref_p<vku_desc_pool_t> create(
            vku_ref_p<vku_device_t> dev,
            vku_ref_p<vku_binding_desc_t> binds,
            uint32_t cnt);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_desc_set_t : public vku_object_t {
    VkDescriptorSet                 vk_desc_set;

    vku_ref_p<vku_desc_pool_t>      dp;
    VkDescriptorSetLayout           layout;
    vku_ref_p<vku_binding_desc_t>   bd;

    static vku_ref_p<vku_desc_set_t> create(
            vku_ref_p<vku_desc_pool_t> dp,
            VkDescriptorSetLayout layout,
            vku_ref_p<vku_binding_desc_t> bd);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct vku_binding_desc_t : public vku_object_t {
    struct binding_desc_t : public vku_object_t {
        VkDescriptorSetLayoutBinding desc;

        virtual ~binding_desc_t() {}

        virtual VkWriteDescriptorSet get_write() const = 0;

    private:
        virtual VkResult _init() = 0;
        virtual VkResult _uninit() override { return VK_SUCCESS; };
    };

    struct buff_binding_t : public binding_desc_t {
        VkDescriptorBufferInfo          desc_buff_info;

        vku_ref_p<vku_buffer_t>         buff;

        static vku_ref_p<buff_binding_t> create(
                VkDescriptorSetLayoutBinding desc,
                vku_ref_p<vku_buffer_t> buff);

        virtual VkWriteDescriptorSet get_write() const override;

    private:
        virtual VkResult _init() override;
    };

    struct sampl_binding_t : public binding_desc_t {
        VkDescriptorImageInfo           imag_info;

        vku_ref_p<vku_img_view_t>       view;
        vku_ref_p<vku_img_sampl_t>      sampl;

        static vku_ref_p<sampl_binding_t> create(
                VkDescriptorSetLayoutBinding desc,
                vku_ref_p<vku_img_view_t> view,
                vku_ref_p<vku_img_sampl_t> sampl);

        virtual VkWriteDescriptorSet get_write() const override;

    private:
        virtual VkResult _init() override;
    };

    std::vector<vku_ref_p<binding_desc_t>> binds;

    static vku_ref_p<vku_binding_desc_t> create(std::vector<vku_ref_p<binding_desc_t>> binds);

    std::vector<VkWriteDescriptorSet> get_writes() const;
    std::vector<VkDescriptorSetLayoutBinding> get_descriptors() const;

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* Internal:
================================================================================================= */

struct vku_swapchain_details_t {
    VkSurfaceCapabilitiesKHR        capab;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
};

inline VkResult vku_create_dbg_messenger(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* dbg_info,
        const VkAllocationCallbacks* alloc,
        VkDebugUtilsMessengerEXT* dbg_msg);

inline void vku_destroy_dbg_messenger(
        VkInstance instance,
        VkDebugUtilsMessengerEXT dbg_msg,
        const VkAllocationCallbacks* alloc);

inline VKAPI_ATTR VkBool32 VKAPI_CALL vku_dbg_cbk(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* _data,
        void* ctx);

inline  vku_swapchain_details_t vku_get_swapchain_details(VkPhysicalDevice dev,
        VkSurfaceKHR surf);

inline vku_gpu_family_ids_t vku_find_queue_families(VkPhysicalDevice dev,
        VkSurfaceKHR surface);

inline VkExtent2D vku_choose_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capab);

inline int vku_score_phydev(VkPhysicalDevice dev, VkSurfaceKHR surf);
inline VkShaderStageFlagBits vku_get_shader_type(vku_shader_stage_t own_type);

#ifndef VKU_HAS_NEW_GLSLANG
inline TBuiltInResource vku_spirv_resources = {};
#else
inline glslang_resource_t vku_spirv_resources = {};
#endif

inline void vku_spirv_uninit();
inline void vku_spirv_init();
inline vku_spirv_t vku_spirv_compile(vku_shader_stage_t vk_stage, const char *code);
inline int vku_spirv_save(const vku_spirv_t& code, const char *filepath);

inline uint32_t vku_find_memory_type(vku_ref_p<vku_device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties);

/* IMPLEMENTATION:
=================================================================================================
=================================================================================================
================================================================================================= */

inline bool vku_init_state = false;
inline VkResult vku_init() {
    if (vku_init_state)
        return VK_SUCCESS;

    vku_spirv_init();
    if (glfwInit() != GLFW_TRUE) {
        DBG("Failed to init glfw: %s", vku_glfw_err().c_str());
        vku_spirv_uninit();
        return VK_ERROR_UNKNOWN;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    vku_init_state = true;
    return VK_SUCCESS;
}

inline VkResult vku_uninit() {
    if (!vku_init_state)
        return VK_ERROR_UNKNOWN;
    glfwTerminate();
    vku_spirv_uninit();
    vku_init_state = false;
    return VK_SUCCESS;
}

inline vku_vertex_input_desc_t vku_vertex_p2n0c3t2_t::get_input_desc() {
    return {
        .bind_desc = {
            .binding = 0,
            .stride = sizeof(vku_vertex_p2n0c3t2_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
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

inline vku_vertex_input_desc_t vku_vertex_p3n3c3t2_t::get_input_desc() {
    return {
        .bind_desc = {
            .binding = 0,
            .stride = sizeof(vku_vertex_p3n3c3t2_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
        .attr_desc = {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vku_vertex_p3n3c3t2_t, pos)
            },
            {
                .location = 1,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vku_vertex_p3n3c3t2_t, normal)
            },
            {
                .location = 2,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = offsetof(vku_vertex_p3n3c3t2_t, color)
            },
            {
                .location = 3,
                .binding = 0,
                .format = VK_FORMAT_R32G32_SFLOAT,
                .offset = offsetof(vku_vertex_p3n3c3t2_t, tex)
            }
        }
    };
}

inline VkDescriptorSetLayoutBinding vku_ubo_t::get_desc_set(uint32_t binding,
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

inline VkDescriptorSetLayoutBinding vku_ssbo_t::get_desc_set(uint32_t binding,
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

/* vku_window_t
================================================================================================= */

inline VkResult vku_window_t::_uninit() {
    if (_window)
        glfwDestroyWindow(_window);
    return VK_SUCCESS;
}

inline VkResult vku_window_t::_init() {
    VK_ASSERT(vku_init());
    _window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);
    if (!_window) {
        DBG("Failed to create a glfw window: %s", vku_glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    return VK_SUCCESS;
}

inline vku_ref_p<vku_window_t> vku_window_t::create(int width, int height, std::string name) {
    auto ret = vku_ref_t<vku_window_t>::create_obj_ref(std::make_unique<vku_window_t>(), {});

    ret->get()->window_name = name;
    ret->get()->width = width;
    ret->get()->height = height;

    VK_ASSERT(ret->get()->_call_init());
    DBG("Done post init");
    return ret;
}

/* vku_instance_t
================================================================================================= */

inline VkResult vku_instance_t::_init() {
    FnScope err_scope;

    /* The instance can be used without a window, so it must also init vku if it was not already
    initialized */
    VK_ASSERT(vku_init());

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
        DBG("Failed to get required extensions for glfw: %s", vku_glfw_err().c_str());
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    std::vector<const char*> exts_c(glfw_exts, glfw_exts + glfw_ext_count);
    for (auto &e : this->extensions)
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
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
    }

    /* set required layers */
    std::vector<const char*> layers_c;
    for (auto &l : this->layers)
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
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
    }

    VkApplicationInfo vk_app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = app_name.c_str(),
        .applicationVersion = ver,
        .pEngineName = engine_name.c_str(),
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

    DBG("Created a vulkan instance!");

    /* Create Vulkan Debug Messenger
    ============================================================================================= */

    VkDebugUtilsMessengerCreateInfoEXT dbg_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vku_dbg_cbk,
        .pUserData = nullptr,
    };

    VK_ASSERT(vku_create_dbg_messenger(vk_instance, &dbg_info, NULL, &vk_dbg_messenger));
    err_scope([&]{ vku_destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL); });

    err_scope.disable();
    DBG("Created vulkan messenger! %p", this);
    return VK_SUCCESS;
}

inline VkResult vku_instance_t::_uninit() {
    vku_destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL);
    vkDestroyInstance(vk_instance, NULL);
    return VK_SUCCESS;
}

inline vku_ref_p<vku_instance_t> vku_instance_t::create(
        const std::string app_name,
        const std::string engine_name,
        const std::vector<std::string>& extensions,
        const std::vector<std::string>& layers)
{
    auto ret = vku_ref_t<vku_instance_t>::create_obj_ref(std::make_unique<vku_instance_t>(), {});

    ret->get()->app_name = app_name;
    ret->get()->engine_name = engine_name;
    ret->get()->extensions = extensions;
    ret->get()->layers = layers;

    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_surface_t (TODO)
================================================================================================= */

inline VkResult vku_surface_t::_init() {
    if (glfwCreateWindowSurface(inst->get()->vk_instance, window->get()->get_window(),
            NULL, &vk_surface) != VK_SUCCESS)
    {
        DBG("Failed to get vk_surface: %s", vku_glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    return VK_SUCCESS;
}
inline VkResult vku_surface_t::_uninit() {
    vkDestroySurfaceKHR(inst->get()->vk_instance, vk_surface, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_surface_t> vku_surface_t::create(
        vku_ref_p<vku_window_t> window,
        vku_ref_p<vku_instance_t> inst)
{
    auto ret = vku_ref_t<vku_surface_t>::create_obj_ref(
            std::make_unique<vku_surface_t>(), {window, inst});
    ret->get()->window = window;
    ret->get()->inst = inst;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_device_t (TODO)
================================================================================================= */

inline VkResult vku_device_t::_init() {
    uint32_t dev_cnt = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(surf->get()->inst->get()->vk_instance, &dev_cnt, NULL));

    if (dev_cnt == 0) {
        DBG("Failed to find a GPU with Vulkan support!")
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    std::vector<VkPhysicalDevice> devices(dev_cnt);
    VK_ASSERT(vkEnumeratePhysicalDevices(
            surf->get()->inst->get()->vk_instance, &dev_cnt, devices.data()));

    vk_phy_dev = VK_NULL_HANDLE;
    int max_score = -1;
    for (const auto &dev : devices) {
        int score = vku_score_phydev(dev, surf->get()->vk_surface);
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

    que_fams = vku_find_queue_families(vk_phy_dev, surf->get()->vk_surface);
    que_ids = { que_fams.graphics_id, que_fams.present_id };
    std::vector<VkDeviceQueueCreateInfo> dev_ques;

    float queue_prio = 1.0f;
    for (auto id : que_ids)
        dev_ques.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = (uint32_t)id,
            .queueCount = 1,
            .pQueuePriorities = &queue_prio
        });

    VkPhysicalDeviceFeatures dev_feat{};
    vkGetPhysicalDeviceFeatures(vk_phy_dev, &dev_feat);

    std::vector<const char*> dev_exts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    std::vector<const char*> dev_layers = { "VK_LAYER_KHRONOS_validation" };
    dev_feat.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo dev_info {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
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

    vkGetDeviceQueue(vk_dev, que_fams.graphics_id, 0, &vk_graphics_que);
    vkGetDeviceQueue(vk_dev, que_fams.present_id, 0, &vk_present_que);

    DBG("Created Vulkan Logical Device");
    return VK_SUCCESS;
}
inline VkResult vku_device_t::_uninit() {
    vkDestroyDevice(vk_dev, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_device_t> vku_device_t::create(vku_ref_p<vku_surface_t> surf) {
    auto ret = vku_ref_t<vku_device_t>::create_obj_ref(std::make_unique<vku_device_t>(), {surf});
    ret->get()->surf = surf;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_swapchain_t (TODO)
================================================================================================= */

inline VkResult vku_swapchain_t::_init() {
    FnScope err_scope;
    auto sc_detail = vku_get_swapchain_details(dev->get()->vk_phy_dev,
            dev->get()->surf->get()->vk_surface);

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
    vk_extent = vku_choose_extent(dev->get()->surf->get()->window->get()->get_window(),
            sc_detail.capab);

    /* choose image count */
    uint32_t img_cnt = sc_detail.capab.minImageCount + 1;
    if (sc_detail.capab.maxImageCount > 0 && img_cnt > sc_detail.capab.maxImageCount)
        img_cnt = sc_detail.capab.maxImageCount;

    std::vector<uint32_t> qf_arr = { dev->get()->que_ids.begin(), dev->get()->que_ids.end() };

    VkSwapchainCreateInfoKHR sc_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = dev->get()->surf->get()->vk_surface,
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

    VK_ASSERT(vkCreateSwapchainKHR(dev->get()->vk_dev, &sc_info, NULL, &vk_swapchain));
    err_scope([&]{ vkDestroySwapchainKHR(dev->get()->vk_dev, vk_swapchain, NULL); });

    img_cnt = 0;
    VK_ASSERT(vkGetSwapchainImagesKHR(dev->get()->vk_dev, vk_swapchain, &img_cnt, NULL));
    vk_sc_images.resize(img_cnt);
    VK_ASSERT(vkGetSwapchainImagesKHR(dev->get()->vk_dev, vk_swapchain, &img_cnt,
            vk_sc_images.data()));

    DBG("Created Swapchain!");

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
        VK_ASSERT(vkCreateImageView(dev->get()->vk_dev, &iv_info, NULL, &vk_sc_image_views[i]));
        err_scope([i, this]{ 
            vkDestroyImageView(dev->get()->vk_dev, vk_sc_image_views[i], NULL); });
    }

    /* TODO: this is problematic, here depth_imag is dependent on dev and not on swapchain, so
    if we delete dev we have a double free */
    depth_imag = vku_image_t::create(dev, vk_extent.width, vk_extent.height, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depth_view = vku_img_view_t::create(depth_imag, VK_IMAGE_ASPECT_DEPTH_BIT);

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult vku_swapchain_t::_uninit() {
    for (auto &iv : vk_sc_image_views)
        vkDestroyImageView(dev->get()->vk_dev, iv, NULL);
    vkDestroySwapchainKHR(dev->get()->vk_dev, vk_swapchain, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_swapchain_t> vku_swapchain_t::create(vku_ref_p<vku_device_t> dev) {
    auto ret = vku_ref_t<vku_swapchain_t>::create_obj_ref(
            std::make_unique<vku_swapchain_t>(), {dev});
    ret->get()->dev = dev;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_shader_t (TODO)
================================================================================================= */

inline VkResult vku_shader_t::_init() {
    if (init_from_path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();

        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            DBG("Failed to read shader data");
            throw vku_err_t(VK_ERROR_UNKNOWN);
        }
        VkShaderModuleCreateInfo shader_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = buffer.size(),
            .pCode = (uint32_t *)buffer.data(),
        };
        VK_ASSERT(vkCreateShaderModule(dev->get()->vk_dev, &shader_info, NULL, &vk_shader));
        DBG("Loaded shader from path [%s] of size: %zu data: %p -> vk_%p",
                path.c_str(), buffer.size(), buffer.data(), vk_shader);
    }
    else {
        VkShaderModuleCreateInfo shader_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = spirv.content.size() * sizeof(uint32_t),
            .pCode = spirv.content.data(),
        };
        type = spirv.type;
        VK_ASSERT(vkCreateShaderModule(dev->get()->vk_dev, &shader_info, NULL, &vk_shader));
        DBG("Loaded shader from buffer of size: %zu data: %p -> vk_%p",
                spirv.content.size() * sizeof(uint32_t), spirv.content.data(), vk_shader);
    }
    return VK_SUCCESS;
}
inline VkResult vku_shader_t::_uninit() {
    vkDestroyShaderModule(dev->get()->vk_dev, vk_shader, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_shader_t> vku_shader_t::create(
        vku_ref_p<vku_device_t> dev,
        const vku_spirv_t& spirv)
{
    auto ret = vku_ref_t<vku_shader_t>::create_obj_ref(std::make_unique<vku_shader_t>(), {dev});
    ret->get()->init_from_path = false;
    ret->get()->dev = dev;
    ret->get()->spirv = spirv;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}
inline vku_ref_p<vku_shader_t> vku_shader_t::create(
        vku_ref_p<vku_device_t> dev,
        const char *path,
        vku_shader_stage_t type)
{
    auto ret = vku_ref_t<vku_shader_t>::create_obj_ref(std::make_unique<vku_shader_t>(), {dev});
    ret->get()->init_from_path = true;
    ret->get()->dev = dev;
    ret->get()->path = path;
    ret->get()->type = type;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}


/* vku_renderpass_t (TODO)
================================================================================================= */

inline VkResult vku_renderpass_t::_init() {
    VkAttachmentDescription color_attach {
        .flags = 0,
        .format = swc->get()->vk_surf_fmt.format,
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
        .format = swc->get()->depth_view->get()->img->get()->fmt,
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

    /* TODO: expose dependency to exterior because else it doesn't make sense to have sems waiting
    on different stages */
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

    VK_ASSERT(vkCreateRenderPass(swc->get()->dev->get()->vk_dev,
            &render_pass_info, NULL, &vk_render_pass));

    return VK_SUCCESS;
}
inline VkResult vku_renderpass_t::_uninit() {
    vkDestroyRenderPass(swc->get()->dev->get()->vk_dev, vk_render_pass, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_renderpass_t> vku_renderpass_t::create(vku_ref_p<vku_swapchain_t> swc) {
    auto ret = vku_ref_t<vku_renderpass_t>::create_obj_ref(
            std::make_unique<vku_renderpass_t>(), {swc});
    ret->get()->swc = swc;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_pipeline_t (TODO)
================================================================================================= */

inline VkResult vku_pipeline_t::_init() {
    FnScope err_scope;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    for (auto sh : shaders) {
        shader_stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = vku_get_shader_type(sh->get()->type),
            .module              = sh->get()->vk_shader,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        });
        DBG("Added shader: %p, type: %x ",
                sh->get()->vk_shader, vku_get_shader_type(sh->get()->type));
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
        .pVertexBindingDescriptions      = &vid.bind_desc,
        .vertexAttributeDescriptionCount = (uint32_t)vid.attr_desc.size(),
        .pVertexAttributeDescriptions    = vid.attr_desc.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = float(width),
        .height     = float(height),
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = rp->get()->swc->get()->vk_extent,
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
        .cullMode                   = VK_CULL_MODE_BACK_BIT,
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

    auto bind_descriptors = bd->get()->get_descriptors();
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

    VK_ASSERT(vkCreateDescriptorSetLayout(rp->get()->swc->get()->dev->get()->vk_dev,
            &desc_set_layout_info, nullptr, &vk_desc_set_layout));
    err_scope([&]{ vkDestroyDescriptorSetLayout(
            rp->get()->swc->get()->dev->get()->vk_dev, vk_desc_set_layout, nullptr); });
    DBGVV("Allocated descriptor set layout: %p", vk_desc_set_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &vk_desc_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };

    VK_ASSERT(vkCreatePipelineLayout(
            rp->get()->swc->get()->dev->get()->vk_dev, &pipeline_layout_info, NULL, &vk_layout));
    err_scope([&]{ vkDestroyPipelineLayout(rp->get()->swc->get()->dev->get()->vk_dev,
            vk_layout, NULL); });
    DBGVV("Allocated pipeline layout: %p", vk_layout);

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
        .layout                 = vk_layout,
        .renderPass             = rp->get()->vk_render_pass,
        .subpass                = 0,
        .basePipelineHandle     = VK_NULL_HANDLE,
        .basePipelineIndex      = -1,
    };

    VK_ASSERT(vkCreateGraphicsPipelines(rp->get()->swc->get()->dev->get()->vk_dev,
            VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    err_scope.disable();
    DBGVV("Allocated pipeline: %p", vk_pipeline);
    return VK_SUCCESS;
}
inline VkResult vku_pipeline_t::_uninit() {
    DBGVV("Dealocating pipeline: %p", vk_pipeline);

    vkDestroyPipeline(rp->get()->swc->get()->dev->get()->vk_dev, vk_pipeline, NULL);
    vkDestroyPipelineLayout(rp->get()->swc->get()->dev->get()->vk_dev, vk_layout, NULL);
    vkDestroyDescriptorSetLayout(rp->get()->swc->get()->dev->get()->vk_dev,
            vk_desc_set_layout, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_pipeline_t> vku_pipeline_t::create(
        int width,
        int height,
        vku_ref_p<vku_renderpass_t> rp,
        const std::vector<vku_ref_p<vku_shader_t>> &shaders,
        VkPrimitiveTopology topology,
        vku_vertex_input_desc_t vid,
        vku_ref_p<vku_binding_desc_t> bd)
{
    std::vector<std::shared_ptr<vku_ref_base_t>> deps;
    for (auto sh : shaders)
        deps.push_back(sh);
    deps.push_back(rp);
    deps.push_back(bd);
    auto ret = vku_ref_t<vku_pipeline_t>::create_obj_ref(
            std::make_unique<vku_pipeline_t>(), deps);
    ret->get()->width = width;
    ret->get()->height = height;
    ret->get()->rp = rp;
    ret->get()->shaders = shaders;
    ret->get()->topology = topology;
    ret->get()->vid = vid;
    ret->get()->bd = bd;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_compute_pipeline_t (TODO)
================================================================================================= */

inline VkResult vku_compute_pipeline_t::_init() {
    FnScope err_scope;

    auto bind_descriptors = bd->get()->get_descriptors();
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

    VK_ASSERT(vkCreateDescriptorSetLayout(dev->get()->vk_dev, &desc_set_layout_info, nullptr,
            &vk_desc_set_layout));
    err_scope([&]{ vkDestroyDescriptorSetLayout(dev->get()->vk_dev, vk_desc_set_layout, nullptr); });
    DBGVV("Allocated descriptor set layout: %p", vk_desc_set_layout);

    VkPipelineLayoutCreateInfo pipeline_layout_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &vk_desc_set_layout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL,
    };

    VK_ASSERT(vkCreatePipelineLayout(dev->get()->vk_dev, &pipeline_layout_info, NULL, &vk_layout));
    err_scope([&]{ vkDestroyPipelineLayout(dev->get()->vk_dev, vk_layout, NULL); });
    DBGVV("Allocated pipeline layout: %p", vk_layout);

    if (vku_get_shader_type(shader->get()->type) != VK_SHADER_STAGE_COMPUTE_BIT) {
        throw vku_err_t("compute_pipeline needs a compute shader");
    }

    VkPipelineShaderStageCreateInfo shader_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .stage                  = vku_get_shader_type(shader->get()->type),
        .module                 = shader->get()->vk_shader,
        .pName                  = "main",
        .pSpecializationInfo    = nullptr,
    };

    VkComputePipelineCreateInfo pipeline_info {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = shader_info,
        .layout             = vk_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex  = 0,
    };

    VK_ASSERT(vkCreateComputePipelines(
            dev->get()->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    DBGVV("Allocated pipeline: %p", vk_pipeline);

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult vku_compute_pipeline_t::_uninit() {
    DBGVV("Dealocating pipeline: %p", vk_pipeline);

    vkDestroyPipeline(dev->get()->vk_dev, vk_pipeline, NULL);
    vkDestroyPipelineLayout(dev->get()->vk_dev, vk_layout, NULL);
    vkDestroyDescriptorSetLayout(dev->get()->vk_dev, vk_desc_set_layout, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_compute_pipeline_t> vku_compute_pipeline_t::create(
        vku_ref_p<vku_device_t> dev,
        vku_ref_p<vku_shader_t> shader,
        vku_ref_p<vku_binding_desc_t> bd)
{
    auto ret = vku_ref_t<vku_compute_pipeline_t>::create_obj_ref(
            std::make_unique<vku_compute_pipeline_t>(), {dev, shader, bd});
    ret->get()->dev = dev;
    ret->get()->shader = shader;
    ret->get()->bd = bd;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_framebuffs_t (TODO)
================================================================================================= */

inline VkResult vku_framebuffs_t::_init() {
    vk_fbuffs.resize(rp->get()->swc->get()->vk_sc_image_views.size());

    FnScope err_scope;
    for (size_t i = 0; i < vk_fbuffs.size(); i++) {
        VkImageView attachs[] = {
            rp->get()->swc->get()->vk_sc_image_views[i],
            rp->get()->swc->get()->depth_view->get()->vk_view
        };
        
        VkFramebufferCreateInfo fbuff_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = rp->get()->vk_render_pass,
            .attachmentCount = 2,
            .pAttachments = attachs,
            .width = rp->get()->swc->get()->vk_extent.width,
            .height = rp->get()->swc->get()->vk_extent.height,
            .layers = 1,
        };

        VK_ASSERT(vkCreateFramebuffer(rp->get()->swc->get()->dev->get()->vk_dev,
                &fbuff_info, NULL, &vk_fbuffs[i]));
        err_scope([this, i] { 
            vkDestroyFramebuffer(rp->get()->swc->get()->dev->get()->vk_dev, vk_fbuffs[i], NULL);
        });
    }

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult vku_framebuffs_t::_uninit() {
    for (auto fbuff : vk_fbuffs)
        vkDestroyFramebuffer(rp->get()->swc->get()->dev->get()->vk_dev, fbuff, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_framebuffs_t> vku_framebuffs_t::create(vku_ref_p<vku_renderpass_t> rp){
    auto ret = vku_ref_t<vku_framebuffs_t>::create_obj_ref(
            std::make_unique<vku_framebuffs_t>(), {rp});
    ret->get()->rp = rp;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_cmdpool_t (TODO)
================================================================================================= */

inline VkResult vku_cmdpool_t::_init() {
    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (uint32_t)dev->get()->que_fams.graphics_id,
    };

    VK_ASSERT(vkCreateCommandPool(dev->get()->vk_dev, &pool_info, NULL, &vk_pool));
    return VK_SUCCESS;
}
inline VkResult vku_cmdpool_t::_uninit() {
    vkDestroyCommandPool(dev->get()->vk_dev, vk_pool, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_cmdpool_t> vku_cmdpool_t::create(vku_ref_p<vku_device_t> dev) {
    auto ret = vku_ref_t<vku_cmdpool_t>::create_obj_ref(
            std::make_unique<vku_cmdpool_t>(), {dev});
    ret->get()->dev = dev;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_cmdbuff_t (TODO)
================================================================================================= */

inline VkResult vku_cmdbuff_t::_init() {
    VkCommandBufferAllocateInfo buff_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cp->get()->vk_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_ASSERT(vkAllocateCommandBuffers(cp->get()->dev->get()->vk_dev, &buff_info, &vk_buff));
    return VK_SUCCESS;
}
inline VkResult vku_cmdbuff_t::_uninit() {
    if (host_free) { /* TODO: what is with this host_free? */
        vkFreeCommandBuffers(cp->get()->dev->get()->vk_dev, cp->get()->vk_pool, 1, &vk_buff);
    }
    return VK_SUCCESS;
}
inline vku_ref_p<vku_cmdbuff_t> vku_cmdbuff_t::create(
        vku_ref_p<vku_cmdpool_t> cp, bool host_free)
{
    auto ret = vku_ref_t<vku_cmdbuff_t>::create_obj_ref(
            std::make_unique<vku_cmdbuff_t>(), {cp});
    ret->get()->cp = cp;
    ret->get()->host_free = host_free;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline void vku_cmdbuff_t::begin(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = NULL,
    };
    VK_ASSERT(vkBeginCommandBuffer(vk_buff, &begin_info));
}

inline void vku_cmdbuff_t::begin_rpass(vku_ref_p<vku_framebuffs_t> fbs, uint32_t img_idx) {
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
        .renderPass = fbs->get()->rp->get()->vk_render_pass,
        .framebuffer = fbs->get()->vk_fbuffs[img_idx],
        .renderArea = {
            .offset = {0, 0},
            .extent = fbs->get()->rp->get()->swc->get()->vk_extent,
        },
        .clearValueCount = 2,
        .pClearValues = clear_color,
    };

    vkCmdBeginRenderPass(vk_buff, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

inline void vku_cmdbuff_t::bind_vert_buffs(uint32_t first_bind,
        std::vector<std::pair<vku_ref_p<vku_buffer_t>, VkDeviceSize>> buffs)
{
    std::vector<VkBuffer> vk_buffs;
    std::vector<VkDeviceSize> vk_offsets;
    for (auto [b, off] : buffs) {
        vk_buffs.push_back(b->get()->vk_buff);
        vk_offsets.push_back(off);
    }

    vkCmdBindVertexBuffers(vk_buff, first_bind, vk_buffs.size(), vk_buffs.data(),
            vk_offsets.data());
}

inline void vku_cmdbuff_t::bind_desc_set(VkPipelineBindPoint bind_point,
        VkPipelineLayout pipeline_layout, vku_ref_p<vku_desc_set_t> desc_set)
{
    DBGVVV("bind desc_set: %p with layout: %p bind_point: %d",
            desc_set->get()->vk_desc_set, pipeline_layout, bind_point);
    vkCmdBindDescriptorSets(vk_buff, bind_point, pipeline_layout, 0, 1,
            &desc_set->get()->vk_desc_set, 0, nullptr);
}

inline void vku_cmdbuff_t::bind_idx_buff(vku_ref_p<vku_buffer_t> ibuff, uint64_t off,
        VkIndexType idx_type)
{
    vkCmdBindIndexBuffer(vk_buff, ibuff->get()->vk_buff, off, idx_type);
}

inline void vku_cmdbuff_draw_helper(VkCommandBuffer vk_buff, vku_ref_p<vku_pipeline_t> pl) {
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->get()->vk_pipeline);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(pl->get()->rp->get()->swc->get()->vk_extent.width),
        .height = float(pl->get()->rp->get()->swc->get()->vk_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(vk_buff, 0, 1, &viewport);

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = pl->get()->rp->get()->swc->get()->vk_extent
    };
    vkCmdSetScissor(vk_buff, 0, 1, &scissor);
}

inline void vku_cmdbuff_t::draw(vku_ref_p<vku_pipeline_t> pl, uint64_t vert_cnt) {
    vku_cmdbuff_draw_helper(vk_buff, pl);
    vkCmdDraw(vk_buff, vert_cnt, 1, 0, 0);
}

inline void vku_cmdbuff_t::draw_idx(vku_ref_p<vku_pipeline_t> pl, uint64_t vert_cnt) {
    vku_cmdbuff_draw_helper(vk_buff, pl);
    vkCmdDrawIndexed(vk_buff, vert_cnt, 1, 0, 0, 0);
}

inline void vku_cmdbuff_t::end_rpass() {
    vkCmdEndRenderPass(vk_buff);
}

inline void vku_cmdbuff_t::end() {
    VK_ASSERT(vkEndCommandBuffer(vk_buff));
}

inline void vku_cmdbuff_t::reset() {
    VK_ASSERT(vkResetCommandBuffer(vk_buff, 0));
}

inline void vku_cmdbuff_t::bind_compute(vku_ref_p<vku_compute_pipeline_t> cpl) {
    DBGVVV("bind compute pipeline: %p", cpl->get()->vk_pipeline);
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_COMPUTE, cpl->get()->vk_pipeline);
}

inline void vku_cmdbuff_t::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(vk_buff, x, y, z);
}

/* vku_sem_t (TODO)
================================================================================================= */

inline VkResult vku_sem_t::_init() {
    VkSemaphoreCreateInfo sem_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VK_ASSERT(vkCreateSemaphore(dev->get()->vk_dev, &sem_info, NULL, &vk_sem));
    return VK_SUCCESS;
}
inline VkResult vku_sem_t::_uninit() {
    vkDestroySemaphore(dev->get()->vk_dev, vk_sem, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_sem_t> vku_sem_t::create(vku_ref_p<vku_device_t> dev) {
    auto ret = vku_ref_t<vku_sem_t>::create_obj_ref(
            std::make_unique<vku_sem_t>(), {dev});
    ret->get()->dev = dev;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_fence_t (TODO)
================================================================================================= */

inline VkResult vku_fence_t::_init() {
    VkFenceCreateInfo fence_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };

    VK_ASSERT(vkCreateFence(dev->get()->vk_dev, &fence_info, NULL, &vk_fence));
    return VK_SUCCESS;
}
inline VkResult vku_fence_t::_uninit() {
    vkDestroyFence(dev->get()->vk_dev, vk_fence, NULL);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_fence_t> vku_fence_t::create(
        vku_ref_p<vku_device_t> dev,
        VkFenceCreateFlags flags)
{
    auto ret = vku_ref_t<vku_fence_t>::create_obj_ref(
            std::make_unique<vku_fence_t>(), {dev});
    ret->get()->dev = dev;
    ret->get()->flags = flags;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_buffer_t (TODO)
================================================================================================= */


inline VkResult vku_buffer_t::_init() {
    VkBufferCreateInfo buff_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = sh_mode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    VK_ASSERT(vkCreateBuffer(dev->get()->vk_dev, &buff_info, nullptr, &vk_buff));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(dev->get()->vk_dev, vk_buff, &mem_req);

    VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = vku_find_memory_type(dev, mem_req.memoryTypeBits, mem_flags)
    };

    VK_ASSERT(vkAllocateMemory(dev->get()->vk_dev, &alloc_info, nullptr, &vk_mem));
    VK_ASSERT(vkBindBufferMemory(dev->get()->vk_dev, vk_buff, vk_mem, 0));
    return VK_SUCCESS;
}
inline VkResult vku_buffer_t::_uninit() {
    if (map_ptr)
        unmap_data();
    vkDestroyBuffer(dev->get()->vk_dev, vk_buff, nullptr);
    vkFreeMemory(dev->get()->vk_dev, vk_mem, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_buffer_t> vku_buffer_t::create(
        vku_ref_p<vku_device_t> dev,
        size_t size,
        VkBufferUsageFlags usage,
        VkSharingMode sh_mode,
        VkMemoryPropertyFlags mem_flags)
{
    auto ret = vku_ref_t<vku_buffer_t>::create_obj_ref(
            std::make_unique<vku_buffer_t>(), {dev});
    ret->get()->dev = dev;
    ret->get()->size = size;
    ret->get()->usage = usage;
    ret->get()->sh_mode = sh_mode;
    ret->get()->mem_flags = mem_flags;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline void *vku_buffer_t::map_data(VkDeviceSize offset, VkDeviceSize size) {
    if (map_ptr) {
        DBG("Memory is already mapped!");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    VK_ASSERT(vkMapMemory(dev->get()->vk_dev, vk_mem, offset, size, 0, &map_ptr));
    return map_ptr;
}

inline void vku_buffer_t::unmap_data() {
    if (!map_ptr) {
        DBG("Memory is not mapped, can't unmap");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vkUnmapMemory(dev->get()->vk_dev, vk_mem);
    map_ptr = nullptr;
}

/* vku_image_t (TODO)
================================================================================================= */

inline VkResult vku_image_t::_init() {
    VkImageCreateInfo image_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = fmt,
        .extent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VK_ASSERT(vkCreateImage(dev->get()->vk_dev, &image_info, nullptr, &vk_img));
    FnScope err_scope([&]{ vkDestroyImage(dev->get()->vk_dev, vk_img, nullptr); });

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(dev->get()->vk_dev, vk_img, &mem_req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = vku_find_memory_type(dev, mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_ASSERT(vkAllocateMemory(dev->get()->vk_dev, &alloc_info, nullptr, &vk_img_mem));
    VK_ASSERT(vkBindImageMemory(dev->get()->vk_dev, vk_img, vk_img_mem, 0));

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult vku_image_t::_uninit() {
    vkDestroyImage(dev->get()->vk_dev, vk_img, nullptr);
    vkFreeMemory(dev->get()->vk_dev, vk_img_mem, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_image_t> vku_image_t::create(
        vku_ref_p<vku_device_t> dev,
        uint32_t width,
        uint32_t height,
        VkFormat fmt,
        VkImageUsageFlags usage)
{
    auto ret = vku_ref_t<vku_image_t>::create_obj_ref(std::make_unique<vku_image_t>(), {dev});
    ret->get()->dev = dev;
    ret->get()->width = width;
    ret->get()->height = height;
    ret->get()->fmt = fmt;
    ret->get()->usage = usage;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline void vku_image_t::transition_layout(vku_ref_p<vku_cmdpool_t> cp,
        VkImageLayout old_layout, VkImageLayout new_layout, vku_ref_p<vku_cmdbuff_t> cbuff)
{
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = vku_cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = vku_fence_t::create(cp->get()->dev);

    if (!existing_cbuff) {
        cbuff->get()->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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
        throw vku_err_t(sformat("unsupported layout transition for image! %d -> %d",
                old_layout, new_layout));
    }

    /* TODO: don't we need a transition from shader_read to transfer_dst? That for transfering
    inside the image later on? */

    vkCmdPipelineBarrier(cbuff->get()->vk_buff, src_stage, dst_stage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

    if (!existing_cbuff) {
        cbuff->get()->end();

        vku_submit_cmdbuff({}, cbuff, fence, {});
        vku_wait_fences({fence});
    }
}

inline void vku_image_t::set_data(vku_ref_p<vku_cmdpool_t> cp, void *data, uint32_t sz,
        vku_ref_p<vku_cmdbuff_t> cbuff)
{
    uint32_t img_sz = width * height * 4;

    if (img_sz != sz)
        throw vku_err_t(sformat("data size(%d) does not match with image size(%d)", sz, img_sz));

    auto buff = vku_buffer_t::create(
        cp->get()->dev,
        img_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    memcpy(buff->get()->map_data(0, img_sz), data, img_sz);
    buff->get()->unmap_data();

    transition_layout(cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cbuff);

    auto fence = vku_fence_t::create(cp->get()->dev);

    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = vku_cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }

    if (!existing_cbuff)
        cbuff->get()->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

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
            .width = width,
            .height = height,
            .depth = 1,
        } 
    };
    vkCmdCopyBufferToImage(
            cbuff->get()->vk_buff,
            buff->get()->vk_buff,
            vk_img,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &region);

    transition_layout(cp, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cbuff);

    if (!existing_cbuff) {
        cbuff->get()->end();

        vku_submit_cmdbuff({}, cbuff, fence, {});
        vku_wait_fences({fence});
    }
}


/* vku_img_view_t (TODO)
================================================================================================= */

inline VkResult vku_img_view_t::_init() {
    VkImageViewCreateInfo view_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = img->get()->vk_img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = img->get()->fmt,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = {
            .aspectMask = aspect_mask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    VK_ASSERT(vkCreateImageView(img->get()->dev->get()->vk_dev, &view_info, nullptr, &vk_view));
    return VK_SUCCESS;
}
inline VkResult vku_img_view_t::_uninit() {
    vkDestroyImageView(img->get()->dev->get()->vk_dev, vk_view, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_img_view_t> vku_img_view_t::create(
        vku_ref_p<vku_image_t> img,
        VkImageAspectFlags aspect_mask)
{
    auto ret = vku_ref_t<vku_img_view_t>::create_obj_ref(std::make_unique<vku_img_view_t>(), {img});
    ret->get()->img = img;
    ret->get()->aspect_mask = aspect_mask;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_img_sampl_t (TODO)
================================================================================================= */

inline VkResult vku_img_sampl_t::_init() {
    VkPhysicalDeviceProperties dev_props;
    vkGetPhysicalDeviceProperties(dev->get()->vk_phy_dev, &dev_props);

    VkSamplerCreateInfo sampler_info {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = filter == VK_FILTER_LINEAR ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                                                  : VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VkBool32(filter == VK_FILTER_NEAREST ? VK_FALSE : VK_TRUE),
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

    VK_ASSERT(vkCreateSampler(dev->get()->vk_dev, &sampler_info, nullptr, &vk_sampler));
    return VK_SUCCESS;
}
inline VkResult vku_img_sampl_t::_uninit() {
    vkDestroySampler(dev->get()->vk_dev, vk_sampler, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_img_sampl_t> vku_img_sampl_t::create(
        vku_ref_p<vku_device_t> dev,
        VkFilter filter)
{
    auto ret = vku_ref_t<vku_img_sampl_t>::create_obj_ref(std::make_unique<vku_img_sampl_t>(), {dev});
    ret->get()->dev = dev;
    ret->get()->filter = filter;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline VkDescriptorSetLayoutBinding vku_img_sampl_t::get_desc_set(uint32_t binding,
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

/* vku_desc_pool_t (TODO)
================================================================================================= */

inline VkResult vku_desc_pool_t::_init() {
    std::vector<VkDescriptorPoolSize> pool_sizes;
    std::map<decltype(binds->get()->binds[0]->get()->desc.descriptorType), uint32_t> type_cnt;
    for (auto &b : binds->get()->binds)
        type_cnt[b->get()->desc.descriptorType] += cnt;

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
        .maxSets = cnt,
        .poolSizeCount = (uint32_t)pool_sizes.size(),
        .pPoolSizes = pool_sizes.data(),
    };

    VK_ASSERT(vkCreateDescriptorPool(dev->get()->vk_dev, &pool_info, nullptr, &vk_descpool));
    DBGVV("Allocated pool: %p", vk_descpool);
    return VK_SUCCESS;
}
inline VkResult vku_desc_pool_t::_uninit() {
    vkDestroyDescriptorPool(dev->get()->vk_dev, vk_descpool, nullptr);
    return VK_SUCCESS;
}
inline vku_ref_p<vku_desc_pool_t> vku_desc_pool_t::create(
        vku_ref_p<vku_device_t> dev,
        vku_ref_p<vku_binding_desc_t> binds,
        uint32_t cnt)
{
    auto ret = vku_ref_t<vku_desc_pool_t>::create_obj_ref(
            std::make_unique<vku_desc_pool_t>(), {dev, binds});
    ret->get()->dev = dev;
    ret->get()->binds = binds;
    ret->get()->cnt = cnt;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_desc_set_t (TODO)
================================================================================================= */

inline VkResult vku_desc_set_t::_init() {
    VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = dp->get()->vk_descpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VK_ASSERT(vkAllocateDescriptorSets(dp->get()->dev->get()->vk_dev, &alloc_info, &vk_desc_set));
    DBGVV("Allocated descriptor set: %p from pool: %p with layout: %p",
            vk_desc_set, dp->get()->vk_descpool, layout);

    /* TODO: this sucks, it references the buffer, but doesn't have a mechanism to do something
    if the buffer is freed without it's knowledge. So the buffer and descriptor set must
    match in size, but the buffer doesn't know that, that's not ok. */

    auto desc_writes = bd->get()->get_writes();
    for (auto &dw : desc_writes)
        dw.dstSet = vk_desc_set;

    DBGVV("writes: %zu", desc_writes.size());
    for (auto &w : desc_writes) {
        DBGVV("write: type: %x, bind: %d, dst_set: %p",
                w.descriptorType, w.dstBinding, w.dstSet);
    }

    vkUpdateDescriptorSets(dp->get()->dev->get()->vk_dev, (uint32_t)desc_writes.size(),
            desc_writes.data(), 0, nullptr);

    return VK_SUCCESS;
}
inline VkResult vku_desc_set_t::_uninit() {
    return VK_SUCCESS;
}
inline vku_ref_p<vku_desc_set_t> vku_desc_set_t::create(
        vku_ref_p<vku_desc_pool_t> dp,
        VkDescriptorSetLayout layout,
        vku_ref_p<vku_binding_desc_t> bd)
{
    auto ret = vku_ref_t<vku_desc_set_t>::create_obj_ref(
            std::make_unique<vku_desc_set_t>(), {dp, bd});
    ret->get()->dp = dp;
    ret->get()->layout = layout;
    ret->get()->bd = bd;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

/* vku_binding_desc_t (TODO):
================================================================================================= */

inline vku_ref_p<vku_binding_desc_t::buff_binding_t> vku_binding_desc_t::buff_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        vku_ref_p<vku_buffer_t> buff)
{
    using bd_t = vku_binding_desc_t::buff_binding_t;
    vku_ref_p<bd_t> ret = vku_ref_t<bd_t>::create_obj_ref(std::make_unique<bd_t>(), {buff});
    ret->get()->desc = desc;
    ret->get()->buff = buff;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline VkResult vku_binding_desc_t::buff_binding_t::_init() {
    if (buff) {
        desc_buff_info = VkDescriptorBufferInfo {
            .buffer = buff->get()->vk_buff,
            .offset = 0,
            .range = buff->get()->size
        };
    }
    return VK_SUCCESS;
}

inline VkWriteDescriptorSet vku_binding_desc_t::buff_binding_t::get_write() const {
    VkWriteDescriptorSet desc_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = 0, /* will be filled later */
        .dstBinding = desc.binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = desc.descriptorType,
        .pImageInfo = nullptr,
        .pBufferInfo = &desc_buff_info,
        .pTexelBufferView = nullptr,
    };

    return desc_write;
}


inline vku_ref_p<vku_binding_desc_t::sampl_binding_t> vku_binding_desc_t::sampl_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        vku_ref_p<vku_img_view_t> view,
        vku_ref_p<vku_img_sampl_t> sampl)
{
    using sb_t = vku_binding_desc_t::sampl_binding_t;
    vku_ref_p<sb_t> ret = vku_ref_t<sb_t>::create_obj_ref(std::make_unique<sb_t>(), {view, sampl});
    ret->get()->desc = desc;
    ret->get()->view = view;
    ret->get()->sampl = sampl;
    VK_ASSERT(ret->get()->_call_init()); /* init does nothing */
    return ret;
}

inline VkResult vku_binding_desc_t::sampl_binding_t::_init() {
    if (view && sampl) {
        imag_info = VkDescriptorImageInfo {
            .sampler = sampl->get()->vk_sampler,
            .imageView = view->get()->vk_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }
    return VK_SUCCESS;
}

inline VkWriteDescriptorSet vku_binding_desc_t::sampl_binding_t::get_write() const {
    VkWriteDescriptorSet desc_write{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = 0, /* will be filled later */
        .dstBinding = desc.binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = desc.descriptorType,
        .pImageInfo = &imag_info,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr,
    };

    return desc_write;
}

inline VkResult vku_binding_desc_t::_init() {
    return VK_SUCCESS;
}
inline VkResult vku_binding_desc_t::_uninit() {
    return VK_SUCCESS;
}
inline vku_ref_p<vku_binding_desc_t> vku_binding_desc_t::create(
        std::vector<vku_ref_p<binding_desc_t>> binds)
{
    std::vector<std::shared_ptr<vku_ref_base_t>> deps;
    for (auto b : binds)
        deps.push_back(b);
    auto ret = vku_ref_t<vku_binding_desc_t>::create_obj_ref(
            std::make_unique<vku_binding_desc_t>(), deps);
    ret->get()->binds = binds;
    VK_ASSERT(ret->get()->_call_init());
    return ret;
}

inline std::vector<VkWriteDescriptorSet> vku_binding_desc_t::get_writes() const {
    std::vector<VkWriteDescriptorSet> ret;

    for (auto &b : binds)
        ret.push_back(b->get()->get_write());

    return ret;
}

inline std::vector<VkDescriptorSetLayoutBinding> vku_binding_desc_t::get_descriptors() const {
    std::vector<VkDescriptorSetLayoutBinding> ret;
    for (auto &b : binds)
        ret.push_back(b->get()->desc);
    return ret;
}


/* Functions: (TODO)
================================================================================================= */

inline void vku_wait_fences(std::vector<vku_ref_p<vku_fence_t>> fences) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->get()->vk_fence);

    VK_ASSERT(vkWaitForFences(fences[0]->get()->dev->get()->vk_dev,
            vk_fences.size(), vk_fences.data(), VK_TRUE, UINT64_MAX));
}

inline void vku_reset_fences(std::vector<vku_ref_p<vku_fence_t>> fences) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw vku_err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->get()->vk_fence);
    VK_ASSERT(vkResetFences(fences[0]->get()->dev->get()->vk_dev,
            vk_fences.size(), vk_fences.data()));
}

inline void vku_aquire_next_img(vku_ref_p<vku_swapchain_t> swc, vku_ref_p<vku_sem_t> sem,
        uint32_t *img_idx)
{
    VK_ASSERT(vkAcquireNextImageKHR(swc->get()->dev->get()->vk_dev, swc->get()->vk_swapchain,
            UINT64_MAX, sem->get()->vk_sem, VK_NULL_HANDLE, img_idx));
}

inline void vku_submit_cmdbuff(
        std::vector<std::pair<vku_ref_p<vku_sem_t>, VkPipelineStageFlags>> wait_sems,
        vku_ref_p<vku_cmdbuff_t> cbuff,
        vku_ref_p<vku_fence_t> fence,
        std::vector<vku_ref_p<vku_sem_t>> sig_sems)
{
    std::vector<VkPipelineStageFlags> vk_wait_stages;
    std::vector<VkSemaphore> vk_wait_sems;
    std::vector<VkSemaphore> vk_sig_sems;

    for (auto [s, wait_stage] : wait_sems) {
        vk_wait_stages.push_back(wait_stage);
        vk_wait_sems.push_back(s->get()->vk_sem);
    }
    for (auto s : sig_sems) {
        vk_sig_sems.push_back(s->get()->vk_sem);
    }
    VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)vk_wait_sems.size(),
        .pWaitSemaphores = vk_wait_sems.size() == 0 ? nullptr : vk_wait_sems.data(),
        .pWaitDstStageMask = vk_wait_sems.size() == 0 ? nullptr : vk_wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cbuff->get()->vk_buff,
        .signalSemaphoreCount = (uint32_t)vk_sig_sems.size(),
        .pSignalSemaphores = vk_sig_sems.size() == 0 ? nullptr : vk_sig_sems.data(),
    };

    VK_ASSERT(vkQueueSubmit(cbuff->get()->cp->get()->dev->get()->vk_graphics_que, 1, &submit_info,
            fence == nullptr ? nullptr : fence->get()->vk_fence));
}

inline void vku_present(
        vku_ref_p<vku_swapchain_t> swc,
        std::vector<vku_ref_p<vku_sem_t>> wait_sems,
        uint32_t img_idx)
{
    std::vector<VkSemaphore> vk_wait_sems;

    for (auto s : wait_sems) {
        vk_wait_sems.push_back(s->get()->vk_sem);
    }

    VkPresentInfoKHR pres_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = (uint32_t)vk_wait_sems.size(),
        .pWaitSemaphores = vk_wait_sems.data(),
        .swapchainCount = 1,
        .pSwapchains = &swc->get()->vk_swapchain,
        .pImageIndices = &img_idx,
        .pResults = NULL,
    };

    VK_ASSERT(vkQueuePresentKHR(swc->get()->dev->get()->vk_present_que, &pres_info));
}

inline void vku_copy_buff(vku_ref_p<vku_cmdpool_t> cp, vku_ref_p<vku_buffer_t> dst,
        vku_ref_p<vku_buffer_t> src, VkDeviceSize sz, vku_ref_p<vku_cmdbuff_t> cbuff)
{
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = vku_cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = vku_fence_t::create(cp->get()->dev);

    if (!existing_cbuff)
        cbuff->get()->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sz
    };
    vkCmdCopyBuffer(cbuff->get()->vk_buff, src->get()->vk_buff, dst->get()->vk_buff,
            1, &copy_region);

    if (!existing_cbuff) {
        cbuff->get()->end();

        vku_submit_cmdbuff({}, cbuff, fence, {});
        vku_wait_fences({fence});
    }
}

/* Internal Functions: (TODO)
================================================================================================= */

inline std::string vku_glfw_err() {
    const char *errstr = NULL;
    int err = glfwGetError(&errstr);
    return sformat("[%s:%d]", errstr, err);
}

inline VkResult vku_create_dbg_messenger(
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

inline void vku_destroy_dbg_messenger(
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

inline VKAPI_ATTR VkBool32 VKAPI_CALL vku_dbg_cbk(
        VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
        VkDebugUtilsMessageTypeFlagsEXT             msg_type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* ctx)
{
    (void)msg_type;
    (void)ctx;
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        DBG("[VK_DBG]: %s", data->pMessage);
    }
    return VK_FALSE;
}

inline vku_swapchain_details_t vku_get_swapchain_details(VkPhysicalDevice dev,
        VkSurfaceKHR surf)
{
    vku_swapchain_details_t ret = {};
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

inline vku_gpu_family_ids_t vku_find_queue_families(VkPhysicalDevice dev,
        VkSurfaceKHR surface)
{
    vku_gpu_family_ids_t ret;

    uint32_t cnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &cnt, NULL);
    std::vector<VkQueueFamilyProperties> queue_families(cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &cnt, queue_families.data());

    for (int i = 0; auto qf : queue_families) {
        if ((qf.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qf.queueFlags & VK_QUEUE_COMPUTE_BIT))
            ret.graphics_id = i;
        VkBool32 res = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &res);
        if (res)
            ret.present_id = i;
        i++;
    }
    return ret;
}

inline VkExtent2D vku_choose_extent(GLFWwindow *window, VkSurfaceCapabilitiesKHR capab) {
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

inline int vku_score_phydev(VkPhysicalDevice dev, VkSurfaceKHR surf) {
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

inline VkShaderStageFlagBits vku_get_shader_type(vku_shader_stage_t own_type) {
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

inline int vku_spirv_save(const vku_spirv_t& code, const char *filepath) {
    std::ofstream file(filepath, std::ios::out | std::ios::binary);
    file.write((const char *)code.content.data(), code.content.size() * sizeof(uint32_t));
    return file.good() ? 0 : -1;
}

#ifdef VKU_HAS_NEW_GLSLANG

inline vku_spirv_t vku_spirv_compile(vku_shader_stage_t vku_stage, const char *code) {
    VK_ASSERT(vku_init());
    DBG("NEW GLSLANG COMPILE");

    glslang_stage_t stage;
    switch (vku_stage) {
        case VKU_SPIRV_VERTEX:    stage = GLSLANG_STAGE_VERTEX;         break;
        case VKU_SPIRV_TESS_CTRL: stage = GLSLANG_STAGE_TESSCONTROL;    break;
        case VKU_SPIRV_TESS_EVAL: stage = GLSLANG_STAGE_TESSEVALUATION; break;
        case VKU_SPIRV_GEOMETRY:  stage = GLSLANG_STAGE_GEOMETRY;       break;
        case VKU_SPIRV_FRAGMENT:  stage = GLSLANG_STAGE_FRAGMENT;       break;
        case VKU_SPIRV_COMPUTE:   stage = GLSLANG_STAGE_COMPUTE;        break;
        default:
            DBG("Unknown shader stage type: %d", (uint32_t)vku_stage);
            throw vku_err_t(VK_ERROR_UNKNOWN);
    }

    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = code,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = &vku_spirv_resources,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        DBG("GLSL preprocessing failed \n");
        DBG("info_log: %s", glslang_shader_get_info_log(shader));
        DBG("debug_log: %s", glslang_shader_get_info_debug_log(shader));
        DBG("source_code: %s", input.code);
        glslang_shader_delete(shader);
        throw vku_err_t("GLSL preprocessing failed");
    }

    if (!glslang_shader_parse(shader, &input)) {
        DBG("GLSL parsing failed");
        DBG("%s", glslang_shader_get_info_log(shader));
        DBG("%s", glslang_shader_get_info_debug_log(shader));
        DBG("%s", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        throw vku_err_t("GLSL parsing failed");
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        DBG("%s", glslang_program_get_info_log(program));
        DBG("%s", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        throw vku_err_t("GLSL linking failed");
    }

    glslang_program_SPIRV_generate(program, stage);

    int size = glslang_program_SPIRV_get_size(program);
    vku_spirv_t ret;
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

inline void vku_spirv_init() {
    /* TODO */
    if (!glslang_initialize_process()) {
        throw vku_err_t("Failed glslang_initialize_process");
    }

    vku_spirv_resources.max_lights = 32;
    vku_spirv_resources.max_clip_planes = 6;
    vku_spirv_resources.max_texture_units = 32;
    vku_spirv_resources.max_texture_coords = 32;
    vku_spirv_resources.max_vertex_attribs = 64;
    vku_spirv_resources.max_vertex_uniform_components = 4096;
    vku_spirv_resources.max_varying_floats = 64;
    vku_spirv_resources.max_vertex_texture_image_units = 32;
    vku_spirv_resources.max_combined_texture_image_units = 80;
    vku_spirv_resources.max_texture_image_units = 32;
    vku_spirv_resources.max_fragment_uniform_components = 4096;
    vku_spirv_resources.max_draw_buffers = 32;
    vku_spirv_resources.max_vertex_uniform_vectors = 128;
    vku_spirv_resources.max_varying_vectors = 8;
    vku_spirv_resources.max_fragment_uniform_vectors = 16;
    vku_spirv_resources.max_vertex_output_vectors = 16;
    vku_spirv_resources.max_fragment_input_vectors = 15;
    vku_spirv_resources.min_program_texel_offset = -8;
    vku_spirv_resources.max_program_texel_offset = 7;
    vku_spirv_resources.max_clip_distances = 8;
    vku_spirv_resources.max_compute_work_group_count_x = 65535;
    vku_spirv_resources.max_compute_work_group_count_y = 65535;
    vku_spirv_resources.max_compute_work_group_count_z = 65535;
    vku_spirv_resources.max_compute_work_group_size_x = 1024;
    vku_spirv_resources.max_compute_work_group_size_y = 1024;
    vku_spirv_resources.max_compute_work_group_size_z = 64;
    vku_spirv_resources.max_compute_uniform_components = 1024;
    vku_spirv_resources.max_compute_texture_image_units = 16;
    vku_spirv_resources.max_compute_image_uniforms = 8;
    vku_spirv_resources.max_compute_atomic_counters = 8;
    vku_spirv_resources.max_compute_atomic_counter_buffers = 1;
    vku_spirv_resources.max_varying_components = 60;
    vku_spirv_resources.max_vertex_output_components = 64;
    vku_spirv_resources.max_geometry_input_components = 64;
    vku_spirv_resources.max_geometry_output_components = 128;
    vku_spirv_resources.max_fragment_input_components = 128;
    vku_spirv_resources.max_image_units = 8;
    vku_spirv_resources.max_combined_image_units_and_fragment_outputs = 8;
    vku_spirv_resources.max_combined_shader_output_resources = 8;
    vku_spirv_resources.max_image_samples = 0;
    vku_spirv_resources.max_vertex_image_uniforms = 0;
    vku_spirv_resources.max_tess_control_image_uniforms = 0;
    vku_spirv_resources.max_tess_evaluation_image_uniforms = 0;
    vku_spirv_resources.max_geometry_image_uniforms = 0;
    vku_spirv_resources.max_fragment_image_uniforms = 8;
    vku_spirv_resources.max_combined_image_uniforms = 8;
    vku_spirv_resources.max_geometry_texture_image_units = 16;
    vku_spirv_resources.max_geometry_output_vertices = 256;
    vku_spirv_resources.max_geometry_total_output_components = 1024;
    vku_spirv_resources.max_geometry_uniform_components = 1024;
    vku_spirv_resources.max_geometry_varying_components = 64;
    vku_spirv_resources.max_tess_control_input_components = 128;
    vku_spirv_resources.max_tess_control_output_components = 128;
    vku_spirv_resources.max_tess_control_texture_image_units = 16;
    vku_spirv_resources.max_tess_control_uniform_components = 1024;
    vku_spirv_resources.max_tess_control_total_output_components = 4096;
    vku_spirv_resources.max_tess_evaluation_input_components = 128;
    vku_spirv_resources.max_tess_evaluation_output_components = 128;
    vku_spirv_resources.max_tess_evaluation_texture_image_units = 16;
    vku_spirv_resources.max_tess_evaluation_uniform_components = 1024;
    vku_spirv_resources.max_tess_patch_components = 120;
    vku_spirv_resources.max_patch_vertices = 32;
    vku_spirv_resources.max_tess_gen_level = 64;
    vku_spirv_resources.max_viewports = 16;
    vku_spirv_resources.max_vertex_atomic_counters = 0;
    vku_spirv_resources.max_tess_control_atomic_counters = 0;
    vku_spirv_resources.max_tess_evaluation_atomic_counters = 0;
    vku_spirv_resources.max_geometry_atomic_counters = 0;
    vku_spirv_resources.max_fragment_atomic_counters = 8;
    vku_spirv_resources.max_combined_atomic_counters = 8;
    vku_spirv_resources.max_atomic_counter_bindings = 1;
    vku_spirv_resources.max_vertex_atomic_counter_buffers = 0;
    vku_spirv_resources.max_tess_control_atomic_counter_buffers = 0;
    vku_spirv_resources.max_tess_evaluation_atomic_counter_buffers = 0;
    vku_spirv_resources.max_geometry_atomic_counter_buffers = 0;
    vku_spirv_resources.max_fragment_atomic_counter_buffers = 1;
    vku_spirv_resources.max_combined_atomic_counter_buffers = 1;
    vku_spirv_resources.max_atomic_counter_buffer_size = 16384;
    vku_spirv_resources.max_transform_feedback_buffers = 4;
    vku_spirv_resources.max_transform_feedback_interleaved_components = 64;
    vku_spirv_resources.max_cull_distances = 8;
    vku_spirv_resources.max_combined_clip_and_cull_distances = 8;
    vku_spirv_resources.max_samples = 4;
    vku_spirv_resources.max_mesh_output_vertices_nv = 256;
    vku_spirv_resources.max_mesh_output_primitives_nv = 512;
    vku_spirv_resources.max_mesh_work_group_size_x_nv = 32;
    vku_spirv_resources.max_mesh_work_group_size_y_nv = 1;
    vku_spirv_resources.max_mesh_work_group_size_z_nv = 1;
    vku_spirv_resources.max_task_work_group_size_x_nv = 32;
    vku_spirv_resources.max_task_work_group_size_y_nv = 1;
    vku_spirv_resources.max_task_work_group_size_z_nv = 1;
    vku_spirv_resources.max_mesh_view_count_nv = 4;
    vku_spirv_resources.limits.non_inductive_for_loops = 1;
    vku_spirv_resources.limits.while_loops = 1;
    vku_spirv_resources.limits.do_while_loops = 1;
    vku_spirv_resources.limits.general_uniform_indexing = 1;
    vku_spirv_resources.limits.general_attribute_matrix_vector_indexing = 1;
    vku_spirv_resources.limits.general_varying_indexing = 1;
    vku_spirv_resources.limits.general_sampler_indexing = 1;
    vku_spirv_resources.limits.general_variable_indexing = 1;
    vku_spirv_resources.limits.general_constant_matrix_vector_indexing = 1;
    vku_spirv_resources.maxDualSourceDrawBuffersEXT = 1;
}

inline void vku_spirv_uninit() {
    glslang_finalize_process();
}

#else /* VKU_HAS_NEW_GLSLANG */

inline vku_spirv_t vku_spirv_compile(vku_shader_stage_t vku_stage, const char *code) {
    VK_ASSERT(vku_init());

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

#endif /* VKU_HAS_NEW_GLSLANG */

inline uint32_t vku_find_memory_type(vku_ref_p<vku_device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(dev->get()->vk_phy_dev, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if (type_filter & (1 << i) && 
                (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    DBG("Couldn't find suitable memory type");
    throw vku_err_t(VK_ERROR_UNKNOWN);
}

inline const char *vk_err_str(VkResult res) {
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

} /* namespace vku_utils */

#endif
