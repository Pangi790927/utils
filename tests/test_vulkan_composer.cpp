#include "vulkan_composer.h"
#include "virt_composer_end.h"

namespace vc = virt_composer;
namespace vku = vulkan_utils;
namespace vkc = vulkan_composer;

int main(int argc, char const *argv[])
{
    (void)argc, (void)argv;

    /* We can add those functions whenever we want because they are not part of the virtual state,
    but part of the executable in a sense. */
    vc::c_function_t::add_internal_func("fill_buffer_with_quad_vertices",
        vc::luaw_function_wrapper<[](void *buff, size_t len) -> int {
            DBG("buff: %p, len: %zu", buff, len);

            struct vertex_t {
                glm::vec2 a;
                glm::vec3 b;
                glm::vec2 c;
            };

            vertex_t quad_points[] = {
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                {{-0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{ 0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
                {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            };

            memcpy(buff, &quad_points, len);
            return 0;
        }, void *, size_t>);

    vc::c_function_t::add_internal_func("fill_buffer_with_quad_indexes",
        vc::luaw_function_wrapper<[](void *buff, size_t len) -> int {
            DBG("buff: %p, len: %zu", buff, len);
            uint32_t indexes[] = { 0, 1, 2, 0, 2, 3};
            memcpy(buff, indexes, sizeof(indexes));
            return 0;
        }, void *, size_t>);

    /* We first create the state of our composer, that means:
         - the lua state,
         - object storage and naming
         - the pool that keeps what objects are yet waiting for dependencies in parsing and such
         - bindings (ex: from c++ members to Lua members), callbacks and name defintions
    */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs));

    /* We register bindings for all the parts of interest from the respective composer, in this case
    the vulkan composer. This will dictate:
        - what vku objects can be created inside lua
        - what objects can be loaded from yaml (and how they are loaded)
        - what member functions and member objects can be used from lua
        - what defines ara available
        - what functions are to be inserted in the virt_composer lib in Lua
    */
    ASSERT_FN(vkc::register_meta(vs.get()));

    /* We optionaly parse one ore more yaml files to populate the virtual state with objects */
    ASSERT_FN(vc::parse_config(vs.get(), "shaders/vulkan_config.yaml"));

    /* During parsing, diverse lua scripts where executed, populating the virt state. Functions
    that where created/rewriten during this step can now be called.

    Those functions return both their integer return value "ret" and an error value from inside
    virt composer. (examples: ret can be set to a string returned by vku_init, err can be set
    to an error value because vku_init is missing). In this case both return an integer (as
    specified by the call_lua's template argument). */
    auto [ret, err] = vc::call_lua<int>(vs.get(), "vku_init");
    ASSERT_FN(ret);
    ASSERT_FN(err);

    while (true) {
        try {
            /* VC_ASSERT_LUAC, does the two assertion in one */
            VC_LUA_ASSERT(vc::call_lua<int>(vs.get(), "on_loop_run"));
        }
        catch (vku::except_t &e) {
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                /* On a resize those need to be uninit and init in this specific order and this
                also shows how do we get an object from the virtual state (if it has a name) */
                vc::get_ref<vku::framebuffs_t>(vs.get(), "fbs")->uninit();
                vc::get_ref<vku::pipeline_t>(vs.get(), "pl")->uninit();
                vc::get_ref<vku::renderpass_t>(vs.get(), "rp")->uninit();
                vc::get_ref<vku::swapchain_t>(vs.get(), "swc")->uninit();
                vc::get_ref<vku::swapchain_t>(vs.get(), "swc")->init();
                vc::get_ref<vku::renderpass_t>(vs.get(), "rp")->init();
                vc::get_ref<vku::pipeline_t>(vs.get(), "pl")->init();
                vc::get_ref<vku::framebuffs_t>(vs.get(), "fbs")->init();
            }
            else {
                DBG("Some other error: %s", e.what());
                throw e;
            }
        }
    }
    return 0;
}
