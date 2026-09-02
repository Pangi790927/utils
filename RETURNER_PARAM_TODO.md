# Returner/Param as source of truth — working notes

Working doc for an in-progress effort on `virt_composer.h`: making `luaw_returner_t<T>` (C++→Lua)
and `luaw_param_t<T,index>` (Lua→C++) the single source of truth for type conversion, instead of
`luaw_push_cpp_object()`/`luaw_lua_to_cpp_object()` duplicating conversion logic inline for the
categories they special-case (string/bool/int/float/vector/tuple/pair/enum/ref_t). Same
one-item-at-a-time, opinion-then-approval process as `tests/virt_composer/DOC_SWEEP_TODO.md` -
update this file as work continues, remove it once the effort is done rather than keeping it as a
changelog.

Everything below was worked out interactively; this file exists so a future session can resume
without re-deriving it. Verify line numbers against current `virt_composer.h` before trusting them
- the file has moved around during this effort already.

## Why this started

`luaw_push_cpp_object()`/`luaw_lua_to_cpp_object()` back `VC_REGISTER_MEMBER_OBJECT`'s getter/
setter. They dispatch on a *closed* `if constexpr` category list, hard-failing
(`demangle_static_assert`) for anything not in it - e.g. `math_expr_composer.h`'s `mexpr_t::symb`/
`symb_off` (types `char_t`/`ImVec2`) can't use `VC_REGISTER_MEMBER_OBJECT` at all today, even
though both types already have real `luaw_returner_t`/`luaw_param_t` specializations from being
usable as ordinary function params/returns. The fix under discussion: give both functions a
generic fallback branch, per type, that delegates to `luaw_returner_t<Type>`/
`luaw_param_t<Type,-1>` once that type's specific differences (if any) have been checked and
reconciled - **not** a blanket fallback added all at once, one type at a time, verified against the
real test suite each time.

## Full category matrix (as of the start of this effort)

`luaw_returner_t` specializations: bool, integral, floating-point, `const char*`, `std::string`,
`void*`, `vc::ref_t<T>`, `std::tuple<Args...>`, `std::vector<T>`, and now (added this session)
enum (`is_vc_enum`-constrained). No `std::pair` specialization exists.

`luaw_param_t` specializations: `void*`, `vc::ref_t<T>`, `bm_t<T>`, bool, integral, floating-point,
`const char*`, `std::tuple<Args...>`, `std::pair<A,B>`, `std::vector<T>`. No `std::string`
specialization exists (only `const char*`).

Compared against what `luaw_push_cpp_object`/`luaw_lua_to_cpp_object` currently handle inline
(same 9 categories on both: string/bool/int/float/vector/tuple/pair/enum/ref_t):

| category | push (`luaw_returner_t`) | pull (`luaw_param_t`) |
|---|---|---|
| bool / integral / float | fold trivially, no behavior change | fold trivially, no behavior change |
| `std::string` | folds (`luaw_returner_t<std::string>` exists) | **can't fold** - no `luaw_param_t<std::string,...>` exists, would need adding one |
| `const char*` | not handled inline today, would be "introduced" for free - **but unused anywhere in utils/cs_vulkan/math_writer**, checked exhaustively (no `VC_REGISTER_MEMBER_OBJECT` member, no `VC_REGISTER_MEMBER_FUNCTION` param/return uses it) | same - deliberately kept out of the member-*setter* path instead, see `luaw_setter_blacklist_t` below |
| `void*` | not handled inline today, "introduced" for free, unused anywhere too | same |
| `std::vector<T>` | folds, differences reconciled and applied - see below | folds, differences reconciled and applied - see below |
| `std::tuple<Args...>` | folds structurally, differences **not yet audited** (should mirror vector's) | folds structurally, differences **not yet audited** |
| `std::pair<A,B>` | **can't fold** - no `luaw_returner_t<std::pair<...>>` exists | folds (`luaw_param_t<std::pair,...>` exists, delegates to the tuple specialization) - **not yet audited** for behavioral differences |
| enum (`is_vc_enum`) | **now folds** - `luaw_returner_t<Enum>` added this session, see below | doesn't fold directly - only `bm_t<T>`-wrapped enums have a `luaw_param_t`; a bare enum type was never meant to be Lua-parseable without that wrapper (see `bm_t` section below), this is by design, not a gap |
| `vc::ref_t<T>` | folds; one behavior delta noted (push: `ASSERT_FN`→`throw except_t` on failure, currently unreachable either way per `push_vc_object`'s own comment) | folds; essentially identical logic already, no real delta |

## `luaw_setter_blacklist_t` (done)

Added in `virt_composer.h`, right before `luaw_lua_to_cpp_object`'s doc comment:

```cpp
template <typename T>
struct luaw_setter_blacklist_t : std::false_type {};

template <>
struct luaw_setter_blacklist_t<const char *> : std::true_type {};
```

...and checked as the *first* `if constexpr` branch in `luaw_lua_to_cpp_object`, ahead of every
other category (including the eventual generic fallback), so a blacklisted type can never reach a
branch that would otherwise happily convert it.

Why: `const char*`'s `luaw_param_t` returns a pointer into Lua's own GC-owned string storage,
valid only for the current call. That's fine for an ordinary function parameter -
`luaw_function_wrapper_impl`/`luaw_member_function_wrapper_impl` call `luaw_param_t` *directly*,
never through `luaw_lua_to_cpp_object` - but not fine for `VC_REGISTER_MEMBER_OBJECT`'s setter,
which stores the value into a long-lived C++ object member. Since `VC_REGISTER_MEMBER_OBJECT`
always registers getter+setter together (no read-only variant), blacklisting the setter path
correctly blocks the whole member-object registration for `const char*`, not just half of it - an
author who wants Lua to *read* such a member anyway falls back to a `VC_REGISTER_MEMBER_FUNCTION`
getter (the existing `mexpr_t::get_symb()`-style pattern).

**Known gap, not yet resolved**: the blacklist only checks the top-level `Type` a given
`luaw_lua_to_cpp_object` instantiation runs for. It does **not** recurse into container element
types - e.g. `std::vector<const char*>` as a member type isn't itself blacklisted, so after the
vector delegation (below) its elements would still convert via `luaw_param_t<const char*,-1>` and
end up stored in a long-lived vector member, the same hazard one level deeper. Currently inert
(`const char*` isn't used as a vector/tuple/pair element type anywhere in the three repos either),
but real once someone tries it. Left as an open decision - fix by making the blacklist check
recurse into `is_vector`/`is_tupple`/`is_pair`'s element types, or leave it and rely on it staying
unused.

## `luaw_returner_t<Enum>` (done)

Added right after the `is_vc_enum` concept definition (moved earlier in the file so it's declared
before the point where it's needed - see below):

```cpp
template <is_vc_enum Enum>
struct luaw_returner_t<Enum> {
    void luaw_ret_push(lua_State *L, Enum x) {
        lua_pushnumber(L, (int)x);
    }
};
```

Confirmed `std::integral<T>` never matches an enum type (`is_integral_v` is false for enums even
though they convert to/from int implicitly) - this was a real gap, not a redundant addition.
Relocates the exact line `luaw_push_cpp_object`'s own `is_vc_enum` branch already used, doesn't
change behavior there yet (that branch hasn't been switched to delegate - straightforward next
step, same shape as the vector delegation below).

**Placement note for future sessions**: the `is_vc_ref_t`/`is_tupple_t`/`is_pair_t`/`is_vector_t`/
`is_vc_enum` helper-trait block was moved from its original spot (right before
`luaw_push_cpp_object`) to right after `luaw_param_t<std::vector<T>,index>`, i.e. before *all* the
`luaw_returner_t` specializations - specifically so `is_vc_enum` is declared early enough for the
new returner to sit in-block rather than exiled somewhere with a comment explaining why. This was
a real placement decision, not a random move - if it needs to move again, think about why it's
there before relocating it.

## `bm_t<T>` (checked, no gap - confirmed correct as designed)

Grepped every `bm_t<` occurrence across `utils`, `cs_vulkan`, `math_writer`: always a bare
top-level entry in a `VC_REGISTER_MEMBER_FUNCTION`/`luaw_function_wrapper` `Params...` list, never
a struct member's declared type, never nested inside `std::vector<...>` in practice. Confirmed by
the file's own doc comment (`:409-412`, line numbers approximate): `bm_t<T>` "has no distinct
existence beyond [parsing] - it's stripped back down to plain T right after parsing... a one-way
(Lua->C++) conversion helper." `luaw_param_t<bm_t<T>,index>::luaw_single_param` returns plain `T`,
never `bm_t<T>`.

Initially mischaracterized `std::vector<bm_t<SomeEnum>>` as unsupported/dormant - **wrong**, missed
that `de_bitmaptizize<std::vector<T>>` (a real specialization, recursively unwraps:
`using Type = std::vector<typename de_bitmaptizize<T>::Type>;`) exists specifically to make this
combination work. Verified live: built a standalone `flagbox_t` with a real member function
`int64_t sum_flags(std::vector<flag_e> flags)` (plain, unwrapped signature), registered as
`VC_REGISTER_MEMBER_FUNCTION(vs, flagbox_t, sum_flags, std::vector<vc::bm_t<flag_e>>)`, called from
Lua as `fb:sum_flags({"A", "B", 2})` - compiled and ran correctly (sum == 3). So: `bm_t` genuinely
never manifests as a real value/storage type anywhere, including inside vectors -
`std::vector<bm_t<T>>` is the intentional, working, registration-time-only spelling for "vector of
enum" params. Bare `SomeEnum` (or `std::vector<SomeEnum>`) was never meant to be Params-declarable
either, by the same rule that applies to the scalar case - not a gap to fix.

Side finding: any enum type used with `bm_t<T>` needs `operator|` defined, even if you never intend
to combine flags - `bm_t`'s single-value parser has a table-combining branch that gets
type-checked unconditionally (not `if constexpr`), regardless of which branch runs at runtime.

## `std::vector<T>` pull-side differences (done - all 5 resolved and applied)

Full audit of `luaw_lua_to_cpp_object`'s inline vector branch vs `luaw_param_t<std::vector<T>,
index>`:

1. **nil handling** - `luaw_param_t` treats nil as "empty vector"; inline had no nil check at all
   (nil failed `lua_istable`, soft-failed, left the target untouched). **Decision: keep
   nil→empty.** Matches the existing nil-as-default-value convention already used elsewhere in
   this file (e.g. `Integer` param's nil→0 - see this project's own memory file
   `virt-composer-int-nil-default.md`).
2. **failure severity on non-table/non-nil** - inline was `DBG()`+`return -1` (soft); `luaw_param_t`
   calls `luaw_push_error()` → `lua_error()`. **Decision: adopt `lua_error` as the one standard.**
   Corrected an earlier claim: in this build `lua_error()` doesn't `longjmp` - `minilua.h` compiles
   as C++, so `LUAI_THROW(L,c)` resolves to `throw(c)` (verified at `minilua.h:16343-16348`), a real
   C++ exception that unwinds destructors properly, not a raw longjmp. Already what
   `try{...}catch(...){luaw_catch_exception(L);}` at every wrapper boundary expects.
3. **recursive/nested element failures** - inline's per-element recursive call
   (`luaw_lua_to_cpp_object(L,-1,to_asign[i-1])`) never checked its own return value, so a failure
   nested inside e.g. `vector<vector<int>>` silently degraded to a default value while the whole
   call reported success. `luaw_param_t`'s per-element `luaw_param_t<T,-1>` call raises immediately
   via #2's mechanism at any depth. **Decision: nested failures must propagate** - fixed as a side
   effect of delegating (no separate code needed).
4. **`bm_t` interaction** - see the `bm_t` section above; not actually a concern for this fold once
   understood correctly.
5. **construction style** - inline pre-sizes (`std::vector<value_type> to_asign(len)`, requires
   `value_type` default-constructible) + assigns by index; `luaw_param_t` used `push_back` (no
   default-constructibility requirement, but also never called `reserve(len)` despite already
   knowing `len`, so it paid real reallocation cost). **Decision: pre-size.** This library's
   contract is default-constructible element types only, stated explicitly now. The actual
   "faster" argument isn't default-construct-vs-move-construct (near a wash for the types actually
   in use: `int64_t`/`ImVec2`/`char_t`/`ref_t<T>`) - it's that pre-sizing gets the single-allocation
   property for free, which `luaw_param_t`'s old unreserved `push_back` loop didn't have.

**Applied** (verified: full `math_writer` build clean, all 17 `tests/virt_composer/*.cpp` pass,
plus the standalone `bm_t`-in-vector check above re-verified against the new code):

- `luaw_param_t<std::vector<T>,index>` rewritten to pre-size (`Ret ret(len)`) + assign by index,
  keeping nil-tolerance and `luaw_push_error` on non-table, keeping `de_bitmaptizize` unwrapping.
- `luaw_lua_to_cpp_object`'s vector branch now delegates: `object = luaw_param_t<Type,-1>{}
  .luaw_single_param(L); return 0;` - replacing ~10 lines of duplicated logic. `index` hardcoded to
  `-1` since every current caller of `luaw_lua_to_cpp_object` already passes `-1` (same assumption
  the pre-existing enum branch already made for `bm_t<T>`).

## `std::vector<T>` push-side differences (partially done)

- Loop counter `int i` → `size_t i` in `luaw_returner_t<std::vector<T>>` - **applied** (was
  comparing signed `int` against `v.size()`, a `size_t`).
- Recursion target differs: inline `luaw_push_cpp_object` recurses into itself (reaching pair/enum
  too); `luaw_returner_t<std::vector<T>>` recurses into `luaw_returner_t<std::decay_t<T>>` only -
  so `vector<pair<...>>` would fail to compile if the inline vector branch were ever replaced by a
  delegation, until/unless `luaw_returner_t<std::pair<...>>` is also added. Not yet acted on -
  `luaw_push_cpp_object`'s vector branch has **not** been switched to delegate yet (only the pull
  side has). Do that once pair's push-side gap is either accepted-as-a-known-limit or closed.

## Regression test added (done)

`tests/virt_composer/009-001-custom_object_members.cpp`: added `point_quadrant_e` enum +
`get_enum_val<T>` specialization + `point_t::quadrant()` member function +
`test9_member_function_enum_return_from_lua()`, exercising `luaw_returner_t<Enum>` through the real
`VC_REGISTER_MEMBER_FUNCTION` return-push path (not just a unit-level check). Passes. Also updated
that directory's own `CLAUDE.md` category table (row 009) to mention the new coverage.

## Other things found along the way (not part of the main effort, noted for awareness)

- `tests/virt_composer/linux.makefile` is missing `-lbacktrace` (needed for
  `boost::stacktrace`'s libbacktrace backend, same flag `math_writer`'s own makefile already
  carries) - that suite's own `CLAUDE.md` already flags the linux makefile as never having been
  run before; this confirms it and is a one-line fix (`LIBS := -lpthread -ldl -lbacktrace`),
  **not yet applied** (worked around by compiling test binaries by hand instead, each time).
- `math_writer`'s own root `makefile`'s dependency tracking on `../utils/virt_composer.h` is
  unreliable (`virt_composer.o` didn't get rebuilt by plain `make` a couple of times despite the
  header changing - its `.d` file is stale from July). Worked around each time by
  `rm -f ../utils/virt_composer.o` before `make`. Not investigated further or fixed.

## Not started yet

- `std::tuple<Args...>` - same 5-point audit as vector, expected to look similar (nil→empty,
  `de_bitmaptizize`-equivalent already recursive, construction is pack-expansion so no
  pre-size-vs-push_back question the way vector had).
- `std::pair<A,B>` - pull side folds already (delegates to tuple internally) but hasn't been
  audited for behavioral differences; push side has no `luaw_returner_t` at all yet, would need
  adding one (trivially: build a 2-element table via `luaw_returner_t<A>`/`luaw_returner_t<B>`,
  mirroring `luaw_param_t<std::pair,...>`'s own delegation-to-tuple).
- `vc::ref_t<T>` - differences already scoped above (push: `ASSERT_FN` vs `throw`, both
  practically unreachable); not yet formally closed out or applied.
- Once vector/tuple/pair/ref_t are all closed out: decide whether `luaw_push_cpp_object`'s enum
  branch should switch to delegating too (now that `luaw_returner_t<Enum>` exists) - same shape as
  the vector delegation, likely the easiest remaining one.
- `std::string` on the pull side still has no `luaw_param_t<std::string,...>` - would need adding
  before that category could ever fold (currently, and permanently unless added, `luaw_param_t`
  offers no string-typed Lua→C++ conversion at all, only `const char*`).
- The `luaw_setter_blacklist_t` recursion gap noted above (vector/tuple/pair element types aren't
  checked, only the top-level type) - open decision, not yet resolved either way.
- `mexpr_t::symb`/`symb_off` (the original motivating example, in `math_writer/math_expr_composer.h`)
  still use `get_symb()`/`get_symb_off()` via `VC_REGISTER_MEMBER_FUNCTION` - haven't been migrated
  to `VC_REGISTER_MEMBER_OBJECT` yet (would need `luaw_push_cpp_object`/`luaw_lua_to_cpp_object` to
  actually grow their generic fallback branch first, which hasn't happened - only individual
  categories have been reconciled and delegated one at a time so far, no catch-all yet).

## How to verify

```bash
# math_writer (the real consumer) - full rebuild
cd ~/workspace/math_writer && rm -f ../utils/virt_composer.o && make

# tests/virt_composer suite - linux.makefile is missing -lbacktrace, so build by hand:
cd ~/workspace/utils/tests/virt_composer
rm -f virt_composer_core.o *.bin
g++ -std=c++2a -O0 -g -Wno-format-security -I../.. -c ../../virt_composer.cpp -o virt_composer_core.o
for f in *.cpp; do
  g++ -std=c++2a -O0 -g -Wno-format-security -I../.. "$f" virt_composer_core.o \
      -o "${f%.cpp}.bin" -lpthread -ldl -lbacktrace
done
python3 run_tests.py *.bin   # expect 17/17
```
