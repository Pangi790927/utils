# Worked example: `force_release_ref` (utils/virt_composer.cpp)

A real revision, 2026-09-08, showing the brief / core / detail / params / notes structure applied to
a function that had accumulated a large, unusable comment. Read it when the recipe needs to be seen
applied rather than described.

The lesson is in the sorting, not in the shortening. The first two rewrites below both failed, and
they failed in opposite directions.

## The function

```c
static int force_release_ref(lua_State *L) {
    auto *box = (box_t *)luaL_testudata(L, 1, "__vc_metatable");
    if (!box) {
        return 0;
    }
    auto vs = luaw_get_virt_state(L);
    auto &obj = box->self_obj;
    if (has(vs->object_to_name, obj.get())) {
        vs->name_to_object.erase(vs->object_to_name[obj.get()]);
        vs->object_to_name.erase(obj.get());
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, vs->weak_cache_ref);
    lua_pushlightuserdata(L, obj.get());
    lua_pushnil(L);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    box->self_obj.reset();
    return 0;
}
```

Five observable things happen here: a non-vc argument is ignored, the object is unregistered from
the name maps, the wrapper cache entry is dropped, the strong reference is released, and the
userdata is deliberately left alive. Keep that list in view - the first two attempts each lost part
of it.

## Attempt 1 - everything, in discovery order (35 lines)

The original carried nearly every fact worth having and was still unusable:

- purpose, GC theory, the ownership precondition and three separate traps arrived in the order they
  were found, not the order a reader needs;
- the traps were welded into running prose, so none could be found or cited alone;
- provenance was told as narrative - "measured directly on this same day, an object survived two
  full forced collectgarbage passes" - where the finding alone was the valuable part;
- it was anchored on mexpr_t, math_expr_composer.h and mexpru.cut(), none of which exist in this
  repo, so a reader here cannot follow any of them;
- and it explained what a tracing GC is to an audience already reading Lua binding internals.

It also never mentioned the name-table erasure or the silent no-op. Length hid the omission: the
comment looked exhaustive, so nobody would think to check.

## Attempt 2 - the same shape, compressed (12 lines)

Three times smaller, nearly all the same facts, and worse. It was rejected on sight, correctly:
"it's not intelligible, it lacks focus, purpose, order."

Compressing prose removes words but preserves structure - one undifferentiated block per topic - so
it became denser without becoming navigable, and three unrelated traps still shared a paragraph. The
two omissions survived the rewrite untouched, because nothing in the process ever compared the
comment against the function.

Squeezing prose is not revising it. A rewrite that only shortens cannot find a missing fact.

## Attempt 3 - sorted by core and detail (30 lines)

```c
/* [INTERNAL] Implements the Lua-visible vc.force_release(obj).
@brief Ends Lua's ownership of a composer object at once, rather than at the collector's
convenience, so the object can be destroyed at a point the caller picks.

Core:
  - On return Lua holds no strong reference to `obj`. If it held the last one, the C++ object is
    already destroyed - not merely eligible. This is why the function exists: Lua's collector is
    tracing, and an unreferenced object can outlive forced collectgarbage("collect") passes
    (2026-09-04: one died only at state teardown).
  - `obj` is unregistered from the composer's name table: it can no longer be found by name,
    from Lua or from C++.
  - No wrapper for `obj` is left anywhere in the state, so an object allocated later can never be
    handed this one.
  - The userdata stays alive but empty, and must not be used again; __gc still runs on it.
  - Only call it once `obj` is cut loose from its owner and nothing else holds it. Everything
    owned beneath `obj` dies with it, as long as that ownership is acyclic (owning shared_ptr
    down, non-owning raw pointers back up).

Detail:
  - Clears self_obj instead of destroying the box, so the later __gc finds a valid empty box_t
    rather than a double-destructed one.
  - The wrapper cache is keyed by raw pointer and only value-weak, so its entry is erased before
    self_obj is reset: the freed address can be reused by a new object, which would then be given
    this empty box (2026-09-04 crash, in a multi-level propagate_rebuild chain).

Params: arg 1 is a vc object userdata; anything else makes the call a silent no-op. Returns
nothing.

Note: exposed as vc.force_release(obj), never obj:force_release() - method calls go through the
per-class __index dispatch, which only sees VC_REGISTER_MEMBER_FUNCTION registrations; a
lua_setfield() on the shared __vc_metatable is invisible to it (__gc/__call work there only
because the VM calls metamethods by name). */
```

## Why each fact landed where it did

Brief - the action in the context of the binding layer: ending Lua's ownership on demand. Not how,
and not the GC reasoning; just what having this function lets a caller do.

Core - the promises. Each one survives any rewrite of the body:

- the object is really gone, not merely collectable: the whole point of the function, and the one
  place the dated measurement earns its space, because no reader can derive it;
- it is unregistered from the name table: a caller can observe this, so it is a promise, not
  housekeeping;
- no wrapper survives anywhere: stated as the guarantee it is; which cache enforces it is not the
  caller's business;
- the userdata stays alive but empty: the surprising half of the contract, and the reason this is
  not simply "destroy it";
- the precondition: nothing else may hold the object. Stated as a property of the ownership graph
  (acyclic, owning shared_ptr down, raw pointers back up) rather than by naming a type from another
  repo, which is what made the comment self-contained without weakening the constraint.

Detail - the mechanisms. Both could be reimplemented differently tomorrow without changing a single
promise above: clearing self_obj instead of destroying the box, and the erase-before-reset ordering
in the wrapper cache. They stay in the comment because each prevents a specific crash, and each
names that crash.

Params - where the silent no-op finally landed. It is interface behaviour, not a quirk of the body,
and no earlier attempt had a section that wanted it.

Note - the plain-function vs method registration gotcha: unexpected for someone using the code, and
not a promise about what the function does.

## The sorting question worth asking

The name-table unregistration is the genuinely ambiguous one. It reads as core here because a caller
can observe it - but it is arguable that it is housekeeping and belongs in detail, and moving it
changes what the comment promises. When a fact sits on that line, ask rather than decide: the answer
is a statement about what the object is, and that is the author's to make.

## Cuts, and why they were safe

- the explanation of what a tracing GC is - general knowledge, not about this code;
- the leaf-node experiment confirming that shared_ptr cascades - its conclusion is in the core
  precondition, so the experiment was redundant with it;
- the pointer to push_vc_object()'s own comment - that comment still says it;
- the step-by-step crash narrative - replaced by the mechanism plus its date.

Nothing cut was a promise, a precondition, or a reason for a line of code. When a revision does drop
something, say so explicitly: shrinking a comment discards knowledge silently, and that is the whole
risk of the operation.
