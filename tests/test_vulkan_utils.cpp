#define LOGGER_VERBOSE_LVL 0

#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace vku_utils;

struct part_t {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 color;
};

static auto create_vbuff(auto dev, auto cp, const std::vector<vku_vertex3d_t>& vertices) {
    size_t verts_sz = vertices.size() * sizeof(vertices[0]);
    auto staging_vbuff = vku_buffer_t::create(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_vbuff->get()->map_data(0, verts_sz), vertices.data(), verts_sz);
    staging_vbuff->get()->unmap_data();

    auto vbuff = vku_buffer_t::create(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku_copy_buff(cp, vbuff, staging_vbuff, verts_sz);
    return vbuff;
}

static auto create_ibuff(auto dev, auto cp, const std::vector<uint16_t>& indices) {
    size_t idxs_sz = indices.size() * sizeof(indices[0]);
    auto staging_ibuff = vku_buffer_t::create(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_ibuff->get()->map_data(0, idxs_sz), indices.data(), idxs_sz);
    staging_ibuff->get()->unmap_data();

    auto ibuff = vku_buffer_t::create(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku_copy_buff(cp, ibuff, staging_ibuff, idxs_sz);
    return ibuff;
}

static auto load_image(auto cp, std::string path) {
    int w, h, chans;
    DBG("Here?");
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &chans, STBI_rgb_alpha);

    /* TODO: some more logs around here */
    VkDeviceSize imag_sz = w*h*4;
    if (!pixels) {
        throw vku_err_t("Failed to load image");
    }

    auto img = vku_image_t::create(cp->get()->dev, w, h, VK_FORMAT_R8G8B8A8_SRGB);
    img->get()->set_data(cp, pixels, imag_sz);

    stbi_image_free(pixels);

    return img;
}

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    DBG_SCOPE();

    // const std::vector<vku_vertex2d_t> vertices = {
    //     {{0.0f, -0.5f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    //     {{0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
    //     {{-0.5f, 0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}
    // };

    const std::vector<vku_vertex3d_t> vertices = {
        {{-0.5f, -0.5f,  0.0f}, {0, 0, 0}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f,  0.0f}, {0, 0, 0}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f,  0.0f}, {0, 0, 0}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f,  0.0f}, {0, 0, 0}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0, 0, 0}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, 0}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, 0}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, 0}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    };

    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    vku_mvp_t mvp;

    auto inst = vku_instance_t::create();
    DBG("Done instance init");

    auto vert = vku_spirv_compile(VKU_SPIRV_VERTEX, R"___(
        #version 450

        layout(binding = 0) uniform ubo_t {
            mat4 model;
            mat4 view;
            mat4 proj;
        } ubo;

        layout(location = 0) in vec3 in_pos;    // those are referenced by
        layout(location = 1) in vec3 in_normal; // vku_vertex3d_t::get_input_desc()
        layout(location = 2) in vec3 in_color;
        layout(location = 3) in vec2 in_tex;

        layout(location = 0) out vec3 out_color;
        layout(location = 1) out vec2 out_tex_coord;

        void main() {
            gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos, 1.0);
            out_color = in_color;
            out_tex_coord = in_tex;
        }

    )___");

    auto frag = vku_spirv_compile(VKU_SPIRV_FRAGMENT, R"___(
        #version 450

        layout(location = 0) in vec3 in_color;      // this is referenced by the vert shader
        layout(location = 1) in vec2 in_tex_coord;  // this is referenced by the vert shader

        layout(location = 0) out vec4 out_color;

        layout(binding = 1) uniform sampler2D tex_sampler;

        void main() {
            out_color = vec4(in_color, 1.0);
            out_color = texture(tex_sampler, in_tex_coord);
        }
    )___");

    int width = 800, height = 600;

    auto window =   vku_window_t::create(width, height);
    auto surf =     vku_surface_t::create(window, inst);
    auto dev =      vku_device_t::create(surf);
    auto cp =       vku_cmdpool_t::create(dev);

    auto img = load_image(cp, "test_image.png");
    auto view = vku_img_view_t::create(img, VK_IMAGE_ASPECT_COLOR_BIT);
    auto sampl = vku_img_sampl_t::create(dev);

    auto mvp_buff = vku_buffer_t::create(
        dev,
        sizeof(mvp),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto mvp_pbuff = mvp_buff->get()->map_data(0, sizeof(vku_mvp_t));

    auto bindings = vku_binding_desc_t::create({
        vku_binding_desc_t::buff_binding_t::create(
            vku_ubo_t::get_desc_set(0, VK_SHADER_STAGE_VERTEX_BIT),
            mvp_buff
        ).get()->to_parent<vku_binding_desc_t::binding_desc_t>(),
        vku_binding_desc_t::sampl_binding_t::create(
            vku_img_sampl_t::get_desc_set(1, VK_SHADER_STAGE_FRAGMENT_BIT),
            view,
            sampl
        ).get()->to_parent<vku_binding_desc_t::binding_desc_t>(),
    });

    auto sh_vert =  vku_shader_t::create(dev, vert);
    auto sh_frag =  vku_shader_t::create(dev, frag);
    auto swc =      vku_swapchain_t::create(dev);
    auto rp =       vku_renderpass_t::create(swc);
    auto pl =       vku_pipeline_t::create(
        width, height,
        rp,
        {sh_vert, sh_frag},
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        vku_vertex3d_t::get_input_desc(),
        bindings
    );
    auto fbs =      vku_framebuffs_t::create(rp);

    auto img_sem =  vku_sem_t::create(dev);
    auto draw_sem = vku_sem_t::create(dev);
    auto fence =    vku_fence_t::create(dev);

    auto cbuff =    vku_cmdbuff_t::create(cp);

    auto vbuff = create_vbuff(dev, cp, vertices);
    auto ibuff = create_ibuff(dev, cp, indices);

    auto desc_pool = vku_desc_pool_t::create(dev, bindings, 1);
    auto desc_set = vku_desc_set_t::create(desc_pool, pl->get()->vk_desc_set_layout, bindings);

    /* TODO: print a lot more info on vulkan, available extensions, size of memory, etc. */

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku_sem_t *> img_sems;
    // std::map<uint32_t, vku_sem_t *> draw_sems;
    // std::map<uint32_t, vku_fence_t *> fences;
    double start_time = get_time_ms();
   
    DBG("Starting main loop"); 
    while (!glfwWindowShouldClose(window->get()->get_window())) {
        if (glfwGetKey(window->get()->get_window(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        try {
            uint32_t img_idx;
            vku_aquire_next_img(swc, img_sem, &img_idx);

            float curr_time = ((double)get_time_ms() - start_time)/100000.;
            curr_time *= 100;
            mvp.model = glm::rotate(glm::mat4(1.0f), curr_time * glm::radians(90.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
            mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
            mvp.proj = glm::perspective(glm::radians(45.0f),
                    swc->get()->vk_extent.width / (float)swc->get()->vk_extent.height, 0.1f, 10.0f);
            mvp.proj[1][1] *= -1;
            memcpy(mvp_pbuff, &mvp, sizeof(mvp));

            cbuff->get()->begin(0);
            cbuff->get()->begin_rpass(fbs, img_idx);
            cbuff->get()->bind_vert_buffs(0, {{vbuff, 0}});
            cbuff->get()->bind_idx_buff(ibuff, 0, VK_INDEX_TYPE_UINT16);
            cbuff->get()->bind_desc_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pl->get()->vk_layout, desc_set);
            cbuff->get()->draw_idx(pl, indices.size());
            cbuff->get()->end_rpass();
            cbuff->get()->end();

            vku_submit_cmdbuff({{img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                    cbuff, fence, {draw_sem});
            vku_present(swc, {draw_sem}, img_idx);

            vku_wait_fences({fence});
            vku_reset_fences({fence});
        }
        catch (vku_err_t &e) {
            /* TODO: fix this (next time write what's wrong with it) */
            DBG("resize?");
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                vkDeviceWaitIdle(dev->get()->vk_dev);

                /* This will rebuild the entire tree following from window */
                window->rebuild();
            }
            else
                throw e;
        }
    }

    return 0;
}