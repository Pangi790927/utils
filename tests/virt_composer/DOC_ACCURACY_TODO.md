# DOC_ACCURACY_TODO.md

Tracks a follow-up audit over `../../virt_composer.h`'s public section (everything before the
`/*! IMPLEMENTATION` banner): every existing doc comment cross-checked against what its code
actually does, looking for comments that are stale, wrong, or misattached - as opposed to
`DOCS_TODO.md`'s pass, which was about *missing* documentation. Compiled by a forked review after
`DOCS_TODO.md`'s 20-item pass completed.

**Process:** same as `DOCS_TODO.md` - one item at a time, in order. For each: Claude gives an
opinion/suggested fix; the user approves or asks for changes, repeat until approved; only then move
to the next item. Mark an item `[x]` here once its fix actually lands in `../../virt_composer.h`
and is approved - not before.

## Items

- [x] 1. `depend_resolver_t<T>` - fixed. Gave it its own accurate doc; the misattached content
      turned out to be a pure duplicate of `resolve_obj()`'s *own* (already-correct, differently
      worded) declaration-site doc, not unique content needing relocation - caught and corrected
      mid-edit rather than leaving a redundant second doc comment at `resolve_obj()`'s definition.
      Confirmed via real-code check that `depend_resolver_t` is only ever constructed inside
      `virt_composer.h`/`.cpp` itself (never by `vulkan_composer.h`/`math_writer`), so `[INTERNAL]`
      correctly stays on it while `resolve_obj()` (which composer authors do call directly) keeps
      the tag dropped.
- [x] 2. `new_anon_name()` - fixed. Confirmed `[INTERNAL]` stays accurate (zero uses in
      `vulkan_composer.h`/`math_writer`).
- [x] 3. `resolve_float()` - fixed (added the missing integer branch, corrected the count to four).
- [x] 4. `build_pseudo_object()` - fixed (added Floats + the `"lua_script"`-named special case,
      described behaviorally rather than naming `init_lua_script()`, which is `static`/private).
- [x] 5. `add_named_builder_callback()` / `add_auto_builder_callback()` - fixed. Also dropped
      `@see build_object_cbks`/`@see build_psudo_object_cbks` from both - dead-end references to
      `virt_state_t` member fields, and `virt_state_t` is fully opaque from the header.
- [x] 6. `object_t` / `ret_t` - fixed, replaced with what they actually alias
      (`virt_object::object_t<Id>`/`int64_t`).
- [x] 7. The repeated "supported types" note (4 occurrences) - fixed, added `std::pair`. Also
      added a TODO near `bm_t<T>`'s own doc flagging a separate asymmetry: `bm_t<T>` is only
      meaningful in the Lua->C++ (parameter) direction, not as a return type - worth investigating
      whether that's intentional.
- [x] 8. `create_state()` - fixed, documented both real `nullptr` paths (a third `ASSERT_RET` is
      currently dead code per item #5's fix, left out of the doc since it can't trigger today).

## Known, already-flagged, not repeated here

- `resolve_obj`/`resolve_int`/`resolve_float`/`resolve_str`'s "Asynchronously..." framing (same
  non-issue already noted in `DOCS_TODO.md`'s item 11 entry - nothing here waits on real I/O, the
  coroutine engine is repurposed purely so objects can be declared in any order).
