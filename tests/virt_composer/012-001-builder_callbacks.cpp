#include "tests_common.h"

/* Test12 - Builder Callbacks (add_named_builder_callback / add_auto_builder_callback)
================================================================================================= */

/* Two distinct extension points for teaching the parser about new object shapes:
- add_named_builder_callback(vs, "some::type", builder) - matched by exact `m_type` string, only
  reachable from build_object() (i.e. the node needs an `m_type` field). See build_object()'s
  `for (auto &[match, cbk] : vs->build_object_cbks) if (match == node["m_type"].as_str())`.
- add_auto_builder_callback(vs, analyser, builder) - matched structurally by an analyser predicate,
  only reachable from build_pseudo_object() (i.e. the node has NO `m_type` field). See
  build_pseudo_object()'s `for (auto &[match, cbk] : vs->build_psudo_object_cbks) if
  (match(name, node))`. */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_TAGGED);

struct tagged_t : public vc::object_t {
    std::string label;

    tagged_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~tagged_t() {}

    static vc::ref_t<tagged_t> create(std::string label) {
        auto ret = std::make_shared<tagged_t>(vc::object_t::Private{type_id_static()});
        ret->label = label;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_TAGGED; }
    static vc::object_type_e type_id_static() { return TC_TYPE_TAGGED; }

    inline std::string to_string() const override {
        return std::format("tagged_t[{}]: label={}", (void*)this, label);
    }
};

#include "../../virt_composer_end.h"

int test12_named_builder_callback() {
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    int call_count = 0;
    ASSERT_FN(add_named_builder_callback(vs.get(), "tc::tagged_t",
        [&call_count](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
        {
            call_count++;
            auto label = co_await vc::resolve_str(vs, node["label"]);
            auto obj = tagged_t::create(label);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return obj->to_related<vc::object_t>();
        }));

    auto path = write_temp_yaml("012-001-named",
        "thing:\n"
        "  m_type: tc::tagged_t\n"
        "  label: \"widget\"\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(call_count == 1));

    auto obj = vc::get_ref<tagged_t>(vs.get(), "thing");
    ASSERT_FN(CHK_PTR(obj.get()));
    ASSERT_FN(CHK_BOOL(obj->label == "widget"));

    return 0;
}

int test12_named_builder_not_matched_for_other_types() {
    /* The analyser is a plain string== match against `m_type` - a node with an unrelated m_type
    must not invoke this builder at all. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    int call_count = 0;
    ASSERT_FN(add_named_builder_callback(vs.get(), "tc::tagged_t",
        [&call_count](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task<vc::ref_t<vc::object_t>>
        {
            call_count++;
            auto label = co_await vc::resolve_str(vs, node["label"]);
            auto obj = tagged_t::create(label);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return obj->to_related<vc::object_t>();
        }));

    auto path = write_temp_yaml("012-001-notmatched",
        "an_int:\n"
        "  m_type: vc::integer_t\n"
        "  value: 5\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(call_count == 0));

    return 0;
}

int test12_auto_builder_callback() {
    /* An auto/pseudo-object builder only ever sees nodes with no `m_type` - here it recognizes
    any mapping node with a `label` key (a shape no builtin pseudo-object recognizer matches:
    scalars only, see build_pseudo_object()'s int/float/string branches in virt_composer.cpp). */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    int call_count = 0;
    ASSERT_FN(add_auto_builder_callback(vs.get(),
        [](const std::string&, fkyaml::node& node) -> bool {
            return node.is_mapping() && node.contains("label");
        },
        [&call_count](vc::virt_state_t *vs, const std::string& name, fkyaml::node& node)
                -> co::task_t
        {
            call_count++;
            auto label = co_await vc::resolve_str(vs, node["label"]);
            auto obj = tagged_t::create(label);
            vc::mark_dependency_solved(vs, name, obj->to_related<vc::object_t>());
            co_return 0;
        }));

    auto path = write_temp_yaml("012-001-auto",
        "thing:\n"
        "  label: \"gizmo\"\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(call_count == 1));

    auto obj = vc::get_ref<tagged_t>(vs.get(), "thing");
    ASSERT_FN(CHK_PTR(obj.get()));
    ASSERT_FN(CHK_BOOL(obj->label == "gizmo"));

    return 0;
}

int test12_builder_callbacks() {
    ASSERT_FN(test12_named_builder_callback());
    ASSERT_FN(test12_named_builder_not_matched_for_other_types());
    ASSERT_FN(test12_auto_builder_callback());
    return 0;
}

int main() {
    int ret = test12_builder_callbacks();
    print_test_result("012-001-builder_callbacks.cpp", ret >= 0);
    return ret;
}
