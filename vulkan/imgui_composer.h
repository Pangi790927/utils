#ifndef IMGUI_COMPOSER_H
#define IMGUI_COMPOSER_H

/* TODO: this...
I need some sort of script that will take imgui.h and create all the bindings
becase writing them myself is not going to work */

/*! @file
 * @brief Draws Dear ImGui with vku: ImGui's own GLFW and Vulkan backends, brought up on the
 * window, the device and the render pass a vku program already has, so ImGui draws inside that
 * program's render pass rather than in a Vulkan context of its own.
 *
 * Core:
 *   - vk_init() starts both backends; vk_new_frame() begins a frame of ImGui's, before
 *     ImGui::NewFrame(); vk_render() records what ImGui::Render() built into a command buffer
 *     inside the render pass; vk_shutdown() stops both backends. The ImGui context itself is the
 *     program's, made before vk_init() and destroyed after vk_shutdown().
 *   - ImGui uploads its textures, such as its font atlas, on the device's first graphics queue,
 *     the one vku submits to, and waits for that queue when it does.
 *   - The Lua bindings of ImGui the TODO above asks for are not here yet; this part is C++ only.
 *
 * Detail:
 *   - The bridge is compiled only where ImGui's headers and its backends' are on the include
 *     path, so that utils' own builds, which compile every .cpp of this folder without ImGui,
 *     keep building; a program that wants the bridge puts ImGui and its backends/ folder on its
 *     include path and compiles imgui_impl_glfw.cpp and imgui_impl_vulkan.cpp.
 *
 * @date 21-10-2026-04:47
 */

#if __has_include("imgui.h") && __has_include("imgui_impl_vulkan.h")

#include "vulkan_utils.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <algorithm>

#define IMGUI_COMPOSER_HAS_VULKAN

#endif

namespace imgui_composer
{

// namespace vo = virt_object;
// namespace vc = virt_composer;
// namespace imc = imgui_composer;

// inline int register_meta(vc::virt_state_t *vs) {
// 	DBG_SCOPE();

//     std::vector<luaL_Reg> ast_tab_funcs = {
//         // {"create_ast_node", vc::luaw_function_wrapper<create_ast_node, vc::bm_t<ast_node_e>>},
//         // {"create_ast_var",  vc::luaw_function_wrapper<create_ast_var,  const char *>},
//         // {"create_ast_int",  vc::luaw_function_wrapper<create_ast_int,  int64_t>},
//     };

//     ASSERT_FN(add_lua_tab_funcs(vs, ast_tab_funcs));
// }

#ifdef IMGUI_COMPOSER_HAS_VULKAN

namespace vku = vulkan_utils;

/* IMGUI_VK_POOL_SIZE is how many textures ImGui's own descriptor pool holds, its font atlas
among them. 21-10-2026-04:47 */
constexpr uint32_t IMGUI_VK_POOL_SIZE = 16;

/*!
 * @brief Starts ImGui's GLFW and Vulkan backends on vku's objects; answers whether both started.
 *
 * Core:
 *   - ImGui reads the input of `win` and draws into subpass 0 of `rp`, on the device and the
 *     swapchain `rp` was made for, with as many images in flight as that swapchain holds.
 *   - ImGui draws with as many samples a pixel as `rp`, renderpass_t::vk_samples; a render pass
 *     made again for another count needs vk_shutdown() and vk_init() again, since ImGui builds
 *     its pipeline here (added 21-10-2026-23:12).
 *   - With `install_callbacks`, ImGui's GLFW backend installs its own GLFW callbacks, which call
 *     those the program installed before, as ImGui_ImplGlfw_InitForVulkan() does.
 *   - The ImGui context must exist; a failed start leaves no backend running.
 *
 * @date 21-10-2026-23:12
 */
inline bool vk_init(vku::ref_t<vku::window_t> win, vku::ref_t<vku::renderpass_t> rp,
        bool install_callbacks)
{
    if (!win || !rp || !ImGui::GetCurrentContext())
        return false;
    vku::ref_t<vku::swapchain_t> swc = rp->m_swapchain;
    vku::ref_t<vku::device_t> dev = swc->m_device;
    if (!ImGui_ImplGlfw_InitForVulkan(win->get_window(), install_callbacks))
        return false;
    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(dev->vk_phy_dev, &props);
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = props.apiVersion;
    info.Instance = dev->m_instance->vk_instance;
    info.PhysicalDevice = dev->vk_phy_dev;
    info.Device = dev->vk_dev;
    info.QueueFamily = (uint32_t)dev->m_que_fams.graphics_id;
    info.Queue = dev->vk_graphics_que[0];
    info.DescriptorPoolSize = IMGUI_VK_POOL_SIZE;
    info.MinImageCount = (std::max)(2u, swc->img_count());
    info.ImageCount = info.MinImageCount;
    info.PipelineInfoMain.RenderPass = rp->vk_render_pass;
    info.PipelineInfoMain.Subpass = 0;
    info.PipelineInfoMain.MSAASamples = rp->vk_samples;
    if (ImGui_ImplVulkan_Init(&info))
        return true;
    ImGui_ImplGlfw_Shutdown();
    return false;
}

/*! Begins a frame of both backends; ImGui::NewFrame() comes after it. The framebuffer scale
 * ImGui is handed is always above 0, so a window resized to no height while the frame begins does
 * not stop ImGui::NewFrame() on its assertion. @date 21-10-2026-06:06 */
inline void vk_new_frame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    /* ImGui's GLFW backend reads the window's size and its framebuffer's size in two calls, so a
    height that falls to 0 between them gives a scale of 0; 1, the scale on Windows and X11,
    stands in for that frame. 21-10-2026-06:06 */
    ImVec2& scale = ImGui::GetIO().DisplayFramebufferScale;
    if (!(scale.x > 0))
        scale.x = 1;
    if (!(scale.y > 0))
        scale.y = 1;
}

/*! Records what the last ImGui::Render() built into `cb`, inside the render pass vk_init() was
 * given; ImGui binds its own pipeline, so the program binds its own again after it.
 * @date 21-10-2026-04:47 */
inline void vk_render(vku::ref_t<vku::cmdbuff_t> cb) {
    ImDrawData *data = ImGui::GetDrawData();
    if (cb && data)
        ImGui_ImplVulkan_RenderDrawData(data, cb->vk_buff);
}

/*! Hands the Vulkan backend the image count of swapchain `swc`, made again, as after a resize:
 * when that count changed, the call waits for the device to go idle and drops ImGui's vertex and
 * index buffers, which ImGui then makes again. ImGui keeps rotating through as many buffers as
 * vk_init() gave it, until vk_shutdown() and vk_init() are called again. @date 21-10-2026-06:06 */
inline void vk_swapchain_remade(vku::ref_t<vku::swapchain_t> swc) {
    if (swc)
        ImGui_ImplVulkan_SetMinImageCount((std::max)(2u, swc->img_count()));
}

/*! Stops both backends once the device `dev` is idle; a null `dev` says the caller made it idle
 * already. The ImGui context stays. When the wait fails, as for a lost device, both backends are
 * stopped all the same and vku::except_t is thrown. @date 21-10-2026-06:06 */
inline void vk_shutdown(vku::ref_t<vku::device_t> dev) {
    FnScope stop([]{
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    });
    if (dev)
        dev->wait_idle();
}

#endif /* IMGUI_COMPOSER_HAS_VULKAN */

}

#endif
