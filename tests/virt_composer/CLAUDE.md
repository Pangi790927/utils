# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this
directory.

## What this directory is

This is the test suite for **virt_composer** (`../../virt_composer.h` + `../../virt_composer.cpp`),
a C++20 framework that builds a pool of objects (`vc::object_t`-derived) whose members and member
functions are backed by C++ but can be configured via YAML and scripted via Lua. It's structured
and built the same way `../../../co-lib/tests/` tests `colib.h` - see that directory's own
`CLAUDE.md` for the template this one was copied from. The two suites are independent: this one
never links against colib.h/co-lib's tests, it only depends on virt_composer.h's own transitive
includes (`co_utils.h`'s coroutines, `yaml.h`'s fkyaml, `minilua.h`, `tinyexpr.h`, `debug.h`).

Test files are numbered standalone `.cpp` programs at the top of this directory (`001-001-*.cpp` ..
`018-001-*.cpp`), each self-contained with its own `main()`. `tests_common.h` provides shared
helpers used by all of them. The `MAJOR` number is a topic category, not a sequence - see the table
below for what each number means.

## Build & run

Same split as co-lib/tests: the top-level `makefile` dispatches to `windows.makefile` (`cl`,
verified working in this environment - MSVC 19.43, Developer Command Prompt on PATH) or
`linux.makefile` (`g++`, written by analogy with `../makefile`'s flags but **not yet built/run
anywhere** - verify it before trusting it blindly).

```bash
make            # Linux/Windows default target, builds + runs every test
make clean
```

To build/run a single test, use the file's basename plus `.exe`/`.bin` as the make target, or
compile directly (Windows, from a Developer Command Prompt):

```bash
make 001-001-builtin_object_create.exe
./001-001-builtin_object_create.exe

# or, equivalent to what windows.makefile does (compile the core object once, then per test):
cl /nologo /EHsc /await:strict /std:c++20 /Zc:preprocessor /Zi /I..\.. /c ..\..\virt_composer.cpp /Fo:virt_composer_core.obj
cl /nologo /EHsc /await:strict /std:c++20 /Zc:preprocessor /Zi /I..\.. 001-001-builtin_object_create.cpp virt_composer_core.obj /Fe:001-001-builtin_object_create.exe /link
```

**`/Zc:preprocessor` is not optional on MSVC.** `debug.h`'s `DBG()`/`ASSERT_FN()` macros use the
GCC `, ##__VA_ARGS__` comma-elision extension; called with zero variadic arguments (which
`virt_composer.cpp` itself does, e.g. `ASSERT_RET(nullptr, CHK_BOOL(VIRT_TYPES_INITIALIZED))` ->
`DBGE("FAILED: " #fn_call)` with no varargs), MSVC's default/legacy preprocessor mangles the macro
into invalid tokens (`error C2760`/`C3878` at the `DBG_RAW` call site). The conformant preprocessor
handles it correctly - this cost real time to track down, don't drop the flag.

**`NOMINMAX` is not optional either**, and has to be defined before *anything* pulls in
`<windows.h>` (which `colib.h` does transitively) - otherwise `windows.h`'s `min`/`max` macros
break `yaml.h`'s `std::numeric_limits<T>::max()` calls with the same kind of cryptic
`illegal token on right side of '::'` errors. `tests_common.h` defines it at the very top, before
including `virt_composer.h` - that's why it's always the *first* include in every test file here
(the opposite order from co-lib/tests, where `../colib.h` comes before `tests_common.h`).

There is no separate lint step - this is compiled-and-linked (not header-only, unlike colib.h),
correctness is judged by compiling, linking against the shared `virt_composer_core.obj`/`.o`, and
running each test binary.

## Conventions for tests in this directory

- **Include order is always `tests_common.h` first** (it defines `NOMINMAX` and pulls in
  `virt_composer.h` itself), then any custom `VIRT_COMPOSER_REGISTER_TYPE`/`vc::object_t`-derived
  struct declarations the test needs, then `"../../virt_composer_end.h"` last, right before
  `main()`. `virt_composer_end.h` locks in `VIRT_TYPE_CNT`/`VIRT_TYPES_INITIALIZED` for the whole
  translation unit, so it must come after every type registration in that file - tests with no
  custom types (most of them) just put it directly under the `tests_common.h` include.
- Use `DBG(fmt, ...)`, `ASSERT_FN(expr)`/`ASSERT_COFN(expr)` (`debug.h`/`co_utils.h`, pulled in
  transitively - no local redefinition needed), `CHK_BOOL(x)`/`CHK_PTR(x)` the same way co-lib's
  tests do.
- `parse_config()` only reads YAML from a file path (no in-memory-string overload) - use
  `write_temp_yaml(name, content)` (`tests_common.h`) to spill an inline YAML literal to
  `<name>.tmp.yaml` next to the binary before parsing it. Matches the `.gitignore`'s
  `*.tmp.yaml` rule.
- A Lua script that wants to reach `vc.<object_name>` has to `vc = require("virt_composer")`
  first - it is not a preset global (see 007-001's `test7_internal_func_called_from_lua`).
  `call_lua()` itself doesn't need this - it calls the target function directly via
  `lua_getglobal()`, `vc.*` access only matters inside the *called* script's own code.
- Each file's `main()` calls its own test function(s), then
  `print_test_result(filename, ret >= 0)`, and returns the result - same shape as co-lib's tests.
- New test files follow `<NNN>-<MMM>-<short_description>.cpp` (both zero-padded to 3 digits) so
  plain lexicographic sort matches numeric order and `make`'s wildcard target discovery works.
- **Category in both the filename and the header comment** - every file's `/* TestN - Category:
  detail` header leads with the category word from the table below.

## Categories (Test Files)

| # | Category | File(s) | Covers |
|---|----------|---------|--------|
| 001 | Builtin Objects | `001-001` | `integer_t`/`float_t`/`string_t` create/value/type_id/to_string, `to_related<T>()` cast success + throw |
| 002 | Virt State | `002-001` | `create_state()` lifecycle, independent states don't share objects, unknown-name lookup |
| 003 | YAML Typed Objects | `003-001` | explicit `m_type: vc::integer_t/float_t/string_t`, unknown `m_type` -> `VC_ERROR_GENERIC` |
| 004 | YAML Pseudo Objects | `004-001` | bare scalar shorthand (no `m_type`) for the same three builtins, wrong-cast throw |
| 005 | References & Dependencies | `005-001` | forward/backward `!ref`, unresolved `!ref` is a silent (non-error) miss |
| 006 | Lua Scripts | `006-001` | `m_source` inline, `m_source_path` file, both/neither field is invalid |
| 007 | Lua Functions | `007-001` | `add_internal_func()` + `"[INTERNAL]"` binding, calling via `vc.<name>(...)`, unknown name fails init |
| 008 | `call_lua()` types | `008-001` | scalar/void/`ref_t`/vector/tuple round trips (both as arguments and as the return type `R`), unknown function -> `VC_ERROR_FAILED_CALL` |
| 009 | Custom Objects & Members | `009-001` | user `vc::object_t` type registration, `VC_REGISTER_MEMBER_OBJECT`/`_FUNCTION`, get/set/call from Lua |
| 010 | Trivial Copy Members | `010-001` | `VC_REGISTER_TRIVIALLY_COPIABLE_MEMBER` + `!copy`/`resolve_memb<T>()`, unregistered member fails |
| 011 | Inheritance | `011-001` | `register_inheritance<T,U>()` member propagation, registration-order sensitivity, 3-level hierarchies need every pair registered (not just adjacent links) |
| 012 | Builder Callbacks | `012-001` | `add_named_builder_callback` (m_type match) vs `add_auto_builder_callback` (structural match) |
| 013 | Expression Resolution | `013-001` | `resolve_int`/`resolve_float` via tinyexpr, `constants` table, rounding, invalid expr |
| 014 | Error Handling | `014-001` | missing file, malformed YAML, unterminated-flow-is-not-an-error, duplicate name, null object |
| 018 | Reproduced Bugs | `018-001` | permanent regression checks for fixed bugs that used to be `BUGS.md` entries (see co-lib's Category 18 for the convention this follows) |
| 019 | Operators | `019-001` | `vc::set_class_operator()`/`operator_e`, all binary ops (ADD/SUB/MUL/EQ/LT/LE/CONCAT) + unary (UNM/LEN), the `which` (1 vs 2) argument's role for non-commutative ops, and the "neither operand has a handler" error path |
| 020 | Lua Objects | `020-001` | `vc::lua_object_t`/`capture_lua_object()` - capturing a Lua callback into C++ (`vc::ref_t<lua_object_t>` as a member-function param), `call<R>(...)` (typed convenience) vs `call(L, nargs)` (raw primitive, incl. `LUA_MULTRET`), and pushing a captured value back to Lua as the original callable (not a re-boxed userdata) |

## Working docs in this directory

`DOCS_TODO.md` tracked a doc-cleanup pass over `../../virt_composer.h`'s public API surface - 20
items (undocumented functions/macros, plus one stale doc comment), all done. Worked one at a time
with the user (opinion + suggested doc -> approval/revision -> next) - same process now continuing
in `DOC_ACCURACY_TODO.md`, a follow-up audit of *existing* doc comments (not missing ones) for
claims that don't match the actual implementation - misattached comments, wrong return/error
claims, omitted branches, dead `@ref`s. Check there before assuming a doc comment in
`virt_composer.h` is accurate; check an item off once its fix actually lands and is approved.

`BUGS.md` logs currently-open bugs found in `../../virt_composer.h`/`.cpp` while writing these
tests (same "remove the entry once fixed, don't keep a changelog" convention as co-lib's `BUGS.md`).
Currently open (see the file for details): `create_yaml_from_lua_object()`'s dict-building loop
corrupting Lua table iteration for any non-string key; `to_related<T>()`'s failed-cast error
message always naming the same wrong type (`typeid` on an un-dereferenced pointer). Four
former entries:
- `get_ref<T>()` segfaulting (not failing gracefully) on a name that isn't registered - fixed
  during the `DOCS_TODO.md` pass (a null check, applied alongside finally documenting the
  function).
- Every object pushed into Lua leaking for the `virt_state_t`'s lifetime (light-userdata `__gc`
  never firing) - fixed; `018-001-reproduced_lua_object_leak.cpp` is the permanent regression check
  (see Category 18 above).
- `call_lua<R>()` failing to compile for any `R` without an implicit single-`int` constructor -
  fixed directly in `call_lua()`'s argument-push failure path (a value-initialized, correctly-typed
  placeholder instead of a bare `0`); no separate `018-*` file needed, `008-001-call_lua_types.cpp`
  and `009-001-custom_object_members.cpp` just stopped routing around it and exercise
  `std::vector<T>`/`std::tuple<...>` as `call_lua()` return types directly now.
- `register_inheritance()` "not propagating past one level" briefly had its own `BUGS.md` entry
  too, but turned out not to be a defect on closer look - `register_inheritance<base_t,
  grandchild_t>(vs)` is a perfectly valid, independent call for a non-adjacent pair (`is_base_of_v`
  is satisfied transitively regardless of hierarchy depth), so a 3+-level hierarchy just needs
  every pair registered, not only the adjacent links. Entry removed without any code fix; see the
  full writeup in `register_inheritance()`'s doc comment in `../../virt_composer.h` and
  `011-001-register_inheritance.cpp`'s two `test11_three_level_hierarchy_*` tests.

## Workflow for adding a new test

Same as `../../../co-lib/tests/CLAUDE.md`'s: one test at a time, compiled and run before moving to
the next; a genuine library defect found along the way gets logged in `BUGS.md` (and the test
adjusted to route around it, with a comment explaining why, if the defect is a *compile*-time one -
unlike a runtime crash/assertion bug, a broken build can't be committed as a "test expected to fail
until fixed" the way co-lib's category-18 tests are, since `make all` builds every `.cpp` in this
directory unconditionally).
