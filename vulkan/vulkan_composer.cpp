#define STB_IMAGE_IMPLEMENTATION

#include "vulkan_composer.h"
#include <stb_image.h>

virt_composer::ref_t<vulkan_utils::image_t> load_image(auto cp, std::string path) {
    int w, h, chans;
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &chans, STBI_rgb_alpha);

    /* TODO: some more logs around here */
    VkDeviceSize imag_sz = w*h*4;
    if (!pixels) {
        throw vku::except_t("Failed to load image");
    }

    auto img = vku::image_t::create(cp->m_device, w, h, VK_FORMAT_R8G8B8A8_SRGB);
    img->set_data(cp, pixels, imag_sz);

    stbi_image_free(pixels);

    return img;
}
