/*
 * Modified version of TINYEXPR. 
 *
 * Changes from the original:
 *  - Merged tinyexpr.c and tinyexpr.h into a single header file tinyexpr.h
 *  - Adapted for C++ (no longer C compatible)
 *  - Renamed and reorganized code to better fit C++
 * 
 * Original project: https://github.com/codeplea/tinyexpr
 * Copyright (c) 2015-2020 Lewis Van Winkle
*/

// SPDX-License-Identifier: Zlib
/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 *
 * Copyright (c) 2015-2020 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef TINYEXPR_H
#define TINYEXPR_H

namespace texpr {

typedef struct te_expr {
    int type;
    union {double value; const double *bound; const void *function;};
    void *parameters[1];
} te_expr;


enum {
    TE_VARIABLE = 0,

    TE_FUNCTION0 = 8, TE_FUNCTION1, TE_FUNCTION2, TE_FUNCTION3,
    TE_FUNCTION4, TE_FUNCTION5, TE_FUNCTION6, TE_FUNCTION7,

    TE_CLOSURE0 = 16, TE_CLOSURE1, TE_CLOSURE2, TE_CLOSURE3,
    TE_CLOSURE4, TE_CLOSURE5, TE_CLOSURE6, TE_CLOSURE7,

    TE_FLAG_PURE = 32
};

typedef struct te_variable {
    const char *name;
    const void *address;
    int type;
    void *context;
} te_variable;



/* Parses the input expression, evaluates it, and frees it. */
/* Returns NaN on error. */
double te_interp(const char *expression, int *error);

/* Parses the input expression and binds variables. */
/* Returns NULL on error. */
te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error);

/* Evaluates the expression. */
double te_eval(const te_expr *n);

/* Prints debugging information on the syntax tree. */
void te_print(const te_expr *n);

/* Frees the expression. */
/* This is safe to call on NULL pointers. */
void te_free(te_expr *n);

/* IMPLEMENTATION:
 * ==============================================================================================
 * ==============================================================================================
 * ============================================================================================== */

/* COMPILE TIME OPTIONS */

/* Exponentiation associativity:
For a^b^c = (a^b)^c and -a^b = (-a)^b do nothing.
For a^b^c = a^(b^c) and -a^b = -(a^b) uncomment the next line.*/
/* #define TE_POW_FROM_RIGHT */

/* Logarithms
For log = base 10 log do nothing
For log = natural log uncomment the next line. */
/* #define TE_NAT_LOG */

#include "tinyexpr.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif


typedef double (*te_fun0)(double, double);
typedef double (*te_fun1)(double, double);
typedef double (*te_fun2)(double, double);

enum {
    TOK_NULL = TE_CLOSURE7+1, TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER, TOK_VARIABLE, TOK_INFIX
};


enum {TE_CONSTANT = 1};


typedef struct state {
    const char *start;
    const char *next;
    int type;
    union {double value; const double *bound; const void *function;};
    void *context;

    const te_variable *lookup;
    int lookup_len;
} state;


#define TYPE_MASK(TYPE) ((TYPE)&0x0000001F)

#define IS_PURE(TYPE) (((TYPE) & TE_FLAG_PURE) != 0)
#define IS_FUNCTION(TYPE) (((TYPE) & TE_FUNCTION0) != 0)
#define IS_CLOSURE(TYPE) (((TYPE) & TE_CLOSURE0) != 0)
#define ARITY(TYPE) ( ((TYPE) & (TE_FUNCTION0 | TE_CLOSURE0)) ? ((TYPE) & 0x00000007) : 0 )
#define CHECK_NULL(ptr, ...) if ((ptr) == NULL) { __VA_ARGS__; return NULL; }

inline te_expr *new_expr(const int type, const te_expr *parameters[]) {
    const int arity = ARITY(type);
    const int psize = sizeof(void*) * arity;
    const int size = (sizeof(te_expr) - sizeof(void*)) + psize + (IS_CLOSURE(type) ? sizeof(void*) : 0);
    te_expr *ret = (te_expr *)malloc(size);
    CHECK_NULL(ret);

    memset(ret, 0, size);
    if (arity && parameters) {
        memcpy(ret->parameters, parameters, psize);
    }
    ret->type = type;
    ret->bound = 0;
    return ret;
}


inline void te_free_parameters(te_expr *n) {
    if (!n) return;
    switch (TYPE_MASK(n->type)) {
        case TE_FUNCTION7: case TE_CLOSURE7: te_free((te_expr *)n->parameters[6]);     /* Falls through. */
        case TE_FUNCTION6: case TE_CLOSURE6: te_free((te_expr *)n->parameters[5]);     /* Falls through. */
        case TE_FUNCTION5: case TE_CLOSURE5: te_free((te_expr *)n->parameters[4]);     /* Falls through. */
        case TE_FUNCTION4: case TE_CLOSURE4: te_free((te_expr *)n->parameters[3]);     /* Falls through. */
        case TE_FUNCTION3: case TE_CLOSURE3: te_free((te_expr *)n->parameters[2]);     /* Falls through. */
        case TE_FUNCTION2: case TE_CLOSURE2: te_free((te_expr *)n->parameters[1]);     /* Falls through. */
        case TE_FUNCTION1: case TE_CLOSURE1: te_free((te_expr *)n->parameters[0]);
    }
}


inline void te_free(te_expr *n) {
    if (!n) return;
    te_free_parameters(n);
    free(n);
}


inline double pi(void) {return 3.14159265358979323846;}
inline double e(void) {return 2.71828182845904523536;}
inline double fac(double a) {/* simplest version of fac */
    if (a < 0.0)
        return NAN;
    if (a > UINT_MAX)
        return INFINITY;
    unsigned int ua = (unsigned int)(a);
    unsigned long int result = 1, i;
    for (i = 1; i <= ua; i++) {
        if (i > ULONG_MAX / result)
            return INFINITY;
        result *= i;
    }
    return (double)result;
}
inline double ncr(double n, double r) {
    if (n < 0.0 || r < 0.0 || n < r) return NAN;
    if (n > UINT_MAX || r > UINT_MAX) return INFINITY;
    unsigned long int un = (unsigned int)(n), ur = (unsigned int)(r), i;
    unsigned long int result = 1;
    if (ur > un / 2) ur = un - ur;
    for (i = 1; i <= ur; i++) {
        if (result > ULONG_MAX / (un - ur + i))
            return INFINITY;
        result *= un - ur + i;
        result /= i;
    }
    return result;
}
inline double npr(double n, double r) {return ncr(n, r) * fac(r);}

#ifdef _MSC_VER
#pragma function (ceil)
#pragma function (floor)
#endif

inline const te_variable functions[] = {
    /* must be in alphabetical order */
    { .name = "abs",     .address = (void *)(+static_cast<double(*)(double)>(&fabs)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "acos",    .address = (void *)(+static_cast<double(*)(double)>(&acos)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "asin",    .address = (void *)(+static_cast<double(*)(double)>(&asin)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "atan",    .address = (void *)(+static_cast<double(*)(double)>(&atan)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "atan2",   .address = (void *)(+static_cast<double(*)(double, double)>(&atan2)),  .type = TE_FUNCTION2 | TE_FLAG_PURE, .context = nullptr},
    { .name = "ceil",    .address = (void *)(+static_cast<double(*)(double)>(&ceil)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "cos",     .address = (void *)(+static_cast<double(*)(double)>(&cos)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "cosh",    .address = (void *)(+static_cast<double(*)(double)>(&cosh)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "e",       .address = (void *)(+static_cast<double(*)()>(&e)),                    .type = TE_FUNCTION0 | TE_FLAG_PURE, .context = nullptr},
    { .name = "exp",     .address = (void *)(+static_cast<double(*)(double)>(&exp)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "fac",     .address = (void *)(+static_cast<double(*)(double)>(&fac)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "floor",   .address = (void *)(+static_cast<double(*)(double)>(&floor)),          .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "ln",      .address = (void *)(+static_cast<double(*)(double)>(&log)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
#ifdef TE_NAT_LOG
    { .name = "log",     .address = (void *)(+static_cast<double(*)(double)>(&log)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
#else
    { .name = "log",     .address = (void *)(+static_cast<double(*)(double)>(&log10)),          .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
#endif
    { .name = "log10",   .address = (void *)(+static_cast<double(*)(double)>(&log10)),          .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "ncr",     .address = (void *)(+static_cast<double(*)(double, double)>(&ncr)),    .type = TE_FUNCTION2 | TE_FLAG_PURE, .context = nullptr},
    { .name = "npr",     .address = (void *)(+static_cast<double(*)(double, double)>(&npr)),    .type = TE_FUNCTION2 | TE_FLAG_PURE, .context = nullptr},
    { .name = "pi",      .address = (void *)(+static_cast<double(*)()>(&pi)),                   .type = TE_FUNCTION0 | TE_FLAG_PURE, .context = nullptr},
    { .name = "pow",     .address = (void *)(+static_cast<double(*)(double, double)>(&pow)),    .type = TE_FUNCTION2 | TE_FLAG_PURE, .context = nullptr},
    { .name = "sin",     .address = (void *)(+static_cast<double(*)(double)>(&sin)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "sinh",    .address = (void *)(+static_cast<double(*)(double)>(&sinh)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "sqrt",    .address = (void *)(+static_cast<double(*)(double)>(&sqrt)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "tan",     .address = (void *)(+static_cast<double(*)(double)>(&tan)),            .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = "tanh",    .address = (void *)(+static_cast<double(*)(double)>(&tanh)),           .type = TE_FUNCTION1 | TE_FLAG_PURE, .context = nullptr},
    { .name = nullptr,   .address = nullptr,                                                    .type = 0,                           .context = nullptr}
};

inline const te_variable *find_builtin(const char *name, int len) {
    int imin = 0;
    int imax = sizeof(functions) / sizeof(te_variable) - 2;

    /*Binary search.*/
    while (imax >= imin) {
        const int i = (imin + ((imax-imin)/2));
        int c = strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0) {
            return functions + i;
        } else if (c > 0) {
            imin = i + 1;
        } else {
            imax = i - 1;
        }
    }

    return 0;
}

inline const te_variable *find_lookup(const state *s, const char *name, int len) {
    int iters;
    const te_variable *var;
    if (!s->lookup) return 0;

    for (var = s->lookup, iters = s->lookup_len; iters; ++var, --iters) {
        if (strncmp(name, var->name, len) == 0 && var->name[len] == '\0') {
            return var;
        }
    }
    return 0;
}



inline double add(double a, double b) {return a + b;}
inline double sub(double a, double b) {return a - b;}
inline double mul(double a, double b) {return a * b;}
inline double divide(double a, double b) {return a / b;}
inline double negate(double a) {return -a;}
inline double comma(double a, double b) {(void)a; return b;}

inline void next_token(state *s) {
    s->type = TOK_NULL;

    do {

        if (!*s->next){
            s->type = TOK_END;
            return;
        }

        /* Try reading a number. */
        if ((s->next[0] >= '0' && s->next[0] <= '9') || s->next[0] == '.') {
            s->value = strtod(s->next, (char**)&s->next);
            s->type = TOK_NUMBER;
        } else {
            /* Look for a variable or builtin function call. */
            if (isalpha(s->next[0])) {
                const char *start;
                start = s->next;
                while (isalpha(s->next[0]) || isdigit(s->next[0]) || (s->next[0] == '_')) s->next++;
                
                const te_variable *var = find_lookup(s, start, s->next - start);
                if (!var) var = find_builtin(start, s->next - start);

                if (!var) {
                    s->type = TOK_ERROR;
                } else {
                    switch(TYPE_MASK(var->type))
                    {
                        case TE_VARIABLE:
                            s->type = TOK_VARIABLE;
                            s->bound = (double *)var->address;
                            break;

                        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:         /* Falls through. */
                        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:         /* Falls through. */
                            s->context = var->context;                                                  /* Falls through. */

                        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:     /* Falls through. */
                        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:     /* Falls through. */
                            s->type = var->type;
                            s->function = var->address;
                            break;
                    }
                }

            } else {
                /* Look for an operator or special character. */
                switch (s->next++[0]) {
                    case '+': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&add)); break;
                    case '-': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&sub)); break;
                    case '*': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&mul)); break;
                    case '/': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&divide)); break;
                    case '^': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&pow)); break;
                    case '%': s->type = TOK_INFIX; s->function = (void *)(+static_cast<double(*)(double, double)>(&fmod)); break;
                    case '(': s->type = TOK_OPEN; break;
                    case ')': s->type = TOK_CLOSE; break;
                    case ',': s->type = TOK_SEP; break;
                    case ' ': case '\t': case '\n': case '\r': break;
                    default: s->type = TOK_ERROR; break;
                }
            }
        }
    } while (s->type == TOK_NULL);
}


inline te_expr *list(state *s);
inline te_expr *expr(state *s);
inline te_expr *power(state *s);

inline te_expr *base(state *s) {
    /* <base>      =    <constant> | <variable> | <function-0> {"(" ")"} | <function-1> <power> | <function-X> "(" <expr> {"," <expr>} ")" | "(" <list> ")" */
    te_expr *ret;
    int arity;

    switch (TYPE_MASK(s->type)) {
        case TOK_NUMBER:
            ret = new_expr(TE_CONSTANT, 0);
            CHECK_NULL(ret);

            ret->value = s->value;
            next_token(s);
            break;

        case TOK_VARIABLE:
            ret = new_expr(TE_VARIABLE, 0);
            CHECK_NULL(ret);

            ret->bound = s->bound;
            next_token(s);
            break;

        case TE_FUNCTION0:
        case TE_CLOSURE0:
            ret = new_expr(s->type, 0);
            CHECK_NULL(ret);

            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[0] = s->context;
            next_token(s);
            if (s->type == TOK_OPEN) {
                next_token(s);
                if (s->type != TOK_CLOSE) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }
            break;

        case TE_FUNCTION1:
        case TE_CLOSURE1:
            ret = new_expr(s->type, 0);
            CHECK_NULL(ret);

            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[1] = s->context;
            next_token(s);
            ret->parameters[0] = power(s);
            CHECK_NULL(ret->parameters[0], te_free(ret));
            break;

        case TE_FUNCTION2: case TE_FUNCTION3: case TE_FUNCTION4:
        case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
        case TE_CLOSURE2: case TE_CLOSURE3: case TE_CLOSURE4:
        case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            arity = ARITY(s->type);

            ret = new_expr(s->type, 0);
            CHECK_NULL(ret);

            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[arity] = s->context;
            next_token(s);

            if (s->type != TOK_OPEN) {
                s->type = TOK_ERROR;
            } else {
                int i;
                for(i = 0; i < arity; i++) {
                    next_token(s);
                    ret->parameters[i] = expr(s);
                    CHECK_NULL(ret->parameters[i], te_free(ret));

                    if(s->type != TOK_SEP) {
                        break;
                    }
                }
                if(s->type != TOK_CLOSE || i != arity - 1) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }

            break;

        case TOK_OPEN:
            next_token(s);
            ret = list(s);
            CHECK_NULL(ret);

            if (s->type != TOK_CLOSE) {
                s->type = TOK_ERROR;
            } else {
                next_token(s);
            }
            break;

        default:
            ret = new_expr(0, 0);
            CHECK_NULL(ret);

            s->type = TOK_ERROR;
            ret->value = NAN;
            break;
    }

    return ret;
}


inline te_expr *power(state *s) {
    /* <power>     =    {("-" | "+")} <base> */
    int sign = 1;
    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        if (s->function == sub) sign = -sign;
        next_token(s);
    }

    te_expr *ret;

    if (sign == 1) {
        ret = base(s);
    } else {
        te_expr *b = base(s);
        CHECK_NULL(b);

        const te_expr *params[] = {b};
        ret = new_expr(TE_FUNCTION1 | TE_FLAG_PURE, params);

        CHECK_NULL(ret, te_free(b));

        ret->function = (void *)negate;
    }

    return ret;
}

#ifdef TE_POW_FROM_RIGHT
inline te_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    te_expr *ret = power(s);
    CHECK_NULL(ret);

    int neg = 0;

    if (ret->type == (TE_FUNCTION1 | TE_FLAG_PURE) && ret->function == negate) {
        te_expr *se = ret->parameters[0];
        free(ret);
        ret = se;
        neg = 1;
    }

    te_expr *insertion = 0;

    while (s->type == TOK_INFIX && (s->function == (void *)(+static_cast<double(*)(double, double)>(&pow)))) {
        te_fun2 t = (te_fun2)s->function;
        next_token(s);

        if (insertion) {
            /* Make exponentiation go right-to-left. */
            te_expr *p = power(s);
            CHECK_NULL(p, te_free(ret));


            te_expr *insert;

            const te_expr *params[] = {insertion->parameters[1], p};
            insert = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);

            CHECK_NULL(insert, te_free(p), te_free(ret));

            insert->function = (void *)t;
            insertion->parameters[1] = insert;
            insertion = insert;
        } else {
            te_expr *p = power(s);
            CHECK_NULL(p, te_free(ret));

            te_expr *prev = ret;

            const te_expr *params[] = {ret, p};
            ret = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);

            CHECK_NULL(ret, te_free(p), te_free(prev));

            ret->function = (void *)t;
            insertion = ret;
        }
    }

    if (neg) {
        te_expr *prev = ret;

        const te_expr *params[] = {ret};
        ret = new_expr(TE_FUNCTION1 | TE_FLAG_PURE, params);

        CHECK_NULL(ret, te_free(prev));

        ret->function = negate;
    }

    return ret;
}
#else
inline te_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    te_expr *ret = power(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == (void *)(+static_cast<double(*)(double, double)>(&pow)))) {
        te_fun2 t = (te_fun2)s->function;
        next_token(s);
        te_expr *p = power(s);
        CHECK_NULL(p, te_free(ret));

        te_expr *prev = ret;

        const te_expr *params[] = {ret, p};
        ret = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);
        CHECK_NULL(ret, te_free(p), te_free(prev));

        ret->function = (void *)t;
    }

    return ret;
}
#endif

inline te_expr *term(state *s) {
    /* <term>      =    <factor> {("*" | "/" | "%") <factor>} */
    te_expr *ret = factor(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == mul || s->function == divide || s->function == (void *)(+static_cast<double(*)(double, double)>(&fmod)))) {
        te_fun2 t = (te_fun2)s->function;
        next_token(s);
        te_expr *f = factor(s);
        CHECK_NULL(f, te_free(ret));

        te_expr *prev = ret;

        const te_expr *params[] = {ret, f};
        ret = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);

        CHECK_NULL(ret, te_free(f), te_free(prev));

        ret->function = (void *)t;
    }

    return ret;
}


inline te_expr *expr(state *s) {
    /* <expr>      =    <term> {("+" | "-") <term>} */
    te_expr *ret = term(s);
    CHECK_NULL(ret);

    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        te_fun2 t = (te_fun2)s->function;
        next_token(s);
        te_expr *te = term(s);
        CHECK_NULL(te, te_free(ret));

        te_expr *prev = ret;

        const te_expr *params[] = {ret, te};
        ret = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);

        CHECK_NULL(ret, te_free(te), te_free(prev));

        ret->function = (void *)t;
    }

    return ret;
}


inline te_expr *list(state *s) {
    /* <list>      =    <expr> {"," <expr>} */
    te_expr *ret = expr(s);
    CHECK_NULL(ret);

    while (s->type == TOK_SEP) {
        next_token(s);
        te_expr *e = expr(s);
        CHECK_NULL(e, te_free(ret));

        te_expr *prev = ret;

        const te_expr *params[] = {ret, e};
        ret = new_expr(TE_FUNCTION2 | TE_FLAG_PURE, params);

        CHECK_NULL(ret, te_free(e), te_free(prev));

        ret->function = (void *)comma;
    }

    return ret;
}


#define TE_FUN(...) ((double(*)(__VA_ARGS__))n->function)
#define MEXPAND(e) te_eval((te_expr *)n->parameters[e])


inline double te_eval(const te_expr *n) {
    if (!n) return NAN;

    switch(TYPE_MASK(n->type)) {
        case TE_CONSTANT: return n->value;
        case TE_VARIABLE: return *n->bound;

        case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
        case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
            switch(ARITY(n->type)) {
                case 0: return TE_FUN(void)();
                case 1: return TE_FUN(double)(MEXPAND(0));
                case 2: return TE_FUN(double, double)(MEXPAND(0), MEXPAND(1));
                case 3: return TE_FUN(double, double, double)(MEXPAND(0), MEXPAND(1), MEXPAND(2));
                case 4: return TE_FUN(double, double, double, double)(MEXPAND(0), MEXPAND(1),
                        MEXPAND(2), MEXPAND(3));
                case 5: return TE_FUN(double, double, double, double, double)(MEXPAND(0),
                        MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4));
                case 6: return TE_FUN(double, double, double, double, double, double)(MEXPAND(0),
                        MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4), MEXPAND(5));
                case 7: return TE_FUN(double, double, double, double, double, double, double)(
                        MEXPAND(0), MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4), MEXPAND(5),
                        MEXPAND(6));
                default: return NAN;
            }

        case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
        case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
            switch(ARITY(n->type)) {
                case 0: return TE_FUN(void*)(n->parameters[0]);
                case 1: return TE_FUN(void*, double)(n->parameters[1], MEXPAND(0));
                case 2: return TE_FUN(void*, double, double)(n->parameters[2], MEXPAND(0),
                        MEXPAND(1));
                case 3: return TE_FUN(void*, double, double, double)(n->parameters[3],
                        MEXPAND(0), MEXPAND(1), MEXPAND(2));
                case 4: return TE_FUN(void*, double, double, double, double)(n->parameters[4],
                        MEXPAND(0), MEXPAND(1), MEXPAND(2), MEXPAND(3));
                case 5: return TE_FUN(void*, double, double, double, double, double)(
                        n->parameters[5], MEXPAND(0), MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4));
                case 6: return TE_FUN(void*, double, double, double, double, double, double)(
                        n->parameters[6], MEXPAND(0), MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4),
                        MEXPAND(5));
                case 7: return TE_FUN(void*, double, double, double, double, double, double, double)(
                        n->parameters[7], MEXPAND(0), MEXPAND(1), MEXPAND(2), MEXPAND(3), MEXPAND(4),
                        MEXPAND(5), MEXPAND(6));
                default: return NAN;
            }

        default: return NAN;
    }

}

#undef TE_FUN
#undef MEXPAND

static void optimize(te_expr *n) {
    /* Evaluates as much as possible. */
    if (n->type == TE_CONSTANT) return;
    if (n->type == TE_VARIABLE) return;

    /* Only optimize out functions flagged as pure. */
    if (IS_PURE(n->type)) {
        const int arity = ARITY(n->type);
        int known = 1;
        int i;
        for (i = 0; i < arity; ++i) {
            optimize((te_expr *)n->parameters[i]);
            if (((te_expr*)(n->parameters[i]))->type != TE_CONSTANT) {
                known = 0;
            }
        }
        if (known) {
            const double value = te_eval(n);
            te_free_parameters(n);
            n->type = TE_CONSTANT;
            n->value = value;
        }
    }
}


te_expr *te_compile(const char *expression, const te_variable *variables, int var_count, int *error) {
    state s;
    s.start = s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;

    next_token(&s);
    te_expr *root = list(&s);
    if (root == NULL) {
        if (error) *error = -1;
        return NULL;
    }

    if (s.type != TOK_END) {
        te_free(root);
        if (error) {
            *error = (s.next - s.start);
            if (*error == 0) *error = 1;
        }
        return 0;
    } else {
        optimize(root);
        if (error) *error = 0;
        return root;
    }
}


double te_interp(const char *expression, int *error) {
    te_expr *n = te_compile(expression, 0, 0, error);

    double ret;
    if (n) {
        ret = te_eval(n);
        te_free(n);
    } else {
        ret = NAN;
    }
    return ret;
}

static void pn (const te_expr *n, int depth) {
    int i, arity;
    printf("%*s", depth, "");

    switch(TYPE_MASK(n->type)) {
    case TE_CONSTANT: printf("%f\n", n->value); break;
    case TE_VARIABLE: printf("bound %p\n", n->bound); break;

    case TE_FUNCTION0: case TE_FUNCTION1: case TE_FUNCTION2: case TE_FUNCTION3:
    case TE_FUNCTION4: case TE_FUNCTION5: case TE_FUNCTION6: case TE_FUNCTION7:
    case TE_CLOSURE0: case TE_CLOSURE1: case TE_CLOSURE2: case TE_CLOSURE3:
    case TE_CLOSURE4: case TE_CLOSURE5: case TE_CLOSURE6: case TE_CLOSURE7:
         arity = ARITY(n->type);
         printf("f%d", arity);
         for(i = 0; i < arity; i++) {
             printf(" %p", n->parameters[i]);
         }
         printf("\n");
         for(i = 0; i < arity; i++) {
             pn((te_expr *)n->parameters[i], depth + 1);
         }
         break;
    }
}


void te_print(const te_expr *n) {
    pn(n, 0);
}

} /* namespace tinyexpr */

#endif /*TINYEXPR_H*/