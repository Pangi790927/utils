#ifndef VIRT_HELPERS_H
#define VIRT_HELPERS_H

#include <vector>
#include <memory>
#include <format>
#include <string>

#include "virt_composer.h"

namespace virt_composer {

/*! std::vector<T>, plus a small bounds-checked, 1-indexed convenience API on top. Leading
underscore on every added method so none of them ever collide with/hide std::vector<T>'s own
same-shaped members (e.g. `_insert(i, val)` sits right alongside the inherited iterator-based
`insert()`, no `using` declarations needed to keep both reachable). */
template <typename T>
struct lua_vector_t : public std::vector<T> {
    using std::vector<T>::vector;    // inherit std::vector<T>'s own constructors too
    using std::vector<T>::operator=; // ...and its assignment operators - NOT inherited by default

    int _len() const { return (int)this->size(); }
    bool _is_empty() const { return this->empty(); }

    /*! 1-indexed, bounds-checked read. */
    T _at(int i) const {
        if (i < 1 || i > _len())
            throw vc::except_t(std::format("lua_vector _at: index {} out of range (1..{})",
                    i, _len()));
        return (*this)[i - 1];
    }

    /*! Overwrites the element already at `i` (1-indexed) - i must already be occupied. */
    void _set(int i, T val) {
        if (i < 1 || i > _len())
            throw vc::except_t(std::format("lua_vector _set: index {} out of range (1..{})",
                    i, _len()));
        (*this)[i - 1] = std::move(val);
    }

    /*! Inserts `val` right after position `i` (1-indexed); i=0 inserts before everything. Valid
    range for i is [0, _len()] - same position insert(begin()+i, val) would use, just int instead of
    an iterator. */
    void _insert(int i, T val) {
        if (i < 0 || i > _len())
            throw vc::except_t(std::format("lua_vector _insert: index {} out of range (0..{})",
                    i, _len()));
        this->insert(this->begin() + i, std::move(val));
    }

    /*! Erases every element from `i` to `j` inclusive (both 1-indexed). _erase(i, i) removes just
    one. */
    void _erase(int i, int j) {
        if (i < 1 || j < i || j > _len())
            throw vc::except_t(std::format("lua_vector _erase: range {}..{} out of range (1..{})",
                    i, j, _len()));
        this->erase(this->begin() + (i - 1), this->begin() + j);
    }

    /*! Replaces the inclusive range i..j (1-indexed) with `vals` in one call - _erase(i,j) then
    insert `vals` at that spot, atomically. j == i-1 means "erase nothing, just insert before i". */
    void _replace(int i, int j, std::vector<T> vals) {
        if (i < 1 || i > _len() + 1 || j < i - 1 || j > _len())
            throw vc::except_t(std::format(
                    "lua_vector _replace: range {}..{} out of range (1..{})", i, j, _len()));
        this->erase(this->begin() + (i - 1), this->begin() + j);
        this->insert(this->begin() + (i - 1), vals.begin(), vals.end());
    }
};

/*  ---------------------------------------------------------------------------------------------
    type_id_static(): WHY IT IS DECLARED HERE AND DEFINED SOMEWHERE ELSE

    Both templates below declare

        static vc::object_type_e type_id_static();   // no body

    and deliberately never define it. That looks like an oversight and is the opposite of one, so
    it is worth stating plainly.

    virt_composer identifies an object type by an `object_type_e` id handed out by
    VIRT_COMPOSER_REGISTER_TYPE, and that macro issues ids PER NAME - one per registered enumerator
    - not per C++ type. A template is not a type; wref_t<mexpr_t> and wref_t<something_else> are two
    different types that would both inherit any single id written into the template, and the
    registry would then be unable to tell them apart. There is no id the template itself could
    honestly return.

    So the choice each concrete T has to make - which registered id am I? - is left as a hole that
    T's own header fills, by explicitly specialising the function for that T:

        // in the header that defines mexpr_t, inside namespace virt_composer:
        template <>
        inline vc::object_type_e wref_t<math_expr_composer::mexpr_t>::type_id_static() {
            return math_expr_composer::MEXPR_TYPE_WREF;
        }

    Four things about that form, each of which is load-bearing:

      - `template <>` with the full type spelled out is an EXPLICIT SPECIALISATION of one member of
        one instantiation. It is not an override and not an overload; it replaces the missing body
        for exactly wref_t<mexpr_t> and nothing else.
      - It must sit in `namespace virt_composer`, because that is where the primary template lives.
        A specialisation declared in another namespace is a different function and will not be
        found.
      - `inline` is required: the definition lives in a header included by several translation
        units, and without it they each emit the symbol and the link fails on a duplicate.
      - It must appear AFTER the enumerator it returns exists, which is why a header doing this
        registers its MEXPR_TYPE_* values above the specialisation and forward-declares the type.

    FORGETTING IT IS SAFE, in the sense that it cannot be missed silently: nothing defines the
    function, so the first use of that instantiation fails to LINK with an undefined reference
    naming `wref_t<your_type>::type_id_static()`. That is a worse error message than a compile
    error and a much better one than a wrong answer at runtime.
    --------------------------------------------------------------------------------------------- */

/*! wref_t<T> - a weak, Lua-creatable reference to a T (T must derive from vc::object_t). Doesn't
keep the target alive - get_obj() returns an empty ref_t<T> if the target's already gone, instead of
a dangling access. One instantiation = one virt_composer object type - see the note above on why
type_id_static() has no body here and what each concrete T must write instead. */
template <typename T>
struct wref_t : public vc::object_t {
    std::weak_ptr<T> o;

    wref_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~wref_t() {}

    /*! No body on purpose - each concrete T specialises this. See the long note above. */
    static vc::object_type_e type_id_static();
    virtual vc::object_type_e type_id() const override { return type_id_static(); }

    static vc::ref_t<wref_t<T>> create(vc::ref_t<T> target) {
        auto ret = std::make_shared<wref_t<T>>(vc::object_t::Private{type_id_static()});
        ret->o = target;
        return ret;
    }

    vc::ref_t<T> get_obj() const { return o.lock(); }

    inline virtual std::string to_string() const override {
        return std::format("wref[{}]: alive={}", (void *)this, !o.expired());
    }
};


/*! rref_t<T> - the raw-pointer counterpart to wref_t<T>: no weak_ptr control-block overhead, but
correspondingly no safety - get_obj() is only valid while the target is still guaranteed alive by
something else (e.g. still reachable from the tree root). Past that it's a plain dangling-pointer
read, same risk a bare T* always carries - prefer wref_t<T> unless that overhead is shown to
matter. */
template <typename T>
struct rref_t : public vc::object_t {
    T *o = nullptr;

    rref_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~rref_t() {}

    /*! No body on purpose - each concrete T specialises this, exactly as for wref_t<T>; the two
     * are separate object types and need separate ids. See the long note above. */
    static vc::object_type_e type_id_static();
    virtual vc::object_type_e type_id() const override { return type_id_static(); }

    static vc::ref_t<rref_t<T>> create(vc::ref_t<T> target) {
        auto ret = std::make_shared<rref_t<T>>(vc::object_t::Private{type_id_static()});
        ret->o = target.get();
        return ret;
    }

    vc::ref_t<T> get_obj() const {
        if (!o)
            throw vc::except_t("rref_t::get_obj: target is null");
        return o->template to_related<T>();
    }

    inline virtual std::string to_string() const override {
        return std::format("rref[{}]: o={}", (void *)this, (void *)o);
    }
};

} /* namespace virt_composer */

#endif
