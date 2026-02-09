#ifndef COMPU_SCOPE_COMPOSER_H
#define COMPU_SCOPE_COMPOSER_H

#include "vulkan_composer.h"

enum cs_freq_e {
    CS_FREQ_100MHZ,
    CS_FREQ_250MHZ,
    CS_FREQ_500MHZ,
};

namespace vulkan_composer {

static std::unordered_map<std::string, cs_freq_e> cs_freq_from_str = {
    {"CS_FREQ_100MHZ", CS_FREQ_100MHZ},
    {"CS_FREQ_250MHZ", CS_FREQ_250MHZ},
    {"CS_FREQ_500MHZ", CS_FREQ_500MHZ},
};

template <> inline cs_freq_e get_enum_val<cs_freq_e>(fkyaml::node &n) {
    return get_enum_val(n, cs_freq_from_str);
}

} /* namespace vulkan_composer */

namespace compu_scope {

namespace vku = vulkan_utils;
namespace vkc = vulkan_composer;
namespace cs = compu_scope;

VULKAN_UTILS_REGISTER_TYPE(CS_TYPE_COMPU_SCOPE);

inline int init();
inline std::string to_string(cs_freq_e freq);

struct compu_scope_t : public vku::object_t {
    cs_freq_e m_freq;
    /* TODO: add all the required options here */

    static vku::object_type_e type_id_static() { return CS_TYPE_COMPU_SCOPE; }
    static vku::ref_t<compu_scope_t> create(cs_freq_e freq /* TODO: all the required fields */) {
        auto ret = vku::ref_t<compu_scope_t>::create_obj_ref(
                std::make_unique<compu_scope_t>(), {});
        ret->m_freq = freq;
        return ret;
    }

    virtual vku::object_type_e type_id() const override { return CS_TYPE_COMPU_SCOPE; }

    int start();
    int stop();

    inline std::string to_string() const override {
        return std::format("CompuScope[addr={}, freq={}]", (void *)this, cs::to_string(m_freq));
    }

private:
    virtual VkResult _init() override { return VK_SUCCESS; }
    virtual VkResult _uninit() override { return VK_SUCCESS; }
};

inline int cs_do_something(int a, int b) {
    DBG("a: %d b: %d", a, b);
    return 12;
}

/* used to add all the hooks inside the composer */
inline int init() {
    vkc::vku_tab_funcs.push_back({"cs_do_something",
            vkc::luaw_function_wrapper<cs_do_something, int, int>});
    vkc::cbk_register_members.push_back([]() {
        VKC_REG_MEMB(compu_scope_t, m_freq);
        VKC_REG_FN(compu_scope_t, start);
        VKC_REG_FN(compu_scope_t, stop);
    });
    vkc::cbk_register_mapping.push_back([](lua_State *L) {
        vkc::register_flag_mapping(L, vkc::cs_freq_from_str);
    });
    vkc::build_object_cbks.push_back({
        [](fkyaml::node& node) -> bool {
            return node["m_type"] == "cs::compu_scope_t";
        },
        [](vkc::ref_state_t *rs, const std::string& name, fkyaml::node& node)
        -> co::task<vku::ref_t<vku::object_t>>
        {
            auto m_freq = vkc::get_enum_val<cs_freq_e>(node["m_freq"]);
            auto obj = compu_scope_t::create(m_freq);
            mark_dependency_solved(rs, name, obj.to_related<vku::object_t>());
            co_return obj.to_related<vku::object_t>();
        }
    });
    return 0;
}

inline std::string to_string(cs_freq_e freq) {
    switch (freq) {
        case CS_FREQ_100MHZ: return "CS_FREQ_100MHZ";
        case CS_FREQ_250MHZ: return "CS_FREQ_250MHZ";
        case CS_FREQ_500MHZ: return "CS_FREQ_500MHZ";
        default: return "cs_unknown_enum";
    }
};

} /* namespace compu_scope */

#endif
