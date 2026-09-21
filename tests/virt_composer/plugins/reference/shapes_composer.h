#ifndef SHAPES_COMPOSER_H
#define SHAPES_COMPOSER_H

/*! @file
 * Two shapes that measure their own area, and what they lend to a virt-state.
 *
 * A composer carries as many types as belong together, not one each. This one holds a rectangle
 * and a circle because they are the same idea twice, and registers both from a single
 * `register_meta()`. vec2_composer.h beside it holds one type, and neither is more correct than
 * the other - what a composer is for is a subject, not a type.
 *
 * 2026-09-20 18:12
 */

#ifndef VIRT_COMPOSER_PLUGIN_COUNTERS
# error "include this through the plugin's final file, which defines \
VIRT_COMPOSER_PLUGIN_COUNTERS before virt_composer.h and declares _type_offset"
#endif

namespace shapes_composer {

namespace vc = virt_composer;

/*! One registration per type, however many a composer holds. Each is a function rather than a
 * constant, so they are written `RECT()` and `CIRCLE()`, with the brackets - a constant would be
 * initialised while the shared object loads, before the host has said where this plugin's types
 * live. 2026-09-20 18:12 */
VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(RECT);
VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(CIRCLE);

/*! A rectangle, by its width and height.
 *
 * Core:
 *   - `w` and `h` are readable and writable from Lua under their own names.
 *   - `area()` answers their product.
 *
 * @date 2026-09-20 18:12
 */
struct rect_t : public vc::object_t {
    int64_t w = 0;
    int64_t h = 0;

    /* The constructor takes a Private, which only this class's create() can make. That is what
    stops an object being built without the type id it is meant to carry. 2026-09-20 18:12 */
    rect_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~rect_t() {}

    static vc::ref_t<rect_t> create(int64_t w, int64_t h) {
        auto ret = std::make_shared<rect_t>(vc::object_t::Private{type_id_static()});
        ret->w = w;
        ret->h = h;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return RECT(); }
    static vc::object_type_e type_id_static() { return RECT(); }

    int64_t area() const { return w * h; }

    virtual std::string to_string() const override {
        return std::format("rect_t[{}]: w={} h={}", (void *)this, w, h);
    }
};

/*! A circle, by its radius.
 *
 * Core:
 *   - `r` is readable and writable from Lua under its own name.
 *   - `area_x100()` answers a hundred times the area, since what crosses to Lua here are integers
 *     and a circle's area is not one.
 *
 * @date 2026-09-20 18:12
 */
struct circle_t : public vc::object_t {
    int64_t r = 0;

    circle_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~circle_t() {}

    static vc::ref_t<circle_t> create(int64_t r) {
        auto ret = std::make_shared<circle_t>(vc::object_t::Private{type_id_static()});
        ret->r = r;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return CIRCLE(); }
    static vc::object_type_e type_id_static() { return CIRCLE(); }

    int64_t area_x100() const { return 314 * r * r; }

    virtual std::string to_string() const override {
        return std::format("circle_t[{}]: r={}", (void *)this, r);
    }
};

/*! Builds a rect_t from two Lua arguments and leaves it on the stack.
 *
 * push_vc_object() answers an error code and leaves the object on the stack, so the count of Lua
 * results is ours to return - handing back what it answered would say "no values" and the caller
 * would see nil. 2026-09-20 18:12 */
inline int lua_make_rect(lua_State *L) {
    auto obj = rect_t::create(lua_tointeger(L, 1), lua_tointeger(L, 2));
    vc::push_vc_object(L, obj->to_related<vc::object_t>());
    return 1;
}

/*! Builds a circle_t from one Lua argument and leaves it on the stack. 2026-09-20 18:12 */
inline int lua_make_circle(lua_State *L) {
    auto obj = circle_t::create(lua_tointeger(L, 1));
    vc::push_vc_object(L, obj->to_related<vc::object_t>());
    return 1;
}

/*! Builds a rect_t from a yaml node, so a config can name one by its type.
 *
 * A plugin's types reach yaml the same way a host's custom types do. build_object() knows the
 * handful of built-in types by name and hands everything else to the builders registered with
 * add_named_builder_callback(), so a type nobody registered a builder for cannot be named in a
 * config at all - plugin or not. 2026-09-20 20:00 */
inline co::task<vc::ref_t<vc::object_t>> yaml_make_rect(vc::virt_state_t *vs,
        const std::string& name, fkyaml::node& node)
{
    auto w = co_await vc::resolve_int(vs, node["w"]);
    auto h = co_await vc::resolve_int(vs, node["h"]);

    auto obj = rect_t::create(w, h);
    vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
    co_return obj->to_related<vc::object_t>();
}

/*! Registers everything this composer lends to the given state, both its types together, and
 * answers 0 on success.
 *
 * Called once per state by the plugin's final file, so everything here is about the state it is
 * handed. Registering the same thing twice is harmless - each step is an assignment - but
 * allocating or opening something here would happen again for the next state.
 *
 * @date 2026-09-20 18:12
 */
inline int register_meta(vc::virt_state_t *vs) {
    VC_REGISTER_MEMBER_OBJECT(vs, rect_t, w);
    VC_REGISTER_MEMBER_OBJECT(vs, rect_t, h);
    VC_REGISTER_MEMBER_FUNCTION(vs, rect_t, area);

    VC_REGISTER_MEMBER_OBJECT(vs, circle_t, r);
    VC_REGISTER_MEMBER_FUNCTION(vs, circle_t, area_x100);

    vc::c_function_t::add_plugin_internal_func(vs, "ref_rect", lua_make_rect);
    vc::c_function_t::add_plugin_internal_func(vs, "ref_circle", lua_make_circle);

    /* What lets a config say `m_type: shapes::rect_t`. The name is this plugin's from here on,
    and another plugin asking for it is refused. 2026-09-20 20:00 */
    ASSERT_FN(vc::add_named_builder_callback(vs, "shapes::rect_t", yaml_make_rect));
    return 0;
}

} /* shapes_composer */

#endif /* SHAPES_COMPOSER_H */
