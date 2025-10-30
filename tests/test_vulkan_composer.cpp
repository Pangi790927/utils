#include "vulkan_composer.h"

int main(int argc, char const *argv[])
{
    (void)argc, (void)argv;
    ASSERT_FN(vkc::parse_config("shaders/vulkan_config.yaml"));

    ASSERT_FN(vkc::luaw_init());
    ASSERT_FN(vkc::luaw_execute_loop_run());
    ASSERT_FN(vkc::luaw_execute_window_resize(800, 600));
    ASSERT_FN(vkc::luaw_uninit());
    return 0;
}