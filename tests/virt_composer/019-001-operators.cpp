#include "tests_common.h"

/* Test19 - Operators
=================================================================================================

Covers vc::set_class_operator()/operator_e - the Lua arithmetic/relational/misc metamethod
dispatch added to virt_composer.h/.cpp. Every vc object shares one Lua metatable, so `__add` (etc)
is wired once, globally, and internally looks up a per-(object_type_e, operator_e) handler
registered via set_class_operator(). The handler itself is a raw lua_CFunction - it sees the Lua
stack exactly as `__add`/etc received it: [operand1, operand2] for binary ops, plus a 3rd `which`
argument (1 or 2) telling it which operand is the one whose type triggered the dispatch (needed
for non-commutative ops, since Lua always passes operands in original left-to-right order
regardless of which side "self" is on) - just [operand1] for the three unary ops (UNM/BNOT/LEN),
where "which" would be meaningless (only one operand exists). */

VIRT_COMPOSER_REGISTER_TYPE(TC_TYPE_OP_VAL);

/* A minimal int64_t-wrapping object, just so operators have something to dispatch on - mirrors
009-001-custom_object_members.cpp's point_t. */
struct op_val_t : public vc::object_t {
    int64_t val = 0;

    op_val_t(vc::object_t::Private priv) : vc::object_t(priv) {}
    virtual ~op_val_t() {}

    static vc::ref_t<op_val_t> create(int64_t val) {
        auto ret = std::make_shared<op_val_t>(vc::object_t::Private{type_id_static()});
        ret->val = val;
        return ret;
    }

    virtual vc::object_type_e type_id() const override { return TC_TYPE_OP_VAL; }
    static vc::object_type_e type_id_static() { return TC_TYPE_OP_VAL; }

    inline std::string to_string() const override {
        return std::format("op_val_t[{}]: val={}", (void*)this, val);
    }
};

#include "../../virt_composer_end.h"

/* Reads the numeric value of the vc object at stack index `idx`, or - if it isn't one (e.g. the
other side of `op_val_obj + 5`) - falls back to treating it as a plain Lua number. This is what
lets every handler below support both "op_val_t op op_val_t" and "op_val_t op <number>". */
static int64_t operand_value(lua_State *L, int idx) {
    if (auto obj = vc::get_object_from_lua(L, idx))
        return obj->to_related<op_val_t>()->val;
    return (int64_t)lua_tointeger(L, idx);
}

/* ADD is commutative, so unlike SUB below it never needs to look at `which` (stack slot 3) -
operand_value(1) + operand_value(2) is correct regardless of which side triggered the dispatch. */
static int op_val_add(lua_State *L) {
    int64_t result = operand_value(L, 1) + operand_value(L, 2);
    vc::push_vc_object(L, op_val_t::create(result));
    return 1;
}

/* SUB is the non-commutative case `which` exists for: Lua always passes [a, b] in original
left-to-right order, so without `which` this handler couldn't tell "I am `a - b`" (self on the
left, which==1) apart from "I am `b - a`" (self on the right, which==2) - it only knows which
stack slot actually held the op_val_t whose SUB handler got invoked. */
static int op_val_sub(lua_State *L) {
    int which = (int)lua_tointeger(L, 3);
    int self_idx = which;
    int other_idx = (which == 1) ? 2 : 1;
    int64_t self_val = operand_value(L, self_idx);
    int64_t other_val = operand_value(L, other_idx);
    int64_t result = (which == 1) ? (self_val - other_val) : (other_val - self_val);
    vc::push_vc_object(L, op_val_t::create(result));
    return 1;
}

static int op_val_mul(lua_State *L) {
    int64_t result = operand_value(L, 1) * operand_value(L, 2);
    vc::push_vc_object(L, op_val_t::create(result));
    return 1;
}

/* Unary - only one operand exists (stack slot 1), so there's no `which` to read at all. */
static int op_val_unm(lua_State *L) {
    vc::push_vc_object(L, op_val_t::create(-operand_value(L, 1)));
    return 1;
}

static int op_val_eq(lua_State *L) {
    lua_pushboolean(L, operand_value(L, 1) == operand_value(L, 2));
    return 1;
}

static int op_val_lt(lua_State *L) {
    lua_pushboolean(L, operand_value(L, 1) < operand_value(L, 2));
    return 1;
}

static int op_val_le(lua_State *L) {
    lua_pushboolean(L, operand_value(L, 1) <= operand_value(L, 2));
    return 1;
}

/* CONCAT's operands are often not vc objects at all (e.g. `op_val_obj .. "!"` - the string literal
has no metatable), so this reads each side as either an op_val_t's val or a raw Lua string. */
static int op_val_concat(lua_State *L) {
    auto side_to_string = [](lua_State *L, int idx) -> std::string {
        if (auto obj = vc::get_object_from_lua(L, idx))
            return std::to_string(obj->to_related<op_val_t>()->val);
        return lua_tostring(L, idx);
    };
    std::string result = side_to_string(L, 1) + side_to_string(L, 2);
    lua_pushstring(L, result.c_str());
    return 1;
}

static int op_val_len(lua_State *L) {
    int64_t v = operand_value(L, 1);
    lua_pushinteger(L, v < 0 ? -v : v);
    return 1;
}

static vc::ref_t<vc::virt_state_t> make_state_with_op_val_registered() {
    auto vs = vc::create_state();
    if (!vs)
        return nullptr;

    VC_REGISTER_MEMBER_OBJECT(vs.get(), op_val_t, val);

    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_ADD, op_val_add);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_SUB, op_val_sub);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_MUL, op_val_mul);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_UNM, op_val_unm);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_EQ, op_val_eq);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_LT, op_val_lt);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_LE, op_val_le);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(),
            vc::VC_OPERATOR_CONCAT, op_val_concat);
    vc::set_class_operator(vs.get(), op_val_t::type_id_static(), vc::VC_OPERATOR_LEN, op_val_len);

    return vs;
}

int test19_add_object_and_number() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-add",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function add_objs(a, b) local r = a + b; return r.val end\n"
        "    function add_num(a, n) local r = a + n; return r.val end\n"
        "    function add_num_reversed(n, a) local r = n + a; return r.val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(3);
    auto b = op_val_t::create(4);

    auto [sum, err] = vc::call_lua<int64_t>(vs.get(), "add_objs", a, b);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(sum == 7));

    auto [sum_num, err2] = vc::call_lua<int64_t>(vs.get(), "add_num", a, 10);
    ASSERT_FN(CHK_BOOL(err2 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(sum_num == 13));

    /* `10 + a`: operand 1 (the plain number) has no metatable at all, so Lua only calls our
    dispatcher because operand 2 (a) has one - exercising the which==2 path even for a commutative
    op (ADD itself ignores `which`, but the dispatcher still has to find the handler on the right
    side first). */
    auto [sum_rev, err3] = vc::call_lua<int64_t>(vs.get(), "add_num_reversed", 10, a);
    ASSERT_FN(CHK_BOOL(err3 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(sum_rev == 13));

    return 0;
}

int test19_sub_which_direction() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-sub",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function sub_left(a, n) local r = a - n; return r.val end\n"
        "    function sub_right(n, a) local r = n - a; return r.val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(10);

    /* a - 5: a is operand 1 (which==1) -> self(10) - other(5) == 5. */
    auto [left, err] = vc::call_lua<int64_t>(vs.get(), "sub_left", a, 5);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(left == 5));

    /* 5 - a: a is operand 2 (which==2) -> other(5) - self(10) == -5. Without `which` telling the
    handler that *it* is on the right this time, there'd be no way to distinguish this call from
    the one above - both pass the handler the same [10, 5] pair of values on the stack. */
    auto [right, err2] = vc::call_lua<int64_t>(vs.get(), "sub_right", 5, a);
    ASSERT_FN(CHK_BOOL(err2 == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(right == -5));

    return 0;
}

int test19_mul() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-mul",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function mul_objs(a, b) local r = a * b; return r.val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(3);
    auto b = op_val_t::create(4);

    auto [prod, err] = vc::call_lua<int64_t>(vs.get(), "mul_objs", a, b);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(prod == 12));

    return 0;
}

int test19_unary_minus() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-unm",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function negate(a) local r = -a; return r.val end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(7);

    auto [neg, err] = vc::call_lua<int64_t>(vs.get(), "negate", a);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(neg == -7));

    return 0;
}

int test19_comparisons() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-cmp",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function are_eq(a, b) return a == b end\n"
        "    function is_lt(a, b) return a < b end\n"
        "    function is_le(a, b) return a <= b end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto five = op_val_t::create(5);
    auto five_b = op_val_t::create(5);
    auto six = op_val_t::create(6);

    {
        auto [eq1, err] = vc::call_lua<bool>(vs.get(), "are_eq", five, five_b);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(eq1 == true));

        auto [eq2, err2] = vc::call_lua<bool>(vs.get(), "are_eq", five, six);
        ASSERT_FN(CHK_BOOL(err2 == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(eq2 == false));
    }
    {
        auto [lt1, err] = vc::call_lua<bool>(vs.get(), "is_lt", five, six);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(lt1 == true));

        auto [lt2, err2] = vc::call_lua<bool>(vs.get(), "is_lt", six, five);
        ASSERT_FN(CHK_BOOL(err2 == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(lt2 == false));
    }
    {
        auto [le1, err] = vc::call_lua<bool>(vs.get(), "is_le", five, five_b);
        ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
        ASSERT_FN(CHK_BOOL(le1 == true));
    }

    return 0;
}

int test19_concat_with_string() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-concat",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function shout(a) return a .. \"!\" end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(5);

    auto [str, err] = vc::call_lua<std::string>(vs.get(), "shout", a);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(str == "5!"));

    return 0;
}

int test19_len() {
    auto vs = make_state_with_op_val_registered();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-len",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function do_len(a) return #a end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = op_val_t::create(-9);

    auto [len, err] = vc::call_lua<int64_t>(vs.get(), "do_len", a);
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_OK));
    ASSERT_FN(CHK_BOOL(len == 9));

    return 0;
}

int test19_incompatible_types_error() {
    /* Neither side registers ADD here: two builtin vc::integer_t objects, which never get any
    operator registered by default. Every vc object shares the same metatable (which always
    defines __add), so Lua invokes our dispatcher regardless - it's the dispatcher's own
    per-object_type_e lookup that comes up empty on both operands, and it must raise a Lua error
    rather than silently letting the operation through undefined. */
    auto vs = vc::create_state();
    ASSERT_FN(CHK_PTR(vs.get()));

    auto path = write_temp_yaml("019-001-incompatible",
        "script:\n"
        "  m_type: vc::lua_script_t\n"
        "  m_source: |\n"
        "    function do_add(a, b) return a + b end\n");
    ASSERT_FN(CHK_BOOL(vc::parse_config(vs.get(), path.c_str()) == vc::VC_ERROR_OK));

    auto a = vc::integer_t::create(1);
    auto b = vc::integer_t::create(2);

    auto [ret, err] = vc::call_lua<int64_t>(vs.get(), "do_add", a, b);
    (void)ret;
    ASSERT_FN(CHK_BOOL(err == vc::VC_ERROR_FAILED_CALL));

    return 0;
}

int test19_operators() {
    ASSERT_FN(test19_add_object_and_number());
    ASSERT_FN(test19_sub_which_direction());
    ASSERT_FN(test19_mul());
    ASSERT_FN(test19_unary_minus());
    ASSERT_FN(test19_comparisons());
    ASSERT_FN(test19_concat_with_string());
    ASSERT_FN(test19_len());
    ASSERT_FN(test19_incompatible_types_error());
    return 0;
}

int main() {
    int ret = test19_operators();
    print_test_result("019-001-operators.cpp", ret >= 0);
    return ret;
}
