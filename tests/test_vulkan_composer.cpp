#include "vulkan_composer.h"

namespace vku = vulkan_utils;

int main(int argc, char const *argv[])
{
    (void)argc, (void)argv;
    ASSERT_FN(vkc::parse_config("shaders/vulkan_config.yaml"));
    ASSERT_FN(vkc::luaw_init());
    while (true) {
        try {
            ASSERT_FN(vkc::luaw_execute_loop_run());
        }
        catch (vku::err_t &e) {
            if (e.vk_err == VK_SUBOPTIMAL_KHR) {
                DBG("resize?");
                ASSERT_FN(vkc::luaw_execute_window_resize(800, 600));
            }
            else {
                DBG("Some other error: %s", e.what());
                throw e;
            }
        }
    }
    ASSERT_FN(vkc::luaw_uninit());
    return 0;
}
