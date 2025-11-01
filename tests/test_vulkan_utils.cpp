#define LOGGER_VERBOSE_LVL 0

#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace vku = vku_utils;

struct part_t {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 color;
};

static auto create_vbuff(auto dev, auto cp, const std::vector<vku::vertex3d_t>& vertices) {
    size_t verts_sz = vertices.size() * sizeof(vertices[0]);
    auto staging_vbuff = vku::buffer_t::create(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_vbuff->map_data(0, verts_sz), vertices.data(), verts_sz);
    staging_vbuff->unmap_data();

    auto vbuff = vku::buffer_t::create(
        dev,
        verts_sz,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku::copy_buff(cp, vbuff, staging_vbuff, verts_sz, nullptr);
    return vbuff;
}

static auto create_ibuff(auto dev, auto cp, const std::vector<uint16_t>& indices) {
    size_t idxs_sz = indices.size() * sizeof(indices[0]);
    auto staging_ibuff = vku::buffer_t::create(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_ibuff->map_data(0, idxs_sz), indices.data(), idxs_sz);
    staging_ibuff->unmap_data();

    auto ibuff = vku::buffer_t::create(
        dev,
        idxs_sz,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku::copy_buff(cp, ibuff, staging_ibuff, idxs_sz, nullptr);
    return ibuff;
}

static auto load_image(auto cp, std::string path) {
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

int main(int argc, char const *argv[])
{
    (void)argc;
    (void)argv;

    DBG_SCOPE();

    const std::vector<vku::vertex3d_t> vertices = {
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

    vku::mvp_t mvp;

    auto inst = vku::instance_t::create();
    DBG("Done instance init");

    auto vert = vku::spirv_compile(VKU_SPIRV_VERTEX, R"___(
        #version 450

        layout(binding = 0) uniform ubo_t {
            mat4 model;
            mat4 view;
            mat4 proj;
        } ubo;

        // This shows us that macros work, but not sure how this would work with includes afterwards
        // but, meh, I don't really care, I'll simply paste the code there whenever I see an include
        // directive (from path or from a known name, not sure how I will decide on the name resplve)
        #define APPLY_ASSIGN(x, y) x = y

        layout(location = 0) in vec3 in_pos;    // those are referenced by
        layout(location = 1) in vec3 in_normal; // vku::vertex3d_t::get_input_desc()
        layout(location = 2) in vec3 in_color;
        layout(location = 3) in vec2 in_tex;

        layout(location = 0) out vec3 out_color;
        layout(location = 1) out vec2 out_tex_coord;

        void main() {
            gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_pos, 1.0);
            APPLY_ASSIGN(out_color, in_color);
            APPLY_ASSIGN(out_tex_coord, in_tex);
        }

    )___");

    auto frag = vku::spirv_compile(VKU_SPIRV_FRAGMENT, R"___(
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

    ASSERT_FN(vku::spirv_save(frag, "./frag.bin"));
    ASSERT_FN(vku::spirv_save(vert, "./vert.bin"));

    int width = 800, height = 600;

    auto window =   vku::window_t::create(width, height);
    auto surf =     vku::surface_t::create(window, inst);
    auto dev =      vku::device_t::create(surf);
    auto cp =       vku::cmdpool_t::create(dev);

    auto img = load_image(cp, "test_image.png");
    auto view = vku::img_view_t::create(img, VK_IMAGE_ASPECT_COLOR_BIT);
    auto sampl = vku::img_sampl_t::create(dev);

    auto mvp_buff = vku::buffer_t::create(
        dev,
        sizeof(mvp),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto mvp_pbuff = mvp_buff->map_data(0, sizeof(vku::mvp_t));

    auto bindings = vku::binding_desc_set_t::create({
        vku::binding_desc_set_t::buff_binding_t::create(
            vku::ubo_t::get_desc_set(0, VK_SHADER_STAGE_VERTEX_BIT),
            mvp_buff
        ).to_base<vku::binding_desc_set_t::binding_desc_t>(),
        vku::binding_desc_set_t::sampl_binding_t::create(
            vku::img_sampl_t::get_desc_set(1, VK_SHADER_STAGE_FRAGMENT_BIT),
            view,
            sampl
        ).to_base<vku::binding_desc_set_t::binding_desc_t>(),
    });

    auto sh_vert =  vku::shader_t::create(dev, vert);
    auto sh_frag =  vku::shader_t::create(dev, frag);
    auto swc =      vku::swapchain_t::create(dev);
    auto rp =       vku::renderpass_t::create(swc);
    auto pl =       vku::pipeline_t::create(
        width, height,
        rp,
        {sh_vert, sh_frag},
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        vku::vertex3d_t::get_input_desc(),
        bindings
    );
    auto fbs =      vku::framebuffs_t::create(rp);

    auto img_sem =  vku::sem_t::create(dev);
    auto draw_sem = vku::sem_t::create(dev);
    auto fence =    vku::fence_t::create(dev);

    auto cbuff =    vku::cmdbuff_t::create(cp);

    auto vbuff = create_vbuff(dev, cp, vertices);
    auto ibuff = create_ibuff(dev, cp, indices);

    auto desc_pool = vku::desc_pool_t::create(dev, bindings, 1);
    auto desc_set = vku::desc_set_t::create(desc_pool, pl, bindings);

    /* TODO: print a lot more info on vulkan, available extensions, size of memory, etc. */

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku::sem_t *> img_sems;
    // std::map<uint32_t, vku::sem_t *> draw_sems;
    // std::map<uint32_t, vku::fence_t *> fences;
    double start_time = get_time_ms();
   
    DBG("Starting main loop"); 
    while (!glfwWindowShouldClose(window->get_window())) {
        if (glfwGetKey(window->get_window(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        try {
            uint32_t img_idx;
            vku::aquire_next_img(swc, img_sem, &img_idx);

            float curr_time = ((double)get_time_ms() - start_time)/100000.;
            curr_time *= 100;
            mvp.model = glm::rotate(glm::mat4(1.0f), curr_time * glm::radians(90.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
            mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
            mvp.proj = glm::perspective(glm::radians(45.0f),
                    swc->vk_extent.width / (float)swc->vk_extent.height, 0.1f, 10.0f);
            mvp.proj[1][1] *= -1;
            memcpy(mvp_pbuff, &mvp, sizeof(mvp));

            cbuff->begin(0);
            cbuff->begin_rpass(fbs, img_idx);
            cbuff->bind_vert_buffs(0, {{vbuff, 0}});
            cbuff->bind_idx_buff(ibuff, 0, VK_INDEX_TYPE_UINT16);
            cbuff->bind_desc_set(VK_PIPELINE_BIND_POINT_GRAPHICS, pl, desc_set);
            cbuff->draw_idx(pl, indices.size());
            cbuff->end_rpass();
            cbuff->end();

            vku::submit_cmdbuff({{img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
                    cbuff, fence, {draw_sem});
            vku::present(swc, {draw_sem}, img_idx);

            vku::wait_fences({fence});
            vku::reset_fences({fence});
        }
        catch (vku::err_t &e) {
            /* TODO: fix this (next time write what's wrong with it) */
            DBG("resize?");
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                vkDeviceWaitIdle(dev->vk_dev);

                /* This will rebuild the entire tree following from window */
                window.rebuild();
            }
            else
                throw e;
        }
    }

    return 0;
}
