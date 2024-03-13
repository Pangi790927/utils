#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

static auto create_vbuff(auto dev, auto cp, const std::vector<vku_vertex2d_t>& vertices) {
    size_t verts_sz = vertices.size() * sizeof(vertices[0]);
    auto staging_vbuff = new vku_buffer_t(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_vbuff->map_data(0, verts_sz), vertices.data(), verts_sz);
    staging_vbuff->unmap_data();

    auto vbuff = new vku_buffer_t(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku_copy_buff(cp, vbuff, staging_vbuff, verts_sz);
    delete staging_vbuff;
    return vbuff;
}

static auto create_ibuff(auto dev, auto cp, const std::vector<uint16_t>& indices) {
    size_t idxs_sz = indices.size() * sizeof(indices[0]);
    auto staging_ibuff = new vku_buffer_t(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_ibuff->map_data(0, idxs_sz), indices.data(), idxs_sz);
    staging_ibuff->unmap_data();

    auto ibuff = new vku_buffer_t(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku_copy_buff(cp, ibuff, staging_ibuff, idxs_sz);
    delete staging_ibuff;
    return ibuff;
}

int main(int argc, char const *argv[])
{
    DBG_SCOPE();

    // const std::vector<vku_vertex2d_t> vertices = {
    //     {{0.0f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    //     {{0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    //     {{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    // };

    const std::vector<vku_vertex2d_t> vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    vku_opts_t opts;
    auto inst = new vku_instance_t(opts);

    auto vert = vku_spirv_compile(inst, VKU_SPIRV_VERTEX, R"___(
        #version 450

        layout(location = 0) in vec2 in_pos;    // those are referenced by
        layout(location = 1) in vec3 in_color;  // vku_vertex2d_t::get_input_desc()
        layout(location = 2) in vec3 in_tex;

        layout(location = 0) out vec3 out_color;

        void main() {
            gl_Position = vec4(in_pos, 0.0, 1.0);
            out_color = in_color;
        }

    )___");

    auto frag = vku_spirv_compile(inst, VKU_SPIRV_FRAGMENT, R"___(
        #version 450

        layout(location = 0) in vec3 in_color;  // this is referenced by the vert shader
        layout(location = 0) out vec4 out_color;

        void main() {
            out_color = vec4(in_color, 1.0);
        }
    )___");

    auto surf =     new vku_surface_t(inst);
    auto dev =      new vku_device_t(surf);
    auto sh_vert =  new vku_shader_t(dev, vert);
    auto sh_frag =  new vku_shader_t(dev, frag);
    auto swc =      new vku_swapchain_t(dev);
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

    auto cbuff =    new vku_cmdbuff_t(cp);

    auto vbuff = create_vbuff(dev, cp, vertices);
    auto ibuff = create_ibuff(dev, cp, indices);

    /* TODO: print a lot more info on vulkan, available extensions, size of memory, etc. */

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku_sem_t *> img_sems;
    // std::map<uint32_t, vku_sem_t *> draw_sems;
    // std::map<uint32_t, vku_fence_t *> fences;

    while (!glfwWindowShouldClose(inst->window)) {
        if (glfwGetKey(inst->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        try {
            uint32_t img_idx;
            vku_aquire_next_img(swc, img_sem, &img_idx);

            cbuff->begin(0);
            cbuff->begin_rpass(fbs, img_idx);
            cbuff->bind_vert_buffs(0, {{vbuff, 0}});
            cbuff->bind_idx_buff(ibuff, 0, VK_INDEX_TYPE_UINT16);
            // cbuff->draw(pl, vertices.size());
            cbuff->draw_idx(pl, indices.size());
            cbuff->end_rpass();
            cbuff->end();

            vku_submit_cmdbuff({{img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                    cbuff, fence, {draw_sem});
            vku_present(swc, {draw_sem}, img_idx);

            vku_wait_fences({fence});
            vku_reset_fences({fence});
        }
        catch (vku_err_t &e) {
            /* TODO: fix this (next time write what's wrong with it) */
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                vk_device_wait_idle(dev->vk_dev);

                delete swc;
                swc = new vku_swapchain_t(dev);
                rp = new vku_renderpass_t(swc);
                pl = new vku_pipeline_t(
                    opts,
                    rp,
                    {sh_vert, sh_frag},
                    vku_vertex2d_t::get_input_desc()
                );
                fbs = new vku_framebuffs_t(rp);
            }
            else
                throw e;
        }
    }

    delete inst;
    return 0;
}