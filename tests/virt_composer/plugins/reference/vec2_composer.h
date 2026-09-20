#ifndef VEC2_COMPOSER_H
#define VEC2_COMPOSER_H

/*! @file
 * A point on the plane, and what it lends to a virt-state.
 *
 * One composer, one header, one `register_meta()` - the same shape a host's `*_composer.h` has.
 * The plugin's final file includes this one among its siblings and calls the `register_meta()`
 * below; nothing here knows it is part of a plugin beyond the type being registered as one.
 *
 * 2026-09-20 18:12
 */

#ifndef VIRT_COMPOSER_PLUGIN_COUNTERS
# error "include this through the plugin's final file, which defines \
VIRT_COMPOSER_PLUGIN_COUNTERS before virt_composer.h and declares _type_offset"
#endif

namespace vec2_composer {

namespace vc = virt_composer;

/*! The type id, and a function rather than a constant: a constant would be initialised while the
 * shared object loads, before the host has said where this plugin's types live. So it is written
 * `VEC2()`, with the brackets. 2026-09-20 18:12 */
VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(VEC2);

/*! A point, by its two coordinates.
 *
 * Core:
 *   - `x` and `y` are readable and writable from Lua under their own names.
 *   - `len2()` answers the squared distance from the origin, squared so that integers stay
 *     integers.
 *
 * @date 2026-09-20 18:12
 */
struct vec2_t : public vc::object_t {
    int64_t x = 0;
    int64_t y = 0;

    /* The constructor takes a Private, which only this class's create() can make. That is what
    stops an object being built without the type id it is meant to carry. 2026-09-20 18:12 */
    vec2_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~vec2_t() {}

    static vc::ref_t<vec2_t> create(int64_t x, int64_t y) {
        auto ret = std::make_shared<vec2_t>(vc::object_t::Private{type_id_static()});
        ret->x = x;
        ret->y = y;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return VEC2(); }
    static vc::object_type_e type_id_static() { return VEC2(); }

    int64_t len2() const { return x * x + y * y; }

    virtual std::string to_string() const override {
        return std::format("vec2_t[{}]: x={} y={}", (void *)this, x, y);
    }
};

/*! Builds a vec2_t from two Lua arguments and leaves it on the stack.
 *
 * push_vc_object() answers an error code and leaves the object on the stack, so the count of Lua
 * results is ours to return - handing back what it answered would say "no values" and the caller
 * would see nil. 2026-09-20 18:12 */
inline int lua_make(lua_State *L) {
    auto obj = vec2_t::create(lua_tointeger(L, 1), lua_tointeger(L, 2));
    vc::push_vc_object(L, obj->to_related<vc::object_t>());
    return 1;
}

/*! Registers everything this composer lends to the given state, and answers 0 on success.
 *
 * Called once per state by the plugin's final file, so everything here is about the state it is
 * handed. Registering the same thing twice is harmless - each step is an assignment - but
 * allocating or opening something here would happen again for the next state.
 *
 * @date 2026-09-20 18:12
 */
inline int register_meta(vc::virt_state_t *vs) {
    VC_REGISTER_MEMBER_OBJECT(vs, vec2_t, x);
    VC_REGISTER_MEMBER_OBJECT(vs, vec2_t, y);
    VC_REGISTER_MEMBER_FUNCTION(vs, vec2_t, len2);

    vc::c_function_t::add_plugin_internal_func(vs, "ref_vec2", lua_make);
    return 0;
}

} /* vec2_composer */

#endif /* VEC2_COMPOSER_H */
