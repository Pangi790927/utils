#define LOGGER_VERBOSE_LVL 0

#include "vulkan_utils.h"
#include "debug.h"
#include "misc_utils.h"
#include "time_utils.h"

namespace vku = vulkan_utils;

struct compute_ubo_t {
    float dt;
    float ang;
};

struct part_t {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 color;

    static vku::vertex_input_desc_t get_input_desc() {
        return vku::vertex_input_desc_t {
            .bind_desc = {
                .binding = 0,
                .stride = sizeof(part_t),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
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

int main()
{
    DBG_SCOPE();

    compute_ubo_t comp_ubo;

    auto inst = vku::instance_t::create();

    auto vert = vku::spirv_compile(VKU_SPIRV_VERTEX, R"___(
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

    auto frag = vku::spirv_compile(VKU_SPIRV_FRAGMENT, R"___(
        #version 450

        layout(location = 0) in vec4 in_color;      // this is referenced by the vert shader

        layout(location = 0) out vec4 out_color;

        void main() {
            vec2 coord = gl_PointCoord - vec2(0.5);
            out_color = vec4(in_color.rgb, 0.5 - length(coord));
        }
    )___");

    auto comp = vku::spirv_compile(VKU_SPIRV_COMPUTE, R"___(
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

    int width = 800, height = 600;

    auto window =   vku::window_t::create(width, height);
    auto surf =     vku::surface_t::create(window, inst);
    auto dev =      vku::device_t::create(inst, surf);
    auto cp =       vku::cmdpool_t::create(dev);

    auto comp_ubo_buff = vku::buffer_t::create(
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

    auto staging_pbuff = vku::buffer_t::create(
        dev,
        part_sz,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    memcpy(staging_pbuff->map_data(0, part_sz), particles.data(), part_sz);
    staging_pbuff->unmap_data();

    auto comp_in = vku::buffer_t::create(
        dev,
        part_sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    vku::copy_buff(cp, comp_in, staging_pbuff, part_sz, nullptr);

    auto comp_out = vku::buffer_t::create(
        dev,
        part_sz,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    DBG("predesc sets create");

    auto comp_bindings = vku::desc_set_initializer_t::create({
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ubo_t::get_desc_set(0, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_ubo_buff
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ssbo_t::get_desc_set(1, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_in
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ssbo_t::get_desc_set(2, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_out
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
    });

    auto comp_bindings2 = vku::desc_set_initializer_t::create({
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ubo_t::get_desc_set(0, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_ubo_buff
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ssbo_t::get_desc_set(1, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_in
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
        vku::desc_set_initializer_t::buff_binding_t::create(
            vku::ssbo_t::get_desc_set(2, VK_SHADER_STAGE_COMPUTE_BIT),
            comp_out
        )->to_related<vku::desc_set_initializer_t::binding_desc_t>(),
    });

    DBG("posts sets create");

    auto sh_comp =  vku::shader_t::create(dev, comp);
    auto comp_pl =  vku::compute_pipeline_t::create(dev, sh_comp, comp_bindings);

    /* here we have the compute pipeline created and ready to do stuff */

    auto bindings = vku::desc_set_initializer_t::create({});

    auto sh_vert =  vku::shader_t::create(dev, vert);
    auto sh_frag =  vku::shader_t::create(dev, frag);
    auto swc =      vku::swapchain_t::create(dev, surf);
    auto rp =       vku::renderpass_t::create(swc);
    auto pl =       vku::pipeline_t::create(
        width, height,
        rp,
        {sh_vert, sh_frag},
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        part_t::get_input_desc(),
        bindings
    );
    auto fbs =      vku::framebuffs_t::create(rp);
    DBG("fbs done");

    auto img_sem =  vku::sem_t::create(dev);
    auto draw_sem = vku::sem_t::create(dev);
    auto comp_sem = vku::sem_t::create(dev);
    auto fence =    vku::fence_t::create(dev);

    DBG("done sync create");

    auto cbuff =      vku::cmdbuff_t::create(cp);
    auto comp_cbuff = vku::cmdbuff_t::create(cp);

    auto comp_desc_pool = vku::desc_pool_t::create(dev, comp_bindings, 2);
    vku::ref_t<vku::desc_set_t> comp_desc_set[2] = {
        vku::desc_set_t::create(comp_desc_pool, comp_pl->vk_desc_set_layout, comp_bindings),
        vku::desc_set_t::create(comp_desc_pool, comp_pl->vk_desc_set_layout, comp_bindings2),
    };
    DBG("done descpool/desc_sets");

    /* TODO: print a lot more info on vulkan, available extensions, size of memory, etc. */

    /* TODO: the program ever only draws on one image and waits on the fence, we need to use
    at least two images to speed up the draw process */
    // std::map<uint32_t, vku_sem_t *> img_sems;
    // std::map<uint32_t, vku_sem_t *> draw_sems;
    // std::map<uint32_t, vku_fence_t *> fences;
   
    uint64_t last_time_ms = get_time_ms();
    DBG("Starting main loop");
    while (!glfwWindowShouldClose(window->get_window())) {
        if (glfwGetKey(window->get_window(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
            break;
        glfwPollEvents();

        try {
            if (glfwGetKey(window->get_window(), GLFW_KEY_R) == GLFW_PRESS) {
                vku::copy_buff(cp, comp_in, staging_pbuff, part_sz, nullptr);
            }
            static bool is_stopped = false;
            static bool space_pressed = false;
            if (glfwGetKey(window->get_window(), GLFW_KEY_SPACE) == GLFW_PRESS && !space_pressed) {
                space_pressed = true;
                is_stopped = !is_stopped;
            }
            else if (glfwGetKey(window->get_window(), GLFW_KEY_SPACE) == GLFW_RELEASE) {
                space_pressed = false;
            }
            uint32_t img_idx;
            vku::aquire_next_img(swc, img_sem, &img_idx);

            if (!is_stopped) {
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
                vku::submit_cmdbuff({}, comp_cbuff, nullptr, {comp_sem});
            }
            else {
                comp_cbuff->begin(0);
                comp_cbuff->end();

                /* start particle computation and signal comp_sem when done */
                vku::submit_cmdbuff({}, comp_cbuff, nullptr, {comp_sem});

                last_time_ms = get_time_ms();
            }

            cbuff->begin(0);
            cbuff->begin_rpass(fbs, img_idx);
            cbuff->bind_vert_buffs(0, {{img_idx % 2 == 0 || is_stopped ? comp_out : comp_in, 0}});
            cbuff->draw(pl, particles.size());
            cbuff->end_rpass();
            cbuff->end();

            /* start drawing when the image is ready and the compute is done
            signal draw_sem when drawing is done so that the image can be drawn on screen */
            vku::submit_cmdbuff(
                {
                    {comp_sem, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},         /* wait for points */
                    {img_sem, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}/* wait for image */
                },
                cbuff,
                fence,
                {draw_sem}
            );
            vku::present(swc, {draw_sem}, img_idx);

            vku::wait_fences({fence});
            vku::reset_fences({fence});
        }
        catch (vku::except_t &e) {
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                vkDeviceWaitIdle(dev->vk_dev);

                /* TODO: this must be rethought */
                fbs->uninit();
                pl->uninit();
                rp->uninit();
                swc->uninit();
                swc->init();
                rp->init();
                pl->init();
                fbs->init();
            }
            else
                throw e;
        }
    }

    return 0;
}