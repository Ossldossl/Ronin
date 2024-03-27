#pragma once
#include "misc.h"
#include "parser.h"

// only return u32 for now
// TODO: proper value handling

typedef struct {
    union {
        u64 uval; // also ptr val
        i64 ival;
        str_t sval;
        double dval;
    } as;
    enum {
        VAL_NONE,
        VAL_U,
        VAL_I,
        VAL_B,
        VAL_S,
        VAL_D,
        VAL_NULL,
        VAL_TRUE,
        VAL_FALSE,
    } kind;
} value_t;

value_t eval_expr(expr_t* e);