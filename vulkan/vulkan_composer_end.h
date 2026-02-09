#ifndef VULKAN_COMPOSER_END_H
#define VULKAN_COMPOSER_END_H

namespace vulkan_composer {

namespace vo = virt_object;
namespace vku = vulkan_utils;
namespace vkc = vulkan_composer;

/* Total number of different types. This file consumes this counter, so all types must be known
before this file */
constexpr vku::object_type_e _VKU_TYPE_CNT{vo::compile_max_id<vku::vulkan_tag_t>() + 1};

static std::unordered_map<std::string, luaw_member_t> _data1[_VKU_TYPE_CNT.value()];
static std::unordered_map<std::string, lua_CFunction> _data2[_VKU_TYPE_CNT.value()];

std::unordered_map<std::string, luaw_member_t> *lua_class_members = _data1;
std::unordered_map<std::string, lua_CFunction> *lua_class_member_setters = _data2;
int VKU_TYPE_CNT = _VKU_TYPE_CNT.value();

}

#endif
