# DOCS_TODO.md

Tracks the doc-cleanup pass over `../../virt_composer.h`'s public/semi-public API surface -
undocumented functions (bare `TODO: desc`/`TODO: DOC`/no comment at all) and one case of a doc
comment that's gone stale (describes behavior the code no longer has). Compiled while searching
for bugs in `virt_composer` (see `BUGS.md` for the bugs found in the same pass).

**Process:** one item at a time, in order. For each: Claude gives an opinion on the current
documentation (missing/stale/why it matters) and a suggested doc comment; the user approves or
asks for changes, repeat until approved; only then move to the next item. Mark an item `[x]` here
once its documentation is actually written into `../../virt_composer.h` and approved - not before.

## Items

- [x] 1. `VIRT_COMPOSER_ENABLE_LUA_IO` - documented. The security-relevant coupling with
      `get_file_string_content()`'s sandboxing check was deliberately kept OUT of the public doc
      (that function is `static`/private, a dead-end reference for a header reader) and instead
      written as an internal comment right above the `#if` guard in `virt_composer.cpp` itself.
- [x] 2. `VIRT_COMPOSER_ENABLE_LUA_OS` - documented, same shape as #1. Also added a shared TODO
      comment right after both macros: they're currently a single compile-time, process-wide
      switch - worth considering making Lua io/os access per-`virt_state_t` and/or dynamically
      toggleable at runtime instead, for whichever of the two is already enabled.
- [x] 3. `VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER` - documented. Clarified it's a YAML-`!copy`/
      `resolve_memb<T>()`-facing mechanism (memcpy accessor), unrelated to Lua's
      `vc.<name>.<member>` access despite the naming pattern shared with `VC_REGISTER_MEMBER_OBJECT`.
- [x] 4. `err_e` enum - documented, each value's meaning traced from its actual return sites
      rather than guessed from the name.
- [x] 5. `except_t` - documented (backtrace-prepending constructor, `luaw_catch_exception()`'s
      special-cased catch clause for it).
- [x] 6. `virt_state_t` - documented. Kept to what's actually actionable (independence between
      instances, and the one exception: `lua_function_t::internal_funcs` is process-wide static) -
      left out an internal field enumeration that didn't help a reader who can't access those
      fields anyway.
- [x] 7. `to_string(object_type_e)` / `to_string(ref_t<T>)` / `to_string(const object_t&)` -
      documented. Found and fixed a real bug along the way: `to_string(const object_t&)` had a
      stray, unused `template <typename T>` (T never appeared in the parameter list, so it was
      never deducible - the function was uncallable without an arbitrary, meaningless explicit
      `<T>`). Confirmed zero existing callers anywhere in the codebase, so removing the template
      parameter (declaration + definition) changes nothing for anyone - just makes it callable.
- [x] 8. `get_ref<T>()` - documented, and its segfault-on-missing-name bug fixed alongside it (null
      check added to the implementation). Note for next time: bundling a code fix with a doc
      change needs its own explicit confirmation, not just "apply it" on the combined proposal -
      see the `feedback-ask-before-code-changes` memory.
- [x] 9. `get_enum_val<T>(node, enum_vals)` - documented, with a real example pulled from
      `vulkan/vulkan_composer.h` (confirmed the same one-line-forwarding pattern also appears
      independently in `../math_writer/math_expr_composer.h`).
- [x] 10. `get_enum_val<T>(node)` (the deleted single-arg overload) - documented, explaining the
       deliberate `= delete`-as-SFINAE-detection-point mechanism `is_vc_enum<T>` relies on.
- [x] 11. `resolve_memb<T>()` - documented. Dropped "asynchronously" from the framing (copied from
       `resolve_obj<T>()`'s existing doc) per user feedback: nothing here waits on real I/O, the
       coroutine engine is only repurposed so objects can be declared in any order in YAML. Note:
       `resolve_obj<T>()`/`resolve_int()`/`resolve_float()`/`resolve_str()`'s *existing* docs (not
       on this list, already written pre-session) likely have the same "asynchronously" framing and
       may be worth revisiting for the same reason later.
- [x] 12. `add_lua_tab_funcs()` - documented. Caught myself repeating item #1's mistake (naming
       `internal_create_object`, a `static`/private function, in the public doc) - fixed before
       applying.
- [x] 13. `add_lua_flag_mapping()` (the `vector<pair<lua_Integer,string>>` overload) - documented,
       precise about it being independent from `get_enum_val<T>`'s string-lookup path rather than
       the same mechanism.
- [x] 14. `add_lua_flag_mapping<T>()` (the `unordered_map<string,T>` overload) - documented, with
      the same-table-backs-both-registrations pattern confirmed via `vulkan_composer.h`.
- [x] 15. `set_trivial_copy_member()` - documented. Noted it shares the same
      registration-order/inheritance-propagation rule as `set_lua_class_member()`.
- [x] 16. `set_base_derived_relation()` - documented (points to `register_inheritance<T,U>()`'s
      full writeup, flags that this raw form skips the compile-time relationship check).
- [x] 17. `get_ref_base()` - documented, complementary to `get_ref<T>()`'s doc rather than
      repeating it.
- [x] 18. `resolve_memb_data()` - documented, points to `resolve_memb<T>()`'s doc for the
      user-facing behavior rather than repeating it.
- [x] 19. `get_object_from_lua()` - documented (added during the `__gc`/leak fix earlier this
      session).
- [x] 20. `push_vc_object()` - documented (the stale one). First draft leaked internal machinery
      (`box_t`, the weak cache table, `lua_newuserdatauv`) into the public doc - trimmed down to
      just the observable contract (identity preserved across pushes, proper GC-tracked lifetime)
      per feedback.

## Not on this list (noted while compiling it, but out of scope)

- `virt_composer.cpp` is all internal (`static`/file-local) - this list only covers the public API
  surface in `virt_composer.h`.
- Line ~900's `/* TODO: add the functions to add the exception callbacks */` is a marker for a
  feature that doesn't exist yet, not an undocumented function - nothing to document there.
- Pure implementation-detail templates in the `/*! IMPLEMENTATION` section (`luaw_param_t`
  specializations, `luaw_function_wrapper_impl`, `is_vc_ref_t`, etc.) are internal
  overload-resolution machinery, not things a user calls directly - left out deliberately.
