#define STB_IMAGE_IMPLEMENTATION

#include "vulkan_composer.h"
#include <stb_image.h>

namespace vulkan_composer {

namespace vku = vulkan_utils;
namespace vc = virt_composer;

vc::ref_t<vku::image_t> load_image(vc::ref_t<vku::cmdpool_t> cp, std::string path) {
    DBG_SCOPE();
    int w, h, chans;
    DBG("Loading rgba image: %s ", path.c_str());
    stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &chans, STBI_rgb_alpha);

    VkDeviceSize imag_sz = w*h*4;
    if (!pixels) {
        throw vku::except_t("Failed to load image");
    }

    auto img = vku::image_t::create(cp->m_device, w, h, VK_FORMAT_R8G8B8A8_SRGB);
    img->set_data(cp, pixels, imag_sz);

    stbi_image_free(pixels);

    return img;
}

}