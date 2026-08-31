# DOC_SWEEP_INTERNAL_TODO.md

Follow-up to [[DOC_SWEEP_TODO.md]] (which covered `../../virt_composer.h`'s *public* section, before
its `/*! IMPLEMENTATION` banner). This file covers everything that pass explicitly left out of
scope: the implementation section of `../../virt_composer.h` (line 1367 onward) and all of
`../../virt_composer.cpp` (entirely internal - it's a `.cpp`, not part of the public header API).

Two kinds of items, per file:
- **Documented** - has an explanatory comment attached already; audit it for accuracy, same as
  `DOC_SWEEP_TODO.md` did (cross-check against the real implementation, check for redundant wording
  or internal-mechanism leakage - though the "user-relevant only" bar from the public sweep doesn't
  apply the same way here, since *this* is the internal implementation; some mechanism detail is
  exactly what belongs in these comments).
- **Undocumented** - no comment at all (or only a bare forward-looking `/* TODO */` with no
  explanation of current behavior). These are candidates for *adding* a comment, not auditing one -
  decide case by case whether a comment is actually warranted (a trivial one-liner delegating to an
  already-documented sibling may not need its own), then write and add it.

Scope note: entries are at **declaration granularity** (functions, structs, macros, templates,
specializations, notable lambdas) - not every small inline comment scattered inside a function body
(e.g. a one-line `// pop sub-table` inside a function is not its own entry).

**Process:** same as `DOC_SWEEP_TODO.md` - work each file's list **bottom to top** (highest line
number first), one item at a time: opinion/proposed fix (or proposed new comment) -> user approves
or asks for changes -> check the box -> move to the next-lowest line number. Line numbers were
gathered via a background scan and may drift slightly from a fresh read - re-verify before trusting
one, especially far from where the sweep currently is.

---

## `../../virt_composer.cpp` (entire file - all internal)

### Documented (audit for accuracy)

- [x] `MAX_NUMBER_OF_OBJECTS` constant (line 28) - max number of named reference objects
- [x] `struct luaw_member_t` (line 31) - holds info for a member (function or object)
- [x] `struct trivial_copy_member_t` (line 38) - copy-fn + type_index to validate/copy a
      trivially-copyable member
- [x] `struct virt_state_t` (line 44) - brief header comment
- [x] `virt_state_t::L` field (line 48) - the Lua state for this virt state
- [x] `virt_state_t::lua_table_idx` field (line 53) - registry index of the "virt_composer" table
- [x] `virt_state_t::anonymous_increment` field (line 56) - names anonymous objects
- [x] `virt_state_t::build_object_cbks` field (line 96) - typed (`m_type`-based) builder callbacks,
      nestable (part of the large block at lines 58-89)
- [x] `virt_state_t::build_psudo_object_cbks` field (line 103) - structure-based builder callbacks,
      not nestable (same block, lines 58-89)
- [x] `virt_state_t::name_to_object` field (line 109) - name->object index
- [x] `virt_state_t::object_to_name` field (line 110) - reverse index
- [x] `virt_state_t::wanted_objects` field (line 117) - coroutines waiting on a named object
- [x] `virt_state_t::weak_cache_ref` field (line 121) - tracks Lua-side objects for `lua_object_t`
- [x] `virt_state_t::lua_object_ref_table` field (line 129) - dedicated non-weak table for
      `lua_object_t` captures
- [x] `virt_state_t::constants` field (line 132) - constants usable in tinyexpr expressions
- [x] `virt_state_t::tab_funcs` field (line 156) - free functions
- [x] `virt_state_t::trivial_copy_member` field (line 160) - memcpy-style member copy functions
- [x] `virt_state_t::lua_class_members` field (line 164) - member function/object getters
- [x] `virt_state_t::lua_class_member_setters` field (line 168) - member object setters
- [x] `virt_state_t::lua_class_operators` field (line 173) - per-class operator handlers
- [x] `virt_state_t::inheritance_table` field (line 178) - base->derived propagation sets
- [x] `box_t::self_obj` field (line 191) - the strong ref, Lua's actual claim on the object
- [x] `app_path` static (line 195) - path the application was run from
- [x] `create_state()` (line 213) - warns against global state creation
- [x] `resolve_int` (line 368) - follows `!ref` or returns direct value
- [x] `resolve_float` (line 380) - same, for float
- [x] `resolve_str` (line 395) - same, for string
- [x] `resolve_memb_data` (line 402) - "used inside resolve_memb"
- [x] `get_file_string_content` (line 427) - app-path filesystem restriction rationale
- [x] `push` member-function lambda inside `create_state` (line 231) - registered directly (not via
      macro) so it gets the real calling `L`
- [x] `__index` lambda inside `luaopen_vc` (line 740)
- [x] `__newindex` lambda inside `luaopen_vc` (line 774)
- [x] `__call` lambda inside `luaopen_vc` (line 793)
- [x] `__gc` lambda inside `luaopen_vc` (line 810) - explains name<->object erasure on collection
- [x] `luaw_binary_operator_dispatch<Op>` (line 653, template at 652)
- [x] `luaw_unary_operator_dispatch<Op>` (line 686, template at 685)
- [x] `luaw_catch_exception` (line 951) - prevents C++ exceptions escaping into Lua
- [x] `set_lua_class_member` (line 1004) - propagates via inheritance_table
- [x] `set_class_member_setter` (line 1019) - same, for setters
- [x] `set_class_operator` (line 1030) - same, for operators
- [x] `push_vc_object` (line 1046) - step-by-step weak-cache lookup/creation
- [x] `lua_object_t::capture_ref` (line 1075) - empty-stack vs nil vs release edge cases
- [x] `create_yaml_from_lua_object` (line 1141) - array-vs-dict table detection trick

### Undocumented (candidates for a new comment)

- [x] `virt_state_t::pool` field (line 45) - documented; also fixed the struct's own truncated
      header comment ("This holds the state of the */") while here
- [x] `virt_state_t::~virt_state_t()` destructor (line 181) - documented, cross-references the
      public @warning about outliving a virt_state_t
- [x] `struct box_t` (line 190) - documented, [INTERNAL]
- [x] forward declarations `luaw_init`, `internal_create_object`, `luaopen_vc` (lines 198-200) - no
      comment needed, their real definitions (further down, now all documented) carry the
      explanation
- [x] `except_t::except_t(const std::string&)` constructor (line 202) - pointer comment (already
      public elsewhere)
- [x] `get_ref_base` (line 248) - pointer comment (already public elsewhere)
- [x] `depend_resolver_internal_t::internal_check_depend` (line 254) - trivial, covered by the
      struct's own doc in virt_composer.h
- [x] `depend_resolver_internal_t::internal_mark_wait` (line 258) - same
- [x] `depend_resolver_internal_t::internal_get_dep_object` (line 262) - same
- [x] `depend_resolver_internal_t::internal_get_obj_type_name` (line 272) - same
- [x] `add_named_builder_callback` (line 277) - pointer comment (already public elsewhere)
- [x] `add_auto_builder_callback` (line 285) - pointer comment (already public elsewhere)
- [x] `add_lua_tab_funcs` (line 293) - pointer comment (already public elsewhere)
- [x] `add_lua_flag_mapping` (line 304) - pointer comment (already public elsewhere)
- [x] `mark_dependency_solved` (line 317) - pointer comment (already public elsewhere)
- [x] `resolve_string_as_expression` (line 344) - documented, [INTERNAL]
- [x] `starts_with` (line 423) - documented, [INTERNAL]
- [x] `init_lua_script` (line 452) - documented, [INTERNAL]
- [x] `exec_lua_src` lambda inside `init_lua_script` (line 466) - documented
- [x] `build_object` (line 498) - pointer comment (already public elsewhere)
- [x] `build_pseudo_object` (line 550) - pointer comment (already public elsewhere)
- [x] `new_anon_name` (line 593) - pointer comment (already public elsewhere)
- [x] `build_schema` (line 597) - documented, [INTERNAL]
- [x] `parse_config` (line 612) - pointer comment (already public elsewhere)
- [x] `luaopen_vc` (line 710) - documented, [INTERNAL]
- [x] `__tostring` lambda inside `luaopen_vc` (line 732) - documented
- [x] operator dispatch registration block (`__add` through `__le`, lines 835-890) - one shared
      comment above the block instead of per-line (each entry is self-evident once explained once)
- [x] `luaw_init` (line 918) - documented, [INTERNAL]
- [x] `luaw_get_virt_state` (line 981) - pointer comment (already public elsewhere)
- [x] `luaw_get_lua_state` (line 989) - pointer comment (already public elsewhere)
- [x] `set_trivial_copy_member` (line 993) - pointer comment (already public elsewhere)
- [x] `set_base_derived_relation` (line 1039) - pointer comment (already public elsewhere)
- [x] `get_object_from_lua` (line 1070) - pointer comment (already public elsewhere)
- [x] `luaw_push_error` (line 1102) - pointer comment (already public elsewhere)
- [x] `line_source` lambda inside `luaw_push_error` (line 1108) - documented
- [x] `internal_create_object` (line 1266) - documented, [INTERNAL]. Bonus: fixed a leftover
      "pops vulkan_utils table" comment inside it (copy-paste mistake - this is `vc`'s own table).

---

## `../../virt_composer.h` (implementation section, line 1367 onward)

### Documented (audit for accuracy)

- [x] `enum luaw_member_e` (line 1383) - member function vs. member object
- [x] `build_object()` (line 1408)
- [x] `build_pseudo_object()` (line 1433)
- [x] `new_anon_name()` (line 1442)
- [x] `luaw_push_error()` (line 1466)
- [x] `luaw_catch_exception()` (line 1491)
- [x] `luaw_get_virt_state()` (line 1501)
- [x] `luaw_get_lua_state()` (line 1511)
- [x] `set_trivial_copy_member()` (line 1536)
- [x] `set_lua_class_member()` (line 1552)
- [x] `set_class_member_setter()` (line 1567)
- [x] `set_base_derived_relation()` (line 1586)
- [x] `get_ref_base()` (line 1603)
- [x] `resolve_memb_data()` (line 1633)
- [x] `struct depend_resolver_internal_t` (line 1647)
- [x] `struct depend_resolver_t<T>` (line 1674)
- [x] `depend_resolver_t<T>::depend_resolver_t()` ctor (line 1676)
- [x] `depend_resolver_t<T>::await_ready()` (line 1680)
- [x] `depend_resolver_t<T>::await_suspend()` (line 1684)
- [x] `resolve_obj<T>()` definition (line 1712) - pointer comment to the public declaration's doc
- [x] `luaw_static_assert<B,T>()` (line 1751)
- [x] `struct luaw_param_t<Param,index>` primary template (line 1774)
- [x] `luaw_param_t<void*, index>` specialization (line 1792)
- [x] `luaw_param_t<vc::ref_t<T>, index>` specialization (line 1803)
- [x] `luaw_param_t<bm_t<T>, index>` specialization (line 1826)
- [x] `luaw_param_t<bool, index>` specialization (line 1895)
- [x] `luaw_param_t<Integer, index>` specialization (line 1903)
- [x] `luaw_param_t<Float, index>` specialization (line 1911)
- [x] `luaw_param_t<const char*, index>` specialization (line 1928)
- [x] `struct de_bitmaptizize<T>` primary template (line 1950)
- [x] `struct luaw_returner_t<T>` primary template (line 2040) - includes a TODO about extensibility
- [x] `luaw_member_function_wrapper<T,member_ptr,Params...>()` (line 2203)
- [x] `struct is_vc_ref_t<T>` primary template (line 2213)
- [x] `concept is_vc_enum` (line 2250) - fixed "an known" typo, rest accurate
- [x] `luaw_push_cpp_object<T>()` (line 2262) - comment noted as terse/somewhat mismatched
- [x] `call_on_stack<R,Args...>()` (line 2535)
- [x] `lua_object_t::capture()` (line 2629) - body comment on null-but-empty `oth`
- [x] `lua_object_t::push()` (line 2653) - body comment on "universe" comparison
- [x] `lua_object_t::call(lua_State*, int)` (line 2670)
- [x] `lua_object_t::call<R,Args...>()` templated overload (line 2697)

### Undocumented (candidates for a new comment)

- [x] `VIRT_TYPES_INITIALIZED` (line 1379) - documented (shared comment with VIRT_TYPE_CNT),
      [INTERNAL]
- [x] `VIRT_TYPE_CNT` (line 1380) - documented, [INTERNAL]
- [x] `get_ref<T>()` template definition (line 1606) - pointer comment (already public elsewhere).
      Bonus: found `get_ref_base()` right above it (line 1603) also has a full public-style doc
      that my scoped scan missed (past the banner but genuinely public) - audited it against the
      implementation, still accurate, no fix needed.
- [x] `depend_resolver_internal_t::depend_resolver_internal_t()` ctor (line 1650) - trivial,
      covered by the struct's own doc
- [x] `depend_resolver_internal_t::internal_mark_wait()` (line 1652) - same
- [x] `depend_resolver_internal_t::internal_check_depend()` (line 1653) - same
- [x] `depend_resolver_internal_t::internal_get_dep_object()` (line 1654) - same
- [x] `depend_resolver_internal_t::internal_get_obj_type_name()` (line 1655) - same
- [x] `depend_resolver_t<T>::await_resume()` (line 1694) - documented: the throw only fires on a
      null dependency, not a real cast failure (that throws std::runtime_error earlier, from
      to_related<T>() itself)
- [x] `resolve_memb<T>()` definition (line 1739) - pointer comment (already public elsewhere)
- [x] `add_lua_flag_mapping<T>()` definition (line 1757) - pointer comment (already public
      elsewhere)
- [x] `de_bitmaptizize<bm_t<T>>` specialization (line 1953) - self-evident, covered by primary's doc
- [x] `de_bitmaptizize<std::tuple<Args...>>` specialization (line 1956) - same
- [x] `de_bitmaptizize<std::pair<T,U>>` specialization (line 1961) - same
- [x] `de_bitmaptizize<std::vector<T>>` specialization (line 1966) - same
- [x] `luaw_param_t<std::tuple<Args...>, index>` specialization (line 1971) - documented: nil
      handling + the reverse-push/negative-index stack trick
- [x] `luaw_param_t<std::pair<Arg1,Arg2>, index>` specialization (line 1998) - documented:
      delegates to the tuple specialization
- [x] `luaw_param_t<std::vector<T>, index>` specialization (line 2009) - documented: nil handling +
      one-element-at-a-time conversion
- [x] `luaw_returner_t<bool>` specialization (line 2048) - self-evident, covered by primary's doc
- [x] `luaw_returner_t<Integer>` specialization (line 2055) - same
- [x] `luaw_returner_t<Floating>` specialization (line 2062) - same
- [x] `luaw_returner_t<const char*>` specialization (line 2069) - same
- [x] `luaw_returner_t<std::string>` specialization (line 2076) - same
- [x] `luaw_returner_t<void*>` specialization (line 2083) - same
- [x] `luaw_returner_t<vc::ref_t<T>>` specialization (line 2090) - documented: nil behavior + dead
      except_t throw
- [x] `luaw_returner_t<std::tuple<Args...>>` specialization (line 2102) - documented: per-element
      table build, cross-referenced against luaw_push_cpp_object()'s parallel implementation
- [x] `luaw_returner_t<std::vector<T>>` specialization (line 2119) - covered by the tuple
      specialization's comment (cross-referenced there)
- [x] `luaw_function_wrapper_impl<function,Params...>()` (line 2130) - documented, [INTERNAL]
- [x] `luaw_member_function_wrapper_impl<T,member_ptr,Params...>()` (line 2151) - documented,
      [INTERNAL]
- [x] `luaw_function_wrapper<Function,Params...>()` definition (line 2172) - pointer comment
      (already public elsewhere)
- [x] `is_vc_ref_t<vc::ref_t<T>>` specialization (line 2216) - covered by the primary's existing
      one-line comment, no separate note added
- [x] `is_vc_ref` constexpr bool (line 2219) - same
- [x] `struct is_tupple_t<T>` primary template (line 2222) - added a one-line comment (matching
      `is_vc_ref_t`'s existing style)
- [x] `is_tupple_t<std::tuple<Args...>>` specialization (line 2225) - covered by primary's comment
- [x] `is_tupple` constexpr bool (line 2228) - covered by primary's comment
- [x] `struct is_pair_t<T>` primary template (line 2231) - added a one-line comment
- [x] `is_pair_t<std::pair<A,B>>` specialization (line 2234) - covered by primary's comment
- [x] `is_pair` constexpr bool (line 2237) - covered by primary's comment
- [x] `struct is_vector_t<T>` primary template (line 2240) - added a one-line comment
- [x] `is_vector_t<std::vector<T,Alloc>>` specialization (line 2243) - covered by primary's comment
- [x] `is_vector` constexpr bool (line 2246) - covered by primary's comment
- [x] `demangle_static_assert<test,ToDisplay>()` (line 2255) - documented, noting the unused
      message param and ToDisplay's real role
- [x] `luaw_member_object_wrapper<T,member_ptr>()` (line 2333) - expanded from a one-word "getter"
      label, tagged [INTERNAL]
- [x] `luaw_lua_to_cpp_object<T>()` (line 2352) - documented, tagged [INTERNAL]
- [x] `luaw_member_setter_object_wrapper<T,member_ptr>()` (line 2469) - documented, tagged
      [INTERNAL] (true internal helper, no public declaration elsewhere)
- [x] `luaw_register_member_function<T,member_ptr,Params...>()` definition (line 2484) - documented
- [x] `luaw_register_member_object<T,member_ptr>()` definition (line 2490) - documented
- [x] `register_trivially_copyable_member<T,member_ptr>()` (line 2498) - documented
- [x] `register_inheritance<T,U>()` definition (line 2514) - documented the base/derived detection
      mechanism
- [x] `call_lua<R,Args...>()` definition (line 2577) - pointer comment to declaration
- [x] `get_enum_val<T>(node, enum_vals)` definition (line 2584) - pointer comment to declaration
- [x] `get_enum_val<T>(node)` deleted-overload definition (line 2605) - pointer comment to
      declaration
- [x] `to_string(object_type_e)` definition (line 2607) - single pointer comment for all three
- [x] `to_string(ref_t<T>)` definition (line 2611) - same
- [x] `to_string(const object_t&)` definition (line 2616) - same
- [x] `lua_object_t::capture_lua_object()` (line 2622) - documented
- [x] `lua_object_t::release()` (line 2639) - documented
- [x] `lua_object_t::~lua_object_t()` (line 2649) - documented
- [x] `c_function_t::create()` (line 2708) - documented: calls `init()`, throws on failure
- [x] `c_function_t::call()` (line 2718) - documented: invokes `_fn`, `-1` if never bound
- [x] `c_function_t::init()` (line 2726) - documented: only `"[INTERNAL]"` works today, DLL/SO is a
      TODO
