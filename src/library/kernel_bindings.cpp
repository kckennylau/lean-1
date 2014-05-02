/*
Copyright (c) 2013-2014 Microsoft Corporation. All rights reserved.
Released under Apache 2.0 license as described in the file LICENSE.

Author: Leonardo de Moura
*/
#include <utility>
#include <string>
#include "util/sstream.h"
#include "util/script_state.h"
#include "util/list_lua.h"
#include "kernel/abstract.h"
#include "kernel/for_each_fn.h"
#include "kernel/free_vars.h"
#include "kernel/instantiate.h"
#include "kernel/metavar.h"
#include "kernel/error_msgs.h"
#include "library/occurs.h"
#include "library/io_state_stream.h"
#include "library/expr_lt.h"
#include "library/kernel_bindings.h"

// Lua Bindings for the Kernel classes. We do not include the Lua
// bindings in the kernel because we do not want to inflate the Kernel.

namespace lean {
static environment get_global_environment(lua_State * L);
io_state * get_io_state(lua_State * L);

// Level
DECL_UDATA(level)
DEFINE_LUA_LIST(level, push_level, to_level)

int push_optional_level(lua_State * L, optional<level> const & l) {  return l ? push_level(L, *l) : push_nil(L); }

static int level_tostring(lua_State * L) {
    std::ostringstream out;
    options opts  = get_global_options(L);
    out << mk_pair(pp(to_level(L, 1), opts), opts);
    lua_pushstring(L, out.str().c_str());
    return 1;
}

static int level_eq(lua_State * L) { return push_boolean(L, to_level(L, 1) == to_level(L, 2)); }
static int level_lt(lua_State * L) { return push_boolean(L, is_lt(to_level(L, 1), to_level(L, 2))); }
static int mk_level_zero(lua_State * L) { return push_level(L, mk_level_zero()); }
static int mk_level_one(lua_State * L)  { return push_level(L, mk_level_one());  }
static int mk_level_succ(lua_State * L) { return push_level(L, mk_succ(to_level(L, 1))); }
static int mk_level_max(lua_State * L)  { return push_level(L, mk_max(to_level(L, 1), to_level(L, 2))); }
static int mk_level_imax(lua_State * L) { return push_level(L, mk_imax(to_level(L, 1), to_level(L, 2))); }
static int mk_param_univ(lua_State * L) { return push_level(L, mk_param_univ(to_name_ext(L, 1))); }
static int mk_meta_univ(lua_State * L)  { return push_level(L, mk_meta_univ(to_name_ext(L, 1))); }
#define LEVEL_PRED(P) static int level_ ## P(lua_State * L) { return push_boolean(L, P(to_level(L, 1))); }
LEVEL_PRED(is_zero)
LEVEL_PRED(is_param)
LEVEL_PRED(is_meta)
LEVEL_PRED(is_succ)
LEVEL_PRED(is_max)
LEVEL_PRED(is_imax)
LEVEL_PRED(is_explicit)
LEVEL_PRED(has_meta)
LEVEL_PRED(has_param)
LEVEL_PRED(is_not_zero)
static int level_get_kind(lua_State * L) { return push_integer(L, static_cast<int>(kind(to_level(L, 1)))); }
static int level_trivially_leq(lua_State * L) { return push_boolean(L, is_trivial(to_level(L, 1), to_level(L, 2))); }
static int level_is_eqp(lua_State * L) { return push_boolean(L, is_eqp(to_level(L, 1), to_level(L, 2))); }

static int level_id(lua_State * L) {
    level const & l = to_level(L, 1);
    if (is_param(l))     return push_name(L, param_id(l));
    else if (is_meta(l)) return push_name(L, meta_id(l));
    else throw exception("arg #1 must be a level parameter/metavariable");
}

static int level_lhs(lua_State * L) {
    level const & l = to_level(L, 1);
    if (is_max(l))       return push_level(L, max_lhs(l));
    else if (is_imax(l)) return push_level(L, imax_lhs(l));
    else throw exception("arg #1 must be a level max/imax expression");
}

static int level_rhs(lua_State * L) {
    level const & l = to_level(L, 1);
    if (is_max(l))       return push_level(L, max_rhs(l));
    else if (is_imax(l)) return push_level(L, imax_rhs(l));
    else throw exception("arg #1 must be a level max/imax expression");
}

static int level_succ_of(lua_State * L) {
    level const & l = to_level(L, 1);
    if (is_succ(l))      return push_level(L, succ_of(l));
    else throw exception("arg #1 must be a level succ expression");
}

static int level_instantiate(lua_State * L) {
    auto ps = to_list_name_ext(L, 2);
    auto ls = to_list_level_ext(L, 3);
    if (length(ps) != length(ls))
        throw exception("arg #2 and #3 size do not match");
    return push_level(L, instantiate(to_level(L, 1), ps, ls));
}

static const struct luaL_Reg level_m[] = {
    {"__gc",            level_gc}, // never throws
    {"__tostring",      safe_function<level_tostring>},
    {"__eq",            safe_function<level_eq>},
    {"__lt",            safe_function<level_lt>},
    {"succ",            safe_function<mk_level_succ>},
    {"kind",            safe_function<level_get_kind>},
    {"is_zero",         safe_function<level_is_zero>},
    {"is_param",        safe_function<level_is_param>},
    {"is_meta",         safe_function<level_is_meta>},
    {"is_succ",         safe_function<level_is_succ>},
    {"is_max",          safe_function<level_is_max>},
    {"is_imax",         safe_function<level_is_imax>},
    {"is_explicit",     safe_function<level_is_explicit>},
    {"has_meta",        safe_function<level_has_meta>},
    {"has_param",       safe_function<level_has_param>},
    {"is_not_zero",     safe_function<level_is_not_zero>},
    {"trivially_leq",   safe_function<level_trivially_leq>},
    {"is_eqp",          safe_function<level_is_eqp>},
    {"id",              safe_function<level_id>},
    {"lhs",             safe_function<level_lhs>},
    {"rhs",             safe_function<level_rhs>},
    {"succ_of",         safe_function<level_succ_of>},
    {"instantiate",     safe_function<level_instantiate>},
    {0, 0}
};

static void open_level(lua_State * L) {
    luaL_newmetatable(L, level_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, level_m, 0);

    SET_GLOBAL_FUN(mk_level_zero,  "level");
    SET_GLOBAL_FUN(mk_level_zero,  "mk_level_zero");
    SET_GLOBAL_FUN(mk_level_one,   "mk_level_one");
    SET_GLOBAL_FUN(mk_level_succ,  "mk_level_succ");
    SET_GLOBAL_FUN(mk_level_max,   "mk_level_max");
    SET_GLOBAL_FUN(mk_level_imax,  "mk_level_imax");
    SET_GLOBAL_FUN(mk_param_univ,  "mk_param_univ");
    SET_GLOBAL_FUN(mk_meta_univ,   "mk_meta_univ");

    SET_GLOBAL_FUN(level_pred, "is_level");

    lua_newtable(L);
    SET_ENUM("Zero",      level_kind::Zero);
    SET_ENUM("Succ",      level_kind::Succ);
    SET_ENUM("Max",       level_kind::Max);
    SET_ENUM("IMax",      level_kind::IMax);
    SET_ENUM("Param",     level_kind::Param);
    SET_ENUM("Meta",      level_kind::Meta);
    lua_setglobal(L, "level_kind");
}

// Expr_binder_info
DECL_UDATA(expr_binder_info)
static int mk_binder_info(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 0)
        return push_expr_binder_info(L, expr_binder_info());
    else if (nargs == 1)
        return push_expr_binder_info(L, expr_binder_info(lua_toboolean(L, 1)));
    else
        return push_expr_binder_info(L, expr_binder_info(lua_toboolean(L, 1), lua_toboolean(L, 2)));
}
static int binder_info_is_implicit(lua_State * L) { return push_boolean(L, to_expr_binder_info(L, 1).is_implicit()); }
static int binder_info_is_cast(lua_State * L) { return push_boolean(L, to_expr_binder_info(L, 1).is_cast()); }
static const struct luaL_Reg binder_info_m[] = {
    {"__gc",            expr_binder_info_gc},
    {"is_implicit",     safe_function<binder_info_is_implicit>},
    {"is_cast",         safe_function<binder_info_is_cast>},
    {0, 0}
};
static void open_binder_info(lua_State * L) {
    luaL_newmetatable(L, expr_binder_info_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, binder_info_m, 0);

    SET_GLOBAL_FUN(mk_binder_info, "binder_info");
    SET_GLOBAL_FUN(expr_binder_info_pred, "is_binder_info");
}

// Expressions
DECL_UDATA(expr)
DEFINE_LUA_LIST(expr, push_expr, to_expr)

int push_optional_expr(lua_State * L, optional<expr> const & e) {  return e ? push_expr(L, *e) : push_nil(L); }

expr & to_app(lua_State * L, int idx) {
    expr & r = to_expr(L, idx);
    if (!is_app(r))
        throw exception(sstream() << "arg #" << idx << " must be an application");
    return r;
}

expr & to_binder(lua_State * L, int idx) {
    expr & r = to_expr(L, idx);
    if (!is_binder(r))
        throw exception(sstream() << "arg #" << idx << " must be a binder (i.e., lambda or Pi)");
    return r;
}

expr & to_macro_app(lua_State * L, int idx) {
    expr & r = to_expr(L, idx);
    if (!is_macro(r))
        throw exception(sstream() << "arg #" << idx << " must be a macro application");
    return r;
}

static int expr_tostring(lua_State * L) {
    std::ostringstream out;
    formatter fmt   = get_global_formatter(L);
    options opts    = get_global_options(L);
    environment env = get_global_environment(L);
    out << mk_pair(fmt(env, to_expr(L, 1), opts), opts);
    return push_string(L, out.str().c_str());
}

static int expr_eq(lua_State * L) { return push_boolean(L, to_expr(L, 1) == to_expr(L, 2)); }
static int expr_lt(lua_State * L) { return push_boolean(L, to_expr(L, 1) < to_expr(L, 2)); }
static int expr_mk_constant(lua_State * L) { return push_expr(L, mk_constant(to_name_ext(L, 1))); }
static int expr_mk_var(lua_State * L) { return push_expr(L, mk_var(luaL_checkinteger(L, 1))); }
static int expr_mk_app(lua_State * L) {
    int nargs = lua_gettop(L);
    expr r;
    r = mk_app(to_expr(L, 1), to_expr(L, 2));
    for (int i = 3; i < nargs; i++)
        r = mk_app(r, to_expr(L, i));
    return push_expr(L, r);
}
static int expr_mk_lambda(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 3)
        return push_expr(L, mk_lambda(to_name_ext(L, 1), to_expr(L, 2), to_expr(L, 3)));
    else
        return push_expr(L, mk_lambda(to_name_ext(L, 1), to_expr(L, 2), to_expr(L, 3), to_expr_binder_info(L, 4)));
}
static int expr_mk_pi(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 3)
        return push_expr(L, mk_pi(to_name_ext(L, 1), to_expr(L, 2), to_expr(L, 3)));
    else
        return push_expr(L, mk_pi(to_name_ext(L, 1), to_expr(L, 2), to_expr(L, 3), to_expr_binder_info(L, 4)));
}
static int expr_mk_arrow(lua_State * L) { return push_expr(L, mk_arrow(to_expr(L, 1), to_expr(L, 2))); }
static int expr_mk_let(lua_State * L) { return push_expr(L, mk_let(to_name_ext(L, 1), to_expr(L, 2), to_expr(L, 3), to_expr(L, 4))); }

static expr get_expr_from_table(lua_State * L, int t, int i) {
    lua_pushvalue(L, t); // push table to the top
    lua_pushinteger(L, i);
    lua_gettable(L, -2);
    expr r = to_expr(L, -1);
    lua_pop(L, 2); // remove table and value
    return r;
}

// t is a table of pairs {{a1, b1}, ..., {ak, bk}}
// Each ai and bi is an expression
static std::pair<expr, expr> get_expr_pair_from_table(lua_State * L, int t, int i) {
    lua_pushvalue(L, t); // push table on the top
    lua_pushinteger(L, i);
    lua_gettable(L, -2); // now table {ai, bi} is on the top
    if (!lua_istable(L, -1) || objlen(L, -1) != 2)
        throw exception("arg #1 must be of the form '{{expr, expr}, ...}'");
    expr ai = get_expr_from_table(L, -1, 1);
    expr bi = get_expr_from_table(L, -1, 2);
    lua_pop(L, 2); // pop table {ai, bi} and t from stack
    return mk_pair(ai, bi);
}

typedef expr (*MkAbst1)(expr const & n, expr const & t, expr const & b);
typedef expr (*MkAbst2)(name const & n, expr const & t, expr const & b);

template<MkAbst1 F1, MkAbst2 F2>
int expr_abst(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs < 2)
        throw exception("function must have at least 2 arguments");
    if (nargs == 2) {
        if (!lua_istable(L, 1))
            throw exception("function expects arg #1 to be of the form '{{expr, expr}, ...}'");
        int len = objlen(L, 1);
        if (len == 0)
            throw exception("function expects arg #1 to be a non-empty table");
        expr r = to_expr(L, 2);
        for (int i = len; i >= 1; i--) {
            auto p = get_expr_pair_from_table(L, 1, i);
            r = F1(p.first, p.second, r);
        }
        return push_expr(L, r);
    } else {
        if (nargs % 2 == 0)
            throw exception("function must have an odd number of arguments");
        expr r = to_expr(L, nargs);
        for (int i = nargs - 1; i >= 1; i-=2) {
            if (is_expr(L, i - 1))
                r = F1(to_expr(L, i - 1), to_expr(L, i), r);
            else
                r = F2(to_name_ext(L, i - 1), to_expr(L, i), r);
        }
        return push_expr(L, r);
    }
}

static int expr_fun(lua_State * L) { return expr_abst<Fun, Fun>(L); }
static int expr_pi(lua_State * L)  { return expr_abst<Pi, Pi>(L); }
static int expr_mk_sort(lua_State * L) { return push_expr(L, mk_sort(to_level(L, 1))); }
static int expr_mk_metavar(lua_State * L) { return push_expr(L, mk_metavar(to_name_ext(L, 1), to_expr(L, 2))); }
static int expr_mk_local(lua_State * L) { return push_expr(L, mk_local(to_name_ext(L, 1), to_expr(L, 2))); }
static int expr_get_kind(lua_State * L) { return push_integer(L, static_cast<int>(to_expr(L, 1).kind())); }

#define EXPR_PRED(P) static int expr_ ## P(lua_State * L) {  return push_boolean(L, P(to_expr(L, 1))); }

EXPR_PRED(is_constant)
EXPR_PRED(is_var)
EXPR_PRED(is_app)
EXPR_PRED(is_lambda)
EXPR_PRED(is_pi)
EXPR_PRED(is_binder)
EXPR_PRED(is_let)
EXPR_PRED(is_macro)
EXPR_PRED(is_metavar)
EXPR_PRED(is_local)
EXPR_PRED(is_mlocal)
EXPR_PRED(is_meta)
EXPR_PRED(has_metavar)
EXPR_PRED(has_local)
EXPR_PRED(has_param_univ)
EXPR_PRED(has_free_vars)
EXPR_PRED(closed)

static int expr_fields(lua_State * L) {
    expr & e = to_expr(L, 1);
    switch (e.kind()) {
    case expr_kind::Var:
        return push_integer(L, var_idx(e));
    case expr_kind::Constant:
        push_name(L, const_name(e)); push_list_level(L, const_level_params(e)); return 2;
    case expr_kind::Sort:
        return push_level(L, sort_level(e));
    case expr_kind::Macro:
        return push_macro_definition(L, macro_def(e));
    case expr_kind::App:
        push_expr(L, app_fn(e)); push_expr(L, app_arg(e)); return 2;
    case expr_kind::Lambda:
    case expr_kind::Pi:
        push_name(L, binder_name(e)); push_expr(L, binder_domain(e)); push_expr(L, binder_body(e)); push_expr_binder_info(L, binder_info(e)); return 4;
    case expr_kind::Let:
        push_name(L, let_name(e));  push_expr(L, let_type(e)); push_expr(L, let_value(e)); push_expr(L, let_body(e)); return 4;
    case expr_kind::Meta:
    case expr_kind::Local:
        push_name(L, mlocal_name(e)); push_expr(L, mlocal_type(e)); return 2;
    }
    lean_unreachable(); // LCOV_EXCL_LINE
    return 0;           // LCOV_EXCL_LINE
}

static int expr_fn(lua_State * L) { return push_expr(L, app_fn(to_app(L, 1))); }
static int expr_arg(lua_State * L) { return push_expr(L, app_arg(to_app(L, 1))); }

static int expr_for_each(lua_State * L) {
    expr & e = to_expr(L, 1);    // expr
    luaL_checktype(L, 2, LUA_TFUNCTION); // user-fun
    for_each(e, [&](expr const & a, unsigned offset) {
            lua_pushvalue(L, 2); // push user-fun
            push_expr(L, a);
            lua_pushinteger(L, offset);
            pcall(L, 2, 1, 0);
            bool r = true;
            if (lua_isboolean(L, -1))
                r = lua_toboolean(L, -1);
            lua_pop(L, 1);
            return r;
        });
    return 0;
}

static int expr_has_free_var(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 2)
        return push_boolean(L, has_free_var(to_expr(L, 1), luaL_checkinteger(L, 2)));
    else
        return push_boolean(L, has_free_var(to_expr(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3)));
}

static int expr_lift_free_vars(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 2)
        return push_expr(L, lift_free_vars(to_expr(L, 1), luaL_checkinteger(L, 2)));
    else
        return push_expr(L, lift_free_vars(to_expr(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3)));
}

static int expr_lower_free_vars(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 2)
        return push_expr(L, lower_free_vars(to_expr(L, 1), luaL_checkinteger(L, 2)));
    else
        return push_expr(L, lower_free_vars(to_expr(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3)));
}

// Copy Lua table/array elements to r
static void copy_lua_array(lua_State * L, int tidx, buffer<expr> & r) {
    luaL_checktype(L, tidx, LUA_TTABLE);
    int n = objlen(L, tidx);
    for (int i = 1; i <= n; i++) {
        lua_rawgeti(L, tidx, i);
        r.push_back(to_expr(L, -1));
        lua_pop(L, 1);
    }
}

static int expr_instantiate(lua_State * L) {
    expr const & e = to_expr(L, 1);
    if (is_expr(L, 2)) {
        return push_expr(L, instantiate(e, to_expr(L, 2)));
    } else {
        buffer<expr> s;
        copy_lua_array(L, 2, s);
        return push_expr(L, instantiate(e, s.size(), s.data()));
    }
}

static int expr_abstract(lua_State * L) {
    expr const & e = to_expr(L, 1);
    if (is_expr(L, 2)) {
        expr const & e2 = to_expr(L, 2);
        return push_expr(L, abstract(e, 1, &e2));
    } else {
        buffer<expr> s;
        copy_lua_array(L, 2, s);
        return push_expr(L, abstract(e, s.size(), s.data()));
    }
}

static int binder_name(lua_State * L) { return push_name(L, binder_name(to_binder(L, 1))); }
static int binder_domain(lua_State * L) { return push_expr(L, binder_domain(to_binder(L, 1))); }
static int binder_body(lua_State * L) { return push_expr(L, binder_body(to_binder(L, 1))); }
static int binder_info(lua_State * L) { return push_expr_binder_info(L, binder_info(to_binder(L, 1))); }

static int expr_occurs(lua_State * L) { return push_boolean(L, occurs(to_expr(L, 1), to_expr(L, 2))); }
static int expr_is_eqp(lua_State * L) { return push_boolean(L, is_eqp(to_expr(L, 1), to_expr(L, 2))); }
static int expr_hash(lua_State * L) { return push_integer(L, to_expr(L, 1).hash()); }
static int expr_depth(lua_State * L) { return push_integer(L, get_depth(to_expr(L, 1))); }
static int expr_is_lt(lua_State * L) { return push_boolean(L, is_lt(to_expr(L, 1), to_expr(L, 2), false)); }

static int expr_mk_macro(lua_State * L) {
    buffer<expr> args;
    copy_lua_array(L, 2, args);
    return push_expr(L, mk_macro(to_macro_definition(L, 1), args.size(), args.data()));
}

static int macro_def(lua_State * L) { return push_macro_definition(L, macro_def(to_macro_app(L, 1))); }
static int macro_num_args(lua_State * L) { return push_integer(L, macro_num_args(to_macro_app(L, 1))); }
static int macro_arg(lua_State * L) { return push_expr(L, macro_arg(to_macro_app(L, 1), luaL_checkinteger(L, 2))); }

static const struct luaL_Reg expr_m[] = {
    {"__gc",             expr_gc}, // never throws
    {"__tostring",       safe_function<expr_tostring>},
    {"__eq",             safe_function<expr_eq>},
    {"__lt",             safe_function<expr_lt>},
    {"__call",           safe_function<expr_mk_app>},
    {"kind",             safe_function<expr_get_kind>},
    {"is_var",           safe_function<expr_is_var>},
    {"is_constant",      safe_function<expr_is_constant>},
    {"is_metavar",       safe_function<expr_is_metavar>},
    {"is_local",         safe_function<expr_is_local>},
    {"is_mlocal",        safe_function<expr_is_mlocal>},
    {"is_app",           safe_function<expr_is_app>},
    {"is_lambda",        safe_function<expr_is_lambda>},
    {"is_pi",            safe_function<expr_is_pi>},
    {"is_binder",        safe_function<expr_is_binder>},
    {"is_let",           safe_function<expr_is_let>},
    {"is_macro",         safe_function<expr_is_macro>},
    {"is_meta",          safe_function<expr_is_meta>},
    {"has_free_vars",    safe_function<expr_has_free_vars>},
    {"closed",           safe_function<expr_closed>},
    {"has_metavar",      safe_function<expr_has_metavar>},
    {"has_local",        safe_function<expr_has_local>},
    {"has_param_univ",   safe_function<expr_has_param_univ>},
    {"arg",              safe_function<expr_arg>},
    {"fn",               safe_function<expr_fn>},
    {"fields",           safe_function<expr_fields>},
    {"data",             safe_function<expr_fields>},
    {"depth",            safe_function<expr_depth>},
    {"binder_name",      safe_function<binder_name>},
    {"binder_domain",    safe_function<binder_domain>},
    {"binder_body",      safe_function<binder_body>},
    {"binder_info",      safe_function<binder_info>},
    {"macro_def",        safe_function<macro_def>},
    {"macro_num_args",   safe_function<macro_num_args>},
    {"macro_arg",        safe_function<macro_arg>},
    {"for_each",         safe_function<expr_for_each>},
    {"has_free_var",     safe_function<expr_has_free_var>},
    {"lift_free_vars",   safe_function<expr_lift_free_vars>},
    {"lower_free_vars",  safe_function<expr_lower_free_vars>},
    {"instantiate",      safe_function<expr_instantiate>},
    {"abstract",         safe_function<expr_abstract>},
    {"occurs",           safe_function<expr_occurs>},
    {"is_eqp",           safe_function<expr_is_eqp>},
    {"is_lt",            safe_function<expr_is_lt>},
    {"hash",             safe_function<expr_hash>},
    {0, 0}
};

static void expr_migrate(lua_State * src, int i, lua_State * tgt) {
    push_expr(tgt, to_expr(src, i));
}

static void open_expr(lua_State * L) {
    luaL_newmetatable(L, expr_mt);
    set_migrate_fn_field(L, -1, expr_migrate);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, expr_m, 0);

    SET_GLOBAL_FUN(expr_mk_constant, "mk_constant");
    SET_GLOBAL_FUN(expr_mk_constant, "Const");
    SET_GLOBAL_FUN(expr_mk_var,      "mk_var");
    SET_GLOBAL_FUN(expr_mk_var,      "Var");
    SET_GLOBAL_FUN(expr_mk_app,      "mk_app");
    SET_GLOBAL_FUN(expr_mk_lambda,   "mk_lambda");
    SET_GLOBAL_FUN(expr_mk_pi,       "mk_pi");
    SET_GLOBAL_FUN(expr_mk_arrow,    "mk_arrow");
    SET_GLOBAL_FUN(expr_mk_let,      "mk_let");
    SET_GLOBAL_FUN(expr_mk_macro,    "mk_macro");
    SET_GLOBAL_FUN(expr_fun,         "fun");
    SET_GLOBAL_FUN(expr_fun,         "Fun");
    SET_GLOBAL_FUN(expr_pi,          "Pi");
    SET_GLOBAL_FUN(expr_mk_let,      "Let");
    SET_GLOBAL_FUN(expr_mk_sort,     "mk_sort");
    SET_GLOBAL_FUN(expr_mk_metavar,  "mk_metavar");
    SET_GLOBAL_FUN(expr_mk_local,    "mk_local");
    SET_GLOBAL_FUN(expr_pred,        "is_expr");

    push_expr(L, Bool);
    lua_setglobal(L, "Bool");

    push_expr(L, Type);
    lua_setglobal(L, "Type");

    lua_newtable(L);
    SET_ENUM("Var",      expr_kind::Var);
    SET_ENUM("Constant", expr_kind::Constant);
    SET_ENUM("Meta",     expr_kind::Meta);
    SET_ENUM("Local",    expr_kind::Local);
    SET_ENUM("Sort",     expr_kind::Sort);
    SET_ENUM("App",      expr_kind::App);
    SET_ENUM("Lambda",   expr_kind::Lambda);
    SET_ENUM("Pi",       expr_kind::Pi);
    SET_ENUM("Let",      expr_kind::Let);
    SET_ENUM("Macro",    expr_kind::Macro);
    lua_setglobal(L, "expr_kind");
}
// macro_definition
DECL_UDATA(macro_definition)

int macro_get_name(lua_State * L) { return push_name(L, to_macro_definition(L, 1).get_name()); }
int macro_trust_level(lua_State * L) { return push_integer(L, to_macro_definition(L, 1).trust_level()); }
int macro_eq(lua_State * L) { return push_boolean(L, to_macro_definition(L, 1) == to_macro_definition(L, 2)); }
int macro_hash(lua_State * L) { return push_integer(L, to_macro_definition(L, 1).hash()); }
static int macro_tostring(lua_State * L) {
    std::ostringstream out;
    formatter fmt   = get_global_formatter(L);
    options opts    = get_global_options(L);
    out << mk_pair(to_macro_definition(L, 1).pp(fmt, opts), opts);
    return push_string(L, out.str().c_str());
}

static const struct luaL_Reg macro_definition_m[] = {
    {"__gc",             macro_definition_gc}, // never throws
    {"__tostring",       safe_function<macro_tostring>},
    {"__eq",             safe_function<macro_eq>},
    {"hash",             safe_function<macro_hash>},
    {"trust_level",      safe_function<macro_trust_level>},
    {"name",             safe_function<macro_get_name>},
    {0, 0}
};

void open_macro_definition(lua_State * L) {
    luaL_newmetatable(L, macro_definition_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, macro_definition_m, 0);

    SET_GLOBAL_FUN(macro_definition_pred, "is_macro_definition");
}

// Formatter
DECL_UDATA(formatter)

static int formatter_call(lua_State * L) {
    int nargs = lua_gettop(L);
    formatter & fmt = to_formatter(L, 1);
    if (nargs == 2) {
        return push_format(L, fmt(get_global_environment(L), to_expr(L, 2), get_global_options(L)));
    } else if (nargs == 3) {
        if (is_expr(L, 2))
            return push_format(L, fmt(get_global_environment(L), to_expr(L, 2), to_options(L, 3)));
        else
            return push_format(L, fmt(to_environment(L, 2), to_expr(L, 3), get_global_options(L)));
    } else {
        return push_format(L, fmt(to_environment(L, 2), to_expr(L, 3), to_options(L, 4)));
    }
}

static const struct luaL_Reg formatter_m[] = {
    {"__gc",            formatter_gc}, // never throws
    {"__call",          safe_function<formatter_call>},
    {0, 0}
};

static char g_formatter_key;
static formatter g_simple_formatter = mk_simple_formatter();

optional<formatter> get_global_formatter_core(lua_State * L) {
    io_state * io = get_io_state(L);
    if (io != nullptr) {
        return optional<formatter>(io->get_formatter());
    } else {
        lua_pushlightuserdata(L, static_cast<void *>(&g_formatter_key));
        lua_gettable(L, LUA_REGISTRYINDEX);
        if (is_formatter(L, -1)) {
            formatter r = to_formatter(L, -1);
            lua_pop(L, 1);
            return optional<formatter>(r);
        } else {
            lua_pop(L, 1);
            return optional<formatter>();
        }
    }
}

formatter get_global_formatter(lua_State * L) {
    auto r = get_global_formatter_core(L);
    if (r)
        return *r;
    else
        return g_simple_formatter;
}

void set_global_formatter(lua_State * L, formatter const & fmt) {
    io_state * io = get_io_state(L);
    if (io != nullptr) {
        io->set_formatter(fmt);
    } else {
        lua_pushlightuserdata(L, static_cast<void *>(&g_formatter_key));
        push_formatter(L, fmt);
        lua_settable(L, LUA_REGISTRYINDEX);
    }
}

static int get_formatter(lua_State * L) {
    io_state * io = get_io_state(L);
    if (io != nullptr) {
        return push_formatter(L, io->get_formatter());
    } else {
        return push_formatter(L, get_global_formatter(L));
    }
}

static int set_formatter(lua_State * L) {
    set_global_formatter(L, to_formatter(L, 1));
    return 0;
}

static void open_formatter(lua_State * L) {
    luaL_newmetatable(L, formatter_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, formatter_m, 0);

    SET_GLOBAL_FUN(formatter_pred, "is_formatter");
    SET_GLOBAL_FUN(get_formatter,  "get_formatter");
    SET_GLOBAL_FUN(set_formatter,  "set_formatter");
}

// Environment
DECL_UDATA(environment)

static int mk_empty_environment(lua_State * L) {
    return push_environment(L, environment());
}

static const struct luaL_Reg environment_m[] = {
    {"__gc",           environment_gc}, // never throws
    {0, 0}
};

static char g_set_environment_key;

void set_global_environment(lua_State * L, environment const & env) {
    lua_pushlightuserdata(L, static_cast<void *>(&g_set_environment_key));
    push_environment(L, env);
    lua_settable(L, LUA_REGISTRYINDEX);
}

set_environment::set_environment(lua_State * L, environment const & env) {
    m_state = L;
    set_global_environment(L, env);
}

set_environment::~set_environment() {
    lua_pushlightuserdata(m_state, static_cast<void *>(&g_set_environment_key));
    lua_pushnil(m_state);
    lua_settable(m_state, LUA_REGISTRYINDEX);
}

static environment get_global_environment(lua_State * L) {
    lua_pushlightuserdata(L, static_cast<void *>(&g_set_environment_key));
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (!is_environment(L, -1))
        return environment(); // return empty environment
    environment r = to_environment(L, -1);
    lua_pop(L, 1);
    return r;
}

int get_environment(lua_State * L) {
    return push_environment(L, get_global_environment(L));
}

static void environment_migrate(lua_State * src, int i, lua_State * tgt) {
    push_environment(tgt, to_environment(src, i));
}

static void open_environment(lua_State * L) {
    luaL_newmetatable(L, environment_mt);
    set_migrate_fn_field(L, -1, environment_migrate);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, environment_m, 0);

    SET_GLOBAL_FUN(mk_empty_environment,   "empty_environment");
    SET_GLOBAL_FUN(environment_pred,       "is_environment");
    SET_GLOBAL_FUN(get_environment,        "get_environment");
    SET_GLOBAL_FUN(get_environment,        "get_env");
}

// IO state
DECL_UDATA(io_state)

int mk_io_state(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 0)
        return push_io_state(L, io_state(mk_simple_formatter()));
    else if (nargs == 1)
        return push_io_state(L, io_state(to_io_state(L, 1)));
    else
        return push_io_state(L, io_state(to_options(L, 1), to_formatter(L, 2)));
}

int io_state_get_options(lua_State * L) { return push_options(L, to_io_state(L, 1).get_options()); }
int io_state_get_formatter(lua_State * L) { return push_formatter(L, to_io_state(L, 1).get_formatter()); }
int io_state_set_options(lua_State * L) { to_io_state(L, 1).set_options(to_options(L, 2)); return 0; }

static mutex g_print_mutex;

static void print(io_state * ios, bool reg, char const * msg) {
    if (ios) {
        if (reg)
            ios->get_regular_channel() << msg;
        else
            ios->get_diagnostic_channel() << msg;
    } else {
        std::cout << msg;
    }
}

/** \brief Thread safe version of print function */
static int print(lua_State * L, int start, bool reg) {
    lock_guard<mutex> lock(g_print_mutex);
    io_state * ios = get_io_state(L);
    int n = lua_gettop(L);
    int i;
    lua_getglobal(L, "tostring");
    for (i = start; i <= n; i++) {
        char const * s;
        size_t l;
        lua_pushvalue(L, -1);
        lua_pushvalue(L, i);
        lua_call(L, 1, 1);
        s = lua_tolstring(L, -1, &l);
        if (s == NULL)
            throw exception("'to_string' must return a string to 'print'");
        if (i > start) {
            print(ios, reg, "\t");
        }
        print(ios, reg, s);
        lua_pop(L, 1);
    }
    print(ios, reg, "\n");
    return 0;
}

static int print(lua_State * L, io_state & ios, int start, bool reg) {
    set_io_state set(L, ios);
    return print(L, start, reg);
}

static int print(lua_State * L) {
    return print(L, 1, true);
}

int io_state_print_regular(lua_State * L) {
    return print(L, to_io_state(L, 1), 2, true);
}

int io_state_print_diagnostic(lua_State * L) {
    return print(L, to_io_state(L, 1), 2, false);
}

static const struct luaL_Reg io_state_m[] = {
    {"__gc",             io_state_gc}, // never throws
    {"get_options",      safe_function<io_state_get_options>},
    {"set_options",      safe_function<io_state_set_options>},
    {"get_formatter",    safe_function<io_state_get_formatter>},
    {"print_diagnostic", safe_function<io_state_print_diagnostic>},
    {"print_regular",    safe_function<io_state_print_regular>},
    {"print",            safe_function<io_state_print_regular>},
    {"diagnostic",       safe_function<io_state_print_diagnostic>},
    {0, 0}
};

void open_io_state(lua_State * L) {
    luaL_newmetatable(L, io_state_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, io_state_m, 0);

    SET_GLOBAL_FUN(io_state_pred, "is_io_state");
    SET_GLOBAL_FUN(mk_io_state, "io_state");
    SET_GLOBAL_FUN(print, "print");
}

static char g_set_state_key;

void set_global_io_state(lua_State * L, io_state & ios) {
    lua_pushlightuserdata(L, static_cast<void *>(&g_set_state_key));
    lua_pushlightuserdata(L, &ios);
    lua_settable(L, LUA_REGISTRYINDEX);
    set_global_options(L, ios.get_options());
}

set_io_state::set_io_state(lua_State * L, io_state & st) {
    m_state = L;
    m_prev  = get_io_state(L);
    lua_pushlightuserdata(m_state, static_cast<void *>(&g_set_state_key));
    lua_pushlightuserdata(m_state, &st);
    lua_settable(m_state, LUA_REGISTRYINDEX);
    if (!m_prev)
        m_prev_options = get_global_options(m_state);
    set_global_options(m_state, st.get_options());
}

set_io_state::~set_io_state() {
    lua_pushlightuserdata(m_state, static_cast<void *>(&g_set_state_key));
    lua_pushlightuserdata(m_state, m_prev);
    lua_settable(m_state, LUA_REGISTRYINDEX);
    if (!m_prev)
        set_global_options(m_state, m_prev_options);
    else
        set_global_options(m_state, m_prev->get_options());
}

io_state * get_io_state(lua_State * L) {
    lua_pushlightuserdata(L, static_cast<void *>(&g_set_state_key));
    lua_gettable(L, LUA_REGISTRYINDEX);
    if (lua_islightuserdata(L, -1)) {
        io_state * r = static_cast<io_state*>(lua_touserdata(L, -1));
        if (r) {
            lua_pop(L, 1);
            options o = get_global_options(L);
            r->set_options(o);
            return r;
        }
    }
    lua_pop(L, 1);
    return nullptr;
}

// Justification
DECL_UDATA(justification)

int push_optional_justification(lua_State * L, optional<justification> const & j) { return j ? push_justification(L, *j) : push_nil(L); }

static int justification_tostring(lua_State * L) {
    std::ostringstream out;
    justification & jst = to_justification(L, 1);
    out << jst;
    lua_pushstring(L, out.str().c_str());
    return 1;
}

#define JST_PRED(P) static int justification_ ## P(lua_State * L) { return push_boolean(L, to_justification(L, 1).P()); }
JST_PRED(is_none)
JST_PRED(is_asserted)
JST_PRED(is_assumption)
JST_PRED(is_composite)
static int justification_get_main_expr(lua_State * L) { return push_optional_expr(L, to_justification(L, 1).get_main_expr()); }
static int justification_pp(lua_State * L) {
    int nargs = lua_gettop(L);
    justification & j = to_justification(L, 1);
    if (nargs == 1)
        return push_format(L, j.pp(get_global_formatter(L), get_global_options(L), nullptr, substitution()));
    else if (nargs == 2 && is_substitution(L, 2))
        return push_format(L, j.pp(get_global_formatter(L), get_global_options(L), nullptr, to_substitution(L, 2)));
    else if (nargs == 2)
        return push_format(L, j.pp(to_formatter(L, 2), get_global_options(L), nullptr, substitution()));
    else if (nargs == 3 && is_substitution(L, 3))
        return push_format(L, j.pp(to_formatter(L, 2), get_global_options(L), nullptr, to_substitution(L, 3)));
    else if (nargs == 3)
        return push_format(L, j.pp(to_formatter(L, 2), to_options(L, 3), nullptr, substitution()));
    else
        return push_format(L, j.pp(to_formatter(L, 2), to_options(L, 3), nullptr, to_substitution(L, 4)));
}
static int justification_assumption_idx(lua_State * L) {
    if (!to_justification(L, 1).is_assumption())
        throw exception("arg #1 must be an assumption justification");
    return push_integer(L, assumption_idx(to_justification(L, 1)));
}
static int justification_child1(lua_State * L) {
    if (!to_justification(L, 1).is_composite())
        throw exception("arg #1 must be a composite justification");
    return push_justification(L, composite_child1(to_justification(L, 1)));
}
static int justification_child2(lua_State * L) {
    if (!to_justification(L, 1).is_composite())
        throw exception("arg #1 must be a composite justification");
    return push_justification(L, composite_child2(to_justification(L, 1)));
}
static int justification_depends_on(lua_State * L) { return push_boolean(L, depends_on(to_justification(L, 1), luaL_checkinteger(L, 2))); }
static int mk_assumption_justification(lua_State * L) { return push_justification(L, mk_assumption_justification(luaL_checkinteger(L, 1))); }
static int mk_composite1(lua_State * L) { return push_justification(L, mk_composite1(to_justification(L, 1), to_justification(L, 2))); }
static int mk_justification(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 0) {
        return push_justification(L, justification());
    } else if (nargs == 1) {
        std::string s = lua_tostring(L, 1);
        return push_justification(L, mk_justification(none_expr(), [=](formatter const &, options const &, substitution const &) {
                    return format(s.c_str());
                }));
    } else {
        std::string s   = lua_tostring(L, 1);
        environment env = to_environment(L, 2);
        expr e          = to_expr(L, 3);
        justification j = mk_justification(some_expr(e), [=](formatter const & fmt, options const & opts, substitution const & subst) {
                expr new_e = subst.instantiate_metavars_wo_jst(e);
                format r;
                r += format(s.c_str());
                r += pp_indent_expr(fmt, env, opts, new_e);
                return r;
            });
        return push_justification(L, j);
    }
}

static const struct luaL_Reg justification_m[] = {
    {"__gc",            justification_gc}, // never throws
    {"__tostring",      safe_function<justification_tostring>},
    {"is_none",         safe_function<justification_is_none>},
    {"is_asserted",     safe_function<justification_is_asserted>},
    {"is_assumption",   safe_function<justification_is_assumption>},
    {"is_composite",    safe_function<justification_is_composite>},
    {"main_expr",       safe_function<justification_get_main_expr>},
    {"pp",              safe_function<justification_pp>},
    {"depends_on",      safe_function<justification_depends_on>},
    {"assumption_idx",  safe_function<justification_assumption_idx>},
    {"child1",          safe_function<justification_child1>},
    {"child2",          safe_function<justification_child2>},
    {0, 0}
};

static void open_justification(lua_State * L) {
    luaL_newmetatable(L, justification_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, justification_m, 0);

    SET_GLOBAL_FUN(mk_justification, "justification");
    SET_GLOBAL_FUN(mk_assumption_justification, "assumption_justification");
    SET_GLOBAL_FUN(mk_composite1, "mk_composite_justification");
    SET_GLOBAL_FUN(justification_pred, "is_justification");
}

// Substitution
DECL_UDATA(substitution)
static int mk_substitution(lua_State * L) { return push_substitution(L, substitution()); }
static int subst_get_expr(lua_State * L) {
    if (is_expr(L, 2))
        return push_optional_expr(L, to_substitution(L, 1).get_expr(to_expr(L, 2)));
    else
        return push_optional_expr(L, to_substitution(L, 1).get_expr(to_name_ext(L, 2)));
}
static int subst_get_level(lua_State * L) {
    if (is_level(L, 2))
        return push_optional_level(L, to_substitution(L, 1).get_level(to_level(L, 2)));
    else
        return push_optional_level(L, to_substitution(L, 1).get_level(to_name_ext(L, 2)));
}
static int subst_assign(lua_State * L) {
    int nargs = lua_gettop(L);
    if (nargs == 3) {
        if (is_expr(L, 3)) {
            if (is_expr(L, 2))
                return push_substitution(L, to_substitution(L, 1).assign(to_expr(L, 2), to_expr(L, 3)));
            else
                return push_substitution(L, to_substitution(L, 1).assign(to_name_ext(L, 2), to_expr(L, 3)));
        } else {
            if (is_level(L, 2))
                return push_substitution(L, to_substitution(L, 1).assign(to_level(L, 2), to_level(L, 3)));
            else
                return push_substitution(L, to_substitution(L, 1).assign(to_name_ext(L, 2), to_level(L, 3)));
        }
    } else {
        if (is_expr(L, 3)) {
            if (is_expr(L, 2))
                return push_substitution(L, to_substitution(L, 1).assign(to_expr(L, 2), to_expr(L, 3), to_justification(L, 4)));
            else
                return push_substitution(L, to_substitution(L, 1).assign(to_name_ext(L, 2), to_expr(L, 3), to_justification(L, 4)));
        } else {
            if (is_level(L, 2))
                return push_substitution(L, to_substitution(L, 1).assign(to_level(L, 2), to_level(L, 3), to_justification(L, 4)));
            else
                return push_substitution(L, to_substitution(L, 1).assign(to_name_ext(L, 2), to_level(L, 3), to_justification(L, 4)));
        }
    }
}
static int subst_is_assigned(lua_State * L) {
    if (is_expr(L, 2))
        return push_boolean(L, to_substitution(L, 1).is_assigned(to_expr(L, 2)));
    else
        return push_boolean(L, to_substitution(L, 1).is_assigned(to_level(L, 2)));
}
static int subst_is_expr_assigned(lua_State * L) { return push_boolean(L, to_substitution(L, 1).is_expr_assigned(to_name_ext(L, 2))); }
static int subst_is_level_assigned(lua_State * L) { return push_boolean(L, to_substitution(L, 1).is_level_assigned(to_name_ext(L, 2))); }
static int subst_occurs(lua_State * L) { return push_boolean(L, to_substitution(L, 1).occurs(to_expr(L, 2), to_expr(L, 3))); }
static int subst_occurs_expr(lua_State * L) { return push_boolean(L, to_substitution(L, 1).occurs_expr(to_name_ext(L, 2), to_expr(L, 3))); }
static int subst_get_expr_assignment(lua_State * L) {
    auto r = to_substitution(L, 1).get_expr_assignment(to_name_ext(L, 2));
    if (r) {
        push_expr(L, r->first);
        push_justification(L, r->second);
    } else {
        push_nil(L); push_nil(L);
    }
    return 2;
}
static int subst_get_level_assignment(lua_State * L) {
    auto r = to_substitution(L, 1).get_level_assignment(to_name_ext(L, 2));
    if (r) {
        push_level(L, r->first);
        push_justification(L, r->second);
    } else {
        push_nil(L); push_nil(L);
    }
    return 2;
}
static int subst_get_assignment(lua_State * L) {
    if (is_expr(L, 2)) {
        auto r = to_substitution(L, 1).get_assignment(to_expr(L, 2));
        if (r) {
            push_expr(L, r->first);
            push_justification(L, r->second);
        } else {
            push_nil(L); push_nil(L);
        }
    } else {
        auto r = to_substitution(L, 1).get_assignment(to_level(L, 2));
        if (r) {
            push_level(L, r->first);
            push_justification(L, r->second);
        } else {
            push_nil(L); push_nil(L);
        }
    }
    return 2;
}
static int subst_instantiate(lua_State * L) {
    if (is_expr(L, 2)) {
        auto r = to_substitution(L, 1).instantiate_metavars(to_expr(L, 2));
        push_expr(L, r.first); push_justification(L, r.second);
    } else {
        auto r = to_substitution(L, 1).instantiate_metavars(to_level(L, 2));
        push_level(L, r.first); push_justification(L, r.second);
    }
    return 2;
}

static const struct luaL_Reg substitution_m[] = {
    {"__gc",                   substitution_gc},
    {"get_expr",               safe_function<subst_get_expr>},
    {"get_level",              safe_function<subst_get_level>},
    {"assign",                 safe_function<subst_assign>},
    {"is_assigned",            safe_function<subst_is_assigned>},
    {"is_expr_assigned",       safe_function<subst_is_expr_assigned>},
    {"is_level_assigned",      safe_function<subst_is_level_assigned>},
    {"occurs",                 safe_function<subst_occurs>},
    {"occurs_expr",            safe_function<subst_occurs_expr>},
    {"get_expr_assignment",    safe_function<subst_get_expr_assignment>},
    {"get_level_assignment",   safe_function<subst_get_level_assignment>},
    {"get_assignment",         safe_function<subst_get_assignment>},
    {"instantiate",            safe_function<subst_instantiate>},
    {0, 0}
};

static void open_substitution(lua_State * L) {
    luaL_newmetatable(L, substitution_mt);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    setfuncs(L, substitution_m, 0);

    SET_GLOBAL_FUN(mk_substitution, "substitution");
    SET_GLOBAL_FUN(substitution_pred, "is_substitution");
}

void open_kernel_module(lua_State * L) {
    // TODO(Leo)
    open_level(L);
    open_list_level(L);
    open_binder_info(L);
    open_expr(L);
    open_list_expr(L);
    open_macro_definition(L);
    open_formatter(L);
    open_environment(L);
    open_io_state(L);
    open_justification(L);
    open_substitution(L);
}
}
