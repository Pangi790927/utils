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
#include "demangle.h"
#include "cpp_backtrace.h"

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
    VkResult vk_err = (fn_call);                                                                   \
    if (vk_err != VK_SUCCESS) {                                                                    \
        DBG("Failed vk assert: [%s: %d]", vk_err_str(vk_err), vk_err);                             \
        throw vku_utils::err_t(vk_err);                                                            \
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

/* This is a common type enumeration for all the types that can be derived from vku_object_t */
enum vku_object_type_e {

    /* Those are the types from this file: */
    VKU_TYPE_OBJECT = 0, /* object_t is pure virtual, so no object should have this type */
    VKU_TYPE_WINDOW,
    VKU_TYPE_INSTANCE,
    VKU_TYPE_SURFACE,
    VKU_TYPE_DEVICE,
    VKU_TYPE_SWAPCHAIN,
    VKU_TYPE_SHADER,
    VKU_TYPE_RENDERPASS,
    VKU_TYPE_PIPELINE,
    VKU_TYPE_COMPUTE_PIPELINE,
    VKU_TYPE_FRAMEBUFFERS,
    VKU_TYPE_COMMAND_POOL,
    VKU_TYPE_COMMAND_BUFFER,
    VKU_TYPE_SEMAPHORE,
    VKU_TYPE_FENCE,
    VKU_TYPE_BUFFER,
    VKU_TYPE_IMAGE,
    VKU_TYPE_IMAGE_VIEW,
    VKU_TYPE_IMAGE_SAMPLER,
    VKU_TYPE_DESCRIPTOR_SET,
    VKU_TYPE_DESCRIPTOR_POOL,
    VKU_TYPE_SAMPLER_BINDING,
    VKU_TYPE_BUFFER_BINDING,
    VKU_TYPE_BINDING_DESCRIPTOR_SET,

    /* Those are the types from the vulkan composer file: */
    VKC_TYPE_SPIRV,
    VKC_TYPE_STRING,
    VKC_TYPE_FLOAT,
    VKC_TYPE_CPU_BUFFER,
    VKC_TYPE_INTEGER,
    VKC_TYPE_LUA_SCRIPT,
    VKC_TYPE_LUA_VARIABLE,
    VKC_TYPE_VERTEX_INPUT_DESC,
    VKC_TYPE_BINDING_DESC,

    /* Total number of different types */
    VKU_TYPE_CNT,
};

namespace vku_utils {

/* TODO:
    - Add logs for all the creations/deletions of objects with type and id(ptr)
    - Continue the tutorial: https://vulkan-tutorial.com/Uniform_buffers/Descriptor_layout_and_buffer
    - Add compute shaders and compute things
    - Create an ImGui backend using this helper
    - Add the "#include ..." macro for shaders and test if the rest work as expected
    - Add the option to use multiple include dirs for shader compilation
 */

struct err_t;
struct gpu_family_ids_t;
struct spirv_t;

struct vertex_input_desc_t;
struct vertex_p2n0c3t2_t;
struct vertex_p3n3c3t2_t;

struct binding_desc_set_t;
struct mvp_t;
struct ubo_t;
struct ssbo_t;

template <typename VkuT>
struct ref_t;

struct object_t;
struct instance_t;      /* uses (opts) */
struct surface_t;       /* uses (instance) */
struct device_t;        /* uses (surface) */
struct swapchain_t;     /* uses (device) */
struct shader_t;        /* uses (device, shader_data) */
struct renderpass_t;    /* uses (swapchain) */
struct pipeline_t;      /* uses (opts, renderpass, shaders, vertex_input_desc)*/
struct framebuffs_t;    /* uses (renderpass) */
struct cmdpool_t;       /* uses (device) */
struct cmdbuff_t;       /* uses (cmdpool) */
struct sem_t;           /* uses (device) */
struct fence_t;         /* uses (device) */
struct buffer_t;        /* uses (device) */
struct image_t;         /* uses (device) */
struct img_view_t;      /* uses (imag) */
struct img_sampl_t;     /* uses (device) */
struct desc_pool_t;     /* uses (device, ?buff?, ?pipeline?) */
struct desc_set_t;      /* uses (desc_pool) */

inline VkResult init();
inline VkResult uninit();

inline void wait_fences(std::vector<ref_t<fence_t>> fences);
inline void reset_fences(std::vector<ref_t<fence_t>> fences);
inline void aquire_next_img(
        ref_t<swapchain_t> swc,
        ref_t<sem_t> sem,
        uint32_t *img_idx);

inline void submit_cmdbuff(
        std::vector<std::pair<ref_t<sem_t>, VkPipelineStageFlagBits>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<ref_t<sem_t>> sig_sems);

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

/* To string for own enums: */
inline std::string to_string(vku_shader_stage_e stage);
inline std::string to_string(vku_object_type_e type);

/* To string for own objects: */
template <typename T>
inline std::string to_string(ref_t<T> ref);
template <typename T>
inline std::string to_string(const object_t& ref);
inline std::string to_string(const vertex_input_desc_t& vid);

/* To string for external types */
inline std::string to_string(VkVertexInputRate rate);
inline std::string to_string(VkFormat format);
inline std::string to_string(VkPrimitiveTopology topol);
inline std::string to_string(VkSharingMode shmod);
inline std::string to_string(VkFilter shmod);
inline std::string to_string(VkDescriptorType dtype);
inline std::string to_string(const VkDescriptorSetLayoutBinding& bind);

/* To string for external flags */
/* OBS: You can't use those directly because the actual values are in the form VkFlags not
VkFlagBits */
inline std::string to_string(VkFenceCreateFlagBits flags);
inline std::string to_string(VkBufferUsageFlagBits flags);
inline std::string to_string(VkMemoryPropertyFlagBits flags);
inline std::string to_string(VkImageUsageFlagBits flags);
inline std::string to_string(VkImageAspectFlagBits flags);
inline std::string to_string(VkShaderStageFlagBits flags);

/* VKU Objects:
================================================================================================= */

/* Those are needed here just for bellow objects */
inline const char *vk_err_str(VkResult res);
inline std::string glfw_err();

struct err_t : public std::exception {
    VkResult vk_err{};
    std::string err_str;

    err_t(VkResult vk_err)
    : vk_err(vk_err)
    {
        err_str = std::format(
                "\n------BACKTRACE------\n"
                "{}"
                "EXCEPTION: VKU_ERROR: {}[{}]"
                "\n---------------------",
                cpp_backtrace(),
                vk_err_str(vk_err), (size_t)vk_err);
    }

    err_t(const std::string& str) {
        err_str = std::format(
                "\n------BACKTRACE------\n"
                "{}"
                "EXCEPTION: {}"
                "\n---------------------",
                cpp_backtrace(),
                str);
    }
    const char *what() const noexcept override { return err_str.c_str(); };
};


/*! Callbacks used by object_t for re-initializing the managed vulkan obeject */
struct object_cbks_t {
    std::shared_ptr<void> usr_ptr;

    /*! This function is called when an ref_t calls the object_t::init function, just before
     * init is called, with the object and usr_ptr as arguments. This call is made if
     * object_t::cbks is not null and if object_cbks_t::pre_init is also not null. */
    std::function<void(object_t *, std::shared_ptr<void> &)> pre_init;

    /*! Same as above, called after init. */
    std::function<void(object_t *, std::shared_ptr<void> &)> post_init;

    /*! Same as for init, but this time for the uninit function */
    std::function<void(object_t *, std::shared_ptr<void> &)> pre_uninit;

    /*! Same as above, called after uninit. */
    std::function<void(object_t *, std::shared_ptr<void> &)> post_uninit;
};

/*! This is virtual only for init/uninit, which need to describe how the object should be
 * initialized once it is created and it's parameters are filled */
struct object_t {
    virtual ~object_t() { _call_uninit(); }

    template <typename VkuT>
    friend struct ref_t;

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

    virtual vku_object_type_e type_id() const = 0;
    virtual std::string to_string() const = 0;

    std::shared_ptr<object_cbks_t> cbks;

private:
    virtual VkResult _init() = 0;
    virtual VkResult _uninit() { return VK_SUCCESS; };
};

/*!
 * The idea:
 * - No object is directly referenced, but they all are referenced by this reference. What this does
 * is it enables us to keep an internal representation of the vulkan data structures while also
 * letting us rebuild the internal object when needed. The vulkan structures will be rebuilt using
 * the last parameters that where used to build them.
 */
/* TODO: Think if it makes sense to implement a locking mechanism, especially for the rebuild stuff.

ref_t practically implements a DAG of dependencies, this means that to protect a node, all it's
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

struct node : public std::enable_shared_from_this<ref_base_t> {
    std::vector<std::weak_ptr<node>>  _dependees;
    // std::vector<node *>  _depends; //  <- maybe like this? And maybe only if locking is enabled?
    Data data;
};

*/

struct base_t;

struct ref_base_t {
    std::shared_ptr<base_t> _base;
};

struct base_t : public std::enable_shared_from_this<base_t> {
protected:
    /*! This is here to force the creation of references by create_obj, this makes sure */
    struct private_param_t { explicit private_param_t() = default; };

    std::unique_ptr<object_t>           _obj;
    std::vector<std::weak_ptr<base_t>>  _dependees;

public:
    template <typename VkuT>
    base_t(private_param_t, std::unique_ptr<VkuT> obj) : _obj(std::move(obj)) {}

    template <typename VkuT>
    friend struct ref_t;

    void rebuild() {
        /* TODO: maybe thread-protect this somehow? */
        _uninit_all();
        _init_all();
    }


    template <typename VkuT> requires std::derived_from<VkuT, object_t>
    static std::shared_ptr<base_t> create_obj_ref(
            std::unique_ptr<VkuT>       obj,
            std::vector<ref_base_t> dependencies)
    {
        auto ret = std::make_shared<base_t>(private_param_t{}, std::move(obj));
        for (auto &d : dependencies)
            d._base->_dependees.push_back(ret);
        return ret;
    }

private:
    void _clean_deps() {
        _dependees.erase(std::remove_if(_dependees.begin(), _dependees.end(), [](auto wp){
                return wp.expired(); }), _dependees.end());
    }

    void _uninit_all() {
        _clean_deps(); /* we lazy clear the deps whenever we want to iterate over them */
        for (auto wd : _dependees)
            wd.lock()->_uninit_all();
        VK_ASSERT(_obj->_call_uninit());
    }

    void _init_all() {
        VK_ASSERT(_obj->_call_init());
        for (auto wd : _dependees)
            wd.lock()->_init_all();
    }

};

/*! This holds a reference to an instance of VkuT, instance that is initiated and held by this
 * library. All objects and the user will use the instance via this reference. This is implemented
 * here because it has a small footprint and I consider making it visible would made the library
 * easier to use. */
template <typename VkuT>
class ref_t : public ref_base_t {
public:
    /* TODO: in constructor check that pointer is not null? */

    ref_t(std::nullptr_t) {}
    ref_t() {}
    ref_t(std::shared_ptr<base_t> obj) : ref_base_t{obj} {
        /* We either hold nullptr or an object that can be casted to VkuT */
        if (obj && !dynamic_cast<VkuT *>(_base->_obj.get())) {
            throw err_t{std::format("Tried to build a reference of invalid type {} to {}",
                    demangle(typeid(*_base->_obj.get()).name()),
                    demangle<VkuT>())};
        }
    }

    template <typename U> requires std::derived_from<U, object_t>
    ref_t(ref_t<U> oth) : ref_base_t{oth._base} {
        /* We either hold nullptr or an object that can be casted to VkuT */
        if (oth && !dynamic_cast<VkuT *>(_base->_obj.get())) {
            throw err_t{std::format("Tried to build a reference of invalid type {} to {}",
                    demangle(typeid(*_base->_obj.get()).name()),
                    demangle<VkuT>())};
        }
    }

    void rebuild() { _base->rebuild(); }

    ref_t &operator = (std::nullptr_t) { _base = nullptr; return *this; } 

    VkuT *operator ->() { return static_cast<VkuT *>(_base->_obj.get()); }
    const VkuT *operator ->() const { return static_cast<const VkuT *>(_base->_obj.get()); }

    VkuT &operator *() { return *static_cast<VkuT *>(_base->_obj.get()); }
    const VkuT &operator *() const { return *static_cast<const VkuT *>(_base->_obj.get()); }

    VkuT *get() { return static_cast<VkuT *>(_base->_obj.get()); }
    const VkuT *get() const { return static_cast<const VkuT *>(_base->_obj.get()); }

    template <typename VkuB> requires std::derived_from<VkuT, VkuB>
    ref_t<VkuB> to_base() { return ref_t<VkuB>{_base}; };

    template <typename VkuB> requires std::derived_from<VkuB, VkuT>
    ref_t<VkuB> to_derived() { return ref_t<VkuB>{_base}; };

    template <typename VkuB> requires std::derived_from<VkuB, VkuT> || std::derived_from<VkuT, VkuB>
    ref_t<VkuB> to_related() { return ref_t<VkuB>{_base}; };

    operator bool() { return !!_base; }
    friend bool operator == (std::nullptr_t, ref_t obj) { return obj._base == nullptr; } 
    friend bool operator == (ref_t obj, std::nullptr_t) { return obj._base == nullptr; } 

    static ref_t<VkuT> create_obj_ref(std::unique_ptr<VkuT> obj,
            std::vector<ref_base_t> dependencies)
    {
        return ref_t<VkuT>{base_t::create_obj_ref(std::move(obj), dependencies)};
    }
};

struct gpu_family_ids_t {
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

struct window_t : public object_t {
    /* Those can be modified at any time, but they need a rebuild to actually take effect (see
    ref_t::rebuild()) */
    std::string window_name;
    int width;
    int height;

    static ref_t<window_t> create(int width = 800, int height = 600,
            std::string name = "vku::window_name_placeholder");

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_WINDOW; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_WINDOW; }
    GLFWwindow *get_window() const { return _window; }

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;

    GLFWwindow *_window = NULL;
};

struct instance_t : public object_t {
    VkInstance                  vk_instance; /* TODO: figure out if those need getter */
    VkDebugUtilsMessengerEXT    vk_dbg_messenger;

    std::string                 app_name;
    std::string                 engine_name;
    std::vector<std::string>    extensions;
    std::vector<std::string>    layers;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_INSTANCE; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_INSTANCE; }
    static ref_t<instance_t> create(
            const std::string app_name = "vku::app_name_placeholder",
            const std::string engine_name = "vku::engine_name_placeholder",
            const std::vector<std::string>& extensions = { "VK_EXT_debug_utils" },
            const std::vector<std::string>& layers = { "VK_LAYER_KHRONOS_validation" });

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* VkSurfaceKHR */
struct surface_t : public object_t {
    VkSurfaceKHR                vk_surface = NULL;

    ref_t<window_t>     window;
    ref_t<instance_t>   inst;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_SURFACE; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_SURFACE; }
    static ref_t<surface_t> create(ref_t<window_t> window, ref_t<instance_t> inst);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct device_t : public object_t {
    VkPhysicalDevice    vk_phy_dev;
    VkDevice            vk_dev;
    VkQueue             vk_graphics_que;
    VkQueue             vk_present_que;
    std::set<int>       que_ids;
    gpu_family_ids_t    que_fams;

    ref_t<surface_t>    surf;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_DEVICE; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_DEVICE; }
    static ref_t<device_t> create(ref_t<surface_t> surf);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* VkSwapchainKHR */
struct swapchain_t : public object_t {
    VkSurfaceFormatKHR          vk_surf_fmt;
    VkPresentModeKHR            vk_present_mode;
    VkExtent2D                  vk_extent;
    VkSwapchainKHR              vk_swapchain;
    std::vector<VkImage>        vk_sc_images;
    std::vector<VkImageView>    vk_sc_image_views;
    ref_t<image_t>              depth_imag;
    ref_t<img_view_t>           depth_view;

    ref_t<device_t>             dev;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_SWAPCHAIN; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_SWAPCHAIN; }
    static ref_t<swapchain_t> create(ref_t<device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct shader_t : public object_t {
    VkShaderModule      vk_shader;

    bool                init_from_path; /* implicit param */
    ref_t<device_t>     dev;
    std::string         path = "not-initialized-from-path";
    spirv_t             spirv;
    vku_shader_stage_e  type;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_SHADER; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_SHADER; }
    /* not init from path */
    static ref_t<shader_t> create(ref_t<device_t> dev, const spirv_t& spirv);

    /* init from path */
    /* Obs: loads shader in binary format, i.e. already compiled */
    static ref_t<shader_t> create(ref_t<device_t> dev, const char *path, vku_shader_stage_e type);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct renderpass_t : public object_t {
    VkRenderPass        vk_render_pass;

    ref_t<swapchain_t>  swc;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_RENDERPASS; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_RENDERPASS; }
    static ref_t<renderpass_t> create(ref_t<swapchain_t> swc);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct pipeline_t : public object_t {
    VkPipeline                      vk_pipeline;
    VkPipelineLayout                vk_layout;
    VkDescriptorSetLayout           vk_desc_set_layout;

    int                             width;
    int                             height;
    ref_t<renderpass_t>             rp;
    std::vector<ref_t<shader_t>>    shaders;
    VkPrimitiveTopology             topology;
    vertex_input_desc_t             vid;
    ref_t<binding_desc_set_t>       bds;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_PIPELINE; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_PIPELINE; }
    static ref_t<pipeline_t> create(
            int                                 width,
            int                                 height,
            ref_t<renderpass_t>                 rp,
            const std::vector<ref_t<shader_t>>& shaders,
            VkPrimitiveTopology                 topology,
            vertex_input_desc_t                 vid,
            ref_t<binding_desc_set_t>           bd);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct compute_pipeline_t : public object_t {
    VkPipeline                  vk_pipeline;
    VkPipelineLayout            vk_layout;
    VkDescriptorSetLayout       vk_desc_set_layout;

    ref_t<device_t>             dev;
    ref_t<shader_t>             shader;
    ref_t<binding_desc_set_t>   bds;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_COMPUTE_PIPELINE; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_COMPUTE_PIPELINE; }
    static ref_t<compute_pipeline_t> create(
            ref_t<device_t>             dev,
            ref_t<shader_t>             shader,
            ref_t<binding_desc_set_t>   bds);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/*engine_create_framebuffs*/
struct framebuffs_t : public object_t {
    std::vector<VkFramebuffer>  vk_fbuffs;

    ref_t<renderpass_t>         rp;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_FRAMEBUFFERS; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_FRAMEBUFFERS; }
    static ref_t<framebuffs_t> create(ref_t<renderpass_t> rp);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/*engine_create_cmdpool*/
struct cmdpool_t : public object_t {
    VkCommandPool   vk_pool;

    ref_t<device_t> dev;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_COMMAND_POOL; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_COMMAND_POOL; }
    static ref_t<cmdpool_t> create(ref_t<device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct cmdbuff_t : public object_t {
    VkCommandBuffer     vk_buff;

    ref_t<cmdpool_t>    cp;
    bool                host_free;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_COMMAND_BUFFER; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_COMMAND_BUFFER; }
    static ref_t<cmdbuff_t> create(ref_t<cmdpool_t> cp, bool host_free = false);

    void begin(VkCommandBufferUsageFlags flags);
    void begin_rpass(ref_t<framebuffs_t> fbs, uint32_t img_idx);
    void bind_vert_buffs(uint32_t first_bind,
            std::vector<std::pair<ref_t<buffer_t>, VkDeviceSize>> buffs);
    void bind_desc_set(VkPipelineBindPoint bind_point, ref_t<pipeline_t> pl,
            ref_t<desc_set_t> desc_set);
    void bind_idx_buff(ref_t<buffer_t> ibuff, uint64_t off, VkIndexType idx_type);
    void draw(ref_t<pipeline_t> pl, uint64_t vert_cnt);
    void draw_idx(ref_t<pipeline_t> pl, uint64_t vert_cnt);
    void end_rpass();
    void end();

    void reset();

    void bind_compute(ref_t<compute_pipeline_t> cpl);
    void dispatch_compute(uint32_t x, uint32_t y = 1, uint32_t z = 1);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct sem_t : public object_t {
    ref_t<device_t> dev;
    VkSemaphore     vk_sem;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_SEMAPHORE; }
    virtual std::string to_string() const override;

    static ref_t<sem_t> create(ref_t<device_t> dev);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct fence_t : public object_t {
    ref_t<device_t>     dev;
    VkFence             vk_fence;

    VkFenceCreateFlags  flags;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_FENCE; }
    virtual std::string to_string() const override;

    static vku_object_type_e type_id_static() { return VKU_TYPE_FENCE; }
    static ref_t<fence_t> create(ref_t<device_t> dev, VkFenceCreateFlags flags = 0);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct buffer_t : public object_t {
    VkBuffer                vk_buff;
    VkDeviceMemory          vk_mem;
    void                    *map_ptr = nullptr;

    ref_t<device_t>         dev;
    size_t                  size;
    VkBufferUsageFlags      usage;
    VkSharingMode           sh_mode;
    VkMemoryPropertyFlags   mem_flags;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_BUFFER; }
    virtual std::string to_string() const override;

    static vku_object_type_e type_id_static() { return VKU_TYPE_BUFFER; }
    static ref_t<buffer_t> create(
            ref_t<device_t>         dev,
            size_t                  size,
            VkBufferUsageFlags      usage,
            VkSharingMode           sh_mode,
            VkMemoryPropertyFlags   mem_flags);

    void *map_data(VkDeviceSize offset, VkDeviceSize size);
    void unmap_data();

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};


struct image_t : public object_t {
    VkImage             vk_img;
    VkDeviceMemory      vk_img_mem;

    ref_t<device_t>     dev;
    uint32_t            width;
    uint32_t            height;
    VkFormat            fmt;
    VkImageUsageFlags   usage;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_IMAGE; }
    virtual std::string to_string() const override;

    static vku_object_type_e type_id_static() { return VKU_TYPE_IMAGE; }
    static ref_t<image_t> create(
            ref_t<device_t>     dev,
            uint32_t            width,
            uint32_t            height,
            VkFormat            fmt,
            VkImageUsageFlags   usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                      | VK_IMAGE_USAGE_SAMPLED_BIT);

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
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct img_view_t : public object_t {
    VkImageView         vk_view;
    ref_t<image_t>      img;
    VkImageAspectFlags  aspect_mask;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_IMAGE_VIEW; }
    virtual std::string to_string() const override;

    static vku_object_type_e type_id_static() { return VKU_TYPE_IMAGE_VIEW; }
    static ref_t<img_view_t> create(ref_t<image_t> img, VkImageAspectFlags aspect_mask);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct img_sampl_t : public object_t {
    VkSampler       vk_sampler;

    ref_t<device_t> dev;
    VkFilter        filter;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_IMAGE_SAMPLER; }
    virtual std::string to_string() const override;

    static vku_object_type_e type_id_static() { return VKU_TYPE_IMAGE_SAMPLER; }
    static ref_t<img_sampl_t> create(ref_t<device_t> dev, VkFilter filter = VK_FILTER_LINEAR);
    static VkDescriptorSetLayoutBinding get_desc_set(uint32_t binding, VkShaderStageFlags stage);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct desc_pool_t : public object_t {
    VkDescriptorPool            vk_descpool;

    ref_t<device_t>             dev;
    ref_t<binding_desc_set_t>   bds;
    uint32_t                    cnt;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_POOL; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_POOL; }
    static ref_t<desc_pool_t> create(
            ref_t<device_t>             dev,
            ref_t<binding_desc_set_t>   bds,
            uint32_t                    cnt);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct desc_set_t : public object_t {
    VkDescriptorSet             vk_desc_set;

    ref_t<desc_pool_t>          dp;
    ref_t<pipeline_t>           pl;
    ref_t<binding_desc_set_t>   bds;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_DESCRIPTOR_SET; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_DESCRIPTOR_SET; }
    static ref_t<desc_set_t> create(
            ref_t<desc_pool_t>          dp,
            ref_t<pipeline_t>           pl,
            ref_t<binding_desc_set_t>   bds);

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

struct binding_desc_set_t : public object_t {
    struct binding_desc_t : public object_t {
        VkDescriptorSetLayoutBinding desc;

        virtual ~binding_desc_t() {}
        virtual VkWriteDescriptorSet get_write() const = 0;

    private:
        virtual VkResult _init() = 0;
        virtual VkResult _uninit() override { return VK_SUCCESS; };
    };

    struct buff_binding_t : public binding_desc_t {
        VkDescriptorBufferInfo  desc_buff_info;

        ref_t<buffer_t>         buff;

        virtual VkWriteDescriptorSet get_write() const override;
        virtual vku_object_type_e type_id() const override { return VKU_TYPE_BUFFER_BINDING; }
        virtual std::string to_string() const override;

        static  vku_object_type_e type_id_static() { return VKU_TYPE_BUFFER_BINDING; }
        static ref_t<buff_binding_t> create(
                VkDescriptorSetLayoutBinding    desc,
                ref_t<buffer_t>                 buff);

    private:
        virtual VkResult _init() override;
    };

    struct sampl_binding_t : public binding_desc_t {
        VkDescriptorImageInfo   imag_info;

        ref_t<img_view_t>       view;
        ref_t<img_sampl_t>      sampl;

        virtual VkWriteDescriptorSet get_write() const override;
        virtual vku_object_type_e type_id() const override { return VKU_TYPE_SAMPLER_BINDING; }
        virtual std::string to_string() const;

        static  vku_object_type_e type_id_static() { return VKU_TYPE_SAMPLER_BINDING; }
        static ref_t<sampl_binding_t> create(
                VkDescriptorSetLayoutBinding    desc,
                ref_t<img_view_t>               view,
                ref_t<img_sampl_t>              sampl);

    private:
        virtual VkResult _init() override;
    };

    std::vector<ref_t<binding_desc_t>> binds;

    virtual vku_object_type_e type_id() const override { return VKU_TYPE_BINDING_DESCRIPTOR_SET; }
    virtual std::string to_string() const override;

    static  vku_object_type_e type_id_static() { return VKU_TYPE_BINDING_DESCRIPTOR_SET; }
    static ref_t<binding_desc_set_t> create(std::vector<ref_t<binding_desc_t>> binds);

    std::vector<VkWriteDescriptorSet> get_writes() const;
    std::vector<VkDescriptorSetLayoutBinding> get_descriptors() const;

private:
    virtual VkResult _init() override;
    virtual VkResult _uninit() override;
};

/* Internal:
================================================================================================= */

struct swapchain_details_t {
    VkSurfaceCapabilitiesKHR        capab;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   present_modes;
};

inline VkResult create_dbg_messenger(
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

#ifndef HAS_NEW_GLSLANG
inline TBuiltInResource spirv_resources = {};
#else
inline glslang_resource_t spirv_resources = {};
#endif

inline void spirv_uninit();
inline void spirv_init();
inline spirv_t spirv_compile(vku_shader_stage_e vk_stage, const char *code);
inline int spirv_save(const spirv_t& code, const char *filepath);

inline uint32_t find_memory_type(ref_t<device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties);

/* IMPLEMENTATION:
=================================================================================================
=================================================================================================
================================================================================================= */

inline bool init_state = false;
inline VkResult init() {
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

inline VkResult uninit() {
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

/* window_t
================================================================================================= */

inline VkResult window_t::_uninit() {
    if (_window)
        glfwDestroyWindow(_window);
    return VK_SUCCESS;
}

inline VkResult window_t::_init() {
    VK_ASSERT(init());
    _window = glfwCreateWindow(width, height, window_name.c_str(), NULL, NULL);
    if (!_window) {
        DBG("Failed to create a glfw window: %s", glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    return VK_SUCCESS;
}

inline ref_t<window_t> window_t::create(int width, int height, std::string name) {
    auto ret = ref_t<window_t>::create_obj_ref(std::make_unique<window_t>(), {});

    ret->window_name = name;
    ret->width = width;
    ret->height = height;

    VK_ASSERT(ret->_call_init());
    DBG("Done post init");
    return ret;
}

inline std::string window_t::to_string() const {
    return std::format("vku::window[{}]: m_width={}, m_height={}, m_window_name={}",
            (void*)this, width, height, window_name);
}

/* instance_t
================================================================================================= */

inline VkResult instance_t::_init() {
    FnScope err_scope;

    /* The instance can be used without a window, so it must also init vku if it was not already
    initialized */
    VK_ASSERT(init());

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
        throw err_t(VK_ERROR_UNKNOWN);
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
            throw err_t(VK_ERROR_UNKNOWN);
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
            throw err_t(VK_ERROR_UNKNOWN);
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
        .pfnUserCallback = dbg_cbk,
        .pUserData = nullptr,
    };

    VK_ASSERT(create_dbg_messenger(vk_instance, &dbg_info, NULL, &vk_dbg_messenger));
    err_scope([&]{ destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL); });

    err_scope.disable();
    DBG("Created vulkan messenger! %p", this);
    return VK_SUCCESS;
}

inline VkResult instance_t::_uninit() {
    destroy_dbg_messenger(vk_instance, vk_dbg_messenger, NULL);
    vkDestroyInstance(vk_instance, NULL);
    return VK_SUCCESS;
}

inline ref_t<instance_t> instance_t::create(
        const std::string app_name,
        const std::string engine_name,
        const std::vector<std::string>& extensions,
        const std::vector<std::string>& layers)
{
    auto ret = ref_t<instance_t>::create_obj_ref(std::make_unique<instance_t>(), {});

    ret->app_name = app_name;
    ret->engine_name = engine_name;
    ret->extensions = extensions;
    ret->layers = layers;

    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string instance_t::to_string() const {
    std::string exts = "[";
    std::string lays;
    for (auto ext: extensions)
        exts += ext + ", ";
    for (auto lay: layers)
        lays += lay + ", ";
    exts += "]";
    lays += "]";
    return std::format("vku::instance[{}]: m_app_name={}, m_engine_name={}, m_extensions={} "
            "m_layers={}",
            (void*)this, app_name, engine_name, exts, lays);
}

/* surface_t (TODO)
================================================================================================= */

inline VkResult surface_t::_init() {
    if (glfwCreateWindowSurface(inst->vk_instance, window->get_window(),
            NULL, &vk_surface) != VK_SUCCESS)
    {
        DBG("Failed to get vk_surface: %s", glfw_err().c_str());
        return VK_ERROR_UNKNOWN;
    }
    return VK_SUCCESS;
}
inline VkResult surface_t::_uninit() {
    vkDestroySurfaceKHR(inst->vk_instance, vk_surface, NULL);
    return VK_SUCCESS;
}
inline ref_t<surface_t> surface_t::create(
        ref_t<window_t> window,
        ref_t<instance_t> inst)
{
    auto ret = ref_t<surface_t>::create_obj_ref(
            std::make_unique<surface_t>(), {window, inst});
    ret->window = window;
    ret->inst = inst;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string surface_t::to_string() const {
    return std::format("vku::surface[{}]: m_window={}, m_instance={}",
            (void*)this, (void*)inst.get(), (void*)window.get());
}

/* device_t (TODO)
================================================================================================= */

inline VkResult device_t::_init() {
    uint32_t dev_cnt = 0;
    VK_ASSERT(vkEnumeratePhysicalDevices(surf->inst->vk_instance, &dev_cnt, NULL));

    if (dev_cnt == 0) {
        DBG("Failed to find a GPU with Vulkan support!")
        throw err_t(VK_ERROR_UNKNOWN);
    }

    std::vector<VkPhysicalDevice> devices(dev_cnt);
    VK_ASSERT(vkEnumeratePhysicalDevices(
            surf->inst->vk_instance, &dev_cnt, devices.data()));

    vk_phy_dev = VK_NULL_HANDLE;
    int max_score = -1;
    for (const auto &dev : devices) {
        int score = score_phydev(dev, surf->vk_surface);
        if (score < 0)
            continue;
        if (score > max_score) {
            max_score = score;
            vk_phy_dev = dev;
        }
    }
    if (max_score < 0) {
        DBG("Failed to get a suitable physical device");
        throw err_t(VK_ERROR_UNKNOWN);
    }

    que_fams = find_queue_families(vk_phy_dev, surf->vk_surface);
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
inline VkResult device_t::_uninit() {
    vkDestroyDevice(vk_dev, NULL);
    return VK_SUCCESS;
}
inline ref_t<device_t> device_t::create(ref_t<surface_t> surf) {
    auto ret = ref_t<device_t>::create_obj_ref(std::make_unique<device_t>(), {surf});
    ret->surf = surf;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string device_t::to_string() const {
    return std::format("vku::device[{}]: m_surface={}", (void*)this, (void*)surf.get());
}

/* swapchain_t (TODO)
================================================================================================= */

inline VkResult swapchain_t::_init() {
    FnScope err_scope;
    auto sc_detail = get_swapchain_details(dev->vk_phy_dev,
            dev->surf->vk_surface);

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
    vk_extent = choose_extent(dev->surf->window->get_window(),
            sc_detail.capab);

    /* choose image count */
    uint32_t img_cnt = sc_detail.capab.minImageCount + 1;
    if (sc_detail.capab.maxImageCount > 0 && img_cnt > sc_detail.capab.maxImageCount)
        img_cnt = sc_detail.capab.maxImageCount;

    std::vector<uint32_t> qf_arr = { dev->que_ids.begin(), dev->que_ids.end() };

    VkSwapchainCreateInfoKHR sc_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = dev->surf->vk_surface,
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

    VK_ASSERT(vkCreateSwapchainKHR(dev->vk_dev, &sc_info, NULL, &vk_swapchain));
    err_scope([&]{ vkDestroySwapchainKHR(dev->vk_dev, vk_swapchain, NULL); });

    img_cnt = 0;
    VK_ASSERT(vkGetSwapchainImagesKHR(dev->vk_dev, vk_swapchain, &img_cnt, NULL));
    vk_sc_images.resize(img_cnt);
    VK_ASSERT(vkGetSwapchainImagesKHR(dev->vk_dev, vk_swapchain, &img_cnt,
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
        VK_ASSERT(vkCreateImageView(dev->vk_dev, &iv_info, NULL, &vk_sc_image_views[i]));
        err_scope([i, this]{ 
            vkDestroyImageView(dev->vk_dev, vk_sc_image_views[i], NULL); });
    }

    /* TODO: this is problematic, here depth_imag is dependent on dev and not on swapchain, so
    if we delete dev we have a double free */
    depth_imag = image_t::create(dev, vk_extent.width, vk_extent.height, VK_FORMAT_D32_SFLOAT,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depth_view = img_view_t::create(depth_imag, VK_IMAGE_ASPECT_DEPTH_BIT);

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult swapchain_t::_uninit() {
    for (auto &iv : vk_sc_image_views)
        vkDestroyImageView(dev->vk_dev, iv, NULL);
    vkDestroySwapchainKHR(dev->vk_dev, vk_swapchain, NULL);
    return VK_SUCCESS;
}
inline ref_t<swapchain_t> swapchain_t::create(ref_t<device_t> dev) {
    auto ret = ref_t<swapchain_t>::create_obj_ref(
            std::make_unique<swapchain_t>(), {dev});
    ret->dev = dev;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string swapchain_t::to_string() const {
    return std::format("vku::swapchain[{}]: m_device={}", (void*)this, (void*)dev.get());
}

/* shader_t (TODO)
================================================================================================= */

inline VkResult shader_t::_init() {
    if (init_from_path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        std::streamsize size = file.tellg();

        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            DBG("Failed to read shader data");
            throw err_t(VK_ERROR_UNKNOWN);
        }
        VkShaderModuleCreateInfo shader_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .codeSize = buffer.size(),
            .pCode = (uint32_t *)buffer.data(),
        };
        VK_ASSERT(vkCreateShaderModule(dev->vk_dev, &shader_info, NULL, &vk_shader));
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
        VK_ASSERT(vkCreateShaderModule(dev->vk_dev, &shader_info, NULL, &vk_shader));
        DBG("Loaded shader from buffer of size: %zu data: %p -> vk_%p",
                spirv.content.size() * sizeof(uint32_t), spirv.content.data(), vk_shader);
    }
    return VK_SUCCESS;
}
inline VkResult shader_t::_uninit() {
    vkDestroyShaderModule(dev->vk_dev, vk_shader, NULL);
    return VK_SUCCESS;
}
inline ref_t<shader_t> shader_t::create(
        ref_t<device_t> dev,
        const spirv_t& spirv)
{
    auto ret = ref_t<shader_t>::create_obj_ref(std::make_unique<shader_t>(), {dev});
    ret->init_from_path = false;
    ret->dev = dev;
    ret->spirv = spirv;
    VK_ASSERT(ret->_call_init());
    return ret;
}
inline ref_t<shader_t> shader_t::create(
        ref_t<device_t> dev,
        const char *path,
        vku_shader_stage_e type)
{
    auto ret = ref_t<shader_t>::create_obj_ref(std::make_unique<shader_t>(), {dev});
    ret->init_from_path = true;
    ret->dev = dev;
    ret->path = path;
    ret->type = type;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string shader_t::to_string() const {
    return std::format("vku::shader[{}]: m_device={} m_type={} {}",
            (void*)this, (void*)dev.get(), vku_utils::to_string(type),
            init_from_path ? path : std::string("[Initialized from string, holds only spirv.]"));
}

/* renderpass_t (TODO)
================================================================================================= */

inline VkResult renderpass_t::_init() {
    VkAttachmentDescription color_attach {
        .flags = 0,
        .format = swc->vk_surf_fmt.format,
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
        .format = swc->depth_view->img->fmt,
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

    VK_ASSERT(vkCreateRenderPass(swc->dev->vk_dev,
            &render_pass_info, NULL, &vk_render_pass));

    return VK_SUCCESS;
}
inline VkResult renderpass_t::_uninit() {
    vkDestroyRenderPass(swc->dev->vk_dev, vk_render_pass, NULL);
    return VK_SUCCESS;
}
inline ref_t<renderpass_t> renderpass_t::create(ref_t<swapchain_t> swc) {
    auto ret = ref_t<renderpass_t>::create_obj_ref(
            std::make_unique<renderpass_t>(), {swc});
    ret->swc = swc;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string renderpass_t::to_string() const {
    return std::format("vku::renderpass[{}]: m_swapchain={}", (void*)this, (void*)swc.get());
}

/* pipeline_t (TODO)
================================================================================================= */

inline VkResult pipeline_t::_init() {
    FnScope err_scope;

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
    for (auto sh : shaders) {
        shader_stages.push_back(VkPipelineShaderStageCreateInfo {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0,
            .stage               = get_shader_type(sh->type),
            .module              = sh->vk_shader,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        });
        DBG("Added shader: %p, type: %x ",
                sh->vk_shader, get_shader_type(sh->type));
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
        .extent = rp->swc->vk_extent,
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

    auto bind_descriptors = bds->get_descriptors();
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

    VK_ASSERT(vkCreateDescriptorSetLayout(rp->swc->dev->vk_dev,
            &desc_set_layout_info, nullptr, &vk_desc_set_layout));
    err_scope([&]{ vkDestroyDescriptorSetLayout(
            rp->swc->dev->vk_dev, vk_desc_set_layout, nullptr); });
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
            rp->swc->dev->vk_dev, &pipeline_layout_info, NULL, &vk_layout));
    err_scope([&]{ vkDestroyPipelineLayout(rp->swc->dev->vk_dev,
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
        .renderPass             = rp->vk_render_pass,
        .subpass                = 0,
        .basePipelineHandle     = VK_NULL_HANDLE,
        .basePipelineIndex      = -1,
    };

    VK_ASSERT(vkCreateGraphicsPipelines(rp->swc->dev->vk_dev,
            VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    err_scope.disable();
    DBGVV("Allocated pipeline: %p", vk_pipeline);
    return VK_SUCCESS;
}
inline VkResult pipeline_t::_uninit() {
    DBGVV("Dealocating pipeline: %p", vk_pipeline);

    vkDestroyPipeline(rp->swc->dev->vk_dev, vk_pipeline, NULL);
    vkDestroyPipelineLayout(rp->swc->dev->vk_dev, vk_layout, NULL);
    vkDestroyDescriptorSetLayout(rp->swc->dev->vk_dev,
            vk_desc_set_layout, nullptr);
    return VK_SUCCESS;
}
inline ref_t<pipeline_t> pipeline_t::create(
        int width,
        int height,
        ref_t<renderpass_t> rp,
        const std::vector<ref_t<shader_t>> &shaders,
        VkPrimitiveTopology topology,
        vertex_input_desc_t vid,
        ref_t<binding_desc_set_t> bds)
{
    std::vector<ref_base_t> deps;
    for (auto sh : shaders)
        deps.push_back(sh);
    deps.push_back(rp);
    deps.push_back(bds);
    auto ret = ref_t<pipeline_t>::create_obj_ref(
            std::make_unique<pipeline_t>(), deps);
    ret->width = width;
    ret->height = height;
    ret->rp = rp;
    ret->shaders = shaders;
    ret->topology = topology;
    ret->vid = vid;
    ret->bds = bds;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string pipeline_t::to_string() const {
    std::string sh_str = "[";
    for (auto sh : shaders)
        sh_str += std::format("{}, ", (void*)sh.get());
    sh_str += "]";
    return std::format("vku::pipeline[{}]: m_width={} m_height={} m_renderpass={} m_shaders={} "
            "m_topology={} m_vertex_input_descriptor={} m_binding_desc_set={}",
            (void*)this, width, height, (void*)rp.get(), sh_str,
            vku_utils::to_string(topology), vku_utils::to_string(vid), (void*)bds.get());
}

/* compute_pipeline_t (TODO)
================================================================================================= */

inline VkResult compute_pipeline_t::_init() {
    FnScope err_scope;

    auto bind_descriptors = bds->get_descriptors();
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

    VK_ASSERT(vkCreateDescriptorSetLayout(dev->vk_dev, &desc_set_layout_info, nullptr,
            &vk_desc_set_layout));
    err_scope([&]{ vkDestroyDescriptorSetLayout(dev->vk_dev, vk_desc_set_layout, nullptr); });
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

    VK_ASSERT(vkCreatePipelineLayout(dev->vk_dev, &pipeline_layout_info, NULL, &vk_layout));
    err_scope([&]{ vkDestroyPipelineLayout(dev->vk_dev, vk_layout, NULL); });
    DBGVV("Allocated pipeline layout: %p", vk_layout);

    if (get_shader_type(shader->type) != VK_SHADER_STAGE_COMPUTE_BIT) {
        throw err_t("compute_pipeline needs a compute shader");
    }

    VkPipelineShaderStageCreateInfo shader_info {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .stage                  = get_shader_type(shader->type),
        .module                 = shader->vk_shader,
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
            dev->vk_dev, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &vk_pipeline));
    DBGVV("Allocated pipeline: %p", vk_pipeline);

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult compute_pipeline_t::_uninit() {
    DBGVV("Dealocating pipeline: %p", vk_pipeline);

    vkDestroyPipeline(dev->vk_dev, vk_pipeline, NULL);
    vkDestroyPipelineLayout(dev->vk_dev, vk_layout, NULL);
    vkDestroyDescriptorSetLayout(dev->vk_dev, vk_desc_set_layout, nullptr);
    return VK_SUCCESS;
}
inline ref_t<compute_pipeline_t> compute_pipeline_t::create(
        ref_t<device_t> dev,
        ref_t<shader_t> shader,
        ref_t<binding_desc_set_t> bds)
{
    auto ret = ref_t<compute_pipeline_t>::create_obj_ref(
            std::make_unique<compute_pipeline_t>(), {dev, shader, bds});
    ret->dev = dev;
    ret->shader = shader;
    ret->bds = bds;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string compute_pipeline_t::to_string() const {
    return std::format("vku::compute_pipeline[{}]: m_device={} m_shader={} m_binding_desc_set={}",
            (void*)this, (void*)dev.get(), (void*)shader.get(), (void*)bds.get());
}

/* framebuffs_t (TODO)
================================================================================================= */

inline VkResult framebuffs_t::_init() {
    vk_fbuffs.resize(rp->swc->vk_sc_image_views.size());

    FnScope err_scope;
    for (size_t i = 0; i < vk_fbuffs.size(); i++) {
        VkImageView attachs[] = {
            rp->swc->vk_sc_image_views[i],
            rp->swc->depth_view->vk_view
        };
        
        VkFramebufferCreateInfo fbuff_info {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = rp->vk_render_pass,
            .attachmentCount = 2,
            .pAttachments = attachs,
            .width = rp->swc->vk_extent.width,
            .height = rp->swc->vk_extent.height,
            .layers = 1,
        };

        VK_ASSERT(vkCreateFramebuffer(rp->swc->dev->vk_dev,
                &fbuff_info, NULL, &vk_fbuffs[i]));
        err_scope([this, i] { 
            vkDestroyFramebuffer(rp->swc->dev->vk_dev, vk_fbuffs[i], NULL);
        });
    }

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult framebuffs_t::_uninit() {
    for (auto fbuff : vk_fbuffs)
        vkDestroyFramebuffer(rp->swc->dev->vk_dev, fbuff, NULL);
    return VK_SUCCESS;
}
inline ref_t<framebuffs_t> framebuffs_t::create(ref_t<renderpass_t> rp){
    auto ret = ref_t<framebuffs_t>::create_obj_ref(
            std::make_unique<framebuffs_t>(), {rp});
    ret->rp = rp;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string framebuffs_t::to_string() const {
    return std::format("vku::framebuffs[{}]: m_renderpass={}", (void*)this, (void*)rp.get());
}

/* cmdpool_t (TODO)
================================================================================================= */

inline VkResult cmdpool_t::_init() {
    VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = (uint32_t)dev->que_fams.graphics_id,
    };

    VK_ASSERT(vkCreateCommandPool(dev->vk_dev, &pool_info, NULL, &vk_pool));
    return VK_SUCCESS;
}
inline VkResult cmdpool_t::_uninit() {
    vkDestroyCommandPool(dev->vk_dev, vk_pool, NULL);
    return VK_SUCCESS;
}
inline ref_t<cmdpool_t> cmdpool_t::create(ref_t<device_t> dev) {
    auto ret = ref_t<cmdpool_t>::create_obj_ref(
            std::make_unique<cmdpool_t>(), {dev});
    ret->dev = dev;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string cmdpool_t::to_string() const {
    return std::format("vku::cmdpool[{}]: m_device={}", (void*)this, (void*)dev.get());
}

/* cmdbuff_t (TODO)
================================================================================================= */

inline VkResult cmdbuff_t::_init() {
    VkCommandBufferAllocateInfo buff_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cp->vk_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VK_ASSERT(vkAllocateCommandBuffers(cp->dev->vk_dev, &buff_info, &vk_buff));
    return VK_SUCCESS;
}
inline VkResult cmdbuff_t::_uninit() {
    if (host_free) { /* TODO: what is with this host_free? */
        vkFreeCommandBuffers(cp->dev->vk_dev, cp->vk_pool, 1, &vk_buff);
    }
    return VK_SUCCESS;
}
inline ref_t<cmdbuff_t> cmdbuff_t::create(
        ref_t<cmdpool_t> cp, bool host_free)
{
    auto ret = ref_t<cmdbuff_t>::create_obj_ref(
            std::make_unique<cmdbuff_t>(), {cp});
    ret->cp = cp;
    ret->host_free = host_free;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string cmdbuff_t::to_string() const {
    return std::format("vku::cmdbuff[{}]: m_cmdpool={} host_free={}",
            (void*)this, (void*)cp.get(), host_free);
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
        .renderPass = fbs->rp->vk_render_pass,
        .framebuffer = fbs->vk_fbuffs[img_idx],
        .renderArea = {
            .offset = {0, 0},
            .extent = fbs->rp->swc->vk_extent,
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
        vk_buffs.push_back(b->vk_buff);
        vk_offsets.push_back(off);
    }

    vkCmdBindVertexBuffers(vk_buff, first_bind, vk_buffs.size(), vk_buffs.data(),
            vk_offsets.data());
}

inline void cmdbuff_t::bind_desc_set(VkPipelineBindPoint bind_point,
        ref_t<pipeline_t> pl, ref_t<desc_set_t> desc_set)
{
    DBGVVV("bind desc_set: %p with layout: %p bind_point: %d",
            desc_set->vk_desc_set, pl->vk_layout, bind_point);
    vkCmdBindDescriptorSets(vk_buff, bind_point, pl->vk_layout, 0, 1,
            &desc_set->vk_desc_set, 0, nullptr);
}

inline void cmdbuff_t::bind_idx_buff(ref_t<buffer_t> ibuff, uint64_t off,
        VkIndexType idx_type)
{
    vkCmdBindIndexBuffer(vk_buff, ibuff->vk_buff, off, idx_type);
}

inline void cmdbuff_draw_helper(VkCommandBuffer vk_buff, ref_t<pipeline_t> pl) {
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->vk_pipeline);

    VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = float(pl->rp->swc->vk_extent.width),
        .height = float(pl->rp->swc->vk_extent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    vkCmdSetViewport(vk_buff, 0, 1, &viewport);

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = pl->rp->swc->vk_extent
    };
    vkCmdSetScissor(vk_buff, 0, 1, &scissor);
}

inline void cmdbuff_t::draw(ref_t<pipeline_t> pl, uint64_t vert_cnt) {
    cmdbuff_draw_helper(vk_buff, pl);
    vkCmdDraw(vk_buff, vert_cnt, 1, 0, 0);
}

inline void cmdbuff_t::draw_idx(ref_t<pipeline_t> pl, uint64_t vert_cnt) {
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
    DBGVVV("bind compute pipeline: %p", cpl->vk_pipeline);
    vkCmdBindPipeline(vk_buff, VK_PIPELINE_BIND_POINT_COMPUTE, cpl->vk_pipeline);
}

inline void cmdbuff_t::dispatch_compute(uint32_t x, uint32_t y, uint32_t z) {
    vkCmdDispatch(vk_buff, x, y, z);
}

/* sem_t (TODO)
================================================================================================= */

inline VkResult sem_t::_init() {
    VkSemaphoreCreateInfo sem_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };

    VK_ASSERT(vkCreateSemaphore(dev->vk_dev, &sem_info, NULL, &vk_sem));
    return VK_SUCCESS;
}
inline VkResult sem_t::_uninit() {
    vkDestroySemaphore(dev->vk_dev, vk_sem, NULL);
    return VK_SUCCESS;
}
inline ref_t<sem_t> sem_t::create(ref_t<device_t> dev) {
    auto ret = ref_t<sem_t>::create_obj_ref(
            std::make_unique<sem_t>(), {dev});
    ret->dev = dev;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string sem_t::to_string() const {
    return std::format("vku::sem_t[{}]: m_device={}", (void*)this, (void*)dev.get());
}

/* fence_t (TODO)
================================================================================================= */

inline VkResult fence_t::_init() {
    VkFenceCreateInfo fence_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags
    };

    VK_ASSERT(vkCreateFence(dev->vk_dev, &fence_info, NULL, &vk_fence));
    return VK_SUCCESS;
}
inline VkResult fence_t::_uninit() {
    vkDestroyFence(dev->vk_dev, vk_fence, NULL);
    return VK_SUCCESS;
}
inline ref_t<fence_t> fence_t::create(
        ref_t<device_t> dev,
        VkFenceCreateFlags flags)
{
    auto ret = ref_t<fence_t>::create_obj_ref(
            std::make_unique<fence_t>(), {dev});
    ret->dev = dev;
    ret->flags = flags;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string fence_t::to_string() const {
    return std::format("vku::fence_t[{}]: m_device={} m_flags={}",
            (void*)this, (void*)dev.get(), vku_utils::to_string((VkFenceCreateFlagBits)flags));
}

/* buffer_t (TODO)
================================================================================================= */

inline VkResult buffer_t::_init() {
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

    VK_ASSERT(vkCreateBuffer(dev->vk_dev, &buff_info, nullptr, &vk_buff));

    VkMemoryRequirements mem_req;
    vkGetBufferMemoryRequirements(dev->vk_dev, vk_buff, &mem_req);

    VkMemoryAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(dev, mem_req.memoryTypeBits, mem_flags)
    };

    VK_ASSERT(vkAllocateMemory(dev->vk_dev, &alloc_info, nullptr, &vk_mem));
    VK_ASSERT(vkBindBufferMemory(dev->vk_dev, vk_buff, vk_mem, 0));
    return VK_SUCCESS;
}
inline VkResult buffer_t::_uninit() {
    if (map_ptr)
        unmap_data();
    vkDestroyBuffer(dev->vk_dev, vk_buff, nullptr);
    vkFreeMemory(dev->vk_dev, vk_mem, nullptr);
    return VK_SUCCESS;
}
inline ref_t<buffer_t> buffer_t::create(
        ref_t<device_t> dev,
        size_t size,
        VkBufferUsageFlags usage,
        VkSharingMode sh_mode,
        VkMemoryPropertyFlags mem_flags)
{
    auto ret = ref_t<buffer_t>::create_obj_ref(
            std::make_unique<buffer_t>(), {dev});
    ret->dev = dev;
    ret->size = size;
    ret->usage = usage;
    ret->sh_mode = sh_mode;
    ret->mem_flags = mem_flags;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string buffer_t::to_string() const {
    return std::format("vku::buffer_t[{}]: m_device={} m_size={} m_usage={} m_share_mode={} "
            "m_mem_flags={}",
            (void*)this, (void*)dev.get(), size,
            vku_utils::to_string((VkBufferUsageFlagBits)usage),
            vku_utils::to_string(sh_mode),
            vku_utils::to_string((VkMemoryPropertyFlagBits)mem_flags));
}

inline void *buffer_t::map_data(VkDeviceSize offset, VkDeviceSize size) {
    if (map_ptr) {
        DBG("Memory is already mapped!");
        throw err_t(VK_ERROR_UNKNOWN);
    }
    VK_ASSERT(vkMapMemory(dev->vk_dev, vk_mem, offset, size, 0, &map_ptr));
    return map_ptr;
}

inline void buffer_t::unmap_data() {
    if (!map_ptr) {
        DBG("Memory is not mapped, can't unmap");
        throw err_t(VK_ERROR_UNKNOWN);
    }
    vkUnmapMemory(dev->vk_dev, vk_mem);
    map_ptr = nullptr;
}

/* image_t (TODO)
================================================================================================= */

inline VkResult image_t::_init() {
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

    VK_ASSERT(vkCreateImage(dev->vk_dev, &image_info, nullptr, &vk_img));
    FnScope err_scope([&]{ vkDestroyImage(dev->vk_dev, vk_img, nullptr); });

    VkMemoryRequirements mem_req;
    vkGetImageMemoryRequirements(dev->vk_dev, vk_img, &mem_req);

    VkMemoryAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = mem_req.size,
        .memoryTypeIndex = find_memory_type(dev, mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VK_ASSERT(vkAllocateMemory(dev->vk_dev, &alloc_info, nullptr, &vk_img_mem));
    VK_ASSERT(vkBindImageMemory(dev->vk_dev, vk_img, vk_img_mem, 0));

    err_scope.disable();
    return VK_SUCCESS;
}
inline VkResult image_t::_uninit() {
    vkDestroyImage(dev->vk_dev, vk_img, nullptr);
    vkFreeMemory(dev->vk_dev, vk_img_mem, nullptr);
    return VK_SUCCESS;
}
inline ref_t<image_t> image_t::create(
        ref_t<device_t> dev,
        uint32_t width,
        uint32_t height,
        VkFormat fmt,
        VkImageUsageFlags usage)
{
    auto ret = ref_t<image_t>::create_obj_ref(std::make_unique<image_t>(), {dev});
    ret->dev = dev;
    ret->width = width;
    ret->height = height;
    ret->fmt = fmt;
    ret->usage = usage;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string image_t::to_string() const {
    return std::format("vku::sem_t[{}]: m_device={} m_width={} m_height={} m_format={} m_usage={}",
            (void*)this, (void*)dev.get(), width, height, vku_utils::to_string(fmt),
            vku_utils::to_string((VkImageUsageFlagBits)usage));
}

inline void image_t::transition_layout(ref_t<cmdpool_t> cp,
        VkImageLayout old_layout, VkImageLayout new_layout, ref_t<cmdbuff_t> cbuff)
{
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = fence_t::create(cp->dev);

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
        throw err_t(sformat("unsupported layout transition for image! %d -> %d",
                old_layout, new_layout));
    }

    /* TODO: don't we need a transition from shader_read to transfer_dst? That for transfering
    inside the image later on? */

    vkCmdPipelineBarrier(cbuff->vk_buff, src_stage, dst_stage,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

    if (!existing_cbuff) {
        cbuff->end();

        submit_cmdbuff({}, cbuff, fence, {});
        wait_fences({fence});
    }
}

inline void image_t::set_data(ref_t<cmdpool_t> cp, void *data, uint32_t sz,
        ref_t<cmdbuff_t> cbuff)
{
    uint32_t img_sz = width * height * 4;

    if (img_sz != sz)
        throw err_t(sformat("data size(%d) does not match with image size(%d)", sz, img_sz));

    auto buff = buffer_t::create(
        cp->dev,
        img_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    memcpy(buff->map_data(0, img_sz), data, img_sz);
    buff->unmap_data();

    transition_layout(cp, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cbuff);

    auto fence = fence_t::create(cp->dev);

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
            .width = width,
            .height = height,
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
        wait_fences({fence});
    }
}


/* img_view_t (TODO)
================================================================================================= */

inline VkResult img_view_t::_init() {
    VkImageViewCreateInfo view_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .image = img->vk_img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = img->fmt,
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

    VK_ASSERT(vkCreateImageView(img->dev->vk_dev, &view_info, nullptr, &vk_view));
    return VK_SUCCESS;
}
inline VkResult img_view_t::_uninit() {
    vkDestroyImageView(img->dev->vk_dev, vk_view, nullptr);
    return VK_SUCCESS;
}
inline ref_t<img_view_t> img_view_t::create(
        ref_t<image_t> img,
        VkImageAspectFlags aspect_mask)
{
    auto ret = ref_t<img_view_t>::create_obj_ref(std::make_unique<img_view_t>(), {img});
    ret->img = img;
    ret->aspect_mask = aspect_mask;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string img_view_t::to_string() const {
    return std::format("vku::img_view_t[{}]: m_image={}", (void*)this, (void*)img.get(),
            vku_utils::to_string((VkImageAspectFlagBits)aspect_mask));
}

/* img_sampl_t (TODO)
================================================================================================= */

inline VkResult img_sampl_t::_init() {
    VkPhysicalDeviceProperties dev_props;
    vkGetPhysicalDeviceProperties(dev->vk_phy_dev, &dev_props);

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

    VK_ASSERT(vkCreateSampler(dev->vk_dev, &sampler_info, nullptr, &vk_sampler));
    return VK_SUCCESS;
}
inline VkResult img_sampl_t::_uninit() {
    vkDestroySampler(dev->vk_dev, vk_sampler, nullptr);
    return VK_SUCCESS;
}
inline ref_t<img_sampl_t> img_sampl_t::create(
        ref_t<device_t> dev,
        VkFilter filter)
{
    auto ret = ref_t<img_sampl_t>::create_obj_ref(std::make_unique<img_sampl_t>(), {dev});
    ret->dev = dev;
    ret->filter = filter;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string img_sampl_t::to_string() const {
    return std::format("vku::img_sampl[{}]: m_device={} m_filter={}", (void*)this, (void*)dev.get(),
            vku_utils::to_string(filter));
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

/* desc_pool_t (TODO)
================================================================================================= */

inline VkResult desc_pool_t::_init() {
    std::vector<VkDescriptorPoolSize> pool_sizes;
    std::map<decltype(bds->binds[0]->desc.descriptorType), uint32_t> type_cnt;
    for (auto &b : bds->binds)
        type_cnt[b->desc.descriptorType] += cnt;

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

    VK_ASSERT(vkCreateDescriptorPool(dev->vk_dev, &pool_info, nullptr, &vk_descpool));
    DBGVV("Allocated pool: %p", vk_descpool);
    return VK_SUCCESS;
}
inline VkResult desc_pool_t::_uninit() {
    vkDestroyDescriptorPool(dev->vk_dev, vk_descpool, nullptr);
    return VK_SUCCESS;
}
inline ref_t<desc_pool_t> desc_pool_t::create(
        ref_t<device_t> dev,
        ref_t<binding_desc_set_t> bds,
        uint32_t cnt)
{
    auto ret = ref_t<desc_pool_t>::create_obj_ref(
            std::make_unique<desc_pool_t>(), {dev, bds});
    ret->dev = dev;
    ret->bds = bds;
    ret->cnt = cnt;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string desc_pool_t::to_string() const {
    return std::format("vku::desc_pool[{}]: m_device={} m_binding_desc_set={} m_cnt={}",
            (void*)this, (void*)dev.get(), (void*)bds.get(), cnt);
}

/* desc_set_t (TODO)
================================================================================================= */

/* TODO:

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

inline VkResult desc_set_t::_init() {
    VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = dp->vk_descpool,
        .descriptorSetCount = 1,
        .pSetLayouts = &pl->vk_desc_set_layout,
    };

    VK_ASSERT(vkAllocateDescriptorSets(dp->dev->vk_dev, &alloc_info, &vk_desc_set));
    DBGVV("Allocated descriptor set: %p from pool: %p with layout: %p",
            vk_desc_set, dp->vk_descpool, pl->vk_desc_set_layout);

    /* TODO: this sucks, it references the buffer, but doesn't have a mechanism to do something
    if the buffer is freed without it's knowledge. So the buffer and descriptor set must
    match in size, but the buffer doesn't know that, that's not ok. */

    auto desc_writes = bds->get_writes();
    for (auto &dw : desc_writes)
        dw.dstSet = vk_desc_set;

    DBG("writes: %zu", desc_writes.size());
    for (auto &w : desc_writes) {
        DBG("write: type: %s, bind: %d, dst_set: %p .pBufferInfo: %p",
                vku_utils::to_string(w.descriptorType).c_str(), w.dstBinding, w.dstSet,
                w.pBufferInfo);
    }

    vkUpdateDescriptorSets(dp->dev->vk_dev, (uint32_t)desc_writes.size(),
            desc_writes.data(), 0, nullptr);

    return VK_SUCCESS;
}
inline VkResult desc_set_t::_uninit() {
    return VK_SUCCESS;
}
inline ref_t<desc_set_t> desc_set_t::create(
        ref_t<desc_pool_t> dp,
        ref_t<pipeline_t> pl,
        ref_t<binding_desc_set_t> bds)
{
    auto ret = ref_t<desc_set_t>::create_obj_ref(
            std::make_unique<desc_set_t>(), {dp, bds});
    ret->dp = dp;
    ret->pl = pl;
    ret->bds = bds;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string desc_set_t::to_string() const {
    return std::format("vku::desc_set[{}]: m_desc_pool={} m_pipeline={} m_binding_desc_set={}",
            (void*)this, (void*)dp.get(), (void*)pl.get(), (void*)bds.get());
}

/* binding_desc_t (TODO):
================================================================================================= */

inline ref_t<binding_desc_set_t::buff_binding_t> binding_desc_set_t::buff_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        ref_t<buffer_t> buff)
{
    using bd_t = binding_desc_set_t::buff_binding_t;
    ref_t<bd_t> ret = ref_t<bd_t>::create_obj_ref(std::make_unique<bd_t>(), {buff});
    ret->desc = desc;
    ret->buff = buff;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string binding_desc_set_t::buff_binding_t::to_string() const {
    return std::format("vku::binding_desc_set_t::buff_binding_t[{}]: m_desc={} m_buffer={}",
            (void*)this, vku_utils::to_string(desc), (void*)buff.get());
}

inline VkResult binding_desc_set_t::buff_binding_t::_init() {
    if (buff) {
        desc_buff_info = VkDescriptorBufferInfo {
            .buffer = buff->vk_buff,
            .offset = 0,
            .range = buff->size
        };
    }
    return VK_SUCCESS;
}

inline VkWriteDescriptorSet binding_desc_set_t::buff_binding_t::get_write() const {
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


inline ref_t<binding_desc_set_t::sampl_binding_t> binding_desc_set_t::sampl_binding_t::create(
        VkDescriptorSetLayoutBinding desc,
        ref_t<img_view_t> view,
        ref_t<img_sampl_t> sampl)
{
    using sb_t = binding_desc_set_t::sampl_binding_t;
    ref_t<sb_t> ret = ref_t<sb_t>::create_obj_ref(std::make_unique<sb_t>(), {view, sampl});
    ret->desc = desc;
    ret->view = view;
    ret->sampl = sampl;
    VK_ASSERT(ret->_call_init()); /* init does nothing */
    return ret;
}

inline std::string binding_desc_set_t::sampl_binding_t::to_string() const {
    return std::format("vku::binding_desc_set_t::sampl_binding_t[{}]: m_desc={} m_view={} "
            "m_sampler={}",
            (void*)this, vku_utils::to_string(desc), (void*)view.get(), (void*)sampl.get());
}

inline VkResult binding_desc_set_t::sampl_binding_t::_init() {
    if (view && sampl) {
        imag_info = VkDescriptorImageInfo {
            .sampler = sampl->vk_sampler,
            .imageView = view->vk_view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }
    return VK_SUCCESS;
}

inline VkWriteDescriptorSet binding_desc_set_t::sampl_binding_t::get_write() const {
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

inline VkResult binding_desc_set_t::_init() {
    return VK_SUCCESS;
}
inline VkResult binding_desc_set_t::_uninit() {
    return VK_SUCCESS;
}
inline ref_t<binding_desc_set_t> binding_desc_set_t::create(
        std::vector<ref_t<binding_desc_t>> binds)
{
    std::vector<ref_base_t> deps;
    for (auto b : binds)
        deps.push_back(b);
    auto ret = ref_t<binding_desc_set_t>::create_obj_ref(
            std::make_unique<binding_desc_set_t>(), deps);
    ret->binds = binds;
    VK_ASSERT(ret->_call_init());
    return ret;
}

inline std::string binding_desc_set_t::to_string() const {
    std::string binds_str = "[";
    for (auto b : binds)
        binds_str += std::format("{}, ", (void*)b.get());
    binds_str += "]";
    return std::format("vku::binding_desc_set_t[{}]: m_bindings={} ",
            (void*)this, binds_str);
}

inline std::vector<VkWriteDescriptorSet> binding_desc_set_t::get_writes() const {
    std::vector<VkWriteDescriptorSet> ret;

    for (auto &b : binds)
        ret.push_back(b->get_write());

    return ret;
}

inline std::vector<VkDescriptorSetLayoutBinding> binding_desc_set_t::get_descriptors() const {
    std::vector<VkDescriptorSetLayoutBinding> ret;
    for (auto &b : binds)
        ret.push_back(b->desc);
    return ret;
}


/* Functions: (TODO)
================================================================================================= */

inline void wait_fences(std::vector<ref_t<fence_t>> fences) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->vk_fence);

    VK_ASSERT(vkWaitForFences(fences[0]->dev->vk_dev,
            vk_fences.size(), vk_fences.data(), VK_TRUE, UINT64_MAX));
}

inline void reset_fences(std::vector<ref_t<fence_t>> fences) {
    std::vector<VkFence> vk_fences;

    if (!fences.size()) {
        DBG("No fences to wait for");
        throw err_t(VK_ERROR_UNKNOWN);
    }
    vk_fences.reserve(fences.size());
    for (auto f : fences)
        vk_fences.push_back(f->vk_fence);
    VK_ASSERT(vkResetFences(fences[0]->dev->vk_dev,
            vk_fences.size(), vk_fences.data()));
}

inline void aquire_next_img(ref_t<swapchain_t> swc, ref_t<sem_t> sem,
        uint32_t *img_idx)
{
    VK_ASSERT(vkAcquireNextImageKHR(swc->dev->vk_dev, swc->vk_swapchain,
            UINT64_MAX, sem->vk_sem, VK_NULL_HANDLE, img_idx));
}

inline void submit_cmdbuff(
        std::vector<std::pair<ref_t<sem_t>, VkPipelineStageFlagBits>> wait_sems,
        ref_t<cmdbuff_t> cbuff,
        ref_t<fence_t> fence,
        std::vector<ref_t<sem_t>> sig_sems)
{
    std::vector<VkPipelineStageFlags> vk_wait_stages;
    std::vector<VkSemaphore> vk_wait_sems;
    std::vector<VkSemaphore> vk_sig_sems;

    for (auto [s, wait_stage] : wait_sems) {
        vk_wait_stages.push_back((VkPipelineStageFlags)wait_stage);
        vk_wait_sems.push_back(s->vk_sem);
    }
    for (auto s : sig_sems) {
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

    VK_ASSERT(vkQueueSubmit(cbuff->cp->dev->vk_graphics_que, 1, &submit_info,
            fence == nullptr ? nullptr : fence->vk_fence));
}

inline void present(
        ref_t<swapchain_t> swc,
        std::vector<ref_t<sem_t>> wait_sems,
        uint32_t img_idx)
{
    std::vector<VkSemaphore> vk_wait_sems;

    for (auto s : wait_sems) {
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

    VK_ASSERT(vkQueuePresentKHR(swc->dev->vk_present_que, &pres_info));
}

inline void copy_buff(ref_t<cmdpool_t> cp, ref_t<buffer_t> dst,
        ref_t<buffer_t> src, VkDeviceSize sz, ref_t<cmdbuff_t> cbuff)
{
    bool existing_cbuff = true;
    if (!cbuff) {
        cbuff = cmdbuff_t::create(cp, true);
        existing_cbuff = false;
    }
    auto fence = fence_t::create(cp->dev);

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
        wait_fences({fence});
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

inline std::string to_string(vku_object_type_e type) {
    switch (type) {
        case VKU_TYPE_OBJECT: return "VKU_TYPE_OBJECT";
        case VKU_TYPE_WINDOW: return "VKU_TYPE_WINDOW";
        case VKU_TYPE_INSTANCE: return "VKU_TYPE_INSTANCE";
        case VKU_TYPE_SURFACE: return "VKU_TYPE_SURFACE";
        case VKU_TYPE_DEVICE: return "VKU_TYPE_DEVICE";
        case VKU_TYPE_SWAPCHAIN: return "VKU_TYPE_SWAPCHAIN";
        case VKU_TYPE_SHADER: return "VKU_TYPE_SHADER";
        case VKU_TYPE_RENDERPASS: return "VKU_TYPE_RENDERPASS";
        case VKU_TYPE_PIPELINE: return "VKU_TYPE_PIPELINE";
        case VKU_TYPE_COMPUTE_PIPELINE: return "VKU_TYPE_COMPUTE_PIPELINE";
        case VKU_TYPE_FRAMEBUFFERS: return "VKU_TYPE_FRAMEBUFFERS";
        case VKU_TYPE_COMMAND_POOL: return "VKU_TYPE_COMMAND_POOL";
        case VKU_TYPE_COMMAND_BUFFER: return "VKU_TYPE_COMMAND_BUFFER";
        case VKU_TYPE_SEMAPHORE: return "VKU_TYPE_SEMAPHORE";
        case VKU_TYPE_FENCE: return "VKU_TYPE_FENCE";
        case VKU_TYPE_BUFFER: return "VKU_TYPE_BUFFER";
        case VKU_TYPE_IMAGE: return "VKU_TYPE_IMAGE";
        case VKU_TYPE_IMAGE_VIEW: return "VKU_TYPE_IMAGE_VIEW";
        case VKU_TYPE_IMAGE_SAMPLER: return "VKU_TYPE_IMAGE_SAMPLER";
        case VKU_TYPE_DESCRIPTOR_SET: return "VKU_TYPE_DESCRIPTOR_SET";
        case VKU_TYPE_DESCRIPTOR_POOL: return "VKU_TYPE_DESCRIPTOR_POOL";
        case VKU_TYPE_SAMPLER_BINDING: return "VKU_TYPE_SAMPLER_BINDING";
        case VKU_TYPE_BUFFER_BINDING: return "VKU_TYPE_BUFFER_BINDING";
        case VKU_TYPE_BINDING_DESCRIPTOR_SET: return "VKU_TYPE_BINDING_DESCRIPTOR_SET";
        case VKC_TYPE_SPIRV: return "VKC_TYPE_SPIRV";
        case VKC_TYPE_STRING: return "VKC_TYPE_STRING";
        case VKC_TYPE_FLOAT: return "VKC_TYPE_FLOAT";
        case VKC_TYPE_CPU_BUFFER: return "VKC_TYPE_CPU_BUFFER";
        case VKC_TYPE_INTEGER: return "VKC_TYPE_INTEGER";
        case VKC_TYPE_LUA_SCRIPT: return "VKC_TYPE_LUA_SCRIPT";
        case VKC_TYPE_LUA_VARIABLE: return "VKC_TYPE_LUA_VARIABLE";
        case VKC_TYPE_VERTEX_INPUT_DESC: return "VKC_TYPE_VERTEX_INPUT_DESC";
        case VKC_TYPE_BINDING_DESC: return "VKC_TYPE_BINDING_DESC";
        case VKU_TYPE_CNT: return "VKC_INVALID_TYPE_CNT"; /* object can't be of this type */
    }
    return "VKC_TYPE_UNKNOWN";
}

template <typename T>
inline std::string to_string(ref_t<T> ref) {
    return "ref: " + ref->to_string();
}

template <typename T>
inline std::string to_string(const object_t& ref) {
    return ref.to_string();
}

inline std::string to_string(const vertex_input_desc_t& vid) {
    std::string ret = "vertex_input_desc_t{";
    ret += std::format(" .binding={}, .stride={}, input_rate={}, .attrs=[",
            vid.bind_desc.binding, vid.bind_desc.stride, to_string(vid.bind_desc.inputRate));
    for (auto &adesc : vid.attr_desc)
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

inline std::string to_string(const VkDescriptorSetLayoutBinding& bind) {
    return std::format("VkDescriptorSetLayoutBinding{{ .binding={} .type={} .count={} "
            ".stage_flags={} .immutable_samplers={} }}",
            bind.binding,
            vku_utils::to_string(bind.descriptorType),
            bind.descriptorCount,
            vku_utils::to_string(VkShaderStageFlagBits(bind.stageFlags)),
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
        ret += "VK_IMAGE_ASPECT_COLOR_BIT";
    if (flags & VK_IMAGE_ASPECT_DEPTH_BIT)
        ret += "VK_IMAGE_ASPECT_DEPTH_BIT";
    if (flags & VK_IMAGE_ASPECT_STENCIL_BIT)
        ret += "VK_IMAGE_ASPECT_STENCIL_BIT";
    if (flags & VK_IMAGE_ASPECT_METADATA_BIT)
        ret += "VK_IMAGE_ASPECT_METADATA_BIT";
    if (flags & VK_IMAGE_ASPECT_PLANE_0_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_0_BIT";
    if (flags & VK_IMAGE_ASPECT_PLANE_1_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_1_BIT";
    if (flags & VK_IMAGE_ASPECT_PLANE_2_BIT)
        ret += "VK_IMAGE_ASPECT_PLANE_2_BIT";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT";
    if (flags & VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT)
        ret += "VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT";
    if (flags & VK_IMAGE_ASPECT_PLANE_0_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_0_BIT_KHR";
    if (flags & VK_IMAGE_ASPECT_PLANE_1_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_1_BIT_KHR";
    if (flags & VK_IMAGE_ASPECT_PLANE_2_BIT_KHR)
        ret += "VK_IMAGE_ASPECT_PLANE_2_BIT_KHR";
    return ret + "]";
}

inline std::string to_string(VkShaderStageFlagBits flags) {
    std::string ret = "[";
    if (flags & VK_SHADER_STAGE_VERTEX_BIT)
        ret += "VK_SHADER_STAGE_VERTEX_BIT";
    if (flags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
        ret += "VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT";
    if (flags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
        ret += "VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT";
    if (flags & VK_SHADER_STAGE_GEOMETRY_BIT)
        ret += "VK_SHADER_STAGE_GEOMETRY_BIT";
    if (flags & VK_SHADER_STAGE_FRAGMENT_BIT)
        ret += "VK_SHADER_STAGE_FRAGMENT_BIT";
    if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
        ret += "VK_SHADER_STAGE_COMPUTE_BIT";
    if (flags & VK_SHADER_STAGE_ALL_GRAPHICS)
        ret += "VK_SHADER_STAGE_ALL_GRAPHICS";
    if (flags & VK_SHADER_STAGE_ALL)
        ret += "VK_SHADER_STAGE_ALL";
    if (flags & VK_SHADER_STAGE_RAYGEN_BIT_NV)
        ret += "VK_SHADER_STAGE_RAYGEN_BIT_NV";
    if (flags & VK_SHADER_STAGE_ANY_HIT_BIT_NV)
        ret += "VK_SHADER_STAGE_ANY_HIT_BIT_NV";
    if (flags & VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV)
        ret += "VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV";
    if (flags & VK_SHADER_STAGE_MISS_BIT_NV)
        ret += "VK_SHADER_STAGE_MISS_BIT_NV";
    if (flags & VK_SHADER_STAGE_INTERSECTION_BIT_NV)
        ret += "VK_SHADER_STAGE_INTERSECTION_BIT_NV";
    if (flags & VK_SHADER_STAGE_CALLABLE_BIT_NV)
        ret += "VK_SHADER_STAGE_CALLABLE_BIT_NV";
    if (flags & VK_SHADER_STAGE_TASK_BIT_NV)
        ret += "VK_SHADER_STAGE_TASK_BIT_NV";
    if (flags & VK_SHADER_STAGE_MESH_BIT_NV)
        ret += "VK_SHADER_STAGE_MESH_BIT_NV";
    return ret + "]";
}

/* Internal Functions: (TODO)
================================================================================================= */

inline std::string glfw_err() {
    const char *errstr = NULL;
    int err = glfwGetError(&errstr);
    return sformat("[%s:%d]", errstr, err);
}

inline VkResult create_dbg_messenger(
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
        DBG("[VK_DBG]: %s", data->pMessage);
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

    /* TODO: add logging functions for our current support */
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

#ifdef HAS_NEW_GLSLANG

inline spirv_t spirv_compile(vku_shader_stage_e stage, const char *code) {
    VK_ASSERT(init());
    DBG("NEW GLSLANG COMPILE");

    glslang_stage_t stage;
    switch (stage) {
        case VKU_SPIRV_VERTEX:    stage = GLSLANG_STAGE_VERTEX;         break;
        case VKU_SPIRV_TESS_CTRL: stage = GLSLANG_STAGE_TESSCONTROL;    break;
        case VKU_SPIRV_TESS_EVAL: stage = GLSLANG_STAGE_TESSEVALUATION; break;
        case VKU_SPIRV_GEOMETRY:  stage = GLSLANG_STAGE_GEOMETRY;       break;
        case VKU_SPIRV_FRAGMENT:  stage = GLSLANG_STAGE_FRAGMENT;       break;
        case VKU_SPIRV_COMPUTE:   stage = GLSLANG_STAGE_COMPUTE;        break;
        default:
            DBG("Unknown shader stage type: %d", (uint32_t)stage);
            throw err_t(VK_ERROR_UNKNOWN);
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
        .resource = &spirv_resources,
    };

    glslang_shader_t* shader = glslang_shader_create(&input);

    if (!glslang_shader_preprocess(shader, &input)) {
        DBG("GLSL preprocessing failed \n");
        DBG("info_log: %s", glslang_shader_get_info_log(shader));
        DBG("debug_log: %s", glslang_shader_get_info_debug_log(shader));
        DBG("source_code: %s", input.code);
        glslang_shader_delete(shader);
        throw err_t("GLSL preprocessing failed");
    }

    if (!glslang_shader_parse(shader, &input)) {
        DBG("GLSL parsing failed");
        DBG("%s", glslang_shader_get_info_log(shader));
        DBG("%s", glslang_shader_get_info_debug_log(shader));
        DBG("%s", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        throw err_t("GLSL parsing failed");
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        DBG("%s", glslang_program_get_info_log(program));
        DBG("%s", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        throw err_t("GLSL linking failed");
    }

    glslang_program_SPIRV_generate(program, stage);

    int size = glslang_program_SPIRV_get_size(program);
    spirv_t ret;
    ret.type = stage;
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
    /* TODO */
    if (!glslang_initialize_process()) {
        throw err_t("Failed glslang_initialize_process");
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

#else /* HAS_NEW_GLSLANG */

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
            throw err_t(VK_ERROR_UNKNOWN);
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
        throw err_t(VK_ERROR_UNKNOWN);
    }

    program.addShader(&shader);
    if (!program.link(messages)) {
        DBG("Link Failed(Log): [%s]", shader.getInfoLog());
        DBG("Link Failed(Dbg): [%s]", shader.getInfoDebugLog());
        throw err_t(VK_ERROR_UNKNOWN);
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

#endif /* HAS_NEW_GLSLANG */

inline uint32_t find_memory_type(ref_t<device_t> dev,
        uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    vkGetPhysicalDeviceMemoryProperties(dev->vk_phy_dev, &mem_props);

    for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
        if (type_filter & (1 << i) && 
                (mem_props.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    DBG("Couldn't find suitable memory type");
    throw err_t(VK_ERROR_UNKNOWN);
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

} /* namespace utils */

#endif

