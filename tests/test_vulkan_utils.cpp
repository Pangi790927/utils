#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

int main(int argc, char const *argv[])
{
    DBG_SCOPE();

    vku_opts_t opts;
    auto inst = new vku_instance_t(opts);

    auto vert = vku_spirv_compile(inst, VKU_SPIRV_VERTEX, R"___(
        #version 450

        layout(location = 0) out vec3 fragColor;

        vec2 positions[3] = vec2[](
            vec2(0.0, -0.5),
            vec2(0.5, 0.5),
            vec2(-0.5, 0.5)
        );

        vec3 colors[3] = vec3[](
            vec3(1.0, 0.0, 0.0),
            vec3(0.0, 1.0, 0.0),
            vec3(0.0, 0.0, 1.0)
        );

        void main() {
            gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
            fragColor = colors[gl_VertexIndex];
        }

    )___");

    auto frag = vku_spirv_compile(inst, VKU_SPIRV_FRAGMENT, R"___(
        #version 450

        layout(location = 0) in vec3 fragColor;
        layout(location = 0) out vec4 outColor;

        void main() {
            outColor = vec4(fragColor, 1.0);
        }
    )___");

    auto surf =     new vku_surface_t(inst);
    auto dev =      new vku_device_t(surf);
    auto swc =      new vku_swapchain_t(dev);
    auto sh_vert =  new vku_shader_t(dev, vert);
    auto sh_frag =  new vku_shader_t(dev, frag);
    auto rp =       new vku_renderpass_t(swc);
    auto pl =       new vku_pipeline_t(
        opts,
        rp,
        {sh_vert, sh_frag},
        vku_vertex2d_t::get_input_desc()
    );
    auto fbs =      new vku_framebuffs_t(rp);
    auto cp =       new vku_cmdpool_t(dev);

    auto img_sem =  new vku_sem_t(dev);
    auto draw_sem = new vku_sem_t(dev);
    auto fence =    new vku_fence_t(dev);

    std::map<uint32_t, vku_cmdbuff_t *> cbuffs;

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku_sem_t *> img_sems;
    // std::map<uint32_t, vku_sem_t *> draw_sems;
    // std::map<uint32_t, vku_fence_t *> fences;

    while (!glfwWindowShouldClose(inst->window)) {
        if (glfwGetKey(inst->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        uint32_t img_idx;
        vku_aquire_next_img(swc, img_sem, &img_idx);

        if (!HAS(cbuffs, img_idx))
            cbuffs.insert({img_idx, new vku_cmdbuff_t(cp)});

        cbuffs[img_idx]->begin();
        cbuffs[img_idx]->begin_rpass(fbs, img_idx);
        cbuffs[img_idx]->draw(pl);
        cbuffs[img_idx]->end_rpass();
        cbuffs[img_idx]->end();

        vku_submit_cmdbuff({{img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                cbuffs[img_idx], fence, {draw_sem});
        vku_present(swc, {draw_sem}, img_idx);

        vku_wait_fences({fence});
        vku_reset_fences({fence});
    }

    delete inst;
    return 0;
}