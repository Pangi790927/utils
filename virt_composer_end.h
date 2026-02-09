#ifndef VIRT_COMPOSER_END_H
#define VIRT_COMPOSER_END_H

namespace virt_composer {

namespace vo = virt_object;
namespace vc = virt_composer;

/* Total number of different types. This file consumes this counter, so all types must be known
before this file */
constexpr vc::object_type_e _VIRT_TYPE_CNT{vo::compile_max_id<vc::virt_tag_t>() + 1};

static std::unordered_map<std::string, luaw_member_t> _data1[_VIRT_TYPE_CNT.value()];
static std::unordered_map<std::string, lua_CFunction> _data2[_VIRT_TYPE_CNT.value()];

std::unordered_map<std::string, luaw_member_t> *lua_class_members = _data1;
std::unordered_map<std::string, lua_CFunction> *lua_class_member_setters = _data2;
int VIRT_TYPE_CNT = _VIRT_TYPE_CNT.value();

}

#endif

