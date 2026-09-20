#ifndef MOCK_COMMON_H
#define MOCK_COMMON_H

/*! @file
 * What the two mock plugins have in common, so that the only difference between them is the names
 * and the numbers they answer with.
 *
 * Each plugin is its own translation unit and its own shared object, so everything here exists
 * once per plugin, `_type_offset` included. That is the point: two plugins built from one source
 * must still end up with ranges of their own.
 *
 * 2026-09-20 17:14
 */

/* Before virt_composer.h, since the header only asks whether this is defined. It says this
translation unit counts its types but publishes no count, leaving the host's alone.
2026-09-20 17:14 */
#define VIRT_COMPOSER_PLUGIN_COUNTERS

#include "../../../virt_composer.h"

namespace vc = virt_composer;
namespace vo = virt_object;

/* The host writes this through plugin_register_meta() before anything asks a type for its id.
Until then every id below is its bare index and means nothing. 2026-09-20 17:14 */
int _type_offset = 0;

/*! Declares one mock type: a struct of the shape virt_composer expects, whose `tag()` answers a
 * number belonging to that type alone and whose `mine()` exists only on this plugin's types.
 *
 * `tag()` is what shows a member landed on the right row of the per-type tables, and `mine()` is
 * what shows it landed on no other - a plugin's types must not answer to another's members.
 *
 * 2026-09-20 17:14 */
#define MOCK_DECLARE_TYPE(sname, own_tag)                                                     \
    VIRT_COMPOSER_REGISTER_PLUGIN_TYPE(sname##_TYPE);                                         \
    struct sname : public vc::object_t {                                                      \
        sname(vc::object_t::Private priv) : vc::object_t(priv) {}                             \
        virtual ~sname() {}                                                                   \
                                                                                              \
        static vc::ref_t<sname> create() {                                                    \
            return std::make_shared<sname>(vc::object_t::Private{type_id_static()});          \
        }                                                                                     \
                                                                                              \
        virtual vc::object_type_e type_id() const override { return sname##_TYPE(); }         \
        static vc::object_type_e type_id_static() { return sname##_TYPE(); }                  \
                                                                                              \
        int64_t tag() const { return own_tag; }                                               \
        int64_t MOCK_OWN_MEMBER() const { return own_tag; }                                   \
                                                                                              \
        virtual std::string to_string() const override { return #sname; }                     \
    }

/*! Hands a member on to VC_REGISTER_MEMBER_FUNCTION through one more macro than looks necessary.
 *
 * That macro names the member with `#fn`, and `#` no more expands what it is given than `##` does:
 * passing MOCK_OWN_MEMBER straight in registers a member literally called "MOCK_OWN_MEMBER", and
 * Lua then has no `a_only` to call. Arriving as an argument here, where it sits next to neither `#`
 * nor `##`, it is expanded first and the name that lands is the real one.
 *
 * 2026-09-20 17:22 */
#define MOCK_REGISTER_MEMBER(vs, sname, memb) VC_REGISTER_MEMBER_FUNCTION(vs, sname, memb)

/*! Registers both member functions of one mock type. 2026-09-20 17:22 */
#define MOCK_REGISTER_TYPE(vs, sname)                                                         \
    do {                                                                                      \
        VC_REGISTER_MEMBER_FUNCTION(vs, sname, tag);                                          \
        MOCK_REGISTER_MEMBER(vs, sname, MOCK_OWN_MEMBER);                                     \
    } while (0)

#endif /* MOCK_COMMON_H */
