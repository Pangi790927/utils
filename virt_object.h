#ifndef VIRT_OBJECT_H
#define VIRT_OBJECT_H

namespace virt_object {

/*!
 * Compile Time Unique ID
 * 
 * This insanity needs some sort of explanation.
 *
 * Origin of the code:
 *   https://stackoverflow.com/a/74453799/7107236
 * The original idea comes from Filip Roséen:
 *   https://refp.se/articles/constexpr-counter
 *
 * In my own words and understanding:
 *
 *   - The core of this code is the function `is_defined`. Initially, it is *not*
 *     defined in the context of the templated `exists`, so the call to
 *     `is_defined(Tag{})` fails SFINAE and selects the second `exists` overload.
 *
 *   - That second overload returns `false`, but it also explicitly instantiates
 *     `generator`. Instantiating `generator` defines `is_defined` for the
 *     respective number. Therefore, the next time `exists` is queried for the
 *     same Id, `is_defined` *is* defined.
 *
 * This code raises `-Wnon-template-friend` (as you would've guessed already) and
 * may become unusable in the future. I hope not.
 */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
template<typename UniqueTag, auto Id>
struct compile_counter {
    using tag = compile_counter;

    struct generator { friend consteval auto is_defined(tag) { return true; } };
    friend consteval auto is_defined(tag);

    struct generator_max { friend consteval auto is_max(tag) { return true; } };
    friend consteval auto is_max(tag);

    template<typename Tag = tag, auto = is_defined(Tag{}), auto = is_max(Tag{})>
    static consteval auto exists(auto) { throw "Unique id was called after max queried;"; };

    template<typename Tag = tag, auto = is_defined(Tag{})>
    static consteval auto exists(auto) { return true; }

    static consteval auto exists(...) { return generator(), false; }


    template<typename Tag = tag, auto = is_defined(Tag{})>
    static consteval auto exists_max(auto) { return generator_max(), true; }

    static consteval auto exists_max(...) { return false; }
};

template<typename UniqueTag, auto Id = int{}, typename = decltype([]{})>
consteval auto compile_max_id() {
    if constexpr (not compile_counter<UniqueTag, Id>::exists_max(Id))
        return Id - 1;
    else
        return compile_max_id<UniqueTag, Id + 1>();
}
template<typename UniqueTag, auto Id = int{}, typename = decltype([]{})>
consteval auto compile_unique_id() {
    if constexpr (not compile_counter<UniqueTag, Id>::exists(Id))
        return Id;
    else
        return compile_unique_id<UniqueTag, Id + 1>();
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

/* Example for the above example, you can use whatever type as a tag */
struct compile_counter_example_tag_t;
static_assert(compile_unique_id<compile_counter_example_tag_t>() == 0);
static_assert(compile_unique_id<compile_counter_example_tag_t>() == 1);
static_assert(compile_unique_id<compile_counter_example_tag_t>() == 2);
static_assert(compile_unique_id<compile_counter_example_tag_t>() == 3);

/* modification of https://stackoverflow.com/a/61922662/3727118 */
template <typename Tag>
class EnumClass {
  private:
    int value_;
  public:
    explicit constexpr EnumClass(int value) : value_(value) {}
    constexpr EnumClass() = default;
    ~EnumClass() = default;
    constexpr EnumClass(const EnumClass &) = default;
    constexpr EnumClass &operator=(const EnumClass &) = default;

    constexpr operator int() const {return    value_;}
    constexpr int value() const {return value_;}

};

template <typename R>
struct object_t;

template <typename T, typename R>
struct ref_t;

/*! Callbacks used by object_t for re-initializing the managed vulkan obeject */
template <typename R>
struct object_cbks_t {
    std::shared_ptr<void> usr_ptr;

    /*! This function is called when an ref_t calls the object_t::init function, just before
     * init is called, with the object and usr_ptr as arguments. This call is made if
     * object_t::cbks is not null and if object_cbks_t::pre_init is also not null. */
    std::function<void(object_t<R> *, std::shared_ptr<void> &)> pre_init;

    /*! Same as above, called after init. */
    std::function<void(object_t<R> *, std::shared_ptr<void> &)> post_init;

    /*! Same as for init, but this time for the uninit function */
    std::function<void(object_t<R> *, std::shared_ptr<void> &)> pre_uninit;

    /*! Same as above, called after uninit. */
    std::function<void(object_t<R> *, std::shared_ptr<void> &)> post_uninit;
};

/*! This is virtual only for init/uninit, which need to describe how the object should be
 * initialized once it is created and it's parameters are filled */
template <typename R>
struct object_t {
    virtual ~object_t() { _call_uninit(); }

    template <typename T, typename _R>
    friend struct ref_t;

    R::ret_t _call_init() {
        if (cbks && cbks->pre_init)
            cbks->pre_init(this, cbks->usr_ptr);
        auto ret = _init();
        if (cbks && cbks->post_init)
            cbks->post_init(this, cbks->usr_ptr);
        return ret;
    }

    R::ret_t _call_uninit() {
        if (cbks && cbks->pre_uninit)
            cbks->pre_uninit(this, cbks->usr_ptr);
        auto ret = _uninit();
        if (cbks && cbks->post_uninit)
            cbks->post_uninit(this, cbks->usr_ptr);
        return ret;
    }

    /* you must use some value generated by compile_unique_id for this one */
    virtual R::type_t type_id() const = 0;
    virtual std::string to_string() const = 0;

    std::shared_ptr<object_cbks_t<R>> cbks;

    virtual void update() {}

private:
    virtual R::ret_t _init() = 0;
    virtual R::ret_t _uninit() { return VK_SUCCESS; };
};

/*!
 * The idea:
 * - No object is directly referenced, but they all are referenced by this reference. What this does
 * is it enables us to keep an internal representation of the vulkan data structures while also
 * letting us rebuild the internal object when needed. The vulkan structures will be rebuilt using
 * the last parameters that where used to build them.
 */
/* TODO: Think if it makes sense to implement a locking mechanism, especially for the rebuild stuff.

ref_t practically implements a DAG of dependencies, this means that to protect a node, all it's
dependees must be also locked. An observation is that even if a dependee can pe added to a
dependency, no dependency from the target node to the leaf nodes can ever be removed before removing
the target node. As such, the ideea is to implement a spinlock for an address variable, such that
the node is protected by a spinlock and a mutex at the same time.

Let's take an example of a graph (nodes in the right, depend on those on the left, in the example,
D depends on A, B and C):

A     E              L
 \   / \            /
  \ /   \          /
B--D     G--------H---N
  / \   /        / \
 /   \ /        /   \
C     F        /     M
     / \      /
    /   \    J
   I     K    \
               \
                O

So in this example, If we want to do a modification to G, we must lock away G, H, M, N, L. If we
want to modify J, we need to lock J, H, M, N, L, O. But the only known fact is that G is a
dependency of H, We don't know that J is a lso a dependency of H, and we must stop H from...

Need to figure this out if I want to implement it, do I really need to remember depends, besides
dependees? This is kinda annoying. So I will need to also hold _depends, and figure out a way to
manage them, somehow. This also does another bad thing, stores twice as many pointers as I really
need, bacause depends are not going to go anywhere, so bad... Maybe I can keep them as raw pointers,
since I already hold them once as shared pointers?

struct node : public std::enable_shared_from_this<ref_base_t> {
    std::vector<std::weak_ptr<node>>  _dependees;
    // std::vector<node *>  _depends; //  <- maybe like this? And maybe only if locking is enabled?
    Data data;
};

*/

template <typename R>
struct base_t;

template <typename R>
struct ref_base_t {
    std::shared_ptr<base_t<R>> _base;
};

template <typename R>
struct base_t : public std::enable_shared_from_this<base_t<R>> {
protected:
    /*! This is here to force the creation of references by create_obj, this makes sure */
    struct private_param_t { explicit private_param_t() = default; };

    std::unique_ptr<object_t<R>>            _obj;
    std::vector<std::weak_ptr<base_t<R>>>   _dependees;

public:
    template <typename T>
    base_t(private_param_t, std::unique_ptr<T> obj) : _obj(std::move(obj)) {}

    template <typename T, typename _R>
    friend struct ref_t;

    R::ret_t rebuild() {
        /* TODO: maybe thread-protect this somehow? */
        auto r1 = _uninit_all();
        if (r1 != typename R::ret_t{})
            return r1;
        auto r2 = _init_all();
        if (r2 != typename R::ret_t{})
            return r2;
        return r2;
    }

    void update() {
        _obj->update();
        for (auto wd : _dependees)
            if (auto d = wd.lock()) {
                d->update();
            }
    }

    template <typename T> requires std::derived_from<T, object_t<R>>
    static std::shared_ptr<base_t> create_obj_ref(
            std::unique_ptr<T>   obj,
            std::vector<ref_base_t<R>> dependencies)
    {
        auto ret = std::make_shared<base_t<R>>(private_param_t{}, std::move(obj));
        for (auto &d : dependencies)
            d._base->_dependees.push_back(ret);
        return ret;
    }

private:
    void _clean_deps() {
        _dependees.erase(std::remove_if(_dependees.begin(), _dependees.end(), [](auto wp){
                return wp.expired(); }), _dependees.end());
    }

    R::ret_t _uninit_all() {
        _clean_deps(); /* we lazy clear the deps whenever we want to iterate over them */
        for (auto wd : _dependees)
            wd.lock()->_uninit_all();
        return _obj->_call_uninit();
    }

    R::ret_t _init_all() {
        auto ret = _obj->_call_init();
        if (ret != typename R::ret_t{})
            return ret;
        for (auto wd : _dependees)
            wd.lock()->_init_all();
        return ret;
    }

};

/*! This holds a reference to an instance of T, instance that is initiated and held by this
 * library. All objects and the user will use the instance via this reference. This is implemented
 * here because it has a small footprint and I consider making it visible would made the library
 * easier to use. */
template <typename T, typename R>
class ref_t : public ref_base_t<R> {
public:
    ref_t(std::nullptr_t) {}
    ref_t() {}
    ref_t(std::shared_ptr<base_t<R>> obj) : ref_base_t<R>{obj} {
        /* We either hold nullptr or an object that can be casted to T */
        if (obj && !dynamic_cast<T *>(ref_base_t<R>::_base->_obj.get())) {
            throw std::runtime_error{
                    std::format("Tried to build a reference of invalid type {} to {}",
                    demangle(typeid(*ref_base_t<R>::_base->_obj.get()).name()), demangle<T>())};
        }
    }

    template <typename U> requires std::derived_from<U, object_t<R>>
    ref_t(ref_t<U, R> oth) : ref_base_t<R>{oth._base} {
        /* We either hold nullptr or an object that can be casted to T */
        if (oth && !dynamic_cast<T *>(ref_base_t<R>::_base->_obj.get())) {
            throw std::runtime_error{
                    std::format("Tried to build a reference of invalid type {} to {}",
                    demangle(typeid(*ref_base_t<R>::_base->_obj.get()).name()), demangle<T>())};
        }
    }

    void rebuild() { ref_base_t<R>::_base->rebuild(); }

    ref_t &operator = (std::nullptr_t) { ref_base_t<R>::_base = nullptr; return *this; } 

    T *operator ->() { return static_cast<T *>(ref_base_t<R>::_base->_obj.get()); }
    const T *operator ->() const { return static_cast<const T *>(ref_base_t<R>::_base->_obj.get()); }

    T &operator *() { return *static_cast<T *>(ref_base_t<R>::_base->_obj.get()); }
    const T &operator *() const { return *static_cast<const T *>(ref_base_t<R>::_base->_obj.get()); }

    T *get() { return static_cast<T *>(ref_base_t<R>::_base->_obj.get()); }
    const T *get() const { return static_cast<const T *>(ref_base_t<R>::_base->_obj.get()); }

    template <typename U> requires std::derived_from<U, T> || std::derived_from<T, U>
    ref_t<U, R> to_related() { return ref_t<U, R>{ref_base_t<R>::_base}; };

    operator bool() { return !!ref_base_t<R>::_base; }
    friend bool operator == (std::nullptr_t, ref_t obj) { return obj._base == nullptr; } 
    friend bool operator == (ref_t obj, std::nullptr_t) { return obj._base == nullptr; } 

    static ref_t<T, R> create_obj_ref(std::unique_ptr<T> obj,
            std::vector<ref_base_t<R>> dependencies)
    {
        return ref_t<T, R>{base_t<R>::create_obj_ref(std::move(obj), dependencies)};
    }
};


} /* virt_object */


#endif
