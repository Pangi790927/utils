#define LOGGER_VERBOSE_LVL 0

#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct compute_ubo_t {
    float dt;
    float ang;
};

struct part_t {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 color;

    static vku_vertex_input_desc_t get_input_desc() {
        return {
            .bind_desc = {
                .binding = 0,
                .stride = sizeof(part_t),
                .input_rate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            .attr_desc = {
                {
                    .location = 0,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(part_t, pos)
                },
                {
                    .location = 1,
                    .binding = 0,
                    .format = VK_FORMAT_R32G32B32A32_SFLOAT,
                    .offset = offsetof(part_t, color)
                }
            }
        };
    }
};

int main(int argc, char const *argv[])
{
    DBG_SCOPE();

    vku_mvp_t mvp;
    compute_ubo_t comp_ubo;

    vku_opts_t opts;
    auto inst = new vku_instance_t(opts);

    auto vert = vku_spirv_compile(inst, VKU_SPIRV_VERTEX, R"___(
        #version 450

        layout(location = 0) in vec2 in_pos;    // those are referenced by
        layout(location = 1) in vec4 in_color;  // part_t::get_input_desc()

        layout(location = 0) out vec4 out_color;

        void main() {
            gl_PointSize = 1.0;
            gl_Position = vec4(in_pos.xy, 0.0, 1.0);
            out_color = in_color;
        }

    )___");

    auto frag = vku_spirv_compile(inst, VKU_SPIRV_FRAGMENT, R"___(
        #version 450

        layout(location = 0) in vec4 in_color;      // this is referenced by the vert shader

        layout(location = 0) out vec4 out_color;

        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            out_color = vec4(in_color.rgb, 0.5 - length(coord));
        }
    )___");

    auto comp = vku_spirv_compile(inst, VKU_SPIRV_COMPUTE, R"___(
        #version 450

        layout (binding = 0) uniform params_ubo_t {
            float dt;
            float ang;
        } ubo;

        struct part_t {
            vec2 pos;
            vec2 vel;
            vec4 color;
        };

        layout(std140, binding = 1) readonly buffer part_ssbo_in_t {
           part_t parts_in[];
        };

        layout(std140, binding = 2) buffer part_ssbo_out_t {
           part_t parts_out[];
        };

        layout (local_size_x = 1024, local_size_y = 1, local_size_z = 1) in;

        void main() 
        {
            uint index = gl_GlobalInvocationID.x;  

            part_t part_in = parts_in[index];

            vec2 pos_pre = part_in.pos;
            parts_out[index].pos = part_in.pos + part_in.vel.xy * ubo.dt;
            parts_out[index].vel = part_in.vel + vec2(cos(ubo.ang), sin(ubo.ang)) * ubo.dt;
            parts_out[index].color = part_in.color;

            if (length(parts_out[index].pos) > 1) {
                parts_out[index].vel = -parts_out[index].vel;
                parts_out[index].pos = pos_pre;
            }

            // Flip movement at window border
            // if ((parts_out[index].pos.x <= -1.0) || (parts_out[index].pos.x >= 1.0)) {
            //     parts_out[index].vel.x = -parts_out[index].vel.x;
            //     parts_out[index].pos = pos_pre;
            // }
            // if ((parts_out[index].pos.y <= -1.0) || (parts_out[index].pos.y >= 1.0)) {
            //     parts_out[index].vel.y = -parts_out[index].vel.y;
            //     parts_out[index].pos = pos_pre;
            // }
        }
    )___");

    auto surf =     new vku_surface_t(inst);
    auto dev =      new vku_device_t(surf);
    auto cp =       new vku_cmdpool_t(dev);

    auto comp_ubo_buff = new vku_buffer_t(
        dev,
        sizeof(compute_ubo_t),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    auto comp_ubp_pbuff = comp_ubo_buff->map_data(0, sizeof(compute_ubo_t));

    std::vector<part_t> particles(1024*4096);
    uint32_t part_sz = sizeof(part_t) * particles.size();

    double pi = 3.141592653589;
    double ang = pi * 2. / particles.size();
    int i = 0;
    for (auto &p : particles) {
        float b = 1 + float(sin(i/100.));
        p.pos = glm::vec2(cos(ang * i) / 2. * b, sin(ang * i) / 2. * b);
        // p.pos = glm::vec2((i / 1024) / 1024. * 2 - 1, (i % 1024) / 1024. * 2 - 1);
        
        float a = sin(i * ang * 6.);
        // p.vel = glm::vec2(cos(ang * i) * a, sin(ang * i) * a);
        p.vel = glm::vec2(p.pos.x * a, p.pos.y * a);
        // p.vel = glm::vec2(0, 0);
        p.color = glm::vec4(
            0.5 + sin(ang * i) / 2.,
            0.5 + cos(ang * i) / 2.,
            0.5 + sin(ang * i) * cos(ang * i) / 2.,
            1.0
        );
        i++;
    }

    /* TODO: initialize particle data here */

    auto staging_pbuff = new vku_buffer_t(
        dev,
        part_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_pbuff->map_data(0, part_sz), particles.data(), part_sz);
    staging_pbuff->unmap_data();

    auto comp_in = new vku_buffer_t(
        dev,
        part_sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku_copy_buff(cp, comp_in, staging_pbuff, part_sz);
    // delete staging_pbuff;

    auto comp_out = new vku_buffer_t(
        dev,
        part_sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    vku_binding_desc_t comp_bindings = {
        .binds = {
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ubo_t::get_desc_set(0, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_ubo_buff
            ),
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ssbo_t::get_desc_set(1, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_in
            ),
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ssbo_t::get_desc_set(2, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_out
            ),
        }
    };

    vku_binding_desc_t comp_bindings2 = {
        .binds = {
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ubo_t::get_desc_set(0, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_ubo_buff
            ),
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ssbo_t::get_desc_set(1, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_out
            ),
            vku_binding_desc_t::buff_binding_t::make_bind(
                vku_ssbo_t::get_desc_set(2, VK_SHADER_STAGE_COMPUTE_BIT),
                comp_in
            ),
        }
    };

    auto sh_comp =  new vku_shader_t(dev, comp);
    auto comp_pl =  new vku_compute_pipeline_t(opts, dev, sh_comp, comp_bindings);

    /* here we have the compute pipeline created and ready to do stuff */

    vku_binding_desc_t bindings = {
        .binds = {},
    };

    auto sh_vert =  new vku_shader_t(dev, vert);
    auto sh_frag =  new vku_shader_t(dev, frag);
    auto swc =      new vku_swapchain_t(dev);
    auto rp =       new vku_renderpass_t(swc);
    auto pl =       new vku_pipeline_t(
        opts,
        rp,
        {sh_vert, sh_frag},
        VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
        part_t::get_input_desc(),
        bindings
    );
    auto fbs =      new vku_framebuffs_t(rp);

    auto img_sem =  new vku_sem_t(dev);
    auto draw_sem = new vku_sem_t(dev);
    auto comp_sem = new vku_sem_t(dev);
    auto fence =    new vku_fence_t(dev);

    auto cbuff =      new vku_cmdbuff_t(cp);
    auto comp_cbuff = new vku_cmdbuff_t(cp);

    auto comp_desc_pool = new vku_desc_pool_t(dev, comp_bindings, 2);
    vku_desc_set_t *comp_desc_set[] = {
        new vku_desc_set_t(comp_desc_pool, comp_pl->vk_desc_set_layout, comp_bindings),
        new vku_desc_set_t(comp_desc_pool, comp_pl->vk_desc_set_layout, comp_bindings2),
    };

    /* TODO: print a lot more info on vulkan, available extensions, size of memory, etc. */

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku_sem_t *> img_sems;
    // std::map<uint32_t, vku_sem_t *> draw_sems;
    // std::map<uint32_t, vku_fence_t *> fences;
    double start_time = get_time_ms();
   
    uint64_t last_time_ms = get_time_ms();
    DBG("Starting main loop");
    while (!glfwWindowShouldClose(inst->window)) {
        if (glfwGetKey(inst->window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        try {
            if (glfwGetKey(inst->window, GLFW_KEY_R) == GLFW_PRESS) {
                vku_copy_buff(cp, comp_in, staging_pbuff, part_sz);
            }
            uint32_t img_idx;
            vku_aquire_next_img(swc, img_sem, &img_idx);

            uint64_t curr_time = get_time_ms();
            comp_ubo.dt = (curr_time - last_time_ms) / 1000.;
            comp_ubo.ang += comp_ubo.dt / 10.;
            memcpy(comp_ubp_pbuff, &comp_ubo, sizeof(comp_ubo));
            last_time_ms = curr_time;

            comp_cbuff->begin(0);
            comp_cbuff->bind_compute(comp_pl);
            comp_cbuff->bind_desc_set(VK_PIPELINE_BIND_POINT_COMPUTE, comp_pl->vk_layout,
                    comp_desc_set[img_idx % 2]);
            comp_cbuff->dispatch_compute(particles.size() / 1024);
            comp_cbuff->end();

            /* start particle computation and signal comp_sem when done */
            vku_submit_cmdbuff({}, comp_cbuff, nullptr, {comp_sem});

            cbuff->begin(0);
            cbuff->begin_rpass(fbs, img_idx);
            cbuff->bind_vert_buffs(0, {{img_idx % 2 == 0 ? comp_out : comp_in, 0}});
            cbuff->draw(pl, particles.size());
            cbuff->end_rpass();
            cbuff->end();

            /* start drawing when the image is ready and the compute is done
            signal draw_sem when drawing is done so that the image can be drawn on screen */
            vku_submit_cmdbuff(
                {
                    {comp_sem, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},         /* wait for points */
                    {img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}/* wait for image */
                },
                cbuff,
                fence,
                {draw_sem}
            );
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
                    VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                    part_t::get_input_desc(),
                    bindings
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