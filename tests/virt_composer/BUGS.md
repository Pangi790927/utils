# BUGS.md

Currently-open bugs found in `../../virt_composer.h`/`../../virt_composer.cpp` while writing these
tests. Once a bug is fixed, remove its entry entirely - this file only tracks what's still open,
not a changelog (mirrors `../../../co-lib/tests/BUGS.md`'s convention).

## `create_yaml_from_lua_object()`'s dict branch corrupts iteration for any non-string table key

**Where:** `virt_composer.cpp`, `create_yaml_from_lua_object()`'s `if (dict_detected) { ... }` loop.
`create_yaml_from_lua_object()` itself is `static` (internal linkage) and not declared anywhere in
`virt_composer.h` - it's private to `virt_composer.cpp`, reachable only indirectly, as the table ->
`fkyaml::node` conversion behind `vc.create_object(name, description)` (`internal_create_object()`,
also file-local). Confirmed: no other file references it, and it isn't part of the public API.

**What's broken:**

```cpp
while (lua_next(L, index) != 0) {
    const char *key = lua_tostring(L, -2);   // <-- mutates the KEY in place if it's a number
    if (key) {
        auto to_add = create_yaml_from_lua_object(L, -1);
        to_ret[key] = to_add;
    }
    lua_pop(L, 1);   // only pops the value - the (possibly now-mutated) key stays for lua_next
}
```

Per the Lua manual, `lua_tostring` on a *number* value converts it and **replaces the value on the
stack with the string** (unlike `lua_tonumber`/`lua_tointeger`, which only read). `lua_next`
requires the key to stay in its original, untouched form between calls to find "the entry after
this key" - mutating it here corrupts the very traversal state `lua_next` needs on its next call.

**How it was found & confirmed:** a scratch repro (`dict_key_repro.cpp`, not checked in) called
`vc.create_object("thing", {[2] = "x", [5] = "y"})` - a table with non-contiguous integer keys, so
`lua_rawlen()` returns `0`, `array_detected` is `false`, and the table is classified as a "dict".
- Wrapped in an inner Lua `pcall`: caught cleanly as `ok=false, err="invalid key to 'next'"` - a
  real Lua VM-level error, not a virt_composer one.
- Not wrapped (relying only on `call_lua()`'s own `lua_pcall`): still caught, `err=VC_ERROR_FAILED_CALL`,
  no crash - every real entry point into Lua code in this file (`luaL_dostring` for scripts,
  `call_lua()` for named calls) already sits behind some `lua_pcall`, and minilua.h's error
  propagation uses real C++ exceptions when compiled as C++ (confirmed by the `luaD_throw`
  "assumed not to throw but does" warning noted in earlier work here), so intervening C++ stack
  frames unwind correctly rather than this being a raw-longjmp safety hazard.
- An **initial** run did produce a hard segfault, but that traced back to a mistake in the repro
  itself (a missing `virt_composer_end.h` include leaving `VIRT_TYPES_INITIALIZED` false, so
  `create_state()` correctly returned `nullptr` and the repro then dereferenced it) - not this bug.
  With that fixed, the actual defect is a catchable Lua error, not a crash.

**Blocks:** `vc.create_object(name, {...})` with any table key that isn't already a string -
sparse/non-1-based integer keys (`{[2]=.., [5]=..}`), or any table deliberately using integers as
dictionary-style keys (a very natural Lua pattern, e.g. `{[100]="health", [200]="mana"}`). Fails
with the cryptic VM-level "invalid key to 'next'" rather than either working or producing an error
that actually explains what went wrong. Separately (same loop, lower severity): a **boolean** key
doesn't trigger the crash (`lua_tostring` returns `NULL` without mutating for non-number,
non-string types), but its entry is silently dropped from the resulting object with no error at
all, since `if (key)` just skips it - silent data loss for any table keyed by `true`/`false`.

**Possible fix:** never call `lua_tostring` directly on the traversal key. Duplicate it first (the
standard idiom for exactly this Lua gotcha) so only the throwaway copy gets mutated:

```cpp
while (lua_next(L, index) != 0) {
    lua_pushvalue(L, -2);              /* duplicate the key: [..., key, value, key_dup] */
    const char *key = lua_tostring(L, -1);
    if (key) {
        auto to_add = create_yaml_from_lua_object(L, -2);   /* value is now at -2 */
        to_ret[key] = to_add;
    }
    lua_pop(L, 2);                     /* pop value + key_dup, original key survives for lua_next */
}
```
For the boolean-key data-loss case, decide deliberately (rather than silently) - e.g. explicitly
reject a table with a non-string/non-number key via `luaw_push_error()` instead of dropping it.

## `to_related<T>()`'s error message always names the same (wrong) type

**Where:** `virt_object.h`, `object_t<Id>::to_related<T>()`.

**What's broken:**

```cpp
template <typename T>
ref_t<T> to_related() {
    auto ret = std::dynamic_pointer_cast<T>(shared_this());
    if (!ret)
        throw std::runtime_error{
                std::format("Tried to build a reference of invalid type {}[{}] to {}",
                demangle(typeid(this).name()), type_id().name(), demangle<T>())};
    return ret;
}
```

`typeid(this)` takes the `typeid` of the *pointer expression* `this`, not of the object it points
to - since `this` isn't dereferenced, this is resolved at compile time to the pointer's *static*
type, `object_t<Id>*`, regardless of which derived class the call actually happens on. The message
is meant to show what the object's real (dynamic) type was, to explain why the cast failed - it
never can, because this half of the message is identical for every single call site.

**How it was found & confirmed:** a scratch repro (`typeid_repro.cpp`, not checked in) built a
`vc::integer_t` named `an_int`, then referenced it as `!ref an_int` from a `vc::string_t`'s `value`
field (a genuine type mismatch: `integer_t` cast to `string_t`). The actual thrown message:

```
Tried to build a reference of invalid type struct virt_object::object_t<class virt_object::EnumClass<...>> * to struct virt_composer::string_t
```

- notice it says `object_t<...>*` (the always-the-same pointer type), not `integer_t` (what the
object actually was). The `[VC_TYPE_INTEGER]` part in the middle of the real message *is* correct
(`type_id()` is virtual, so that call dispatches properly) - only the first `{}` is broken, so the
message is misleading rather than completely useless.

**Aside (found while confirming this one, not itself a live bug):** `depend_resolver_t<T>::await_resume()`
in `virt_composer.h` has the same `typeid`-on-a-pointer pattern in `internal_get_obj_type_name()`
(`virt_composer.cpp`) feeding its own "Invalid reference..." message - but that whole branch
(`if (!ret) throw ...`) is unreachable dead code: `to_related<T>()` above never actually *returns*
null on a failed cast, it throws immediately, so `depend_resolver_t`'s own null-check after calling
it can never fire. Not worth a separate entry, but worth knowing if either function gets touched.

**Blocks:** nothing crashes here, but the diagnostic is actively misleading rather than merely
unhelpful - every failed cast, from any source type, reports the exact same generic base-pointer
type name for "what it actually was."

**Possible fix:** dereference before taking the `typeid` - `object_t<Id>` is polymorphic (has
virtual functions), so `typeid(*this)` correctly performs the vtable-based RTTI lookup for the
real, most-derived type: `demangle(typeid(*this).name())`.
